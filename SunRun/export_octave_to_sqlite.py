#!/usr/bin/env python3

import sys
import os.path
import urllib
from datetime import datetime
from datetime import timedelta
from datetime import timezone
import sqlite3

import requests

import sun_run_settings

OCTAVE_API_URL = "https://octave-api.sierrawireless.io/v5.0"
OCTAVE_CREDENTIALS = {
    "X-Auth-Token": sun_run_settings.octave["token"],
    "X-Auth-User": sun_run_settings.octave["user"]
}

vancouver_utc_delta = timedelta(hours=-7)
vancouver_timezone = timezone(vancouver_utc_delta)
start_time = datetime(2019, 4, 14, 8, 55, tzinfo=vancouver_timezone)
end_time = start_time + timedelta(hours=3, minutes=50)

end_time_ms = int(end_time.timestamp() * 1000)
start_time_ms = int(start_time.timestamp() * 1000)


def get_device_names():
    tag_filter = "filter={}".format(
        urllib.parse.quote("tags.{}==\"true\"".format(sun_run_settings.octave["device_tag"])))
    url = "{}/{}/device/?{}".format(
        OCTAVE_API_URL, sun_run_settings.octave["company"], tag_filter)
    response_json = requests.get(url, headers=OCTAVE_CREDENTIALS).json()['body']

    device_names = [d["name"] for d in response_json]
    return device_names


def get_events_for_device_stream(device_name,
                                 stream_name,
                                 filter=None,
                                 sort=None,
                                 order=None,
                                 limit=None):
    """
    Get the events in the device stream according to the query parameters
    """
    url = "https://octave-api.sierrawireless.io/v5.0/{}/event/?path=/{}/devices/{}/{}".format(
        sun_run_settings.octave["company"], sun_run_settings.octave["company"], device_name, stream_name)
    if filter is not None:
        url = "{}&filter={}".format(url, urllib.parse.quote(filter))
    if sort is not None:
        url = "{}&sort={}".format(url, sort)
    if order is not None:
        if order != "asc" and order != "desc":
            raise Exception("Invalid order parameter: {}".format(order))
        url = "{}&order={}".format(url, order)
    if limit is not None:
        url = "{}&limit={}".format(url, limit)
    return requests.get(url, headers=OCTAVE_CREDENTIALS).json()['body']


class DataPoint:
    def __init__(self, generated_ts, data):
        self.generated_ts = generated_ts
        self.data = data

    def __repr__(self):
        return "{{'generated_ts': {}, 'data': {}}}".format(self.generated_ts, self.data)


def datapoints_from_events(events, data_path):
    res = list()
    for e in events:
        ts = e['generatedDate']
        data = e
        for p in data_path:
            data = data.get(p)
            if data is None:
                break
        if data is None:
            continue
        res.append(DataPoint(ts, data))
    return res


def fetch_device_data(device_name):
    octave_filter = "generatedDate>={}&&generatedDate<={}".format(start_time_ms, end_time_ms)

    def fetch(arg):
        (key, stream, path) = arg
        events = get_events_for_device_stream(
            device_name,
            stream,
            filter=octave_filter,
            sort="GeneratedDate",
            order="asc",
            limit=100000)
        return (key, datapoints_from_events(events, path))

    key_stream_and_path = [
        ("battery_percentages", "battper2c", ["elems", "battery", "BatteryPercentage"]),
        ("battery_currents", "batcurrent2c", ["elems", "battery", "mA"]),
        ("temperatures", "temp2c", ["elems", "yellowSensor", "bsec", "temperature"]),
        ("pressures", "pressure2c", ["elems", "yellowSensor", "bsec", "pressure"]),
        ("humidity_readings", "humidity2c", ["elems", "yellowSensor", "bsec", "humidity"]),
        ("light_readings", "light2c", ["elems", "yellowSensor", "light"]),
        ("locations", "location", ["elems", "location", "coordinates"]),
    ]
    device_data = {k: v for (k, v) in map(fetch, key_stream_and_path)}

    return device_data


if __name__ == "__main__":
    global output_dir
    output_dir = sys.argv[1]
    device_names = get_device_names()

    db_conn = sqlite3.connect(os.path.join(output_dir, "output.sqlite"))
    db_cursor = db_conn.cursor()
    db_cursor.execute("CREATE TABLE data (device text, type text, generated_ts integer, data text)")
    db_conn.commit()

    for dev in device_names:
        print("Populating DB for device: {}".format(dev))
        for (datatype, data) in fetch_device_data(dev).items():
            print("  Inserting items of type: {}".format(datatype))
            db_data = [(dev, datatype, dp.generated_ts, repr(dp.data)) for dp in data]
            db_cursor.executemany("INSERT INTO data VALUES (?, ?, ?, ?)", db_data)
            db_conn.commit()
    db_conn.close()
