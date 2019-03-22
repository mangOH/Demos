
## Setup

1. setup python virtualenv and activate it
2. `pip install -r requirements.txt`
3. `python app.py`

## Environment vars

* `TOKEN` - Octave Read only token
* `USER` - Octave User (will not be needed long term)
* `COMPANY` - Octave company to use
* `MAPBOX_ACCESS` - mapbox access token


## VPS Setup
Follow this list of instructions to prepare a VPS with a bare Debian 9 (stretch)
installation to run the application in a Docker container.
1. Use scp to append your ssh key to `/root/.ssh/authorized_keys` on the server
1. Create the file `/etc/apt/sources.list.d/docker.list` and write the content
   `deb https://apt.dockerproject.org/repo debian-stretch main`
1. Run `dpkg-reconfigure locales` and enable `en_CA.UTF-8` and `en_US.UTF-8` then set the default locale to `en_CA.UTF-8`
1. Run `apt-get install apt-transport-https`
1. Run `apt-get update`
1. Run `apt-get upgrade`
1. Run `apt-get install docker-engine`

## Docker

### Build Image
`docker build --tag=mangoh_sun_run:0.0.1 .`

### Run Container For Development
Following these instructions will launch the application. It will be listening
on port 8050. The `app.py` file is bind mounted into the container overriding
the `app.py` that already exists in the container. This means that the Docker
image doesn't need to be re-built every time the code changes during
development.
1. `cp sun_run_settings.template.py sun_run_settings.py`
1. Fill in the required settings in `sun_run_settings.py`
1. `docker run -p 8050:8050 -v sun_run_settings.py:/app/sun_run_settings.py -v app.py:/app/app.py mangoh_sun_run:0.0.1`

### Deployment
Follow the instructions below assuming that you have already followed the "VPS
Setup" instructions.
1. `dev$ docker save mangoh_sun_run:0.0.1 | xz -0 > mangoh_sun_run-0.0.1.tar.xz`
1. `dev$ scp mangoh_sun_run-0.0.1.tar.xz sun_run_settings.py nginx.conf root@iotlabs.mangoh.io:~/`
1. `dev$ scp mangoh-sun-run.service nginx-reverse-proxy.service root@iotlabs.mangoh.io:/etc/systemd/system/`
1. `dev$ ssh root@iotlabs.mangoh.io`
1. `iotlabs# xzcat mangoh_sun_run-0.0.1.tar.xz | docker load`
1. `iotlabs# docker tag mangoh_sun_run:0.0.1 mangoh_sun_run:latest`
1. `iotlabs# systemctl daemon-reload`
1. `iotlabs# systemctl stop nginx-reverse-proxy.service mangoh-sun-run.service`
1. `iotlabs# systemctl enable mangoh-sun-run.service nginx-reverse-proxy.service`
1. `iotlabs# systemctl start mangoh-sun-run.service`
