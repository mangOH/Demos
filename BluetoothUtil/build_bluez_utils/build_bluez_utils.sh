#!/bin/bash

# This script builds the bluetooth utility programs that are bundled with the bluetoothUtil app.
# This script expects that the compiler, linker, etc are available in your $PATH.

function usage {
    echo "Usage:"
    echo "  Get the upstream sources     --> $0 fetch"
    echo "  Apply patches to sources     --> $0 patch"
    echo "  Build the code               --> $0 build"
    echo "  Copy the binaries into place --> $0 build"
    echo "  All of the above at once     --> $0 all"
}

function do_fetch {
    echo "Fetching source code"

    # 5.10 is the first official build following commit id bc70450 which was the one that Vincent
    # built originally
    if [ ! -d "bluez" ]; then
        git clone --branch "5.10" https://git.kernel.org/pub/scm/bluetooth/bluez.git || command_fail
    else
        echo "bluez source code already fetched"
    fi

    if [ ! -d "dbus" ]; then
        git clone --branch "dbus-1.10.6" git://anongit.freedesktop.org/dbus/dbus || command_fail
    else
        echo "dbus source code already fetched"
    fi

    if [ ! -d "expat" ]; then
        git clone --branch "R_2_2_0" git://git.code.sf.net/p/expat/code_git expat || command_fail
    else
        echo "expat source code already fetched"
    fi
}

# Apply all patches in the form bluez-NN-patch_topic.patch and  dbus-NN-patch_topic.patch where NN
# is a digit in the range 0-9. Patches are applied in numeric order.
function do_patch {
    echo "Applying patches to bluez"
    for p in `ls bluez-[0-9][0-9]-*.patch 2>/dev/null`; do
        echo "  --> $p"
        (cd bluez && patch -p1 < ../$p) || command_fail
    done

    echo "Applying patches to dbus"
    for p in `ls dbus-[0-9][0-9]-*.patch 2>/dev/null`; do
        echo "  --> $p"
        (cd dbus && patch -p1 < ../$p) || command_fail
    done

    echo "Applying patches to expat"
    for p in `ls expat-[0-9][0-9]-*.patch 2>/dev/null`; do
        echo "  --> $p"
        (cd expat && patch -p1 < ../$p) || command_fail
    done
}

function do_build {
    echo "Building expat"
    (
        cd expat/expat && \
        ./buildconf.sh && \
        ./configure --build=x86_64-linux-gnu --host=arm-poky-linux-gnueabi && \
        make
    ) || command_fail

    echo "Building dbus"
    (
        cd dbus && \
        (export NOCONFIGURE=1; ./autogen.sh) && \
        (
            export CPPFLAGS="-I`pwd`/../expat/expat/lib";
            export LDFLAGS="-L`pwd`/../expat/expat/.libs";
            ./configure --build=x86_64-linux-gnu --host=arm-poky-linux-gnueabi
        ) && \
        make
    ) || command_fail

    echo "Building bluez"
    (
        cd bluez && \
        ./bootstrap && \
        (
            export CPPFLAGS="-I`pwd`/../dbus/dbus";
            export LDFLAGS="-L`pwd`/../dbus/dbus/.libs";
            ./configure --build=x86_64-linux-gnu --host=arm-poky-linux-gnueabi --disable-udev --disable-systemd --disable-obex
        ) && \
        make
    ) || command_fail
}

function do_copy {
    echo "Copying binaries into place"
    cp -a bluez/attrib/gatttool bluez/tools/hciattach bluez/tools/hciconfig bluez/tools/hcitool ../bin/ || command_fail
}

function command_fail {
    echo "Failed in ${BASH_SOURCE[0]}:${BASH_LINENO[0]}"
    exit 1
}

if [ "$#" -ne 1 ]; then
    usage
    exit 1;
fi

case "$1" in
    fetch)
        do_fetch
        ;;
    patch)
        do_patch
        ;;
    build)
        do_build
        ;;
    copy)
        do_copy
        ;;
    all)
        do_fetch
        do_patch
        do_build
        do_copy
        ;;
    *)
        echo "Invalid command \"$1\""
        usage
        exit 1
esac
