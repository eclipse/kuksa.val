import sys, os
import time

scriptDir= os.path.dirname(os.path.realpath(__file__))
sys.path.append(os.path.join(scriptDir, ".."))

from replay import stressClient

import argparse
import csv
from pickle import FALSE,TRUE
import string

def workaround_dict(a,b,timeout=0):
    pass

def getTimeDiff(timestamps):
    times = []
    time = []
    hours = []
    minutes = []
    seconds = []
    microseconds = []
    timeDiff = [0]

    for i in range(0,len(timestamps)):
        times.append(timestamps[i][timestamps[i].find(" ")+1:])     #take only the information about time from the timestamp
        
        #cut out corresponding information from the time string
        hours.append(int(times[i][0:2]))
        minutes.append(int(times[i][3:5]))
        seconds.append(int(times[i][6:8]))
        microseconds.append(int(times[i][9:]))

        #calculate time delta in seconds by adding up all the values with their correlating factor (i.e. 1h = 3600s)
        time.append((float(microseconds[i]*10**-6)+float(seconds[i])+float(minutes[i]*60)+float(hours[i]*3600)))

    for i in range(1,len(time)):
        timeDiff.append(time[i]-time[i-1])

    return timeDiff        #return all time differences in one list


rowIDs = [
    "timestamp",
    "ID",
    "action",
    "path",
    "value"
]
timestapIDs =  []

cmdParser = argparse.ArgumentParser(description="Initialise replay function")

cmdParser.add_argument('path', type=str, help="Specify path for replay file")
cmdParser.add_argument('-g', '--getValue', action='store_true', help="Enable replay of getValue function for debugging purposes (might overstress the server)")
args=cmdParser.parse_args()

try:
    VissClient = stressClient.StressClient()

    actionFunctions = {
        "get": VissClient.commThread.getValue,
        "set": VissClient.commThread.setValue
        }

    if not args.getValue:
        actionFunctions["get"] = workaround_dict   #don't call get functions when getValue is not specified

        try:
            open(args.path,"r")
        except:
            print("Could not open log file at " + args.path)

        with open(args.path,"r") as recordFile:
            fileData = csv.DictReader(recordFile,rowIDs,delimiter=';')

            timestamps = []

            for row in fileData:
                timestamps.append(row["timestamp"])

        timeDiff = getTimeDiff(timestamps)              #Array with all time deltas in seconds

    with open(args.path,"r") as recordFile:             #fileData gets reset to empty list, reopen for further action
        fileData = csv.DictReader(recordFile,rowIDs,delimiter=';')

        for i,row in enumerate(fileData):
            time.sleep(timeDiff[i])
            actionFunctions.get(row['action'])(row['path'],row['value'],timeout=0)

    print("Replay successful")

except:
    print("Aborted replay script")

os._exit(1)
