
## Setup

1. setup python virtualenv and activate it
2. `pip install -r requirements.txt`
3. `python app.py`

## Environment vars

* `TOKEN` - Octave Read only token
* `USER` - Octave User (will not be needed long term)
* `COMPANY` - Octave company to use
* `MAPBOX_ACCESS` - mapbox access token

## Docker

### Build Image
`docker build --tag=mangoh_sun_run:0.0.1 .`

### Run Container
Run this command assuming that your current directory contains a file named `app.py`:
`docker run -p 8050:8050 -v `pwd`:/app/user mangoh_sun_run:0.0.1`

