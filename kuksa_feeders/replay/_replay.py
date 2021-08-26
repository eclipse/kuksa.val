import sys, os
import time, datetime
import traceback
import configparser
import csv
import string

scriptDir= os.path.dirname(os.path.realpath(__file__))
sys.path.append(os.path.join(scriptDir, ".."))

from kuksa_viss_client import *

def workaround_dict(a,b):
    pass

rowIDs = [
    "timestamp",
    "ID",
    "action",
    "path",
    "value"
]

try:
    config = configparser.ConfigParser()
    config.read('config.ini')
except:
    print("Unable to read config file")
    os._exit(0)

args=config['replay']               #get replay data
vsscfg = config['vss']              #get Client data from config file
csv_path = args.get('path')

try:
    commThread = KuksaClientThread(vsscfg)       #make new thread
    commThread.start()
    commThread.authorize(token=commThread.tokenfile)
    print("Connected successfully")
except:
    print("Could not connect successfully")
    sys.exit(-1)

try:

    actionFunctions = {
        "get": commThread.getValue,
        "set": commThread.setValue
        }

    if args.get('mode') == 'Set':
        actionFunctions["get"] = workaround_dict   #don't call get functions when getValue is not specified
    elif args.get('mode') == 'SetGet':
            pass
    else:
        raise AttributeError

    with open(csv_path,"r") as recordFile:
        fileData = csv.DictReader(recordFile,rowIDs,delimiter=';')

        timestamp_pre = 0
        for row in fileData:
            debug_delta1 = time.perf_counter()      #perf_counter for time delta calculation
            timestamp_curr = row["timestamp"]

            if timestamp_pre != 0:
                curr = datetime.datetime.strptime(timestamp_curr, '%Y-%b-%d %H:%M:%S.%f')
                pre = datetime.datetime.strptime(timestamp_pre, '%Y-%b-%d %H:%M:%S.%f')
                delta = (curr-pre).total_seconds()          #get time delta between the timestamps
            else:
                delta=0
            
            timestamp_pre = timestamp_curr

            debug_delta3 = time.perf_counter()

            time.sleep(delta)
            debug_delta4 = time.perf_counter()
            actionFunctions.get(row['action'])(row['path'],row['value'])

            debug_delta2 = time.perf_counter()
            
            #time debugging
            print("delta (code): "+ str(delta))
            print("debug delta total: " + str(debug_delta2 - debug_delta1))
            print("delta without sleep: " + str(debug_delta3 - debug_delta1))
            print("sleep: " + str(debug_delta4 - debug_delta3))
            print("get/set: " + str(debug_delta2 - debug_delta4))

        print("Replay successful")

except AttributeError:
    print("Wrong attributes used. Please check config.ini")

except:
    traceback.print_exc()

os._exit(1)
