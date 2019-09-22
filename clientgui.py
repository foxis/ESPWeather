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
        print('call')
        with self.lock:
            for observer in self.observers:
                observer(*args, **kwargs)

    def __iadd__(self, observer):
        print('add', observer)
        if not hasattr(observer, '__call__'):
            raise TypeError('Must be callable')

        with self.lock:
            self.observers.add(observer)

    def __isub__(self, observer):
        print('sub', observer)
        with self.lock:
            self.observers.remove(observer)

    def __contains__(self, observer):
        with self.lock:
            return observer in self.observers


class UserData:
    TOPIC_ANNOUNCE = 'announce'
    DEFAULT_TOPICS = 'temperature,humidity,pressure,light,uv-index,events,ntp,battery'

    def __init__(self, stations, topics=DEFAULT_TOPICS, file=None):
        self.lock = thrd.Lock()
        self.stations = {
            i.strip(): {
                i.strip(): []
                for i in (topics + ',_timestamp').split(',')
            } for i in stations.split(',')
        } if stations else {}
        self.topics = [i.strip() for i in topics.split(',')]
        self.connected = False
        self.last_connected = False
        self.announce_listeners = Observer(lambda station: print(f'Adding new station: {station}'))
        self.readings_listeners = Observer(lambda station, topic, reading: None)
        self.connection_listeners = Observer(lambda c, rc: print(f'Connected {rc}' if c else f'Disconnected {rc}'))

    def on_connect(self, client, userdata, flags, rc):
        print('connect')
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
                    self.stations[data] = {}
                    self.announce_listeners(data)
            return

        station, topic = message.topic.split('/')
        try:
            with self.lock:
                self.stations[station][topic] = data
                self.stations[station]['_timestamp'] = datetime.now()
            self.readings_listeners(station, topic, data)
        except Exception as e:
            print('Exception occured: %s' % repr(e))

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


class StationWidget(tk.Frame):
    def __init__(self, master, name, userdata):
        tk.Frame.__init__(self, master, width=100, relief='sunken', bd=1, padx=3, pady=3)
        self.userdata = userdata

        self.title = tk.Label(self, text=name, font='bold')
        self.title.grid(row=0, columnspan=2)

        self.topics = {
            topic: tk.Label(self, text='NA')
            for topic in self.userdata.topics
        }
        for idx, t in enumerate(self.topics.items()):
            tk.Label(self, text=t[0]).grid(row=idx+1, column=0, sticky='w')
            t[1].grid(row=idx+1, column=1, sticky='w')

    def update(self, topic, data):
        if topic and data:
            self.topics[topic]['text'] = f'{data}'
        else:
            for topic, data in self.userdata.get_latest(self.title).items():
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
        canvas = FigureCanvasTkAgg(self.plot, self)
        canvas.get_tk_widget().pack(expand=1, fill='both')

        self.userdata = userdata
        self.topic = topic
        self.plots = None
        self.recreate()

    def recreate(self, plot=True):
        default_plot = self.userdata.get_all()
        N = len(default_plot)
        self.plots = {
            station: {
                'x': default_plot[station]['_timestamp'],
                'y': default_plot[station][self.topic],
                'plot': self.plot.add_subplot(N, 1, i + 1)
            }
            for i, station in enumerate(self.userdata.stations)
        }
        self.plot.suptitle(self.topic)
        for station in self.plots.values():
            station['plot'].clear()
            station['plot'].plot(
                station['x'],
                station['y']
            )

    def update(self, station, data):
        if station not in self.plots:
            self.recreate()
        else:
            self.plots[station]['x'] += datetime.now()
            self.plots[station]['y'] += data

        self.plots[station]['plot'].clear()
        self.plots[station]['plot'].plot(
            self.plots[station]['x'],
            self.plots[station]['y']
        )


class WeatherMonitor(tk.Frame):
    def __init__(self, master, userdata, *args, **kwargs):
        tk.Frame.__init__(self, master, *args, **kwargs)
        master.title('Weather Monitor')
        self.userdata = userdata
        userdata.connection_listeners += self.on_connection
        userdata.announce_listeners += self.on_announced
        userdata.readings_listeners += self.on_reading

        self.statusbar = tk.Label(self, text="Waiting…", bd=1, relief=tk.SUNKEN, anchor=tk.W)
        self.statusbar.pack(side='bottom', fill='x')

        self.tabs = ttk.Notebook(self)
        self.tabs.pack(side='top', expand=1, fill='both')

        container = tk.Frame(self.tabs)
        self.tabs.add(container, text='Stations')

        self.stations = {
            station: StationWidget(container, station, userdata)
            for station in userdata.stations.keys()
        }

        self.topics = {
            topic: GraphWidget(self.tabs, topic, userdata)
            for topic in userdata.topics
        }

        for topic, w in self.topics.items():
            self.tabs.add(w, text=topic)

        self.arrange()

    def on_connection(self, connected, rc):
        self.statusbar['text'] = f'Connected={connected} | {rc}'

    def on_announced(self, station):
        if station not in self.stations:
            self.stations[station] = StationWidget(self, station, self.userdata)
            self.arrange()
        self.stations[station].announced = True

    def on_reading(self, station, topic, data):
        self.stations[station].update(topic, data)
        self.topics[topic].update(station, data)

    def arrange(self):
        M = math.ceil(len(self.stations) ** .5)
        for i, w in enumerate(self.stations.values()):
            w.grid(row=i//M, column=i % M)


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
