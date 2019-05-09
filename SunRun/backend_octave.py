#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Python built-in modules
import time
import urllib
from datetime import timedelta
from concurrent.futures import ThreadPoolExecutor
import threading

# Dependencies from pip
import requests

# Local modules
from backend_common import DataPoint
from app_cache import cache


def run_periodically(fn, period=timedelta(seconds=20)):
    """
    Run a given function in a loop with a delay before each subsequent run
    """
    while True:
        fn()
        time.sleep(period.total_seconds())


@cache.memoize(timeout=20)
def datapoints_from_events(events, data_path, logger):
    res = list()
    for e in events:
        ts = e['generatedDate']
        data = e
        for p in data_path:
            data = data.get(p)
            if data is None:
                logger.debug("Found bad data. Couldn't access path {} in event {}".format(
                    "/".join(data_path), e))
                break
        if data is None:
            continue
        res.append(DataPoint(ts, data))
    return res


class BackendOctave():
    def __init__(self, app, settings):
        self.app = app
        self.app.logger.warning("Initializing Octave Backend")
        self.settings = settings
        self.creds = {"X-Auth-Token": self.settings["token"], "X-Auth-User": self.settings["user"]}
        self.devices = []
        executor = ThreadPoolExecutor(max_workers=1)
        executor.submit(run_periodically, self.__update_devices, period=timedelta(seconds=60))
        while self.devices is None:
            time.sleep(2)  # make sure devices get loaded


    def __update_devices(self):
        url = 'https://octave-api.sierrawireless.io/v5.0/{}/device/?filter=tags.{}%3D%3D%22true%22'.format(
            self.settings["company"], self.settings["device_tag"])
        all_devs = [d["name"] for d in requests.get(url, headers=self.creds).json()['body']]
        self.devices = all_devs
        self.app.logger.warning("Updating devices to: {}".format(", ".join([d for d in self.devices])))


    def get_events_for_device_stream(self,
                                     device_name,
                                     stream_name,
                                     filter=None,
                                     sort=None,
                                     order=None,
                                     limit=None):
        """
        Get the events in the device stream according to the query parameters
        """
        self.app.logger.warning("device_name: {}".format(device_name))
        url = "https://octave-api.sierrawireless.io/v5.0/{}/event/?path=/{}/devices/{}/{}".format(
            self.settings["company"], self.settings["company"], device_name, stream_name)
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
        self.app.logger.warning("get_events_for_device_stream: {}".format(url))
        return requests.get(url, headers=self.creds).json()['body']


    def get_locations_at_time(self, timestamp_ms, min_timestamp_ms):
        results = None
        with ThreadPoolExecutor() as ex:
            results = ex.map(
                lambda d: (
                    d,
                    self.get_events_for_device_stream(
                        d,
                        'location',
                        filter="elems.location.coordinates.ts<={} && elems.location.coordinates.ts>={}".format(
                            timestamp_ms, min_timestamp_ms),
                        sort="elems.location.coordinates.ts",
                        order="desc",
                        limit=1)),
                self.devices)
        ret = []
        for (device_name, events) in results:
            if events:
                e = events[0]
                ts = e['generatedDate']
                coords = e['elems']['location']['coordinates']
                lat = coords['lat']
                lon = coords['lon']
                ret.append((device_name, ts, lat, lon))
        return ret


    def fetch_device_data(self, device_name, start_time_ms, end_time_ms):
        octave_filter = "generatedDate>={}&&generatedDate<={}".format(start_time_ms, end_time_ms)

        def fetch(arg):
            (key, stream, path) = arg
            events = self.get_events_for_device_stream(
                device_name,
                stream,
                filter=octave_filter,
                sort="GeneratedDate",
                order="asc",
                limit=100000)
            return (key, datapoints_from_events(events, path, self.app.logger))

        key_stream_and_path = [
            ("battery_percentages", "battper2c", ["elems", "battery", "BatteryPercentage"]),
            ("battery_currents", "batcurrent2c", ["elems", "battery", "mA"]),
            ("temperatures", "temp2c", ["elems", "yellowSensor", "bsec", "temperature"]),
            ("pressures", "pressure2c", ["elems", "yellowSensor", "bsec", "pressure"]),
            ("humidity_readings", "humidity2c", ["elems", "yellowSensor", "bsec", "humidity"]),
            ("light_readings", "light2c", ["elems", "yellowSensor", "light"]),
            ("locations", "location", ["elems", "location", "coordinates"]),
        ]
        device_data = None
        with ThreadPoolExecutor() as ex:
            device_data = {k: v for (k, v) in ex.map(fetch, key_stream_and_path)}

        return device_data
