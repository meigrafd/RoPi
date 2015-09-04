#!/usr/bin/python
# -*- coding: utf-8 -*-

#
# for manually send Commands to RoPi
#

import socket
import sys

HOST, PORT = "127.0.0.1", 7060
data = " ".join(sys.argv[1:])

# Create a socket (SOCK_STREAM means a TCP socket)
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

try:
    # Connect to server and send data
    sock.connect((HOST, PORT))
    sock.sendall(data + "\n")

    # Receive data from the server and shut down
    received = sock.recv(1024)
finally:
    sock.close()

print "Sent: {}".format(data)
print "Received: {}".format(received),