import zmq
import sys
import time

port = "5556"

context = zmq.Context()
print "Connecting to server..."
socket = context.socket(zmq.PULL)
socket.connect ("tcp://127.0.0.1:%s" % port)

while True:
    message = socket.recv()
    print message
