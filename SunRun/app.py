#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Python built-in modules
from datetime import datetime
from datetime import timedelta
from datetime import timezone
import time

# Dependencies from pip
from dash.dependencies import Input, Output, State
import dash
import dash_core_components as dcc
import dash_html_components as html
import dash_bootstrap_components as dbc
import plotly
import plotly.graph_objs as go
import flask

# Local modules
import sun_run_settings
from app_cache import cache


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
                dcc.Markdown("# Vancouver Sun Run mangOH Yellow Tracking"),
                html.Div(
                    [
                        dcc.Markdown("""
Real-time tracking of Sierra Wireless employee participants in the 2019 [Vancouver Sun
Run](https://vancouversunrun.com) on Sunday, April 14th at 9 a.m.

The trackers are built with a [mangOH Yellow](https://mangoh.io/mangoh-yellow) and Sierra Wireless
technologies ([learn more](https://mangoh.io/project/vancouver-sun-run-2019-runner-live-tracking))

Register for the [mangOH Yellow Beta Giveaway
Contest](https://mangoh.io/project/mangoh-yellow-beta-giveaway-2019) for a chance to win a mangOH
Yellow.
                        """)
                    ],
                    style={"font-size": "1.125em"}
                )
            ]),
        ]),
        dbc.Row([
            dbc.Col([
                html.A(
                    [html.Img(src=app.get_asset_url("mangoh_320.png"))],
                    href="https://mangoh.io"),
            ], style={"text-align": "center"}),
            dbc.Col([
                html.A(
                    [html.Img(src=app.get_asset_url('swi_320.png'))],
                    href="https://sierrawireless.com"),
            ], style={"text-align": "center"}),
            dbc.Col([
                html.A(
                    [html.Img(src=app.get_asset_url(sun_run_settings.partner["image"]))],
                    href=sun_run_settings.partner["link"]),
            ], style={"text-align": "center"}),
        ]),
        dbc.Row([
            dbc.Col([
                dcc.Checklist(
                    id="auto-update-checklist",
                    labelStyle={"font-weight": "bold"},
                    inputStyle={"margin-left": "10px", "margin-right": "5px"},
                    options=[{"label": "Update Automatically", "value": "update-automatically"}],
                    values=["update-automatically"]),
                html.Div([
                    dcc.Slider(
                        id="time-slider",
                        min=start_time_ms / 1000,
                        max=end_time_ms / 1000,
                        value=start_time_ms / 1000,
                        marks=gen_marks(start_time, end_time, 15),
                        updatemode='mouseup'),
                ],
                         style={"margin-bottom": 50}),
                dcc.Graph(id="live-update-map", config={"scrollZoom": True}),
                dcc.Graph(id='history-location-map', config={"scrollZoom": True}),
            ])
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
        dcc.Interval(id='live-update-interval', interval=30*1000),
    ])


vancouver_utc_delta = timedelta(hours=-7)
vancouver_timezone = timezone(vancouver_utc_delta)
#start_time = datetime.now(tz=vancouver_timezone) - timedelta(hours=1)
start_time = datetime(2019, 4, 14, 8, 55, tzinfo=vancouver_timezone)
end_time = start_time + timedelta(hours=3, minutes=50)

end_time_ms = int(end_time.timestamp() * 1000)
start_time_ms = int(start_time.timestamp() * 1000)
time_delta = 10

external_stylesheets = [dbc.themes.BOOTSTRAP]

server = flask.Flask(__name__)
server.config["PROPAGATE_EXCEPTIONS"] = True

pathname_params = dict()
if sun_run_settings.hosting_path is not None:
    pathname_params["routes_pathname_prefix"] = "/"
    pathname_params["requests_pathname_prefix"] = "/{}/".format(sun_run_settings.hosting_path)
app = dash.Dash(
    __name__, external_stylesheets=external_stylesheets, server=server, **pathname_params)
app.title = 'mangOH Sun Run'
app.layout = generate_layout()

cache.init_app(app.server, config={'CACHE_TYPE': 'filesystem', 'CACHE_DIR': 'cache-directory'})



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


def get_map_data_from_devices(timestamp_s):
    loc_data = backend.get_locations_at_time(timestamp_s * 1000, start_time_ms)
    text = []
    customdata = []
    lat = []
    lon = []
    for (device_name, ts, lat_val, lon_val) in loc_data:
        runner_name = sun_run_settings.device_name_to_user_name.get(device_name, device_name)
        dt = utc_timestamp_to_local_datetime(ts)
        text.append("{} @ {}".format(runner_name, datetime_to_time_string(dt)))
        customdata.append(device_name)
        lat.append(lat_val)
        lon.append(lon_val)
    return [
        dict(
            type='scattermapbox',
            lon=lon,
            lat=lat,
            text=text,
            customdata=customdata,
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
    Output("time-slider", "value"),
    [Input("auto-update-checklist", "values"),
     Input("live-update-interval", "n_intervals")]
)
def interval_callback(checklist_values, n_intervals):
    if len(checklist_values) == 0:
        raise dash.exceptions.PreventUpdate("Skipping interval based on checkbox")
    now = datetime.now(tz=vancouver_timezone)
    now_s = int(now.timestamp())
    def clamp(value, lower_bound, upper_bound):
        return max(min(value, upper_bound), lower_bound)
    return clamp(now_s, start_time_ms / 1000 , end_time_ms / 1000)


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
                'accesstoken': sun_run_settings.mapbox_access_token,
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


def update_location_history(locations, device_description):
    fig = {
        'data': create_scattermapbox_data(locations),
        'layout': {
            'autosize': True,
            'title': "Location History for {}".format(device_description),
            'mapbox': {
                'accesstoken': sun_run_settings.mapbox_access_token,
                'center': {
                    'lat': 49.281191,
                    'lon': -123.125991
                },
                'zoom': 12,
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


def generic_update_scatterplot(datapoints, graph_title_prefix, data_description, yaxis_label):
    return {
        'data': create_scatterplot(datapoints, data_description),
        'layout': {
            'title': "{} for {}".format(graph_title_prefix, data_description),
            'plot_bgcolor': colors['background'],
            'paper_bgcolor': colors['background'],
            'font': {
                'color': colors['text']
            },
            'yaxis': {'title': yaxis_label},
        }
    }


@app.callback([
    Output('history-location-map', 'figure'), Output('battpercent-time-series', 'figure'),
    Output('battcurrent-time-series', 'figure'), Output('temp-time-series', 'figure'),
    Output('pressure-time-series', 'figure'), Output('humidity-time-series', 'figure'),
    Output('airqual-time-series', 'figure')
], [Input('live-update-map', 'clickData')])
def selected_runner_callback(clickData):
    app.logger.debug("In selected_runner_callback({})".format(clickData))
    if not clickData: return ({}, {}, {}, {}, {}, {}, {})
    device_name = clickData['points'][0]['customdata']
    device_data = backend.fetch_device_data(device_name, start_time_ms, end_time_ms)
    runner_name = sun_run_settings.device_name_to_user_name.get(device_name, device_name)
    return (update_location_history(device_data["locations"], runner_name),
            generic_update_scatterplot(device_data["battery_percentages"], "Battery Percentage",
                                       runner_name, "%"),
            generic_update_scatterplot(device_data["battery_currents"],
                                       "Battery Current Consumption", runner_name, "mAh"),
            generic_update_scatterplot(device_data["temperatures"], "Temperature", runner_name, "°C"),
            generic_update_scatterplot(device_data["pressures"], "Air Pressure", runner_name, "Pa"),
            generic_update_scatterplot(device_data["humidity_readings"], "Humidity", runner_name, "%"),
            generic_update_scatterplot(device_data["light_readings"], "Light Level", runner_name, "nW/cm²"))


app.logger.warning("start time: {}, end time: {}".format(
    datetime_to_datetime_string(start_time), datetime_to_datetime_string(end_time)))


backend = None
if sun_run_settings.backend == "octave":
    from backend_octave import BackendOctave
    backend = BackendOctave(app, sun_run_settings.octave)
elif sun_run_settings.backend == "sqlite":
    from backend_sqlite import BackendSqlite
    backend = BackendSqlite(app, sun_run_settings.sqlite)
else:
    print("Invalid backend specified in sun_run_settings: {}".format(sun_run_settings.backend))
    sys.exit(1)


if __name__ == '__main__':
    app.run_server(host='0.0.0.0', port=8050, debug=True)
