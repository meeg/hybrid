#!/usr/bin/env python
import csv
import sys

if (len(sys.argv)!=2):
    print "need .tp file"
    sys.exit(-1)

channel_dict = dict()
channel_id = 0
for feb in range(0,10):
    for hyb in range(0,4):
        if (feb==2 or feb==9) and hyb>1:
            continue
        for channel in range(0,640):
            channel_string = "{} {} {}".format(feb,hyb,channel)
            #print channel_string,channel_id
            channel_dict[channel_string] = channel_id
            channel_id+=1

f = open(sys.argv[1])
reader = csv.reader(f,delimiter='\t')
#tp: rce feb hyb ch amp t0 tp1 tp2 chi2
#tp_conditions: svt_channel_id amplitude t0 tp tp2
numcols = 9
print "svt_channel_id,amplitude,t0,tp,tp2"
try:
    for row in reader:
        if len(row)==numcols:
            #print row
            feb=row[1]
            hyb=row[2]
            channel=row[3]
            channel_string = "{} {} {}".format(feb,hyb,channel)
            channel_id = channel_dict[channel_string]
            print "{},{},{},{},{}".format(channel_id,row[4],row[5],row[6],row[7])
except csv.Error as e:
    print e
