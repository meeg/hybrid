#!/usr/bin/env python
import csv
import sys

if (len(sys.argv)!=2):
    print "need .base file"
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
#tp: rce feb hyb ch p0 n0 p1 n1 p2 n2 p3 n3 p4 n4 p5 n5 pall nall
numcols = 19
print "svt_channel_id,pedestal_0,noise_0,pedestal_1,noise_1,pedestal_2,noise_2,pedestal_3,noise_3,pedestal_4,noise_4,pedestal_5,noise_5"
try:
    for row in reader:
        if len(row)==numcols:
            #print row
            feb=row[1]
            hyb=row[2]
            channel=row[3]
            channel_string = "{} {} {}".format(feb,hyb,channel)
            channel_id = channel_dict[channel_string]
            print "{},{},{},{},{},{},{},{},{},{},{},{},{}".format(channel_id,row[4],row[5],row[6],row[7],row[8],row[9],row[10],row[11],row[12],row[13],row[14],row[15])
except csv.Error as e:
    print e
