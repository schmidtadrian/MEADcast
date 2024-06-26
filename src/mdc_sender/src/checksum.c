/*
 *  IP checksumming functions.
 *  (c) 2008 Gerd Hoffmann <kraxel@redhat.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; under version 2 or later of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>

uint32_t net_checksum_add(int len, uint8_t *buf)
{
    uint32_t sum = 0;
    int i;

    for (i = 0; i < len; i++) {
	    if (i & 1)
	        sum += (uint32_t)buf[i];
	    else
	        sum += (uint32_t)buf[i] << 8;
    }
    return sum;
}

uint16_t net_checksum_finish(uint32_t sum)
{
    while (sum>>16)
	    sum = (sum & 0xFFFF)+(sum >> 16);
    return ~sum;
}

uint16_t net_checksum_tcpudp(uint16_t length, uint16_t npad, uint16_t proto,
                             uint8_t *src, uint8_t *dst, uint8_t *buf)
{
    uint32_t sum = 0;

    sum += net_checksum_add(length+npad, buf);              // payload
    sum += net_checksum_add(sizeof(struct in6_addr), src);  // src addr
    sum += net_checksum_add(sizeof(struct in6_addr), dst);  // dst addr
    sum += proto + length;                                  // proto & len
    return net_checksum_finish(sum);
}

