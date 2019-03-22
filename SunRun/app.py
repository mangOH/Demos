#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Python built-in modules
from concurrent.futures import ThreadPoolExecutor, ProcessPoolExecutor
from datetime import datetime
from datetime import timedelta
from datetime import timezone
from os import getenv
import time
import urllib

# Dependencies from pip
from dash.dependencies import Input, Output, State
from flask_caching import Cache
import dash
import dash_core_components as dcc
import dash_html_components as html
import dash_bootstrap_components as dbc
import plotly
import plotly.graph_objs as go
import requests
import flask

# Local modules
import sun_run_settings


def time_generator(start_time, end_time, minute_increment):
    minute = (int(start_time.minute / minute_increment) + 1) * minute_increment
    t = start_time + timedelta(
        minutes=(minute - start_time.minute),
        seconds=-start_time.second,
        microseconds=-start_time.microsecond)

    delta = timedelta(minutes=minute_increment)
    while (t < end_time):
        yield t
        t += delta


def gen_marks(start_time, end_time, minute_increment):
    """
    Create a marker dict from start/end datetime and a minute increment

    NOTE: 60 % minute_increment should probably == 0
    """
    times = time_generator(start_time, end_time, minute_increment)
    marks = {int(t.timestamp()): '{:02d}:{:02d}'.format(t.hour, t.minute) for t in times}
    app.logger.warning("gen_marks gave: {}".format(marks))
    return marks


def generate_layout():
    return html.Div([
        dbc.Row([
            dbc.Col([
                dcc.Markdown("""
# Vancouver Sun Run mangOH Yellow Tracking
Continuous tracking of runners using Sierra Wireless's WP, Legato and Octave technologies.
For more information, visit [mangoh.io](https://mangoh.io).
                    """),
                dcc.Graph(id="live-update-map"),
                html.Div([
                    dcc.Slider(
                        id="time-slider",
                        min=start_time_ms / 1000,
                        max=end_time_ms / 1000,
                        value=start_time_ms / 1000,
                        marks=gen_marks(start_time, end_time, 15),
                        updatemode='mouseup'),
                ], style={"margin-bottom": 50}),
                dcc.Graph(id='history-location-map'),
            ]),
        ]),
        dbc.Row([
            dbc.Col([dcc.Graph(id='battpercent-time-series')]),
            dbc.Col([dcc.Graph(id='battcurrent-time-series')])
        ]),
        dbc.Row([
            dbc.Col([dcc.Graph(id='temp-time-series')]),
            dbc.Col([dcc.Graph(id='pressure-time-series')])
        ]),
        dbc.Row([
            dbc.Col([dcc.Graph(id='humidity-time-series')]),
            dbc.Col([dcc.Graph(id='airqual-time-series')])
        ]),
    ])


vancouver_utc_delta = timedelta(hours=-7)
vancouver_timezone = timezone(vancouver_utc_delta)
#end_time = datetime.now(tz=vancouver_timezone)
#start_time = end_time + timedelta(hours=-12)
start_time = datetime(2019, 3, 17, 9, tzinfo=vancouver_timezone)
end_time = start_time + timedelta(hours=3)

end_time_ms = int(end_time.timestamp() * 1000)
start_time_ms = int(start_time.timestamp() * 1000)
time_delta = 10

external_stylesheets = [dbc.themes.BOOTSTRAP]

server = flask.Flask(__name__)

pathname_params = dict()
if sun_run_settings.hosting_path is not None:
    pathname_params["routes_pathname_prefix"] = "/"
    pathname_params["requests_pathname_prefix"] = "/{}/".format(sun_run_settings.hosting_path)
app = dash.Dash(__name__, external_stylesheets=external_stylesheets, server=server, **pathname_params)
app.title = 'mangOH Sun Run'
app.layout = generate_layout()

cache = Cache(app.server, config={'CACHE_TYPE': 'filesystem', 'CACHE_DIR': 'cache-directory'})

creds = {
    'X-Auth-Token': getenv('OCTAVE_TOKEN', sun_run_settings.octave_token),
    'X-Auth-User': getenv('OCTAVE_USER', sun_run_settings.octave_user)
}

company = getenv('COMPANY', sun_run_settings.octave_company)
device_update_interval = int(getenv('DEVICE_UPDATE_INTERVAL', '60'))

mapbox_access_token = getenv('MAPBOX_ACCESS', sun_run_settings.mapbox_access_token)

colors = {'background': '#111111', 'text': '#7FDBFF'}


def utc_timestamp_to_local_datetime(utc_ts_ms):
    """
    Create a datetime from a UTC unix timestamp in ms
    """
    return datetime.fromtimestamp(utc_ts_ms / 1000.0, vancouver_timezone)


def datetime_to_datetime_string(dt):
    """
    Produces a nicely formatted datetime string

    YYYY-MM-DD hh:mm:ss-hhmm
    """
    return dt.strftime("%Y-%m-%d %H:%M:%S%z")

def datetime_to_time_string(dt):
    """
    Produces a nicely formatted time string

    hh:mm:ss
    """
    return dt.strftime("%H:%M:%S")


# TODO: Currently, only the "name" property of the devices in the devices list
# is used to construct a query for the map data. As a result, "devices" could
# be a much smaller/simpler object. Really, it just needs to be device_names.
def update_devices():
    global devices
    url = 'https://octave-api.sierrawireless.io/v5.0/{}/device/?filter=tags.{}%3D%3D%22true%22'.format(
        sun_run_settings.octave_company, sun_run_settings.octave_device_tag)
    all_devs = [d for d in requests.get(url, headers=creds).json()['body']]
    for d in all_devs:
        if 'location' not in d.keys():
            d.update(location={'lat': 49.172477, 'lon': -123.071298})
    devices = all_devs
    app.logger.warning("Updating devices to: {}".format(", ".join([d["name"] for d in devices])))


def run_periodically(fn, period=timedelta(seconds=20)):
    """
    Run a given function in a loop with a delay before each subsequent run
    """
    while True:
        fn()
        time.sleep(period.total_seconds())


def get_events_for_device_stream(device_name, stream_name, filter=None, sort=None, order=None, limit=None):
    """
    Get the events in the device stream according to the query parameters
    """
    url = "https://octave-api.sierrawireless.io/v5.0/{}/event/?path=/{}/devices/{}/{}".format(
        company, company, device_name, stream_name)
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
    app.logger.warning("get_events_for_device_stream: {}".format(url))
    return requests.get(url, headers=creds).json()['body']


class DataPoint:
    def __init__(self, generated_ts, data):
        self.generated_ts = generated_ts
        self.data = data


@cache.memoize(timeout=20)
def datapoints_from_events(events, data_path):
    res = list()
    for e in events:
        ts = e['generatedDate']
        data = e
        for p in data_path:
            data = data.get(p)
            if data is None:
                app.logger.debug("Found bad data. Couldn't access path {} in event {}".format("/".join(data_path),  e))
                break
        if data is None:
            continue
        res.append(DataPoint(ts, data))
    return res


def fetch_device_data(device_name):
    device_data = dict()
    octave_filter = "generatedDate>={}&&generatedDate<={}".format(start_time_ms, end_time_ms)

    batt_percentage_events = get_events_for_device_stream(device_name, "battper2c", filter=octave_filter, sort="GeneratedDate", order="asc", limit=100000)
    device_data["battery_percentages"] = datapoints_from_events(batt_percentage_events, ["elems", "battery", "BatteryPercentage"])

    batt_current_events = get_events_for_device_stream(device_name, "batcurrent2c", filter=octave_filter, sort="GeneratedDate", order="asc", limit=100000)
    device_data["battery_currents"] = datapoints_from_events(batt_current_events, ["elems", "battery", "mA"])

    temperature_events = get_events_for_device_stream(device_name, "temp2c", filter=octave_filter, sort="GeneratedDate", order="asc", limit=100000)
    device_data["temperatures"] = datapoints_from_events(temperature_events, ["elems", "yellowSensor", "bsec", "temperature"])

    pressure_events = get_events_for_device_stream(device_name, "pressure2c", filter=octave_filter, sort="GeneratedDate", order="asc", limit=100000)
    device_data["pressures"] = datapoints_from_events(pressure_events, ["elems", "yellowSensor", "bsec", "pressure"])

    humidity_events = get_events_for_device_stream(device_name, "humidity2c", filter=octave_filter, sort="GeneratedDate", order="asc", limit=100000)
    device_data["humidity_readings"] = datapoints_from_events(humidity_events, ["elems", "yellowSensor", "bsec", "humidity"])

    iaq_events = get_events_for_device_stream(device_name, "iaq2c", filter=octave_filter, sort="GeneratedDate", order="asc", limit=100000)
    device_data["iaq_readings"] = datapoints_from_events(iaq_events, ["elems", "yellowSensor", "bsec", "iaqValue"])

    location_events = get_events_for_device_stream(device_name, "location", filter=octave_filter, sort="GeneratedDate", order="asc", limit=100000)
    device_data["locations"] = datapoints_from_events(location_events, ["elems", "location", "coordinates"])

    return device_data


def get_map_data_from_devices(timestamp_s):
    text = []
    lat = []
    lon = []
    for d in devices:
        device_name = d['name']
        events = get_events_for_device_stream(
            device_name, 'location',
            filter="elems.location.coordinates.ts<={} && elems.location.coordinates.ts>={}".format(timestamp_s * 1000, start_time_ms),
            sort="elems.location.coordinates.ts", order="desc", limit=1)
        if events:
            coords = events[0]['elems']['location']['coordinates']
            dt = utc_timestamp_to_local_datetime(coords['ts'])
            text.append("{} @ {}".format(device_name, datetime_to_time_string(dt)))
            lat.append(coords['lat'])
            lon.append(coords['lon'])
    return [
        dict(
            type='scattermapbox',
            lon=lon,
            lat=lat,
            text=text,
            mode='line+markers',
            marker=dict(size=8))
    ]


def create_scattermapbox_data(locations):
    labels = []
    latitudes = []
    longitudes = []
    for l in locations:
        ts = utc_timestamp_to_local_datetime(l.generated_ts)
        lat = l.data["lat"]
        lon = l.data["lon"]
        label = datetime_to_time_string(ts)
        labels.append(label)
        latitudes.append(lat)
        longitudes.append(lon)
    return [
        dict(
            type='scattermapbox',
            lon=longitudes,
            lat=latitudes,
            text=labels,
            mode='lines+markers',
            marker={'size': 15,
                    'opacity': 0.5,
                    'line': {
                        'width': 0.5,
                        'color': 'white'
                    }})
    ]


@app.callback(
    Output('live-update-map', 'figure'),
    [Input('time-slider', 'value')],
    [State('live-update-map', 'relayoutData')])
def update_location_map(slider_timestamp, mapdata):
    fig = {
        'data': get_map_data_from_devices(slider_timestamp),
        'layout': {
            'autosize': True,
            'title': 'Runners Carrying mangOH Yellow',
            'height': 800,
            'mapbox': {
                'accesstoken': mapbox_access_token,
            },
            'uirevision': 1,
            'plot_bgcolor': colors['background'],
            'paper_bgcolor': colors['background'],
            'font': {
                'color': colors['text']
            },
            'hovermode': 'closest'
        },
    }

    if not mapdata or 'mapbox.center' not in mapdata.keys(): mapdata = {}
    fig['layout']['mapbox']['center'] = mapdata.get('mapbox.center',
                                                    {'lat': 49.281191,
                                                     'lon': -123.125991})
    fig['layout']['mapbox']['zoom'] = mapdata.get('mapbox.zoom', 13)
    fig['layout']['mapbox']['bearing'] = mapdata.get('mapbox.bearing', 0)
    fig['layout']['mapbox']['pitch'] = mapdata.get('mapbox.pitch', 0)
    return fig


def update_location_history(locations):
    fig = {
        'data': create_scattermapbox_data(locations),
        'layout': {
            'autosize': True,
            'title': 'Location History',
            'mapbox': {
                'accesstoken': sun_run_settings.mapbox_access_token,
                'center': {
                    'lat': 49.172477,
                    'lon': -123.071298
                },
                'zoom': 8,
            },
            'uirevision': 1,
            'plot_bgcolor': colors['background'],
            'paper_bgcolor': colors['background'],
            'font': {
                'color': colors['text']
            },
        },
    }
    return fig


def create_scatterplot(data_points, title):
    x = list()
    y = list()
    for d in data_points:
        # It seems that the graphs will show the UTC time if the datetime object is timezone aware,
        # so just construct a naive object which has been adjusted to Vancouver time.
        x.append(datetime.fromtimestamp(d.generated_ts / 1000.0) + vancouver_utc_delta)
        y.append(d.data)
    return [go.Scatter(x=x, y=y, name=title)]


def generic_update_scatterplot(datapoints, graph_title_prefix, data_description):
    return {
        'data': create_scatterplot(datapoints, data_description),
        'layout': {
            'title': "{} for {}".format(graph_title_prefix, data_description),
            'plot_bgcolor': colors['background'],
            'paper_bgcolor': colors['background'],
            'font': {
                'color': colors['text']
            }
        }
    }


@app.callback(
    [Output('history-location-map', 'figure'),
     Output('battpercent-time-series', 'figure'),
     Output('battcurrent-time-series', 'figure'),
     Output('temp-time-series', 'figure'),
     Output('pressure-time-series', 'figure'),
     Output('humidity-time-series', 'figure'),
     Output('airqual-time-series', 'figure')],
    [Input('live-update-map', 'clickData')])
def selected_runner_callback(clickData):
    app.logger.debug("In selected_runner_callback({})".format(clickData))
    if not clickData: return ({}, {}, {}, {}, {}, {}, {})
    device_name = clickData['points'][0]['text'].split(" @ ")[0]
    device_data = fetch_device_data(device_name)
    return (update_location_history(device_data["locations"]),
            generic_update_scatterplot(device_data["battery_percentages"], "Battery Percentage", device_name),
            generic_update_scatterplot(device_data["battery_currents"], "Battery Current Consumption", device_name),
            generic_update_scatterplot(device_data["temperatures"], "Temperature", device_name),
            generic_update_scatterplot(device_data["pressures"], "Air Pressure", device_name),
            generic_update_scatterplot(device_data["humidity_readings"], "Humidity", device_name),
            generic_update_scatterplot(device_data["iaq_readings"], "Air Quality", device_name))


app.logger.warning("start time: {}, end time: {}".format(
    datetime_to_datetime_string(start_time), datetime_to_datetime_string(end_time)))

devices = None
executor = ThreadPoolExecutor(max_workers=1)
executor.submit(run_periodically, update_devices, period=timedelta(seconds=device_update_interval))
while (devices is None):
    time.sleep(2)  # make sure devices get loaded

if __name__ == '__main__':
    app.run_server(host='0.0.0.0', port=8050, debug=True)
