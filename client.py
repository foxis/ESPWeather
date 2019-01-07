""" A simple MQTT Client application to gather Weather Station data.

This application subscribes to topics for each station and gathers data into
a dictionary of dictionaries. Saves the data into a CSV file.


usage: client.py [-h] [-c NAME] -u USERNAME -l PASSWORD -s SERVER [-p PORT]
                 [-t TOPICS] [-a STATIONS]

MQTT Client for Weather Station readings monitoring.

optional arguments:
  -h, --help            show this help message and exit
  -c NAME, --name NAME  MQTT Client Name (default P1)
  -u USERNAME, --username USERNAME
                        MQTT Client Username
  -l PASSWORD, --password PASSWORD
                        MQTT Client Password
  -s SERVER, --server SERVER
                        MQTT Server Address
  -p PORT, --port PORT  MQTT Server Port
  -t TOPICS, --topics TOPICS
                        Comma separated list of topics to monitor (default
                        temperature,battery,pressure,humidity,light)
  -a STATIONS, --stations STATIONS
                        Comma separated list of stations to monitor (default
                        68:C6:3A:AC:3B:D4,DC:4F:22:18:F3:B7)
  -f FILE, --file FILE  CSV file to write readings to (default {client name}.csv)
  -v, --verbose         Verbose output to stdout
  -g, --plot            Plot graphs of the readings
  
"""

import paho.mqtt.client as mqtt
import argparse
from datetime import datetime
import matplotlib.pyplot as plt
import os
import threading as thrd


class UserData(object):
    def __init__(self, filename, topics, stations, verbose, plot):
        self.filename = filename
        self.topics = topics
        self.stations = stations
        self.verbose = verbose
        self.plot = plot
        self.lock = thrd.Lock()
        self.readings = {station: {topic: [] for topic in topics} for station in stations}
        self.plots = None
        self._connected = False
        self._last_connected = False

    @property
    def connected(self):
        with self.lock:
            return self._connected

    @connected.setter
    def connected(self, value):
        with self.lock:
            self._connected = value

    @property
    def last_connected(self):
        with self.lock:
            return self._last_connected

    @last_connected.setter
    def connected(self, value):
        with self.lock:
            self._last_connected = value

    def add_reading(self, station, topic, data):
        with self.lock:
            readings = self.readings
            if station not in readings or topic not in readings[station]:
                return
            readings[station][topic] += [data]

            all_data = all(readings[station].get(_topic, []) for _topic in self.topics)

            if self.verbose:
                print(station, topic, data)

            if self.plot:
                self.plot_reading(station, topic, data)

            if all_data:
                self.write_last_reading(station)

    def write_last_reading(self, station):
        readings = self.readings
        line = "{},{},{}".format(datetime.now(), station, ",".join(readings[station][_topic][-1] for _topic in self.topics))
        for _topic in self.topics:
            readings[station][_topic] = []
        write_headers = not os.path.exists(self.filename)
        with open(self.filename, "a") as f:
            if write_headers:
                header = "date,station,{}".format(",".join(self.topics))
                print(header)
                f.writelines([header])
            print(line)
            f.writelines([line])

    def create_plots(self, station, topics):
        plots = dict(fig=plt.figure())
        N = len(topics)
        plots.update({
            topic: {
                'x':[],
                'y':[],
                'plot':plots['fig'].add_subplot(N, 1, i + 1)
            } for i, topic in enumerate(topics)
        })
        plots['fig'].suptitle(station)
        for topic in topics:
            plots[topic]['plot'].set_ylabel(topic)
            #plots[topic]['plot'].relim()
            #plots[topic]['plot'].autoscale_view(True,True,True)
            #plots[topic]['plot'], = plots[topic]['plot'].plot([], [])
        return plots

    def plot_reading(self, station, topic, data):
        if data == 'nan':
            return            

        if not self.plots:
            self.plots = {station: self.create_plots(station, self.topics) for station in self.stations}

        self.plots[station][topic]['x'] += [datetime.now()]
        self.plots[station][topic]['y'] += [float(data)]

        self.plots[station][topic]['x'] = self.plots[station][topic]['x'][-30:]
        self.plots[station][topic]['y'] = self.plots[station][topic]['y'][-30:]

        self.plots[station][topic]['plot'].clear()
        self.plots[station][topic]['plot'].plot(
            self.plots[station][topic]['x'],
            self.plots[station][topic]['y'])


def on_disconnect(client, userdata, rc):
    print("Disconnected %i" % rc)
    userdata.connected = False


def on_connect(client, userdata, flags, rc):
    print("Connected %i" %rc)
    if rc == 0:
        for topic in userdata.topics:
            for station in userdata.stations:
                client.subscribe("{}/{}".format(station, topic))
        userdata.connected = True
        userdata.last_connected = True
        

def on_message(client, userdata, message):
    station, topic = message.topic.split("/")
    data = str(message.payload.decode("utf-8"))
    try:
        userdata.add_reading(station, topic, data)
    except Exception as e:
        print "Exception occured: %s" % repr(e)


def main(name, username, password, url, port, stations, topics, filename, verbose, plot):

    userdata = UserData(filename, topics, stations, verbose, plot)

    """userdata.add_reading('pirmas', 'temperature', '17.0')
    userdata.add_reading('pirmas', 'battery', '17.0')
    userdata.add_reading('pirmas', 'pressure', '17.0')
    userdata.add_reading('pirmas', 'humidity', '17.0')
    """
    client = mqtt.Client(name, userdata=userdata)
    client.username_pw_set(username, password)
    client.connect(url, port)

    client.on_message = on_message
    client.on_connect = on_connect
    client.on_disconnect = on_disconnect

    if not plot:
        client.loop_start()
        raw_input("Press Enter or Ctrl+C to stop")
        client.loop_stop()
    else:
        while True:
            client.loop()
            plt.draw()
            plt.pause(0.01)
            if not userdata.connected and userdata.last_connected:
                userdata.last_connected = False
                client.connect(url, port)
        

if __name__ == "__main__":
    topics = "temperature,battery,pressure,humidity,light"
    stations = "68:C6:3A:AC:3B:D4,DC:4F:22:18:F3:B7"

    parser = argparse.ArgumentParser(description="MQTT Client for Weather Station readings monitoring.")
    parser.add_argument("-c", "--name", default="P1",
                        help="MQTT Client Name (default P1)")
    parser.add_argument("-u", "--username", required=True,
                        help="MQTT Client Username")
    parser.add_argument("-l", "--password", required=True, 
                        help="MQTT Client Password")
    parser.add_argument("-s", "--server", required=True,
                        help="MQTT Server Address")
    parser.add_argument("-p", "--port", type=int, default=16769,
                        help="MQTT Server Port")
    parser.add_argument("-t", "--topics", default=topics,
                        help="Comma separated list of topics to monitor (default %s)" % topics)
    parser.add_argument("-a", "--stations", default=stations,
                        help="Comma separated list of stations to monitor (default %s)" % stations)
    parser.add_argument("-f", "--file", default=None,
                        help="CSV file to write readings to (default {client name}.csv)")
    parser.add_argument("-v", "--verbose", const=True, default=False, action='store_const',
                        help="Verbose output to stdout")
    parser.add_argument("-g", "--plot", const=True, default=False, action='store_const',
                        help="Plot graphs of the readings")
    args = parser.parse_args()

    filename = args.file if args.file else "{}.csv".format(args.name)

    main(args.name, args.username, args.password,
         args.server, args.port, args.stations.split(','),
         args.topics.split(','), filename, args.verbose, args.plot)
    
