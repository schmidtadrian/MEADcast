import json
import os
import re
import shutil
import yaml
from argparse import ArgumentParser, HelpFormatter
from dataclasses import dataclass
from ipaddress import IPv6Address, IPv6Network, IPv4Interface
from os import path
from subprocess import run
from sys import argv
from typing import List

MGM_IF = 'enp1s0'
MGM_NET = 'mgm'
STUB_GW = 0xFFFF
STUB_IF = 'enp2s0'

NETPLAN_CFG = '90-default.yaml'
NETPLAN_DIR = '/etc/netplan/'
IMG_DIR = '/var/lib/libvirt/images/'
IMG_FORMAT = 'qcow2'

FRR_CFG = 'frr.conf'
FRR_DIR = '/etc/frr/'
FRR_STUB_CFG = """!
interface {IFNAME}
 ipv6 mld
 ipv6 pim passive
exit
"""
FRR_TRAN_CFG = """!
interface {IFNAME}
 ipv6 ospf6 area 0
 ipv6 pim
exit
"""


def init_argparse() -> ArgumentParser:
    ap = ArgumentParser(
        usage="%(prog)s [OPTION] [FILE]...",
        description="Build network topology",
        formatter_class=lambda prog: HelpFormatter(prog, max_help_position=26))

    args = [('-t', '--topo',             'topology file', dict(required=True, type=open)),
            ('-o', '--outdir',           'output dir for generated configs', dict(default='./', type=str)),
            ('-n', '--net',              'xml template to create libvirt net', dict(required=True, type=open)),
            ('-u', '--user',             'vm user', dict(default='user', type=str)),
            ('-k', '--kernel',           'router direct kernel boot', dict(type=str)),
            ('-f', '--frr',              'router frr config template', dict(required=True, type=open)),
            (None, '--ssh',              'ssh pub key injected into vms', dict(required=True, type=str)),
            (None, '--client-cpu',       'client vm vcpus', dict(default=1, type=int)),
            (None, '--client-mem',       'client vm mem', dict(default=512, type=int)),
            (None, '--client-disksize',  'client vm disk size', dict(default="8G", type=str)),
            (None, '--client-firstboot', 'script to run on first boot in client vm', dict(nargs='+', default=[])),
            (None, '--router-cpu',       'router vm vcpus', dict(default=1, type=int)),
            (None, '--router-mem',       'router vm mem', dict(default=512, type=int)),
            (None, '--router-disksize',  'router vm disk size', dict(default="8G", type=str)),
            (None, '--router-firstboot', 'script to run on first boot inside router vm', dict(nargs='+', default=[])),
            (None, '--backing-file',     'set backing file of disk', dict(required=True, type=str)),
            (None, '--backing-format',   'set backing file format', dict(default='raw', type=str)),
            (None, '--os-variant',       'set os variant parm', dict(default='debian11', type=str))
            ]

    for arg1, arg2, desc, opt in args:
        if arg1:
            ap.add_argument(arg1, arg2, help=desc, **opt)
        else:
            ap.add_argument(arg2, help=desc, **opt)

    return ap.parse_args(args=None if argv[1:] else ['--help'])


def create_net(name: str, cfg: str, auto: bool = True, start: bool = True):
    run(f'virsh net-define --file {cfg}'.split(), check=True)
    if auto:
        run(f'virsh net-autostart {name}'.split(), check=True)
    if start:
        run(f'virsh net-start {name}'.split(), check=True)


def create_net_cfg(outdir: str, template: str, name: str, bridge: str,
                   network: str, netmask: str):
    """Writes `name`.xml libvirt network file into `outdir`"""
    buf = template.format(NAME=name,
                          BRIDGE=f'br_{name}',
                          NETWORK=network,
                          NETMASK=netmask)

    cfg = path.join(outdir, name + '.xml')
    with open(cfg, 'w') as stream:
        stream.write(buf)
    print('Written ' + cfg)

    return cfg


def create_img(name: str, size: str, bfile: str, bformat: str,
               format: str = 'qcow2'):
    """Creates img based on given backing file"""
    run(['qemu-img', 'create',
         '-f', format,
         '-o', f'backing_file={bfile}',
         '-F', bformat,
         name, size], check=True)


def modify_img(img: str, host: str, user: str, ssh: str, lcpy: [str] = [],
               lrun: [str] = [], lboot: [str] = [], dirs: [str] = []):
    """Customizes img. For further infos about params see
    virt-customize man pages."""
    cmd = ['virt-customize', '-a', img,
           '--hostname', host,
           '--run-command', f'adduser {user}',
           '--run-command', f'adduser {user} sudo',
           '--run-command', 'ssh-keygen -A',
           '--append-line', f'/etc/sudoers:{user} ALL=(ALL) NOPASSWD:ALL',
           '--ssh-inject', f'{user}:file:{ssh}']

    for d in dirs:
        cmd += ['--mkdir', d]
    for c in lcpy:
        cmd += ['--copy-in', c]
    for r in lrun:
        cmd += ['--run', r]
    for b in lboot:
        cmd += ['--firstboot', b]

    run(cmd, check=True)


def install_vm(name: str, cpus: int, mem: int, disk: str,
               format: str, nets: [str], boot: str = None,
               variant: str = "debian11"):
    """Creates vm for img"""
    cmd = ['virt-install',
           '--name', name,
           '--vcpus', str(cpus),
           '--memory', str(mem),
           '--disk', f'path={disk},format={format}',
           '--os-variant', variant,
           '--import',
           '--noautoconsole']
    for net in nets:
        cmd += ['--network', f'network={net}']
    if boot:
        cmd += ['--boot', boot]

    run(cmd, check=True)


def set_mgm_if(eths: dict, net: IPv4Interface, netid: int, id: int):
    ifaddr = re.sub(r'\.\d\.\d\/', f'.{netid}.{id}/', str(net))
    eths[MGM_IF] = {
        'dhcp4': False,
        'dhcp6': False,
        'addresses': [ifaddr],
        'routes': [{'to': 'default', 'via': str(net.ip)}],
        'nameservers': {'addresses': [str(net.ip)]}
    }


@dataclass
class VM_CFG:
    user: str
    ssh: str
    size: str
    bfile: str
    bformat: str
    cpus: int
    mem: int
    cpy: [str]
    run: [str]
    boot: [str]
    img_dir: str = IMG_DIR
    img_format: str = IMG_FORMAT
    kernel: str = None


class Stub:

    PREF = 'stub'

    def __init__(self, id: int, prefix: IPv6Network, router: int,
                 mask: int = 112, clients: int = 0):
        self.id = id
        self.router = router
        self.mask = mask
        self.clients = clients
        self.net = self.set_net(prefix)
        self.name = self.set_name()
        self.gwaddr = self.net.network_address + STUB_GW

    def set_net(self, prefix: IPv6Network) -> IPv6Network:
        """Creates network based on the following pattern:
           Prefix::self.id:0/self.mask"""
        net_addr = IPv6Address(int(prefix.network_address) + (self.id << 16))
        return IPv6Network(f'{net_addr}/{self.mask}')

    def set_name(self) -> str:
        return Stub.PREF + str(self.id).zfill(2)

    def create_vms(self, outdir: str, mgm: IPv4Interface, routes: [str],
                   vm_cfg: VM_CFG):
        """Creates netplan config, disk img, customizes img & creates vm"""
        tmp = path.join(outdir, NETPLAN_CFG)

        for i in range(1, self.clients + 1):
            cfg = {'network': {'version': 2, 'ethernets': {}}}
            eths = cfg["network"]['ethernets']
            set_mgm_if(eths, mgm, self.id, i)

            ifaddr = self.net.network_address + i
            eths[STUB_IF] = {
                'dhcp4': False,
                'dhcp6': False,
                'addresses': [f'{ifaddr}/{self.mask}'],
                'routes': [{'to': dst, 'via': str(self.gwaddr)} for dst in routes]
            }

            name = f'c{self.id:02d}{i:02d}'
            out = path.join(outdir, name + '.yaml')
            img = path.join(vm_cfg.img_dir, f'{name}.{vm_cfg.img_format}')

            with open(out, 'w') as stream:
                yaml.dump(cfg, stream, sort_keys=False)
            print(f'Written {out}')

            shutil.copy(out, tmp)
            create_img(img, vm_cfg.size, vm_cfg.bfile, vm_cfg.bformat)
            modify_img(img, name, vm_cfg.user, vm_cfg.ssh,
                       vm_cfg.cpy + [f'{tmp}:{NETPLAN_DIR}'],
                       vm_cfg.run, vm_cfg.boot)
            install_vm(name, vm_cfg.cpus, vm_cfg.mem, img,
                       vm_cfg.img_format, [MGM_NET, self.name])

        os.remove(tmp)

    def create(stubs: List, routers: dict, outdir: str, template: str,
               mgm: IPv4Interface, routes: [str], vm_cfg: VM_CFG):
        """High-level wrapper, creates stub network & all its clients"""
        for stub in stubs:
            cfg = create_net_cfg(outdir, template, stub.name,
                                 'br_' + stub.name, stub.net.network_address,
                                 stub.mask)
            create_net(stub.name, cfg)

            r = routers.get(stub.router)
            if r:
                if stub not in r.stubs:
                    r.stubs.append(stub)
            else:
                routers[stub.router] = Router(stub.router, [stub], [])

            stub.create_vms(outdir, mgm, routes, vm_cfg)


class Tran:

    PREF = "tran_"

    def __init__(self, r1: int, r2: int, prefix: IPv6Network, mask: int = 126):
        self.r1 = r1
        self.r2 = r2
        self.mask = mask
        self.net = self.set_net(prefix)
        self.name = self.set_name()

    def set_net(self, prefix: IPv6Network) -> IPv6Network:
        """Creates network based on the following pattern:
           Prefix::self.r1self.r2:0/self.mask"""
        id = ((self.r1 << 8) + self.r2) << 16
        net_addr = IPv6Address(int(prefix.network_address) + id)
        return IPv6Network(f'{net_addr}/{self.mask}')

    def set_name(self) -> str:
        return f'{Tran.PREF}r{self.r1:02d}r{self.r2:02d}'

    def create(trans, routers: dict, outdir: str, template: str):
        """High-level wrapper, creates transit network"""
        for tran in trans:
            cfg = create_net_cfg(outdir, template, tran.name,
                                 'br_' + tran.name, tran.net.network_address,
                                 tran.mask)
            create_net(tran.name, cfg)

            for id in [tran.r1, tran.r2]:
                r = routers.get(id)
                if r:
                    if tran not in r.trans:
                        r.trans.append(tran)
                else:
                    routers[id] = Router(id, [], [tran])


class Router:
    """Holds a list of stub and transit networks it is connected to."""

    NET_ID = 99
    IFF_OFF = 2

    def __init__(self, id: int, stubs: List[Stub], trans: List[Tran]):
        self.id = id
        self.name = f'r{self.id:02d}'
        self.stubs = stubs
        self.trans = trans

    def get_ifname(i: int):
        return f'enp{i + Router.IFF_OFF}s0'

    def create(routers: dict, outdir: str, template: str, prefix: IPv6Network,
               mgm: IPv4Interface, vm_cfg: VM_CFG):
        """High-level function, creates netplan & frr config, img and vm"""
        np_tmp = path.join(outdir, NETPLAN_CFG)
        frr_tmp = path.join(outdir, FRR_CFG)

        for _, r in routers.items():
            frr_buf = template.format(NETWORK=str(prefix))
            np_buf = {'network': {'version': 2, 'ethernets': {}}}
            eths = np_buf["network"]['ethernets']
            set_mgm_if(eths, mgm, Router.NET_ID, r.id)

            i = 0
            nets: [str] = [MGM_NET]

            def add_net(i: int, template: str, addr: str, name: str):
                nonlocal frr_buf
                nonlocal eths
                nonlocal nets

                ifname = Router.get_ifname(i)
                frr_buf += template.format(IFNAME=ifname)
                eths[ifname] = {
                    'dhcp4': False,
                    'dhcp6': False,
                    'addresses': [addr]}
                nets.append(name)
                return i + 1

            for stub in r.stubs:
                addr = f'{stub.gwaddr}/{stub.mask}'
                i = add_net(i, FRR_STUB_CFG, addr, stub.name)

            for tran in r.trans:
                addr = f'{tran.net.network_address + r.id}/{tran.mask}'
                i = add_net(i, FRR_TRAN_CFG, addr, tran.name)

            np_cfg = path.join(outdir, r.name + '.yaml')
            frr_cfg = path.join(outdir, r.name + '.conf')
            img = path.join(vm_cfg.img_dir, f'{r.name}.{vm_cfg.img_format}')

            with open(np_cfg, 'w') as stream:
                yaml.dump(np_buf, stream, sort_keys=False)
            print(f'Written {np_cfg}')
            shutil.copy(np_cfg, np_tmp)

            with open(frr_cfg, 'w') as stream:
                stream.write(frr_buf)
            print(f'Written {frr_cfg}')
            shutil.copy(frr_cfg, frr_tmp)

            cpy = vm_cfg.cpy + [f'{np_tmp}:{NETPLAN_DIR}'] + [f'{frr_tmp}:{FRR_DIR}']
            create_img(img, vm_cfg.size, vm_cfg.bfile, vm_cfg.bformat)
            modify_img(img, r.name, vm_cfg.user, vm_cfg.ssh, cpy,
                       vm_cfg.run, vm_cfg.boot, [FRR_DIR])
            install_vm(r.name, vm_cfg.cpus, vm_cfg.mem, img,
                       vm_cfg.img_format, nets, vm_cfg.kernel)

        os.remove(np_tmp)
        os.remove(frr_tmp)


@dataclass
class Cfg:
    stub_range: IPv6Network
    tran_range: IPv6Network
    mgm_gw: IPv4Interface
    stubs: List[Stub]
    trans: List[Tran]

    def new(json: dict):
        srange = IPv6Network(json['stub_range'])
        trange = IPv6Network(json['tran_range'])

        return Cfg(srange, trange,
                   IPv4Interface(json['mgm_gw']),
                   [Stub(prefix=srange, **v) for v in json['stubs']],
                   [Tran(prefix=trange, *v) for v in json['trans']])


if __name__ == '__main__':
    args = init_argparse()
    json = json.load(args.topo)
    cfg = Cfg.new(json)
    net_tmpl = args.net.read()
    frr_tmpl = args.frr.read()
    client_cfg = VM_CFG(args.user, args.ssh, args.client_disksize,
                        args.backing_file, args.backing_format,
                        args.client_cpu, args.client_mem, [], [],
                        args.client_firstboot)
    router_cfg = VM_CFG(args.user, args.ssh, args.router_disksize,
                        args.backing_file, args.backing_format,
                        args.router_cpu, args.router_mem, [], [],
                        args.router_firstboot, kernel=args.kernel)

    # Dict of all routers, will be filled during Stub and Tran create.
    routers: dict[Router] = {}
    Stub.create(cfg.stubs, routers, args.outdir, net_tmpl, cfg.mgm_gw,
                [str(cfg.stub_range)], client_cfg,)
    Tran.create(cfg.trans, routers, args.outdir, net_tmpl)
    Router.create(routers, args.outdir, frr_tmpl, cfg.tran_range, cfg.mgm_gw,
                  router_cfg)
