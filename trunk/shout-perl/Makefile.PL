use ExtUtils::MakeMaker;
# See lib/ExtUtils/MakeMaker.pm for details of how to influence
# the contents of the Makefile that is written.

$pkgconfig = 'PKG_CONFIG_PATH=${PKG_CONFIG_PATH:-/usr/local/lib/pkgconfig} pkg-config';
if (system("$pkgconfig --exists shout") == 0) {
  $ccflags = `$pkgconfig --cflags shout`;
  $libs = `$pkgconfig --libs shout`;
} elsif (system("shout-config --help >/dev/null") == 0) {
  $ccflags = `shout-config --cflags`;
  $libs = `shout-config --libs`;
} else {
  die "Couldn't find pkg-config shout OR shout-config!";
}

WriteMakefile(
    'NAME'	=> 'Shout',
    'ABSTRACT'  => 'Perl bindings for the libshout audio streaming library',
    'VERSION_FROM' => 'Shout.pm', # finds $VERSION
    'CCFLAGS'   => $ccflags,
    'LIBS'	=> $libs 
);
