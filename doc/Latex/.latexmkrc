$pdf_mode = 1;
$out_dir = 'out';
@default_files = ('schm24.tex');
$pdflatex = 'pdflatex -interaction=nonstopmode --shell-escape';

add_cus_dep('glo', 'gls', 0, 'run_makeglossaries');
add_cus_dep('acn', 'acr', 0, 'run_makeglossaries');

sub run_makeglossaries {
    my ($base_name, $path) = fileparse( $_[0] );
    return system "makeglossaries -d '$path' '$base_name'";
}
