''' A simple MQTT Client application to gather Weather Station data. GUI variant.

'''

import paho.mqtt.client as mqtt
import argparse
from datetime import datetime
import copy
import threading as thrd
import tkinter as tk
from tkinter import ttk
import math
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg


class Observer:
    def __init__(self, initial=None):
        self.lock = thrd.Lock()
        self.observers = set()
        if initial:
            self.__iadd__(initial)

    def __call__(self, *args, **kwargs):
        with self.lock:
            for observer in self.observers:
                try:
                    observer(*args, **kwargs)
                except Exception as e:
                    print(f'Observer {observer} failed: {repr(e)}')
                    raise

    def __iadd__(self, observer):
        if not hasattr(observer, '__call__'):
            raise TypeError('Must be callable')

        with self.lock:
            self.observers.add(observer)
        return self

    def __isub__(self, observer):
        with self.lock:
            self.observers.remove(observer)
        return self

    def __contains__(self, observer):
        with self.lock:
            return observer in self.observers


class UserData:
    TOPIC_ANNOUNCE = 'announce'
    DEFAULT_TOPICS = 'temperature,humidity,pressure,light,uv-index,events,ntp,battery'
    FLOAT_TOPICS = 'temperature,humidity,pressure,light,uv-index,battery'
    INT_TOPICS = 'events'
    TIMESTAMP_TOPICS = 'ntp,rtc'

    def __init__(self, stations, topics=DEFAULT_TOPICS, file=None):
        self.lock = thrd.Lock()
        self.topics = [i.strip() for i in topics.split(',')]
        self.stations = {
            i.strip(): {
                i.strip(): []
                for i in self.topics + ['_timestamp']
            } for i in stations.split(',')
        } if stations else {}
        self.connected = False
        self.last_connected = False
        self.announce_listeners = Observer(lambda station: print(f'Announcement: {station}'))
        self.readings_listeners = Observer(lambda station, topic, reading: None)
        self.connection_listeners = Observer(lambda c, rc: print(f'Connected {rc}' if c else f'Disconnected {rc}'))

    def on_connect(self, client, userdata, flags, rc):
        self.connection_listeners(True, rc)
        if rc == 0:
            client.subscribe(self.TOPIC_ANNOUNCE)
            with self.lock:
                for topic in self.topics:
                    for station in self.stations:
                        client.subscribe(f'{station}/{topic}')
            self.connected = True
            self.last_connected = True

    def on_message(self, client, userdata, message):
        data = str(message.payload.decode('utf-8'))

        if message.topic == self.TOPIC_ANNOUNCE:
            with self.lock:
                if data not in self.stations:
                    self.stations[data] = {
                        i.strip(): []
                        for i in self.topics + ['_timestamp']
                    }
                    for topic in self.topics:
                        client.subscribe(f'{data}/{topic}')
                    self.announce_listeners(data)
            return

        station, topic = message.topic.split('/')
        data = getattr(self, f'process_{topic}')(data)
        with self.lock:
            self.stations[station][topic] += [data]
            now = datetime.now()
            max_len = max(len(i) for i in self.stations[station].values())
            if max_len > len(self.stations[station]['_timestamp']):
                self.stations[station]['_timestamp'] += [now]
        self.readings_listeners(station, topic, data)

    def on_disconnect(self, client, userdata, rc):
        self.connection_listeners(False, rc)

    def get_latest(self, station):
        with self.lock:
            station = copy.deepcopy(self.stations[station])
            return {
                k: v[-1:]
                for k, v in station.items()
            }

    def get(self, station):
        with self.lock:
            return copy.deepcopy(self.stations[station])

    def get_all(self):
        with self.lock:
            return copy.deepcopy(self.stations)

    def __getattr__(self, topic):
        def process_float(x):
            try:
                return float(x)
            except TypeError:
                return 'NA'

        def process_int(x):
            try:
                return int(x)
            except TypeError:
                return 'NA'

        def process_timestamp(x):
            try:
                return datetime.strptime(x, '%Y-%m-%dT%H:%M:%SZ')
            except TypeError:
                return 'NA'

        if topic.startswith('process_'):
            for i in self.FLOAT_TOPICS.split(','):
                if topic.endswith(i):
                    return process_float
            for i in self.INT_TOPICS.split(','):
                if topic.endswith(i):
                    return process_int
            for i in self.TIMESTAMP_TOPICS.split(','):
                if topic.endswith(i):
                    return process_timestamp
            return lambda x: x


class StationWidget(tk.Frame):
    def __init__(self, master, name, userdata):
        tk.Frame.__init__(self, master, width=100, relief='sunken', bd=1, padx=3, pady=3)
        self.userdata = userdata
        self.station = name
        self.title = tk.Label(self, text=name, font='bold')
        self.title.grid(row=0, columnspan=2)

        self.topics = {
            topic: tk.Label(self, text='NA')
            for topic in self.userdata.topics
        }
        for idx, t in enumerate(self.topics.items()):
            tk.Label(self, text=t[0]).grid(row=idx+1, column=0, sticky='w')
            t[1].grid(row=idx+1, column=1, sticky='w')

        self.update(None, None)

    def update(self, topic, data):
        if topic and data:
            self.topics[topic]['text'] = f'{data}'
        else:
            for topic, data in self.userdata.get_latest(self.station).items():
                if topic in self.userdata.topics and data:
                    self.topics[topic]['text'] = f'{data}'

    @property
    def announced(self):
        return True

    @announced.setter
    def announced(self, value):
        pass


class GraphWidget(tk.Frame):
    def __init__(self, master, topic, userdata, *args, **kwargs):
        tk.Frame.__init__(self, master, *args, **kwargs)
        self.plot = plt.Figure(figsize=(6, 6), dpi=100)
        self.axis = self.plot.add_subplot(111)

        canvas = FigureCanvasTkAgg(self.plot, self)
        canvas.get_tk_widget().pack(expand=1, fill='both')

        self.userdata = userdata
        self.topic = topic
        self.plots = None
        self.draw()

    def draw(self):
        data = self.userdata.get_all()

        self.axis.clear()
        for station, val in data.items():
            if len(val[self.topic]):
                self.axis.plot(
                    val['_timestamp'],
                    val[self.topic],
                    label=station,
                )
        self.axis.legend(loc='best')

    def update(self, station, data):
        self.draw()


class WeatherMonitor(tk.Frame):
    def __init__(self, master, userdata, *args, **kwargs):
        tk.Frame.__init__(self, master, *args, **kwargs)
        master.title('Weather Monitor')
        self.userdata = userdata
        self.stations = {}
        self.topics = {}

        userdata.connection_listeners += self.on_connection
        userdata.announce_listeners += self.on_announced
        userdata.readings_listeners += self.on_reading

        self.statusbar = tk.Label(self, text="Waiting…", bd=1, relief=tk.SUNKEN, anchor=tk.W)
        self.statusbar.pack(side='bottom', fill='x')

        self.tabs = ttk.Notebook(self)
        self.tabs.pack(side='top', expand=1, fill='both')

        self.container = tk.Text(
            self.tabs,
            wrap="char",
            borderwidth=0,
            highlightthickness=0,
            state="disabled"
        )

        self.tabs.add(self.container, text='Stations')

        for station in userdata.stations.keys():
            self.add_station(station)

        for topic in userdata.topics:
            self.add_topic(topic)

    def add_station(self, station, widget=None):
        self.stations[station] = StationWidget(self.container, station, self.userdata) if not widget else widget
        self.container.configure(state="normal")
        self.container.window_create("end", window=self.stations[station])
        self.container.configure(state="disabled")

    def add_topic(self, topic, widget=None):
        self.topics[topic] = GraphWidget(self.tabs, topic, self.userdata) if not widget else widget
        self.tabs.add(self.topics[topic], text=topic)

    def on_connection(self, connected, rc):
        self.statusbar['text'] = f'Connected={connected} | {rc}'

    def on_announced(self, station):
        if station not in self.stations:
            self.add_station(station)
        self.stations[station].announced = True

    def on_reading(self, station, topic, data):
        self.stations[station].update(topic, data)
        self.topics[topic].update(station, data)


def main(client_name, username, password, server, port, stations):
    userdata = UserData(stations)

    client = mqtt.Client(client_name, userdata=userdata)
    client.username_pw_set(username, password)

    client.on_message = userdata.on_message
    client.on_connect = userdata.on_connect
    client.on_disconnect = userdata.on_disconnect

    root = tk.Tk()
    WeatherMonitor(root, userdata).pack(side="top", fill="both", expand=True)

    client.connect(server, port)

    client.loop_start()
    root.mainloop()
    client.loop_stop()


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='MQTT Client for Weather Station readings monitoring.')
    parser.add_argument('server',
                        help='MQTT Server Address')
    parser.add_argument('name',
                        help='MQTT Client Name (default P1)')
    parser.add_argument('-u', '--username',
                        help='MQTT Client Username')
    parser.add_argument('-l', '--password',
                        help='MQTT Client Password')
    parser.add_argument('-p', '--port', type=int, default=16769,
                        help='MQTT Server Port')
    parser.add_argument('-s', '--stations', default=None,
                        help='Comma separated list of stations to monitor (default: None)')
    parser.add_argument('-v', '--verbose', const=True, default=False, action='store_const',
                        help='Verbose output to stdout')
    args = parser.parse_args()

    main(
        args.name,
        args.username,
        args.password,
        args.server,
        args.port,
        args.stations
    )
