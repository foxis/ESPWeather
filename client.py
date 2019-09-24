''' A simple MQTT Client application to gather Weather Station data. GUI variant.

'''

import paho.mqtt.client as mqtt
import argparse
from datetime import datetime
import copy
import threading as thrd
import tkinter as tk
from tkinter import ttk
import os
import csv
from itertools import zip_longest
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg


class Observer:
    def __init__(self, initial=None, enabled=True):
        self.enabled = enabled
        self.lock = thrd.Lock()
        self.observers = set()
        if initial:
            self.__iadd__(initial)

    def __call__(self, *args, **kwargs):
        with self.lock:
            if not self.enabled:
                return
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


class WeatherStationMQTTClient:
    TOPIC_ANNOUNCE = 'announce'
    DEFAULT_TOPICS = 'temperature,humidity,pressure,light,uv-index,events,ntp,battery'.split(',')
    FLOAT_TOPICS = 'temperature,humidity,pressure,light,uv-index,battery'.split(',')
    INT_TOPICS = 'events'.split(',')
    TIMESTAMP_TOPICS = 'ntp,rtc'.split(',')

    def __init__(self, client_name, username, password, server, port,
                 stations, topics=','.join(DEFAULT_TOPICS),
                 file=None, write_all=False, verbose=False):
        self.announce_listeners = Observer()  # (lambda station: print(f'Announcement: {station}'))
        self.readings_listeners = Observer()  # (lambda station, topic, reading, last: None)
        self.connection_listeners = Observer()  # (lambda c, rc: print(f'Connected {rc}' if c else f'Disconnected {rc}'))
        self.debug = Observer(enabled=verbose)

        self.lock = thrd.Lock()
        self.topics = [i.strip() for i in topics.split(',')]
        self.stations = {}
        self.connected = False
        self.last_connected = False
        self.server = server
        self.port = port
        self.file = file
        self.write_all = write_all
        self.header = None

        if stations:
            for station in stations.split(','):
                self.add_station(station)

        self.client = mqtt.Client(client_name, userdata=self)
        self.client.username_pw_set(username, password)

        self.client.on_message = self.on_message
        self.client.on_connect = self.on_connect
        self.client.on_disconnect = self.on_disconnect

    def start(self):
        self.debug('Starting')
        self.populate_data()
        self.client.connect(self.server, self.port)
        self.client.loop_start()

    def stop(self):
        self.client.loop_stop()
        if self.write_all:
            self.dump_data()

    def add_station(self, name):
        station = name.strip()
        self.stations[station] = {
            i: [] for i in self.topics
        }
        self.stations[station]['_timestamp'] = []
        self.stations[station]['_topics'] = set()

    def populate_data(self):
        if self.file and os.path.exists(self.file):
            self.debug(f'Reading {self.file} as csv...')
            with open(self.file, newline='') as f:
                for row in csv.reader(f):
                    if not self.header:
                        self.debug(f'Found header: {row}')
                        self.header = row
                    else:
                        timestamp, station = row[:2]
                        row = dict(zip(self.header[2:], row[2:]))

                        if station not in self.stations:
                            self.add_station(station)

                        for topic in self.topics:
                            value = getattr(self, f'process_{topic}')(row.get(topic, None))
                            self.stations[station][topic] += [value]
                            if value:
                                self.stations[station]['_topics'] |= {topic}
                        self.stations[station]['_timestamp'] += [datetime.fromisoformat(timestamp)]
                        self.debug(f'Loaded {station} = {self.get_latest(station)}')
            self.readings_listeners(None, None, None, True)

    def dump_data(self):
        def reading_generator(station_name, readings):
            keys = tuple(readings.keys())
            values = list(readings.values())
            timestamp_idx = keys.index('_timestamp')
            topics_idx = keys.index('_topics')
            del values[topics_idx]
            for reading in zip(*values):
                yield [
                    reading[timestamp_idx],
                    station_name
                ] + [
                    reading[keys.index(topic)] for topic in self.topics
                ]

        with self.lock:
            output = [
                data
                for station in self.stations.items()
                for data in reading_generator(*station)
            ]
            output.sort(key=lambda x: x[0])
            with open(self.file, 'w', newline='') as f:
                w = csv.writer(f)
                header = ['date', 'station'] + self.topics
                w.writerow(header)
                for row in output:
                    w.writerow(row)

    def write_last(self, station):
        if self.file:
            data = self.get_latest(station)
            with open(self.file, 'a', newline='') as f:
                w = csv.writer(f)
                if not self.header:
                    self.header = ['date', 'station']
                    self.header += self.topics
                    w.writerow(self.header)
                    self.debug(f'Writing header for {station} = {self.header}')

                row = [
                    data['_timestamp'],
                    station
                ] + [
                    isinstance(data[topic], datetime) and data[topic].strftime('%Y-%m-%dT%H:%M:%SZ') or data[topic]
                    for topic in self.topics
                ]
                w.writerow(row)
                self.debug(f'Writing row for {station} = {row}')

    def on_connect(self, client, userdata, flags, rc):
        self.connection_listeners(True, rc)
        if rc == 0:
            self.debug(f'Subscribing {self.TOPIC_ANNOUNCE}')
            client.subscribe(self.TOPIC_ANNOUNCE)
            with self.lock:
                for topic in self.topics:
                    for station in self.stations:
                        self.debug(f'Subscribing {station}/{topic}')
                        client.subscribe(f'{station}/{topic}')
            self.connected = True
            self.last_connected = True

    def on_message(self, client, userdata, message):
        data = str(message.payload.decode('utf-8'))

        if message.topic == self.TOPIC_ANNOUNCE:
            with self.lock:
                if data not in self.stations:
                    self.add_station(data)
                    for topic in self.topics:
                        self.debug(f'Subscribing {data}/{topic}')
                        client.subscribe(f'{data}/{topic}')
            self.announce_listeners(data)
            return

        station_name, topic = message.topic.split('/')
        with self.lock:
            station = self.stations[station_name]
            now = datetime.now()

            num_entries = len(station['_timestamp'])
            topics_covered = num_entries >= 2 and all(station[i][-1] is not None for i in station['_topics'])
            should_create_new_row = (
                num_entries == 0   # first entry ever
                or (now - station['_timestamp'][-1]).total_seconds() > 15  # last entry was long ago
                or (topics_covered and (now - station['_timestamp'][-1]).total_seconds() < 15)  # entry is part of a burst
            )

            if should_create_new_row:
                for t in self.topics:
                    station[t] += [None]
                station['_timestamp'] += [now]

            if topic in self.topics:
                station[topic][-1] = getattr(self, f'process_{topic}')(data)
                station['_topics'] |= {topic}

            topics_covered = (
                all(station[i][-1] is not None for i in self.topics)
                or (num_entries >= 2 and all(station[i][-1] is not None for i in station['_topics']))
            )

        self.readings_listeners(station_name, topic, data, topics_covered)

        if topics_covered:
            self.debug(f'last log: {station_name} {station["_topics"]}')
            self.write_last(station_name)

    def on_disconnect(self, client, userdata, rc):
        self.connection_listeners(False, rc)

    def count(self, station):
        with self.lock:
            return len(self.stations[station]['_timestamp'])

    def get_latest(self, station):
        with self.lock:
            station = self.stations[station]
            data = {k: v for k, v in station.items() if k != '_topics'}
            all = list(zip_longest(*data.values(), fillvalue=None))
            if all:
                return {
                    k: v for k, v in zip(data.keys(), all[-1])
                }
            else:
                return {
                    k: [] for k in data.keys()
                }

    def get(self, station):
        with self.lock:
            return copy.deepcopy(self.stations[station])

    def get_all(self):
        with self.lock:
            return copy.deepcopy(self.stations)

    def __getattr__(self, topic):
        def value_validator(f):
            def wrapper(x):
                try:
                    return f(x)
                except (AttributeError, ValueError):
                    return None
            return wrapper

        @value_validator
        def process_float(x):
            return float(x)

        @value_validator
        def process_int(x):
            return int(x)

        @value_validator
        def process_timestamp(x):
            return datetime.strptime(x, '%Y-%m-%dT%H:%M:%SZ')

        if topic.startswith('process_'):
            topic = topic[8:]
            if topic in self.FLOAT_TOPICS:
                return process_float
            elif topic in self.INT_TOPICS:
                return process_int
            elif topic in self.TIMESTAMP_TOPICS:
                return process_timestamp
            else:
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
            self.topics[topic]['text'] = f'{data}' if data else 'NA'
            self.title['text'] = f'{self.station}({self.userdata.count(self.station)})'
        else:
            for topic, data in self.userdata.get_latest(self.station).items():
                if topic in self.topics and data:
                    self.topics[topic]['text'] = f'{data}' if data else 'NA'

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

        self.canvas = FigureCanvasTkAgg(self.plot, self)
        self.canvas.get_tk_widget().pack(expand=1, fill='both')

        self.userdata = userdata
        self.topic = topic
        self.plots = None
        self.draw()

    def draw(self):
        data = self.userdata.get_all()

        self.axis.clear()
        for station, val in data.items():
            if len(val[self.topic]) == len(val['_timestamp']) and any(val[self.topic]):
                self.axis.plot(
                    val['_timestamp'],
                    [i is None and float('nan') or i for i in val[self.topic]],
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

        self.log = tk.Listbox(self.tabs)

        stations = userdata.get_all()
        for station in stations:
            self.add_station(station)

        for topic in userdata.topics:
            self.add_topic(topic)

        self.tabs.add(self.log, text="Activity Log")

        self.info('Created...')

        userdata.connection_listeners += self.on_connection
        userdata.announce_listeners += self.on_announced
        userdata.readings_listeners += self.on_reading
        userdata.debug += self.debug

    def info(self, msg):
        self.log.insert(0, f'{datetime.now()} INFO: {msg}')

    def debug(self, msg):
        self.log.insert(0, f'{datetime.now()} DEBUG: {msg}')

    def add_station(self, station, widget=None):
        self.stations[station] = StationWidget(self.container, station, self.userdata) if not widget else widget
        self.container.configure(state="normal")
        self.container.window_create("end", window=self.stations[station])
        self.container.configure(state="disabled")
        self.info(f'Added station: {station}')

    def add_topic(self, topic, widget=None):
        self.topics[topic] = GraphWidget(self.tabs, topic, self.userdata) if not widget else widget
        self.tabs.add(self.topics[topic], text=topic)

    def on_connection(self, connected, rc):
        self.statusbar['text'] = f'Connected={connected} | {rc}'
        self.info('Connected' if connected else 'Disconnected')

    def on_announced(self, station):
        if station not in self.stations:
            self.add_station(station)
        self.stations[station].announced = True
        self.info(f'Announced: {station}')

    def on_reading(self, station, topic, data, last):
        if station and topic:
            if station not in self.stations:
                self.add_station(station)

            self.stations[station].update(topic, data)
            if last:
                self.topics[topic].update(station, data)
        else:
            for station in self.stations.values():
                station.update(None, None)
            for topic in self.topics.values():
                topic.update(None, None)


class WeatherMonitorGUI:
    def __init__(self, userdata, *args, **kwargs):
        self.root = tk.Tk()
        WeatherMonitor(self.root, userdata).pack(side="top", fill="both", expand=1)

    def run(self):
        self.root.mainloop()


class WeatherMonitorConsole:
    def __init__(self, userdata, *args, **kwargs):
        self.userdata = userdata
        userdata.connection_listeners += lambda connected, rc: print('Connected' if connected else 'Disconnected')
        userdata.announce_listeners += lambda station: print(f'Station announced itself: {station}')
        userdata.readings_listeners += self.on_reading
        userdata.debug += lambda msg: print(f'{datetime.now()} DEBUG: {msg}')

    def on_reading(self, station, topic, data, last):
        if last and station and topic:
            print(f'{datetime.now()} {station}: {self.userdata.get_latest(station).values()}')

    def run(self):
        print('Press Ctrl+C to stop')
        try:
            while True:
                pass
        except KeyboardInterrupt:
            pass


def main(client_name, username, password, server, port, stations, file, write_all, verbose, gui):
    userdata = WeatherStationMQTTClient(
        client_name,
        username,
        password,
        server,
        port,
        stations,
        file=file,
        write_all=write_all,
        verbose=verbose
    )

    app = (WeatherMonitorConsole, WeatherMonitorGUI)[gui](userdata)

    userdata.start()
    app.run()
    userdata.stop()


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
    parser.add_argument('-f', '--file', default=None,
                        help='CSV filename of readings storage (default: None)')
    parser.add_argument('-w', '--write-all', const=True, default=False, action='store_const',
                        help='Write all collected data when the app is closed (default: False)')
    parser.add_argument('-g', '--gui', const=True, default=False, action='store_const',
                        help='Launch GUI instead of console (default: False)')
    parser.add_argument('-v', '--verbose', const=True, default=False, action='store_const',
                        help='Verbose output to stdout')
    args = parser.parse_args()

    main(
        args.name,
        args.username,
        args.password,
        args.server,
        args.port,
        args.stations,
        args.file,
        args.write_all,
        args.verbose,
        args.gui,
    )
