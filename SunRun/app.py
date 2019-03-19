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
import plotly
import plotly.graph_objs as go
import requests

# Local modules
import sun_run_settings


def generate_layout():
    return html.Div(children=[
        html.H1(children='Vancouver Sun Run mangOH Yellow Tracking'),
        html.Div(children='''
           Continous tracking of runners using Sierra Wireless's WP, Legato and Octave technologies.
        '''),
        html.Div(children=[
            html.A("For more information visit mangoh.io",
                   href='https://mangoh.io',
                   target="_blank")
        ]),
        html.Div(
            className='row',
            children=[
                html.Div(
                    className='col-12',
                    children=[
                        dcc.Graph(id='live-update-map'),
                        #dcc.Slider(id='time-slider', min=start_time_ms, max=end_time_ms, marks={'{}'.format(gen_marks(start_time,end_time, time_delta))},value=start_time_ms, updatemode='mouseup'),
                        #  dcc.Slider(id='time-slider', min=start_time_ms, max=end_time_ms, marks=gen_marks(start_time,end_time, time_delta),value=start_time_ms, updatemode='mouseup'),
                        dcc.Slider(
                            id='time-slider',
                            min=start_time_ms,
                            max=end_time_ms,
                            value=start_time_ms,
                            updatemode='mouseup'),
                        # html.Div(id='live', style={'margin-top': 20}
                        dcc.Interval(
                            id='interval-component', interval=10 * 1000, n_intervals=0)
                    ]),
            ]),
        html.Div(className='row',
                 children=[
                     html.Div(className='col-12', children=[dcc.Graph(id='history-location-map'),
                                                            ]),
                 ]),
        html.Div(
            className='row',
            children=[
                html.Div(className='col-6', children=[dcc.Graph(id='battpercent-time-series')]),
                html.Div(className='col-6', children=[dcc.Graph(id='battcurrent-time-series')]),
            ]),
        html.Div(className='row',
                 children=[
                     html.Div(className='col-6', children=[dcc.Graph(id='temp-time-series')]),
                     html.Div(className='col-6', children=[dcc.Graph(id='pressure-time-series')]),
                 ]),
        html.Div(className='row',
                 children=[
                     html.Div(className='col-6', children=[dcc.Graph(id='humidity-time-series')]),
                     html.Div(className='col-6', children=[dcc.Graph(id='airqual-time-series')]),
                 ]),
    ])


vancouver_timezone = timezone(timedelta(hours=-7))
end_time = datetime.now(tz=vancouver_timezone)
start_time = end_time + timedelta(hours=-12)

end_time_ms = int(end_time.timestamp() * 1000)
start_time_ms = int(start_time.timestamp() * 1000)
time_delta = 10

external_stylesheets = ['https://stackpath.bootstrapcdn.com/bootstrap/4.1.3/css/bootstrap.min.css']

app = dash.Dash(__name__, external_stylesheets=external_stylesheets)
app.title = 'mangOH Sun Run'
app.layout = generate_layout()

cache = Cache(app.server, config={'CACHE_TYPE': 'filesystem', 'CACHE_DIR': 'cache-directory'})

creds = {
    'X-Auth-Token': getenv('OCTAVE_TOKEN', sun_run_settings.octave_token),
    'X-Auth-User': getenv('OCTAVE_USER', sun_run_settings.octave_user)
}

company = getenv('COMPANY', sun_run_settings.octave_company)
device_update_interval = int(getenv('DEVICE_UPDATE_INTERVAL', '20'))

mapbox_access_token = getenv('MAPBOX_ACCESS', sun_run_settings.mapbox_access_token)

colors = {'background': '#111111', 'text': '#7FDBFF'}


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
    app.logger.warning("times is {}".format(times))
    return {int(t.timestamp()): '{:02d}:{:02d}'.format(t.hour, t.minute) for t in times}


def utc_timestamp_to_local_datetime(utc_ts_ms):
    """
    Create a datetime from a UTC unix timestamp in ms
    """
    return datetime.fromtimestamp(utc_ts_ms / 1000.0, vancouver_timezone)


def datetime_to_nice_string(dt):
    """
    Produces a nicely formatted datetime stringo

    YYYY-MM-DD hh:mm:ss-hhmm
    """
    return dt.strftime("%Y-%m-%d %H:%M:%S%z")


def update_devices():
    global devices
    url = 'https://octave-api.sierrawireless.io/v5.0/{}/device/?filter=tags.{}%3D%3D%22true%22'.format(
        sun_run_settings.octave_company, sun_run_settings.octave_device_tag)
    all_devs = [d for d in requests.get(url, headers=creds).json()['body']]
    for d in all_devs:
        if 'location' not in d.keys():
            d.update(location={'lat': 49.172477, 'lon': -123.071298})
    devices = all_devs
    app.logger.warning("Updating devices to: {}".format([d["name"] for d in devices]))


def run_periodically(fn, period=timedelta(seconds=20)):
    """
    Run a given function in a loop with a delay before each subsequent run
    """
    while True:
        fn()
        time.sleep(period.total_seconds())


@cache.memoize(timeout=2)
def get_events_for_device_stream(device_name, stream_name, my_filter, my_limit):
    url = 'https://octave-api.sierrawireless.io/v5.0/%s/event/?path=/%s/devices/%s/%s&filter=%s&limit=%s' % (
        company, company, device_name, stream_name, urllib.parse.quote(my_filter), my_limit)
    app.logger.warning("get_events_for_device_stream: {}".format(url))
    return sorted(
        [e for e in requests.get(url, headers=creds).json()['body']],
        key=lambda x: x['generatedDate'])


def create_scatterplot_for_device(device_name, stream_name, graph_name, data_path):
    events = get_events_for_device_stream(device_name, stream_name,
                                          "generatedDate>={}&&generatedDate<={}".format(
                                              start_time_ms, end_time_ms), 100000)
    x = list()
    y = list()
    for e in events:
        x_elem = datetime.fromtimestamp(e['generatedDate'] / 1000.0)
        y_elem = e
        bad_data = False
        for p in data_path:
            y_elem = y_elem.get(p)
            if y_elem is None:
                bad_data = True
                break
        if bad_data:
            app.logger.warning("Bad data  while creating scatterplot for {}/{}. e={}".format(
                device_name, stream_name, e))
            continue
        x.append(x_elem)
        y.append(y_elem)

    return [go.Scatter(x=x, y=y, name=graph_name)]


def get_battpercent_for_device(device_name):
    return create_scatterplot_for_device(device_name, "battper2c", "Battery Percentage",
                                         ["elems", "battery", "BatteryPercentage"])


def get_battcurrent_for_device(device_name):
    return create_scatterplot_for_device(device_name, "batcurrent2c", "Battery Current Consumption",
                                         ["elems", "battery", "mA"])


def get_temp_for_device(device_name):
    return create_scatterplot_for_device(device_name, "temp2c", "Ambient Temperature",
                                         ["elems", "yellowSensor", "bsec", "temperature"])


def get_pressure_for_device(device_name):
    return create_scatterplot_for_device(device_name, "pressure2c", "Atmospheric Pressure",
                                         ["elems", "yellowSensor", "bsec", "pressure"])


def get_humidity_for_device(device_name):
    return create_scatterplot_for_device(device_name, "humidity2c", "Humidity",
                                         ["elems", "yellowSensor", "bsec", "humidity"])


def get_airqual_for_device(device_name):
    return create_scatterplot_for_device(device_name, "iaq2c", "Air Quality",
                                         ["elems", "yellowSensor", "bsec", "iaqValue"])


def get_map_data_from_devices(time_stamp):
    text = []
    lat = []
    lon = []
    for d in devices:
        device_name = d['name']
        events = get_events_for_device_stream(
            device_name, 'location',
            'elems.location.coordinates.ts<={} && elems.location.coordinates.ts>={}'.format(
                time_stamp, start_time_ms), 1000)
        if len(events) >= 1:
            coords = events[-1]['elems']['location']['coordinates']
            ts = coords['ts']
            #            text.append('{} at {}'.format(device_name, ts))
            text.append('{}'.format(device_name))
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


def get_map_history_from_device(device_name):
    labels = []
    latitudes = []
    longitudes = []

    events = get_events_for_device_stream(
        device_name, 'location',
        'elems.location.coordinates.ts>={} && elems.location.coordinates.ts<={}'.format(
            start_time_ms, end_time_ms), 10000)
    for e in events:
        coords = e['elems']['location']['coordinates']
        ts = utc_timestamp_to_local_datetime(coords['ts'])
        lat = coords['lat']
        lon = coords['lon']
        label = "{}, {} @ {}".format(lat, lon, ts)
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
            #marker = dict(size = 8)
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
    #     Input('drop-down', 'value')],
    [State('live-update-map', 'relayoutData')])
#def update_location_map(n, mapdata):
def update_location_map(slider_timestamp, mapdata):
    fig = {
        'data': get_map_data_from_devices(slider_timestamp),
        'layout': {
            'autosize': True,
            'title': 'MangOHs tagged with {}: true'.format(sun_run_settings.octave_device_tag),
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


@app.callback(
    Output('history-location-map', 'figure'), [Input('live-update-map', 'clickData')],
    [State('history-location-map', 'relayoutData')])
#def update_location_map(n, mapdata):
def update_location_history(clickData, mapdata):
    if not clickData: return {}
    device_name = clickData['points'][0]['text']
    location_data = get_map_history_from_device(device_name)
    fig = {
        'data': location_data,
        'layout': {
            'autosize': True,
            'title': 'Location History',
            'mapbox': {
                'center': {},
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


def update_batterypercent_common(device_name):
    battpercent_data = get_battpercent_for_device(device_name)
    return {
        'data': battpercent_data,
        'layout': {
            'title': 'Batt Percent Data for "%s"' % device_name,
            'plot_bgcolor': colors['background'],
            'paper_bgcolor': colors['background'],
            'font': {
                'color': colors['text']
            }
        }
    }


@app.callback(Output('battpercent-time-series', 'figure'), [Input('live-update-map', 'clickData')])
def update_batterypercent(clickData):
    if not clickData: return {}
    device_name = clickData['points'][0]['text']
    return update_batterypercent_common(device_name)


def update_batterycurrent_common(device_name):
    battcurrent_data = get_battcurrent_for_device(device_name)
    return {
        'data': battcurrent_data,
        'layout': {
            'title': 'Batt Curent  Data for "%s"' % device_name,
            'plot_bgcolor': colors['background'],
            'paper_bgcolor': colors['background'],
            'font': {
                'color': colors['text']
            }
        }
    }


@app.callback(Output('battcurrent-time-series', 'figure'), [Input('live-update-map', 'clickData')])
def update_batterycurrent(clickData):
    if not clickData: return {}
    device_name = clickData['points'][0]['text']
    return update_batterycurrent_common(device_name)


def update_temp_common(device_name):
    temp_data = get_temp_for_device(device_name)
    return {
        'data': temp_data,
        'layout': {
            'title': 'Temp  Data for "%s"' % device_name,
            'plot_bgcolor': colors['background'],
            'paper_bgcolor': colors['background'],
            'font': {
                'color': colors['text']
            }
        }
    }


@app.callback(Output('temp-time-series', 'figure'), [Input('live-update-map', 'clickData')])
def update_temp(clickData):
    if not clickData: return {}
    device_name = clickData['points'][0]['text']
    return update_temp_common(device_name)


def update_pressure_common(device_name):
    pressure_data = get_pressure_for_device(device_name)
    return {
        'data': pressure_data,
        'layout': {
            'title': 'Pressure  Data for "%s"' % device_name,
            'plot_bgcolor': colors['background'],
            'paper_bgcolor': colors['background'],
            'font': {
                'color': colors['text']
            }
        }
    }


@app.callback(Output('pressure-time-series', 'figure'), [Input('live-update-map', 'clickData')])
def update_pressure(clickData):
    if not clickData: return {}
    device_name = clickData['points'][0]['text']
    return update_pressure_common(device_name)


def update_humidity_common(device_name):
    humidity_data = get_humidity_for_device(device_name)
    return {
        'data': humidity_data,
        'layout': {
            'title': 'Humidity  Data for "%s"' % device_name,
            'plot_bgcolor': colors['background'],
            'paper_bgcolor': colors['background'],
            'font': {
                'color': colors['text']
            }
        }
    }


@app.callback(Output('humidity-time-series', 'figure'), [Input('live-update-map', 'clickData')])
def update_humidity(clickData):
    if not clickData: return {}
    device_name = clickData['points'][0]['text']
    return update_humidity_common(device_name)


def update_airqual_common(device_name):
    airqual_data = get_airqual_for_device(device_name)
    return {
        'data': airqual_data,
        'layout': {
            'title': 'Air Quality  Data for "%s"' % device_name,
            'plot_bgcolor': colors['background'],
            'paper_bgcolor': colors['background'],
            'font': {
                'color': colors['text']
            }
        }
    }


@app.callback(Output('airqual-time-series', 'figure'), [Input('live-update-map', 'clickData')])
def update_airqual(clickData):
    if not clickData: return {}
    device_name = clickData['points'][0]['text']
    return update_airqual_common(device_name)


app.logger.warning("start time: {}, end time: {}".format(
    datetime_to_nice_string(start_time), datetime_to_nice_string(end_time)))
app.logger.warning("gen_marker :{}".format(gen_marks(start_time, end_time, 10)))
app.logger.warning("time_generator :{}".format(time_generator(start_time, end_time, 10)))

devices = None
executor = ThreadPoolExecutor(max_workers=1)
executor.submit(run_periodically, update_devices, period=timedelta(seconds=device_update_interval))
while (devices is None):
    time.sleep(2)  # make sure devices get loaded

if __name__ == '__main__':
    app.run_server(host='0.0.0.0', debug=True)
