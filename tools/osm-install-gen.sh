#!/bin/sh
#
# This script generates an `osm-install.sh` script, which in turn can be used to
# install a cross compiling GCC, YAMS, and our YAMS helper scripts.

cd "$(dirname "$0")"

cat > osm-install.sh <<"EOFGEN"
#!/bin/sh
#
# READ THIS SCRIPT BEFORE RUNNING IT!
#
# Installs a cross-compiler and YAMS.  Requires a *NIX system.
#
# This script compiles GCC (GNU C Compiler) and GNU Binutils for MIPS, which
# takes a while, and then installs YAMS.  This approach should work on all
# modern Linux systems, and possibly also on *BSD and Mac OS X.  It likely does
# not work (well) on Microsoft Windows.
#
# Your OS might already have cross-compilation packages for GCC and Binutils,
# in which case you can install those instead.  If you don't want this script
# to install GCC and Binutils for MIPS and x86_64, set the INSTALL_MIPS_UTILS
# environment variable to false, i.e. run
#
#   export INSTALL_MIPS_UTILS=false
#
# before running this script.
#
# This script installs YAMS into the directory "$HOME/osm" by default along with
# various helper scripts.  Once installed, add "$HOME/osm/bin" to your PATH,
# i.e. append the line
#
#   export PATH="$HOME/osm/bin:$PATH"
#
# to your `~/.profile`, `~/.bash_profile` or similar.  You can then run Kudos
# with `yams-term` and `yams-sim` as described in the guide on Absalon.
#
# If you wish to install the various utilities to another directory, run
#
#   export OSM_DIR=/path/to/your/directory
#
# before running this script.
#
# GCC and GNU Binutils have several dependencies, all of which *should be* built
# and installed automatically by this script.  However, this might not work, in
# which case you need to install these dependencies manually.  Install those
# listed at <http://gcc.gnu.org/install/prerequisites.html>.  You can
# *either* download them and then install them manually, or you can use your
# package manager to automatically install them.
#
# Manual download of dependencies (after the automatic download, you need to
# manually make and install the dependencies):
#
#   wget ftp://ftp.mpi-sb.mpg.de/pub/gnu/mirror/gcc.gnu.org/pub/gcc/releases/gcc-5.3.0/gcc-5.3.0.tar.gz
#   tar xf gcc-5.3.0.tar.gz
#   cd gcc-5.3.0
#   ./contrib/download_prerequisites
#
# Automatic dependencies install on Debian and Ubuntu:
#
#   sudo apt-get build-dep gcc-5 binutils
#
# Other Linux distros have similar features.
#
# If the install fails, and you don't know what to do, do one of these two
# things:
#
#  + Use the virtual appliance from Absalon (recommended).
#
#  + Ask on the Piazza forum, and upload (part of) the log file at
#    "$HOME/osm/install_log" to e.g. <http://pastebin.com/>.
#
# Installation requires approximately 3 GB space, but most of it is used for
# temporary build files (by default in `$HOME/osm/build`) which can be removed
# afterwards.  If something goes wrong during the installation, the easiest
# action is probably to remove the entire $HOME/osm directory before starting
# over.
#
# By default, this script builds each program with 4 make jobs.  To change
# that, run
#
#  export NJOBS=<a number>
#
# before running this script.

set -e # Stop on first error.
set -x # Print each command when executing it.

if ! [ "$INSTALL_MIPS_UTILS" ]; then
    INSTALL_MIPS_UTILS=true
fi
if ! [ "$NJOBS" ]; then # Passed as -j to all `make` runs
    NJOBS=4
fi
if ! [ "$OSM_DIR" ]; then
    OSM_DIR="$HOME/osm"
fi
BUILD_DIR="$OSM_DIR/build"

BINUTILS_VERSION=2.26
GCC_VERSION=5.3.0
YAMS_VERSION=1.4.1

mkdir "$OSM_DIR"
cd "$OSM_DIR"

{
    mkdir "$BUILD_DIR"
    cd "$BUILD_DIR"

    # Download.
    if [ "$INSTALL_MIPS_UTILS" = true ]; then
        wget http://ftp.gnu.org/gnu/binutils/binutils-$BINUTILS_VERSION.tar.gz
        wget ftp://ftp.mpi-sb.mpg.de/pub/gnu/mirror/gcc.gnu.org/pub/gcc/releases/gcc-$GCC_VERSION/gcc-$GCC_VERSION.tar.gz
    fi
    wget http://www.niksula.hut.fi/~buenos/dist/yams-$YAMS_VERSION.tar.gz

    if [ "$INSTALL_MIPS_UTILS" = true ]; then
        # Extract and build binutils for MIPS and x86_64.
        cd "$BUILD_DIR"
        tar zxvf binutils-$BINUTILS_VERSION.tar.gz
        mkdir build-binutils
        cd build-binutils
        CFLAGS='-Wno-unused-value -Wno-logical-not-parentheses' \
            ../binutils-$BINUTILS_VERSION/configure \
            --target=mips-elf --prefix="$OSM_DIR"
        make -j $NJOBS
        make install
        cd ..
        rm -rf build-binutils
        mkdir build-binutils
        cd build-binutils
        CFLAGS='-Wno-unused-value -Wno-logical-not-parentheses' \
            ../binutils-$BINUTILS_VERSION/configure \
            --target=x86_64-elf --prefix="$OSM_DIR"
        make -j $NJOBS
        make install

        # Extract and build gcc for MIPS and x86_64.
        cd "$BUILD_DIR"
        tar xf gcc-$GCC_VERSION.tar.gz

        # On Linux (and Maybe OS X), download GMP 4.3+, MPCGMP 4.2+,
        # MPFR 2.4.0+, and MPC 0.8.0+.  These will then be automatically built
        # before gcc.
        cd "gcc-$GCC_VERSION"
        ./contrib/download_prerequisites
        cd "$BUILD_DIR"

        mkdir build-gcc
        cd build-gcc
        ../gcc-$GCC_VERSION/configure --with-gnu-ld --with-gnu-as \
            --without-nls --enable-languages=c --disable-multilib \
            --disable-libssp --disable-libquadmath --target=mips-elf \
            --disable-shared --prefix="$OSM_DIR"
        make -j $NJOBS
        make install
        cd ..
        rm -rf build-gcc
        mkdir build-gcc
        cd build-gcc
        ../gcc-$GCC_VERSION/configure --with-gnu-ld --with-gnu-as \
            --without-nls --enable-languages=c --disable-multilib \
            --disable-libssp --disable-libquadmath --target=x86_64-elf \
            --disable-shared --prefix="$OSM_DIR"
        make -j $NJOBS
        make install
    fi

    # Extract and build YAMS.
    cd "$BUILD_DIR"
    tar xf yams-$YAMS_VERSION.tar.gz
    cd yams-$YAMS_VERSION

    # The current YAMS does not work with modernly put together ELF files, so we
    # patch its ELF parser to just ignore any non-basic headers.
    cd src
    patch <<"EOF"
--- elf.c	2016-02-03 13:40:38.988061863 +0100
+++ elf.c	2016-02-03 13:41:30.760430581 +0100
@@ -180,9 +180,9 @@
 	case PT_INTERP:
 	    printf("ELF: detected interpreter request\n");
 	    return LOADELF_FAILURE;
-	default:
-	    printf("ELF: unsupported program header\n");
-	    return LOADELF_FAILURE;
+
+    /* Other program headers indicate an incompatible file *or* a file
+       with extra headers.  Just ignore. */
 	}

 	/* to the next program header */
EOF
    cd ..

    # Actually build and install YAMS.
    ./configure --prefix=$OSM_DIR
    make -j $NJOBS
    make install

    # Write config files and utilities.
    cd "$OSM_DIR/bin"
    cat > yams-term <<"EOF"
EOFGEN
cat yams-term >> osm-install.sh
cat >> osm-install.sh <<"EOFGEN"
EOF
    cat > yams-sim <<"EOF"
EOFGEN
cat yams-sim >> osm-install.sh
cat >> osm-install.sh <<"EOFGEN"
EOF
    cat > yams-init <<"EOF"
EOFGEN
cat yams-init >> osm-install.sh
cat >> osm-install.sh <<"EOFGEN"
EOF
    cat > yams-files <<"EOF"
EOFGEN
cat yams-files >> osm-install.sh
cat >> osm-install.sh <<"EOFGEN"
EOF
    cat > yams-tfs <<"EOF"
EOFGEN
cat yams-tfs >> osm-install.sh
cat >> osm-install.sh <<"EOFGEN"
EOF
    chmod +x yams-term yams-sim yams-init yams-files yams-tfs
} | tee "$OSM_DIR/install_log" 2>&1
EOFGEN

chmod +x osm-install.sh
