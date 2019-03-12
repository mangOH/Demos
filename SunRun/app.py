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

def unix_seconds_now():
    now = time.gmtime()
    print("time now is {}".format(now))
    return calendar.timegm(now)
now_s = unix_seconds_now()
start_time_ms = (now_s - (60 * 60 * 19)) * 1000
end_time_ms = (now_s - (60 * 60 * 17)) * 1000

external_stylesheets = ['https://stackpath.bootstrapcdn.com/bootstrap/4.1.3/css/bootstrap.min.css']

app = dash.Dash(__name__, external_stylesheets=external_stylesheets)

cache = Cache(app.server, config={
    'CACHE_TYPE': 'filesystem',
    'CACHE_DIR': 'cache-directory'
})

creds = {
    'X-Auth-Token': getenv('TOKEN', 'YOURCRED'),
    'X-Auth-User': getenv('USER', 'YOURUSERNAME')
}

company = getenv('COMPANY', 'mango_dev')
device_update_interval = int(getenv('DEVICE_UPDATE_INTERVAL', '20'))

mapbox_access_token = getenv('MAPBOX_ACCESS', 'YOURACCESPASS')

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

def get_gyro_for_device(device_name):
    print("processing gyro {}".format(device_name))
    events = get_events_for_device_stream(device_name, 'gyro', 'generatedDate>={}&&generatedDate<={}'.format(start_time_ms, end_time_ms), 100000)

    def scatter_for_dimension(dim):
        return go.Scatter(
            x=[datetime.fromtimestamp(e['generatedDate']/1000.0) for e in events],
            #y=[e['elems']['yellowSensor']['gyro'][dim] for e in events],
            y=[e['elems']['yellowSensor']['gyro'][dim] for e in events],
            name='Gyro %s' % dim
        )

    return [scatter_for_dimension(i) for i in ['x', 'y', 'z']]

def get_accel_for_device(device_name):
    print("processing {}".format(device_name))
    events = get_events_for_device_stream(device_name, 'accel2c', 'generatedDate>={}&&generatedDate<={}'.format(start_time_ms, end_time_ms), 100000)

    def scatter_for_dimension(dim):
        return go.Scatter(
            x=[datetime.fromtimestamp(e['generatedDate']/1000.0) for e in events],
            y=[e['elems']['yellowSensor']['accel'][dim] for e in events],
            name='Accel %s' % dim
        )

    return [scatter_for_dimension(i) for i in ['x', 'y', 'z']]

def get_battpercent_for_device(device_name):
    print("processing  battery{}".format(device_name))
    events = get_events_for_device_stream(device_name, 'battper2c', 'generatedDate>={}&&generatedDate<={}'.format(start_time_ms, end_time_ms), 100000)
   
    def scatter_for_battery():
        return go.Scatter(
            x=[datetime.fromtimestamp(e['generatedDate']/1000.0) for e in events],
            y=[e['elems']['battery']['BatteryPercentage'] for e in events],
            name='Battery Percentage' 
        )

    return [scatter_for_battery()]

def get_battcurrent_for_device(device_name):
    print("processing  battery current{}".format(device_name))
    events = get_events_for_device_stream(device_name, 'batcurrent2c', 'generatedDate>={}&&generatedDate<={}'.format(start_time_ms, end_time_ms), 100000)
   
    def scatter_for_battcurrent():
        return go.Scatter(
            x=[datetime.fromtimestamp(e['generatedDate']/1000.0) for e in events],
            y=[e['elems']['battery']['mA'] for e in events],
            name='Battery Current Consumption' 
        )

    return [scatter_for_battcurrent()]


def get_temp_for_device(device_name):
    print("processing  temperature{}".format(device_name))
    events = get_events_for_device_stream(device_name, 'temp2c', 'generatedDate>={}&&generatedDate<={}'.format(start_time_ms, end_time_ms), 100000)
   
    def scatter_for_temp():
        return go.Scatter(
            x=[datetime.fromtimestamp(e['generatedDate']/1000.0) for e in events],
            y=[e['elems']['yellowSensor']['environment']['temp'] for e in events],
            name='Ambient Temperature' 
        )

    return [scatter_for_temp()]

def get_pressure_for_device(device_name):
    print("processing  pressure{}".format(device_name))
    events = get_events_for_device_stream(device_name, 'pressure2c', 'generatedDate>={}&&generatedDate<={}'.format(start_time_ms, end_time_ms), 100000)
   
    def scatter_for_pressure():
        return go.Scatter(
            x=[datetime.fromtimestamp(e['generatedDate']/1000.0) for e in events],
            y=[e['elems']['yellowSensor']['environment']['pressure'] for e in events],
            name='Ambient Pressure' 
        )

    return [scatter_for_pressure()]


def get_humidity_for_device(device_name):
    print("processing  humidity{}".format(device_name))
    events = get_events_for_device_stream(device_name, 'humidity', 'generatedDate>={}&&generatedDate<={}'.format(start_time_ms, end_time_ms), 100000)
   
    def scatter_for_humidity():
        return go.Scatter(
            x=[datetime.fromtimestamp(e['generatedDate']/1000.0) for e in events],
            y=[e['elems']['yellowSensor']['environment']['humidity'] for e in events],
            name='Ambient Humidity' 
        )

    return [scatter_for_humidity()]

def get_airqual_for_device(device_name):
    print("processing  airqual{}".format(device_name))
    events = get_events_for_device_stream(device_name, 'airqual2c', 'generatedDate>={}&&generatedDate<={}'.format(start_time_ms, end_time_ms), 100000)
   
    def scatter_for_airquality():
        return go.Scatter(
            x=[datetime.fromtimestamp(e['generatedDate']/1000.0) for e in events],
            y=[e['elems']['yellowSensor']['environment']['gas'] for e in events],
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
    
    events = get_events_for_device_stream(device_name, 'location', 'elems.location.coordinates.ts<={} && elems.location.coordinates.ts>={}'.format(start_time_ms, end_time_ms), 10000)
    print("processing {} with {} events".format(device_name, len(events)))
    for e in events:
        coords = e['elems']['location']['coordinates']
        ts = coords['ts']
        lat = coords['lat']
        lon = coords['lon']
        label = "{}, {} @ {}".format(lat, lon, ts)
        labels.append(label)
        latitudes.append(lat)
        longiitudes.append(lon)
    return [dict(
        type = 'scattermapbox',
        lon = longitudes,
        lat = latitudes,
        text = labels,
        mode = 'line+markers',
        marker = dict(size = 8)
        )]



def generate_layout():
    
    return html.Div(children=[
        html.Div(className='row', children=[
            html.Div(className='col-12', children=[
                dcc.Graph(id='live-update-map'),
                #dcc.RangeSlider(id='time-slider', min=start_time_ms, max=end_time_ms, step=step_time_ms, value=[start_time_ms, end_time_ms], updatemode='mouseup'),
                dcc.Slider(id='time-slider', min=start_time_ms, max=end_time_ms, value=start_time_ms, updatemode='mouseup'),
                #dcc.Dropdown(id='drop-down',options=[{'label':d['name']} for d in devices]),  
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
                #dcc.RangeSlider(id='time-slider', min=start_time_ms, max=end_time_ms, step=step_time_ms, value=[start_time_ms, end_time_ms], updatemode='mouseup'),
                #dcc.Slider(id='time-slider-2', min=start_time_ms, max=end_time_ms, value=start_time_ms, updatemode='mouseup'),
                #dcc.Dropdown(id='drop-down',options=[{'label':d['name']} for d in devices]),  
           ]),
        ]),

        #html.Div(className='row', children=[
        #    html.Div(className='col-6', children=[
        #        dcc.Graph(id='accel-time-series')
        #    ]),
            #html.Div(className='col-6', children=[
            #    dcc.Graph(id='accel-time-series')
            #]),
        #]),    

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
            'mapbox': {
                'center': {},
                'accesstoken': mapbox_access_token,
                'center': {
                    'lat': 49.172477,
                    'lon':-123.071298
                },
                'zoom': 12,
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
    if not clickData: return {}
    device_name = clickData['points'][0]['text']
    print(mapdata)
    location_data = get_map_history_from_device(device_name)
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


def update_gyro_common(device_name):
    gyro_data = get_gyro_for_device(device_name)
    return {
        'data': gyro_data,
        'layout': {
            'title': 'Gyro Data for "%s"' % device_name,
            'plot_bgcolor': colors['background'],
            'paper_bgcolor': colors['background'],
            'font': {
                    'color': colors['text']
            }
        }
    }

#@app.callback(
#    Output('gyro-time-series', 'figure'),
#    [Input('drop-down', 'value')]
#)
#def update_gyro_dropdown(device_name):
#    return update_gyro_common(device_name)


#@app.callback(
#    Output('gyro-time-series', 'figure'),
#    [Input('live-update-map', 'clickData')]
#)
#def update_gyro(clickData):
#    if not clickData: return {}
#    device_name = clickData['points'][0]['text']
#    return update_gyro_common(device_name)
#

#def update_accel_common(device_name):
#    accel_data = get_accel_for_device(device_name)
#    return {
#        'data': accel_data,
#        'layout': {
#            'title': 'Accel Data for "%s"' % device_name,
#            'plot_bgcolor': colors['background'],
#            'paper_bgcolor': colors['background'],
#            'font': {
#                    'color': colors['text']
#            }
#        }
#    }

#@app.callback(
#    Output('accel-time-series', 'figure'),
#    [Input('drop-down', 'value')]
#)
#def update_accel_dropdown(device_name):
#    return update_accel_common(device_name)

#@app.callback(
#    Output('accel-time-series', 'figure'),
#    [Input('live-update-map', 'clickData')]
#)
#def update_accel(clickData):
#    if not clickData: return {}
#    device_name = clickData['points'][0]['text']
#    return update_accel_common(device_name)

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

#@app.callback(
#    Output('temp-time-series', 'figure'),
#    [Input('live-update-map', 'clickData')]
#)
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
    app.run_server(host='0.0.0.0', debug=True)   
	
