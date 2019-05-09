# Hosting path should be "sunrun" if the site is hosted at
# http://example.com/sunrun. If running as a developer without a reverse proxy
# infront, set hosting_path=None.
hosting_path = "sunrun"

backend = "sqlite"
#backend = "octave"

sqlite_settings = {
    "database": "your_db.sqlite"
}

# Sierra Wireless Octave settings used when backend == "octave"
octave_settings = {
    "user": "",
    "company": "",
    "token": "",
    "device_tag": "",
}

# Mapbox
mapbox_access_token = ""

# User mapping
# This is expected to be a dictionary mapping the device name to the runner's
# public facing name.
device_name_to_user_name = {
}
