#!/usr/bin/env python2.7
import zmq
import sys
import time

endpoint = "tcp://127.0.0.1:"
port = "5556"
sub = ""

try:
    sub = sys.argv[1]
except:
    pass

psub = sub
if sub == "":
    psub = "all"

print "Connecting to server %s:%s subscribing for: %s" % (endpoint, port, psub)

context = zmq.Context()
socket = context.socket(zmq.SUB)
socket.setsockopt(zmq.SUBSCRIBE, b"%s" % sub )
endpoint = endpoint + port
socket.connect(endpoint)
print "\n"
while True:
    message = socket.recv()
    print message
    print "\n"
