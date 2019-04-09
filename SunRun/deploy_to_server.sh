#!/bin/sh

if [ "$#" -ne 1 ]
then
    echo "Expected one parameter - the version number of mangoh_sun_run image to build/deploy" > /dev/stderr
    exit 1
fi

VERSION=$1
SERVER=iotlabs.mangoh.io

set -x

docker build --tag=mangoh_sun_run:$VERSION . || exit 1
docker save mangoh_sun_run:$VERSION | xz -0 > mangoh_sun_run-${VERSION}.tar.xz || exit 2
scp mangoh_sun_run-${VERSION}.tar.xz sun_run_settings.py nginx.conf root@${SERVER}:~/sunrun || exit 3
scp mangoh-sun-run.service nginx-reverse-proxy.service root@${SERVER}:/etc/systemd/system/ || exit 4

ssh root@${SERVER} <<EOF
VERSION=$VERSION
cd ~/sunrun || exit 1
xzcat mangoh_sun_run-${VERSION}.tar.xz | docker load || exit 2
docker tag mangoh_sun_run:${VERSION} mangoh_sun_run:latest || exit 3
systemctl daemon-reload || exit 4
systemctl stop nginx-reverse-proxy.service mangoh-sun-run.service || exit 5
systemctl enable mangoh-sun-run.service nginx-reverse-proxy.service || exit 6
systemctl start mangoh-sun-run.service || exit 7
sleep 5 || exit 8
systemctl start nginx-reverse-proxy.service || exit 9
EOF
