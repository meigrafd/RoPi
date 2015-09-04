#!/usr/bin/python
# -*- coding: utf-8 -*-
#
# Starts one Thread for Serial Reading/Writing RaspberryPI<->Arduino
# Starts another Thread for Socket Server waiting for Connections from WebSocketControl.py
#
# version: 0.8
#
# DONT use CTRL+C to Terminate, else Port is blocked
#
import threading
import serial
import time
import sys
import socket
import subprocess
#import signal
try:
  import SocketServer as socketserver
except ImportError:
  import socketserver


#-------------------------------------------------------------------
DEBUG = True
#DEBUG = False
SerialPort = "/dev/ttyACM0"
SerialBaudrate = 38400
SocketHost = "0.0.0.0"
SocketPort = 7060
#-------------------------------------------------------------------

# thread docu: http://www.tutorialspoint.com/python/python_multithreading.htm
# serial docu: http://pyserial.sourceforge.net/pyserial.html
# signal docu: http://stackoverflow.com/questions/1112343/how-do-i-capture-sigint-in-python
# socketserver docu: https://docs.python.org/2/library/socketserver.html

#initialization and open the port.
#possible timeout values:
#    1. None: wait forever, block call
#    2. 0: non-blocking mode, return immediately
#    3. x, x is bigger than 0, float allowed, timeout block call
ser = serial.Serial()
ser.port = SerialPort
ser.baudrate = SerialBaudrate
ser.bytesize = serial.EIGHTBITS #number of bits per bytes
ser.parity = serial.PARITY_NONE #set parity check: no parity
ser.stopbits = serial.STOPBITS_ONE #number of stop bits
#ser.timeout = None    #block read
ser.timeout = 1        #non-block read
#ser.timeout = 2       #timeout block read
ser.xonxoff = False    #disable software flow control
ser.rtscts = False     #disable hardware (RTS/CTS) flow control
ser.dsrdtr = False     #disable hardware (DSR/DTR) flow control
ser.writeTimeout = 2   #timeout for write

#-------------------------------------------------------------------

# Dictionary docu: http://www.tutorialspoint.com/python/python_dictionary.htm
defaultSetting = {};
currentValue = {};
MapCoords = {};

MapIndex = 0

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
    sys.stdout.flush()

#http://www.gossamer-threads.com/lists/python/python/375364
def awk_it(instring,index,delimiter=" "):
  try:
    return [instring,instring.split(delimiter)[index-1]][max(0,min(1,index))]
  except:
    return ""

# Arduino-> STARTMAP
# Arduino-> MAP: 255,255,255,255,255,255,255,255,255,255,
# Arduino-> MAP: 255,0,0,0,255,0,0,0,0,255,
# Arduino-> MAP: 255,0,254,0,255,0,1,0,0,255,
# Arduino-> MAP: 255,0,0,0,255,0,0,0,0,255,
# Arduino-> MAP: 255,0,0,0,255,255,255,0,0,255,
# Arduino-> MAP: 255,0,255,0,255,0,0,0,0,255,
# Arduino-> MAP: 255,0,255,0,255,0,0,0,0,255,
# Arduino-> MAP: 255,0,255,255,255,0,255,255,255,255,
# Arduino-> MAP: 255,0,0,0,0,0,0,0,0,255,
# Arduino-> MAP: 255,255,255,255,255,255,255,255,255,255,

#parse outputs from Arduino - more AI stuff to do here..
def ParseSerialInput(line):
  global MapIndex
  line = line.strip()
  prefix = awk_it(line, 1)
  if (prefix == "STARTMAP"):
    MapIndex = 0;
  elif (prefix == "MAP:"):
    Values = awk_it(line, 2)
    #printD("MAP "+str(MapIndex)+" "+Values)
    MapCoords.update({ MapIndex: Values });
    MapIndex = MapIndex + 1;
  elif (prefix == "distance:"):
    distance = awk_it(line, 2)
    currentValue.update({ "distance": distance });
    printD("Distance: "+distance)
  elif (prefix == "Bearing:"):
    Bearing = awk_it(line, 2)
    currentValue.update({ "bearing": Bearing });
    printD("Bearing: "+Bearing),
  elif (prefix == "Pitch:"):
    Pitch = awk_it(line, 2)
    currentValue.update({ "pitch": Pitch });
    printD("Pitch: "+Pitch),
  elif (prefix == "Roll:"):
    Roll = awk_it(line, 2)
    currentValue.update({ "roll": Roll });
    printD("Roll: "+Roll)
  elif (prefix == "IRfrontright:"):
    IRfrontright = awk_it(line, 2)
    currentValue.update({ "IRfrontright": IRfrontright });
    printD("IR front right: "+IRfrontright)
  elif (prefix == "IRfrontleft:"):
    IRfrontleft = awk_it(line, 2)
    currentValue.update({ "IRfrontleft": IRfrontleft });
    printD("IR front left: "+IRfrontleft)
  elif (prefix == "IRrearleft:"):
    IRrearleft = awk_it(line, 2)
    currentValue.update({ "IRrearleft": IRrearleft });
    printD("IR rear left: "+IRrearleft)
  elif (prefix == "IRrearright:"):
    IRrearright = awk_it(line, 2)
    currentValue.update({ "IRrearright": IRrearright });
    printD("IR rear right: "+IRrearright)
  elif (prefix == "LeftEncoder:"):
    LeftEncoder = awk_it(line, 2)
    currentValue.update({ "LeftEncoder": LeftEncoder });
    printD("Left Encoder: "+LeftEncoder)
  elif (prefix == "RightEncoder:"):
    RightEncoder = awk_it(line, 2)
    currentValue.update({ "RightEncoder": RightEncoder });
    printD("Right Encoder: "+RightEncoder)
  elif (prefix == "DEFAULT:"):
    Setting = awk_it(line, 2)
    Value = awk_it(line, 3)
    defaultSetting.update({ Setting: Value });
    if DEBUG:
       print("Added Default Setting: "+Setting),
       print(" Value: "+Value)

  sys.stdout.flush()


#read from Arduino
def Serial_read_Thread():
  print("Starte Thread: serial read")
  while True:
    try:
      # Remove newline character '\n'
      response = ser.readline().strip()
      #response = ser.readline()
      if response:
        printD("Arduino-> "+response),
        ParseSerialInput(response)
    except serial.SerialException as e4:
      print("Could not open serial port '{}': {}".format(SerialPort, e4))
      #_exit()
    except (KeyboardInterrupt, SystemExit):
      _exit()
  ser.close()

#parse Socket request (e.g. from WebSocketControl.py)
def ParseSocketRequest(request):
  printD("Parsing SocketRequest: "+request)
  returnlist = ""
  request = request.strip()
  requestsplit = request.split(':')
  requestsplit.append("dummy")
  command = requestsplit[0]
  value = requestsplit[1]
  if value == "dummy":
     value = "0"

  elif command == "get.bearing":
     if currentValue.get('bearing'):
        returnlist += "\n get.bearing:" + currentValue.get('bearing')
  elif command == "get.pitch":
     if currentValue.get('pitch'):
        returnlist += "\n get.pitch:" + currentValue.get('pitch')
  elif command == "get.roll":
     if currentValue.get('roll'):
        returnlist += "\n get.roll:" + currentValue.get('roll')

  elif command == "get.status.us1":
     if currentValue.get('distance'):
        returnlist += "\n get.status.us1:" + currentValue.get('distance')
  elif command == "get.status.ir1":
     if currentValue.get('IRfrontright'):
        returnlist += "\n get.status.ir1:" + currentValue.get('IRfrontright')
  elif command == "get.status.ir2":
     if currentValue.get('IRfrontleft'):
        returnlist += "\n get.status.ir2:" + currentValue.get('IRfrontleft')
  elif command == "get.status.ir3":
     if currentValue.get('IRrearleft'):
        returnlist += "\n get.status.ir3:" + currentValue.get('IRrearleft')
  elif command == "get.status.ir4":
     if currentValue.get('IRrearright'):
        returnlist += "\n get.status.ir4:" + currentValue.get('IRrearright')

  elif command == "get.encoder.left":
     if currentValue.get('LeftEncoder'):
        returnlist += "\n get.encoder.left:" + currentValue.get('LeftEncoder')
  elif command == "get.encoder.right":
     if currentValue.get('RightEncoder'):
        returnlist += "\n get.encoder.right:" + currentValue.get('RightEncoder')

  elif command == "get.defaultSettings":
     if any(defaultSetting):
        for key in defaultSetting:
           returnlist += "\n get.defaultSettings:" + key + ":" + defaultSetting.get(key)
        returnlist += "\n get.defaultSettings:END:END" #terminate list
     else:
        returnlist += "\n get.defaultSettings:NONE:NONE"

  elif command == "get.currentValues":
     if currentValue:
        for key in currentValue:
           returnlist += "\n get.currentValues:" + key + " " + currentValue.get(key)
     else:
        returnlist += "\n get.currentValues:NONE"

  elif command == "get.Map":
     if any(MapCoords):
        for key in MapCoords:
           returnlist += "\n get.Map:" + str(key) + ":" + MapCoords.get(key)
        returnlist += "\n get.Map:END:END" #terminate list
     else:
        returnlist += "\n get.Map:NONE:NONE"

  else:
     send_serial_data(command+":"+value)
     returnlist += "\n "+command+":ok"

  return returnlist


#https://code.google.com/p/rpicopter/source/browse/RPiQuadroServer.py?r=44fec034628c6086af320f89f1c91a60fbdd9310
#send to Arduino
def send_serial_data(line):
  output = "%s\r\n" % (line)
  try:
    bytes = ser.write(output)
  except Exception, e3:
    print("Error sending via serial port: " + str(e3))
  # Flush input buffer, if there is still some unprocessed data left
  ser.flush()       # Try to send old message
  #ser.flushInput()  # Delete what is still inside the buffer


#socket server <-> webinterface
class ThreadedTCPRequestHandler(socketserver.StreamRequestHandler):
  def handle(self):
    # self.rfile is a file-like object created by the handler;
    # we can now use e.g. readline() instead of raw recv() calls
    self.data = self.rfile.readline().strip()
    printD("SocketRequest: {}".format(self.data))
    try:
      self.wfile.write(ParseSocketRequest(self.data))
    except Exception, e2:
      print("Error in ThreadedTCPRequestHandler: " + str(e2))
    except (KeyboardInterrupt, SystemExit):
      _exit()

class ThreadedTCPServer(socketserver.ThreadingMixIn, socketserver.TCPServer):
  pass


if __name__ == '__main__':
  try:
    ser.open()
  except Exception, e:
    print("Error open serial port: " + str(e))
    exit()
  if ser.isOpen():
    try:
      ser.flushInput()  #flush input buffer, discarding all its contents
      ser.flushOutput() #flush output buffer, aborting current output and discard all that is in buffer
      HOST, PORT = SocketHost, SocketPort
      socket_server = ThreadedTCPServer((HOST, PORT), ThreadedTCPRequestHandler)
      ip, port = socket_server.server_address
      # Start a thread with the server - that thread will then start one more thread for each request
      socket_server_thread = threading.Thread(target=socket_server.serve_forever)
      socket_server_thread.start()
      # Start Serial read Thread
      read_thread = threading.Thread(target=Serial_read_Thread)
      read_thread.start()
      print("read_Thread ist aktiv: %s" % read_thread.isAlive())
      print("socket_server_thread ist aktiv: %s" % socket_server_thread.isAlive())
      #print("Socket Server loop running in thread: %s" % socket_server_thread.name)
      #thread_count = threading.active_count()
      #print("Insgesamt sind %s Threads aktiv." % thread_count)
    except Exception, e1:
      print("Error...: " + str(e1))
    except (KeyboardInterrupt, SystemExit):
      print("Schliesse Programme..")
      _exit()
  else:
    print("Cannot open serial port ")

def _exit():
  try:
    read_thread.exit()
    socket_server_thread.exit()
    sys.exit()
  except Exception, e3:
    exit()

# this is the main (3.) thread which exits after all is done, so Dont close ser here!