#!/bin/bash
# vim: set et ts=4 sw=4 sts=4 fdm=marker:
#
# Script to convert Fedora mingw64 packages to Debian packages usable for the
# pokefinder nightly build system.
#
# Work in progress, use at your own risk.
#
# -- Compyx
#
# Vastly improved by Lars Kellogg-Stedman <lars@oddbit.com>
#



show_help()
{
    echo "Usage: `basename $0` <rpm-file>"
    cat <<"EOF"

Convert an RPM file to a DEB file which can be used on the nightlies server.

This script will convert <rpm-file> to a .deb file with Debian-specific paths
so it can be used directly via dpkg to install on the nightlies server.
It will alter the internal directory structure of the package and update any
pkg-config files to point to the correct (Debian) paths of required files.

It will not check for any dependencies, or do any sanity checks. It also needs
root access to work. So you have been warned =)
EOF
}


# Check if we are root
check_root()
{
    if [ "$EUID" -ne 0 ]; then
        echo "Root access required, aborting."
        exit 1
    fi
}


# Entry point
#
# Check command line arguments
#
if [ -z "$1" ]; then
    show_help
    exit 1
fi
if [ "$1" = "-h"  -o "$1" = "--help" ]; then
    show_help
    exit 0
fi

# Are we root?
check_root


# Remember path to RPM and its name
rpmpath="$1"
rpmfile="${rpmpath##*/}"

# cut doesn't cut it when it comes to splitting on separator scanning from the
# end of a string, so we'll use Awk:
rpmversion=$(rpm -qp $rpmpath --qf "%{VERSION}")
rpmrelease=$(rpm -qp $rpmpath --qf "%{RELEASE}")
rpmname=$(rpm -qp $rpmpath --qf "%{NAME}")

# Fix release (strip off '.<fedora-version>.<arch>.rpm')
rpmrelease="${rpmrelease%%.*}"

# Debug info
echo "Working on RPM file $rpmfile"
echo "  name    : $rpmname"
echo "  version : $rpmversion"
echo "  release : $rpmrelease"
# Generate proper Debian package name."
debname="${rpmname}_${rpmversion}-${rpmrelease}_all.deb"
echo "  deb file: $debname"
# Generate .deb dirname
debdir="${debname%.deb}"

# Start actual work

# Use alien to generate .deb
echo -n "Running alien on $rpmpath ... "
alien --scripts --to-deb --bump 0 $rpmpath
if [ $? != 0 ]; then
    echo "Alien failed."
    exit 1
fi

cp $debname /tmp
# Sanity check:
if [ -f /tmp/$debname ]; then
    echo "OK"
else
    echo "fail"
    exit 1
fi

# Extract alien-generated .deb
echo -n "Extracting alien-generated files ... "
if [ -d /tmp/$debdir ]; then
    rm -rfd /tmp/$debdir
fi
dpkg-deb -R /tmp/$debname /tmp/$debdir
if [ "$?" -ne "0" ]; then
    echo "failed, aborting."
    exit 1
else
    echo "OK."
fi


# Change paths from Fedora to Debian
echo "Moving mingw files into proper Debian paths."
mv /tmp/$debdir/usr/x86_64-w64-mingw32/sys-root/mingw/* /tmp/$debdir/usr/x86_64-w64-mingw32


# Update pkg-config files
echo "Patching pkg-config files:"
for pc in /tmp/$debdir/usr/x86_64-w64-mingw32/lib/pkgconfig/*.pc; do
    echo "  patching $pc"
    sed -i 's@/sys-root/mingw@@' "$pc"
done


# Build deb package for use with Debian 
echo "Generating .deb package."
dpkg-deb --build /tmp/$debdir
echo "Copying to current dir."
cp -f /tmp/$debname .
