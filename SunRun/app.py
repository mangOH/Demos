# -*- coding: utf-8 -*-
import dash
import dash_core_components as dcc
import dash_html_components as html
import plotly
import plotly.graph_objs as go
import requests
import urllib
from concurrent.futures import ThreadPoolExecutor, ProcessPoolExecutor
from dash.dependencies import Input, Output, State
from datetime import datetime
from flask_caching import Cache
from os import getenv
from time import sleep
import time
import calendar
from datetime import datetime
from datetime import timezone
from datetime import timedelta

vancouver_timezone = timezone(timedelta(hours=-7))
end_time = datetime.now(tz=vancouver_timezone)
start_time = end_time + timedelta(hours=-12)

end_time_ms = int (end_time.timestamp()*1000)
start_time_ms = int (start_time.timestamp()*1000)
time_delta = 10

def time_generator(start_time, end_time, minute_increment):
    minute = (int(start_time.minute / minute_increment) + 1) * minute_increment
    t = start_time +  timedelta(
        minutes=(minute - start_time.minute),
        seconds=-start_time.second, microseconds=-start_time.microsecond)

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
    print("times is {}".format(times))
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

print("start time: {}, end time: {}".format(datetime_to_nice_string(start_time), datetime_to_nice_string(end_time)))
print("gen_marker :{}".format(gen_marks(start_time, end_time, 10)))
print("time_generator :{}".format(time_generator(start_time, end_time, 10)))

external_stylesheets = ['https://stackpath.bootstrapcdn.com/bootstrap/4.1.3/css/bootstrap.min.css']

app = dash.Dash(__name__, external_stylesheets=external_stylesheets)

cache = Cache(app.server, config={
    'CACHE_TYPE': 'filesystem',
    'CACHE_DIR': 'cache-directory'
})

creds = {
    'X-Auth-Token': getenv('TOKEN', 'YOUR_OCTAVE_TOKEN'),
    'X-Auth-User': getenv('USER', 'YOUR_OCTAVE_USERNAME')
}

company = getenv('COMPANY', 'YOUR_COMPANY')
device_update_interval = int(getenv('DEVICE_UPDATE_INTERVAL', '20'))

mapbox_access_token = getenv('MAPBOX_ACCESS', 'YOUR_MAPBOX_API_KEY')

colors = {
    'background': '#111111',
    'text': '#7FDBFF'
}


def update_devices():
    global devices
    print('Updating Devices')
    url = 'https://octave-api.sierrawireless.io/v5.0/{}/device/?filter=tags.dashdemo%3D%3D%22true%22'.format(company)
    all_devs = [d for d in requests.get(url, headers=creds).json()['body']]
    for d in all_devs:
        print("found device{}".format(d["name"]))
        if 'location' not in d.keys():
            d.update(location={'lat': 49.172477, 'lon': -123.071298})
    devices = all_devs
    

def update_devices_every(period=device_update_interval):
    while True:
        update_devices()
        sleep(period)

update_devices()
executor = ThreadPoolExecutor(max_workers=1)
executor.submit(update_devices_every)
sleep(2) # make sure devices get loaded

@cache.memoize(timeout=2)
def get_events_for_device_stream(device_name, stream_name, my_filter, my_limit):
    url = 'https://octave-api.sierrawireless.io/v5.0/%s/event/?path=/%s/devices/%s/%s&filter=%s&limit=%s' % (company,company, device_name, stream_name, urllib.parse.quote(my_filter),my_limit)
    print("url test {}".format(url))
    return sorted([e for e in requests.get(url, headers=creds).json()['body']], key=lambda x: x['generatedDate'])

def get_battpercent_for_device(device_name):
    print("processing  battery{}".format(device_name))
    events = get_events_for_device_stream(device_name, 'battper2c', 'generatedDate>={}&&generatedDate<={}&&CONTAINS=BatteryPercentage'.format(start_time_ms, end_time_ms), 100000)
   
    def scatter_for_batterypercent():
        return go.Scatter(
            x=[datetime.fromtimestamp(e['generatedDate']/1000.0) for e in events],
            y=[e['elems']['battery']['BatteryPercentage'] for e in events],
            name='Battery Percentage' 
        )

    return [scatter_for_batterypercent()]

def get_battcurrent_for_device(device_name):
    print("processing  battery current{}".format(device_name))
    events = get_events_for_device_stream(device_name, 'batcurrent2c', 'generatedDate>={}&&generatedDate<={}&&CONTAINS=mA'.format(start_time_ms, end_time_ms), 100000)
   
    def scatter_for_battcurrent():
        return go.Scatter(
            x=[datetime.fromtimestamp(e['generatedDate']/1000.0) for e in events],
            y=[e['elems']['battery']['mA'] for e in events],
            name='Battery Current Consumption' 
        )

    return [scatter_for_battcurrent()]


def get_temp_for_device(device_name):
    print("processing  temperature{}".format(device_name))
    events = get_events_for_device_stream(device_name, 'temp2c', 'generatedDate>={}&&generatedDate<={}&&CONTAINS=temperature'.format(start_time_ms, end_time_ms), 100000)
   
    def scatter_for_temp():
        return go.Scatter(
            x=[datetime.fromtimestamp(e['generatedDate']/1000.0) for e in events],
            y=[e['elems']['yellowSensor']['bsec']['temperature'] for e in events],
            name='Ambient Temperature' 
        )

    return [scatter_for_temp()]

def get_pressure_for_device(device_name):
    print("processing  pressure{}".format(device_name))
    events = get_events_for_device_stream(device_name, 'pressure2c', 'generatedDate>={}&&generatedDate<={}&&CONTAIN=pressure'.format(start_time_ms, end_time_ms), 100000)
   
    def scatter_for_pressure():
        return go.Scatter(
            x=[datetime.fromtimestamp(e['generatedDate']/1000.0) for e in events],
            y=[e['elems']['yellowSensor']['bsec']['pressure'] for e in events],
            name='Ambient Pressure' 
        )

    return [scatter_for_pressure()]


def get_humidity_for_device(device_name):
    print("processing  humidity{}".format(device_name))
    events = get_events_for_device_stream(device_name, 'humidity2c', 'generatedDate>={}&&generatedDate<={}&&CONTAINS=humidity'.format(start_time_ms, end_time_ms), 100000)
   
    def scatter_for_humidity():
        return go.Scatter(
            x=[datetime.fromtimestamp(e['generatedDate']/1000.0) for e in events],
            y=[e['elems']['yellowSensor']['bsec']['humidity'] for e in events],
            name='Ambient Humidity' 
        )

    return [scatter_for_humidity()]

def get_airqual_for_device(device_name):
    print("processing  airqual{}".format(device_name))
    events = get_events_for_device_stream(device_name, 'iaq2c', 'generatedDate>={}&&generatedDate<={}&&CONTAINS=iaqValue'.format(start_time_ms, end_time_ms), 100000)
   
    def scatter_for_airquality():
        return go.Scatter(
            x=[datetime.fromtimestamp(e['generatedDate']/1000.0) for e in events],
            y=[e['elems']['yellowSensor']['bsec']['iaqValue'] for e in events],
            name='Ambient Air Quality Index' 
        )

    return [scatter_for_airquality()]



def get_map_data_from_devices(time_stamp):
    text = []
    lat = []
    lon = []
    for d in devices:
        device_name = d['name']
        events = get_events_for_device_stream(device_name, 'location', 'elems.location.coordinates.ts<={} && elems.location.coordinates.ts>={}'.format(time_stamp, start_time_ms), 1000)
        print("processing {} with {} events".format(device_name, len(events)))
        if len(events) >= 1:
            coords = events[-1]['elems']['location']['coordinates']
            ts = coords['ts']
#            text.append('{} at {}'.format(device_name, ts))
            text.append('{}'.format(device_name))
            lat.append(coords['lat'])
            lon.append(coords['lon'])
    return [dict(
        type = 'scattermapbox',
        lon = lon,
        lat = lat,
        text = text,
        mode = 'line+markers',
        marker = dict(size = 8)
        )]
def get_map_history_from_device(device_name):
    labels = []
    latitudes = []
    longitudes = []
    
    events = get_events_for_device_stream(device_name, 'location', 'elems.location.coordinates.ts>={} && elems.location.coordinates.ts<={}'.format(start_time_ms, end_time_ms), 10000)
    print("processing {} with {} events".format(device_name, len(events)))
    for e in events:
        coords = e['elems']['location']['coordinates']
        ts = utc_timestamp_to_local_datetime(coords['ts'])
        lat = coords['lat']
        lon = coords['lon']
        label = "{}, {} @ {}".format(lat, lon, ts)
        labels.append(label)
        latitudes.append(lat)
        longitudes.append(lon)
    return [dict(
        type = 'scattermapbox',
        lon = longitudes,
        lat = latitudes,
        text = labels,
        mode = 'lines+markers',
        #marker = dict(size = 8)
        marker={
            'size': 15,
            'opacity': 0.5,
            'line': {'width': 0.5, 'color': 'white'}
        }
        )]



def generate_layout():
    
    return html.Div(children=[

        html.H1(children='Vancouver Sun Run mangOH Yellow Tracking'),

        html.Div(children='''
           Continous tracking of runners using Sierra Wireless's WP, Legato and Octave technologies.
        '''),
        
        html.Div(children=[
            html.A("For more information visit mangoh.io", href='https://mangoh.io', target="_blank")
            ]),


        html.Div(className='row', children=[
            html.Div(className='col-12', children=[
                dcc.Graph(id='live-update-map'),
                #dcc.Slider(id='time-slider', min=start_time_ms, max=end_time_ms, marks={'{}'.format(gen_marks(start_time,end_time, time_delta))},value=start_time_ms, updatemode='mouseup'),
              #  dcc.Slider(id='time-slider', min=start_time_ms, max=end_time_ms, marks=gen_marks(start_time,end_time, time_delta),value=start_time_ms, updatemode='mouseup'),
                dcc.Slider(id='time-slider', min=start_time_ms, max=end_time_ms,value=start_time_ms, updatemode='mouseup'), 
               # html.Div(id='live', style={'margin-top': 20}
                dcc.Interval(
                    id='interval-component',
                    interval=10*1000,
                    n_intervals=0
                )
            ]),
        ]),
        html.Div(className='row', children=[
            html.Div(className='col-12', children=[
                dcc.Graph(id='history-location-map'),
           ]),
        ]),


        html.Div(className='row', children=[
            html.Div(className='col-6', children=[
                dcc.Graph(id='battpercent-time-series')
            ]),
            html.Div(className='col-6', children=[
                dcc.Graph(id='battcurrent-time-series')
            ]),
        ]),
        
        html.Div(className='row', children=[
            html.Div(className='col-6', children=[
                dcc.Graph(id='temp-time-series')
            ]),
            html.Div(className='col-6', children=[
                dcc.Graph(id='pressure-time-series')
            ]),
        ]),
       
	 html.Div(className='row', children=[
            html.Div(className='col-6', children=[
                dcc.Graph(id='humidity-time-series')
            ]),
            html.Div(className='col-6', children=[
                dcc.Graph(id='airqual-time-series')
            ]),
        ]),

    ])

app.layout = generate_layout()

app.title = 'My title'

@app.callback(
    Output('live-update-map', 'figure'),
    [Input('time-slider', 'value')],
#     Input('drop-down', 'value')],
    [State('live-update-map', 'relayoutData')]
)
#def update_location_map(n, mapdata):
def update_location_map(slider_timestamp, mapdata):
    print("The time is: {}".format(slider_timestamp))
    print(mapdata)
    fig = {
        'data': get_map_data_from_devices(slider_timestamp),
        'layout': {
            'autosize': True,
            'title': 'MangOHs tagged with dashdemo: true',
            'height': 800,
            'mapbox': {
                'center': {},
                'accesstoken': mapbox_access_token,
                'center': {
                    'lat': 49.172477,
                    'lon':-123.071298
                },
                'zoom': 8,
            },
            'uirevision': 1,
            'plot_bgcolor': colors['background'],
            'paper_bgcolor': colors['background'], 
            'font': {
                    'color': colors['text']
            },
            'hovermode':'closest'
        },
    }

    if not mapdata or 'mapbox.center' not in mapdata.keys(): mapdata = {}
    fig['layout']['mapbox']['center'] = mapdata.get('mapbox.center', { 'lat': 49.172477, 'lon':-123.071298 })
    fig['layout']['mapbox']['zoom'] = mapdata.get('mapbox.zoom', 15)
    fig['layout']['mapbox']['bearing'] = mapdata.get('mapbox.bearing', 0)
    fig['layout']['mapbox']['pitch'] = mapdata.get('mapbox.pitch', 0)
    return fig

@app.callback(
    Output('history-location-map', 'figure'),
    [Input('live-update-map', 'clickData')],
    [State('history-location-map', 'relayoutData')]
)
#def update_location_map(n, mapdata):
def update_location_history(clickData, mapdata):
    print("in update location history")
    if not clickData: return {}
    device_name = clickData['points'][0]['text']
    print(mapdata)
    location_data = get_map_history_from_device(device_name)
    print("location data {}".format(location_data))
    fig = {
        'data': location_data,
        'layout': {
            'autosize': True,
            'title': 'Location History',
            'mapbox': {
                'center': {},
                'accesstoken': mapbox_access_token,
                'center': {
                    'lat': 49.172477,
                    'lon':-123.071298
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

@app.callback(
    Output('battpercent-time-series', 'figure'),
    [Input('live-update-map', 'clickData')]
)
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

@app.callback(
   Output('battcurrent-time-series', 'figure'),
    [Input('live-update-map', 'clickData')]
)
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

@app.callback(
    Output('temp-time-series', 'figure'),
    [Input('live-update-map', 'clickData')]
)
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

@app.callback(
    Output('pressure-time-series', 'figure'),
    [Input('live-update-map', 'clickData')]
)

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

@app.callback(
    Output('humidity-time-series', 'figure'),
    [Input('live-update-map', 'clickData')]
)
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

@app.callback(
    Output('airqual-time-series', 'figure'),
    [Input('live-update-map', 'clickData')]
)
def update_airqual(clickData):
    if not clickData: return {}
    device_name = clickData['points'][0]['text']
    return update_airqual_common(device_name)



if __name__ == '__main__':
    app.run_server(host='10.1.70.13', debug=True)   
	
