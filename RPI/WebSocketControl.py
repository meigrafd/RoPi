#!/usr/bin/python
# -*- coding: utf-8 -*-
#
# Starts tornado and Socket for Reading/Writing WebInterface<->RoPi_Socket.py
#
# DONT use CTRL+Z to terminate script, else Port is blocked
#
###
# import the libraries
import tornado.web
import tornado.websocket
import tornado.ioloop
from datetime import timedelta
import threading
import time
import sys
import socket
import subprocess
try:
  import SocketServer as socketserver
except ImportError:
  import socketserver
#import psutil

#-------------------------------------------------------------------
DEBUG = True
#DEBUG = False
WebSocketPort = 7070
RoPiSocketPort = 7060
#-------------------------------------------------------------------

# thread docu: http://www.tutorialspoint.com/python/python_multithreading.htm
# socketserver docu: https://docs.python.org/2/library/socketserver.html
# tornado & serial docu: http://forums.lantronix.com/showthread.php?p=3131
# psutil: https://github.com/giampaolo/psutil

#-------------------------------------------------------------------

try:
  DEBUG
except NameError:
  DEBUG = False

if DEBUG:
  print("Enabling DEBUG mode!")
else:
  print("Disabling DEBUG mode!")


def printD(message):
  if DEBUG:
    print(message)

#http://www.gossamer-threads.com/lists/python/python/375364
def awk_it(instring,index,delimiter=" "):
  try:
    return [instring,instring.split(delimiter)[index-1]][max(0,min(1,index))]
  except:
    return ""

def getUptime():
  with open('/proc/uptime', 'r') as f:
    uptime_seconds = float(f.readline().split()[0])
    uptime = str(timedelta(seconds = uptime_seconds))
  return uptime

def getPiRAM():
  with open('/proc/meminfo', 'r') as mem:
    tmp = 0
    for i in mem:
      sline = i.split()
      if str(sline[0]) == 'MemTotal:':
        total = int(sline[1])
      elif str(sline[0]) in ('MemFree:', 'Buffers:', 'Cached:'):
        tmp += int(sline[1])
    free = tmp
    used = int(total) - int(free)
    usedPerc = (used * 100) / total
    return usedPerc


### Reconnect to RoPiSocket
def ReconnectRoPiSocket():
  try:
    # Create a socket (SOCK_STREAM means a TCP socket)
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    # Connect to server and send data
    HOST, PORT = "127.0.0.1", RoPiSocketPort
    sock.connect((HOST, PORT))
  except Exception:
    pass
  else:
    reconTimer.cancel()

### Send Request to RoPi_Socket.py over Socket
def Send2RoPi(request):
  global reconTimer
  try:
    # Create a socket (SOCK_STREAM means a TCP socket)
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    # Connect to server and send data
    HOST, PORT = "127.0.0.1", RoPiSocketPort
    sock.connect((HOST, PORT))
    sock.sendall(request + "\n")
    # Receive data from the server and shut down
    received = sock.recv(4096)
    return received
  except Exception, e2:
    print "Error in Send2RoPi: " + str(e2)
    #start threading.Timer for reconnect
    reconTimer = threading.Timer(5, ReconnectRoPiSocket)
    reconTimer.start()
    return FAILED
    raise
  finally:
    sock.close()

### Parse request from webif
#required format-> command:value
def WebRequestHandler(requestlist):
    returnlist = ""
    for request in requestlist:
        request =  request.strip()
        requestsplit = request.split(':')
        requestsplit.append("dummy")
        command = requestsplit[0]
        value = requestsplit[1]
        if value == "dummy":
           value = "0"
        if command == "localping":
           returnlist += "\n localping:ok"

        elif command == "get.system.loadavg":
           returnlist += "\n get.system.loadavg:"+open("/proc/loadavg").readline().split(" ")[:3][0]
        elif command == "get.system.uptime":
           returnlist += "\n get.system.uptime:"+str(getUptime()).split(".")[0]
        elif command == "get.system.ram":
           returnlist += "\n get.system.ram:"+str(getPiRAM())
           #returnlist += "\n get.system.ram:"+str(psutil.phymem_usage().percent)
        elif command == "set.system.power":
           if value == "off":
              subprocess.Popen(["shutdown","-h","now"])
              return "set.system.power:ok"
           elif value == "reboot":
              subprocess.Popen(["shutdown","-r","now"])
              return "set.system.power:ok"

        elif command == "forward":
           response = Send2RoPi("forward:-1")
           returnlist += "\n " + response
        elif command == "backward":
           response = Send2RoPi("backward:-1")
           returnlist += "\n " + response
        elif command == "left":
           response = Send2RoPi("left:-1")
           returnlist += "\n " + response
        elif command == "right":
           response = Send2RoPi("right:-1")
           returnlist += "\n " + response

        else:
           response = Send2RoPi(command+":"+value)
           returnlist += "\n " + response

    return returnlist

### WebSocket server tornado <-> WebInterface
class WebSocketHandler(tornado.websocket.WebSocketHandler):
  connections = []
  # the client connected
  def open(self):
      printD("New client connected")
      self.write_message("You are connected")
      self.connections.append(self)
  # the client sent the message
  def on_message(self, message):
      printD("Message from WebIf: >>>"+message+"<<<")
      requestlist = message.splitlines()
      self.write_message(WebRequestHandler(requestlist))
  # client disconnected
  def on_close(self):
      printD("Client disconnected")
      self.connections.remove(self)

# start a new WebSocket Application
# use "/" as the root, and the WebSocketHandler as our handler
application = tornado.web.Application([
    (r"/", WebSocketHandler),
])


if __name__ == "__main__":
  try:
    # start the tornado webserver on port WebSocketPort
    application.listen(WebSocketPort)
    tornado.ioloop.IOLoop.instance().start()
  except Exception, e1:
    print("Error...: " + str(e1))
  except KeyboardInterrupt:
    print("Schliesse Programm..")
    exit()

# this is the main (3.) thread which exits after all is done, so Dont close it here!