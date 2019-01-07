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
  -f FILE, --file FILE  CSV file to write readings to (default output.csv)
  -v, --verbose         Verbose output to stdout
  
"""

import paho.mqtt.client as mqtt
import argparse
from datetime import datetime
from collections import namedtuple
import os

def on_disconnect(client, userdata, rc):
    print("Disconnected %i" % rc)


def on_connect(client, userdata, flags, rc):
    print("Connected %i" %rc)
    if rc == 0:
        for topic in userdata.topics:
            for station in userdata.stations:
                client.subscribe("{}/{}".format(station, topic))
        

def on_message(client, userdata, message):
    station, topic = message.topic.split("/")
    data = str(message.payload.decode("utf-8"))
    readings = userdata.readings

    if station in readings:
        if topic in readings[station]:
            readings[station][topic] += [data]
        else:
            readings[station][topic] = [data]
    else:
        readings[station] = {topic: [data]}

    all_data = all(readings[station].get(_topic, []) for _topic in userdata.topics)

    if userdata.verbose:
        print(station, topic, data)

    if all_data:
        line = "{},{},{}".format(datetime.now(), station, ",".join(readings[station][_topic][-1] for _topic in userdata.topics))
        for _topic in userdata.topics:
            readings[station][_topic] = []
        write_headers = not os.path.exists(userdata.filename)
        with open(userdata.filename, "a") as f:
            if write_headers:
                header = "date,station,{}".format(",".join(userdata.topics))
                print(header)
                f.writelines([header])
            print(line)
            f.writelines([line])


def main(name, username, password, url, port, stations, topics, filename, verbose):
    UserData = namedtuple("UserData", "filename topics stations verbose readings")
    userdata = UserData(filename, topics, stations, verbose, {})

    client = mqtt.Client(name, userdata=userdata)
    client.username_pw_set(username, password)
    client.connect(url, port)

    client.on_message = on_message
    client.on_connect = on_connect
    client.on_disconnect = on_disconnect

    client.loop_start()
    raw_input("Press Enter or Ctrl+C to stop")
    client.loop_stop()


if __name__ == "__main__":
    topics = "temperature,battery,pressure,humidity,light"
    stations = "68:C6:3A:AC:3B:D4,DC:4F:22:18:F3:B7"
    #B7 - black screws = antras
    #D4 - tape = pirmas

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
    parser.add_argument("-f", "--file", default="output.csv",
                        help="CSV file to write readings to (default output.csv)")
    parser.add_argument("-v", "--verbose", const=True, default=False, action='store_const',
                        help="Verbose output to stdout")
    args = parser.parse_args()

    main(args.name, args.username, args.password,
         args.server, args.port, args.stations.split(','),
         args.topics.split(','), args.file, args.verbose)
    
