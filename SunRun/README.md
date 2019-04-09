# mangOH Sun Run Tracker

## mangOH Yellow Setup

1. Get Legato 19.01.0 and apply the patches from mangOH's Legato_Patches repository
1. Get a newer version of the DataHub by running: `pushd $LEGATO_ROOT/apps/sample/dataHub && git checkout 19.02.0 && popd`
1. Get the latest mangOH code
1. Obtain the Octave applications `actions.YOUR_MODULE_TARGET.app` and
   `cloudInterface.YOUR_MODULE_TARGET.app` and place the files in `$MANGOH_ROOT/apps`
1. Modify the `mangOH.sdef` to add `apps/actions.${LEGATO_TARGET}.app` and
   `apps/cloudInterface.${LEGATO_TARGET].app` into the `applications` section of the file.
1. Build the system using `make YOUR_TARGET`
1. Update your module to the latest firmware
1. Follow the [instructions distributed with the Cypress WiFi
   driver](https://github.com/mangOH/mangOH/blob/master/linux_kernel_modules/cypwifi/README.md) on
   how to modify `/etc/network/interfaces` and `/etc/init.d/startlegato.sh` to get Cypress wifi to
   work.
1. Apply the `.update` file built above to the target.
1. `config set location:/ApiKey YOUR_COMBAIN_API_KEY`
1. `config set openWeatherMapAmbientTemperature:/ApiKey YOUR_OPENWEATHERMAP_API_KEY`


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
Following these instructions will launch the application. It will be listening on port 8050. The
`app.py` file is bind mounted into the container overriding the `app.py` that already exists in the
container. This means that the Docker image doesn't need to be re-built every time the code changes
during development.
1. `cp sun_run_settings.template.py sun_run_settings.py`
1. Fill in the required settings in `sun_run_settings.py`
1. `docker run -p 8050:8050 -v `pwd`/sun_run_settings.py:/app/sun_run_settings.py -v `pwd`/app.py:/app/app.py -v `pwd`/assets:/app/assets mangoh_sun_run:0.0.1`

### Deployment
To build and deploy, run `./deploy_to_server.sh`.  The "VPS Setup" steps are a prerequisite.
