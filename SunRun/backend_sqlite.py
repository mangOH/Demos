#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Python built-in modules
import sqlite3
import ast

# Local modules
from backend_common import DataPoint

class BackendSqlite():
    def __init__(self, app, settings):
        self.app = app
        self.db_conn = sqlite3.connect(settings["database"])
        self.db_cursor = self.db_conn.cursor()


    def get_locations_at_time(self, timestamp_ms, min_timestamp_ms):
        ret = []
        for row in self.db_cursor.execute(
            "SELECT device, max(generated_ts), data FROM data where type='locations' AND generated_ts >= ? AND generated_ts <= ? GROUP BY device", (min_timestamp_ms, timestamp_ms)):
            (device_name, ts, data) = row
            data_dict = ast.literal_eval(data)
            lat = data_dict["lat"]
            lon = data_dict["lon"]
            ret.append((device_name, ts, lat, lon))
        return ret


    def fetch_device_data(self, device_name, start_time_ms, end_time_ms):
        device_data = dict()
        for row in self.db_cursor.execute(
                "SELECT type, generated_ts, data from data where device = ? AND generated_ts >= ? AND generated_ts <= ?",
                (device_name, start_time_ms, end_time_ms)):
            (data_type, ts, data) = row
            device_data.setdefault(data_type, []).append(DataPoint(ts, ast.literal_eval(data)))
        return device_data
