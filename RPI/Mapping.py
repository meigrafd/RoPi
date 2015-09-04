#!/usr/bin/python
# -*- coding: utf-8 -*-
#
#   RobotMapping - Class for mapping functions
#
#   Created 15 Jan 2015
#   by meigrafd 
#
#
# version: 0.1
#
import sys
import time
import csv
from PIL import Image, ImageDraw
import logging

class mappingClass(object):
	"""
	Responsible for managing maps for the robot.
	"""
	# Size of initial map and the map itself
	yMax = 240
	xMax = 320
	map = ""
	
	# Cache for saved vectors from a radar scan
	cachedVectors = []
	# Pointer to the file where we will store all vectors for a session
	mapVectorFile = ""
	# Timestamp for vector file name
	timestamp=""
	
	# Controls driving how we display the radar image
	rangeMax = 160	# Uncertainty beyond
	rangeCertainty = {0:5, 10:4, 20:3, 30:2, 40:2, 50:1, 60:1}
	certaintyColors = {
		"K":"#303030",
		"J":"#404040",
		"I":"#505050",
		"H":"#606060",
		"G":"#707070",
		"F":"#808080",
		"E":"#909090",
		"D":"#A0A0A0",
		"C":"#B0B0B0",
		"B":"#C0C0C0",
		"A":"#D0D0D0",
		"?":"red",
		" ":"white",
		"-":"yellow",
		"@":"blue"
	}
	
	# Used for sonar beam diffusion
	vectorLeft = []
	vectorRight = []

	logger = ""

	def __init__(self, arduino, sensor, radar, navigation, utility, intercom):
		self.arduino = arduino
		self.sensor = sensor
		self.radar = radar
		self.navigation = navigation
		self.utility = utility
		self.intercom = intercom

		# Setup logging by connecting to our main logger
		self.logger = logging.getLogger('Main.' + __name__)

		# Initialize a blank map
		self.map = [ [ 0 for i in range(self.xMax) ] for j in range(self.yMax) ]
		for y in range(self.yMax):
			for x in range(self.xMax):
				self.map[y][x] = "-"

		if self.intercom["options"].map == "":
			# Open a new file to store vectors for saved map
			self.timestamp = "{:02}".format(time.gmtime().tm_mon)
			self.timestamp += "{:02}".format(time.gmtime().tm_mday)
			self.timestamp += "{:02}".format(time.gmtime().tm_hour)
			self.timestamp += "{:02}".format(time.gmtime().tm_min)
			self.mapVectorFile = file( "/home/Robot/maps/RobotMap" + "-" + self.timestamp + ".csv", "w" )
			self.mapVectorFile.write("'bearing','range','y','x'\n")
		else:
			self.loadMap(self.intercom["options"].map)
			self.mapVectorFile = file( "/home/Robot/maps/RobotMap" + "-" + \
				self.intercom["options"].map + ".csv", "a" )

		# Draw the front of the robot
		bearing = self.navigation.bearing()
		bearingLeft = bearing - 90
		if bearingLeft < 0:
			bearingLeft = 360 + bearingLeft
		self.applyVectorToMap(bearingLeft, 4, "@", self.navigation.y, self.navigation.x)
		bearingRight = bearing + 90
		if bearingRight > 360:
			bearingRight = bearingRight - 360
		self.applyVectorToMap(bearingRight, 4, "@", self.navigation.y, self.navigation.x)
		
		# Save bearing lines of at least 11 to each side of the 'bot sensor
		self.vectorLeft = self.vector(bearingLeft, 20,
			self.navigation.y, self.navigation.x, False)
		self.vectorRight = self.vector(bearingRight, 20,
			self.navigation.y, self.navigation.x, False)

		# Draw the right and left sides of the robot
		bearingBack = bearing - 180
		if bearingBack < 0:
			bearingBack = 360 + bearingBack
		vector = self.vector(bearingLeft, 4,
			self.navigation.y, self.navigation.x, False)
		self.applyVectorToMap(bearingBack, 8, "@", vector[3][0], vector[3][1])
		vector = self.vector(bearingRight, 4, self.navigation.y, self.navigation.x, False)
		self.applyVectorToMap(bearingBack, 8, "@", vector[3][0], vector[3][1])
		
		self.utility.logger("debug", "Initialized mapping")
		self.arduino.LCD(0, "Mapping up")

	# ------------------------------------------------------------------------
	def execute(self):
		"""
		Executes any commands pending in the intercom for mapping
		"""

		if self.intercom["updateMap"]:
			self.intercom["updateMap"] = False
			self.updateMap()
			# Save a copy of the text map
			self.saveMap(self.dumpTextMap())
			self.dumpImageMap()
			self.mapVectorFile.close()
			self.mapVectorFile = file( "/home/Robot/maps/RobotMap" + "-" + self.timestamp + ".csv", "a" )
			
	# ------------------------------------------------------------------------
	def loadMap(self, mapTimestamp):
		"""
		Loads a map from a previous execution for further use - display or update.  Obviously
		needs to be from a known (current) position!
		"""

		try:
			self.mapVectorFile = file( "maps/RobotMap" + "-" + mapTimestamp + ".csv", "r" )
		except IOError:
			self.utility.logger("debug", "Map file timestamp specified by -m option, '" + \
				mapTimestamp + "' does not exist.")
			sys.exit(1)

		reader = csv.reader(self.mapVectorFile)
		rownum = 0
		for row in reader:
			# Save header row.
			if rownum == 0:
				header = row
			else:
				self.applyVectorToMap(int(row[0]), float(row[1]), " ", int(row[2]), int(row[3]))
			rownum += 1
		self.mapVectorFile.close()

	# ------------------------------------------------------------------------
	def updateMap(self):
		"""
		Updates the current map with a sensor scan as specified via the intercom (start, end, increment) 
		"""
		if self.intercom["mapping"]["endOrient"] > self.intercom["mapping"]["startOrient"]:
			self.utility.logger("debug", "Ending orientation has to be lower than starting!")
			return
		# Scan the range communicated via the intercom
		bearing = self.navigation.bearing()
		orient = self.intercom["mapping"]["startOrient"]
		vectorCount = 0
		while orient >= self.intercom["mapping"]["endOrient"]:
			self.sensor.horizontal = orient
			self.sensor.vertical = 0
			self.sensor.execute()
			# Sleep after movement depending on increment
			time.sleep(.5 + float("." + str(self.intercom["mapping"]["increment"] * 2)))
			range = self.radar.range()
			self.utility.logger("debug", "Range at orientation " + str(orient) + " is " + str(range) + \
				" (" + str(float(range)/12) + " feet)")
			vector = bearing + orient
			if vector < 0:
				vector = 360 + vector
			self.saveVector(vectorCount, vector, range,
				" ", self.navigation.y, self.navigation.x)
			orient -= self.intercom["mapping"]["increment"]
			vectorCount += 1
		self.applySavedVectorsToMap()

	# ------------------------------------------------------------------------
	def applySavedVectorsToMap(self):
		self.cachedVectors.sort()
		for radarRange, vectorBearing, symbol, y, x in self.cachedVectors:
			self.applyRadarToMap(vectorBearing, radarRange, symbol, y, x)

	# ------------------------------------------------------------------------
	def applyRadarToMap(self, vectorBearing, radarRange, symbol, y, x):
		"""
		Applies the multiple vectors that make up a radar reading, incorporating the beam spread, 
		to the text map. 
		"""
		beamSpread = {
			2:2,	 3:2,	4:3,
			5:3,	 6:4,	7:4,
			8:5,	 9:5,   10:6,
			11:6,   12:7,   13:7,
			14:8,   15:8,   16:9,
			17:9,   18:10,  19:10,
			20:10,  21:10,  22:11
		}
		self.utility.logger("debug", "Applying radar image to map, bearing = " + \
			str(vectorBearing) + ", range = " + str(radarRange))
		for rangeStart in range(2, 22):
			spread = beamSpread[rangeStart]
			# The last entry in this vector will be the start for the one we wish to paint
			for iterate in range(1, spread):
				# Left side
				tempVector = self.vector(vectorBearing, rangeStart,
					self.vectorLeft[iterate - 1][0], self.vectorLeft[iterate - 1][1], True)
				tempY = tempVector[len(tempVector) - 1][0]
				tempX = tempVector[len(tempVector) - 1][1]
				# Apply the new vector alongside the others
				self.applyVectorToMap(vectorBearing,
					self.adjustRange(vectorBearing, radarRange - rangeStart), symbol, tempY, tempX)

				# Right side
				tempVector = self.vector(vectorBearing, rangeStart,
					self.vectorRight[iterate - 1][0], self.vectorRight[iterate - 1][1], True)
				tempY = tempVector[len(tempVector) - 1][0]
				tempX = tempVector[len(tempVector) - 1][1]
				# Apply the new vector alongside the others
				self.applyVectorToMap(vectorBearing,
					self.adjustRange(vectorBearing, radarRange - rangeStart), symbol, tempY, tempX)
				print "Range Spr = " + str(self.adjustRange(vectorBearing, radarRange - rangeStart))

		# Apply the centerline vector
		self.applyVectorToMap(vectorBearing,
			self.adjustRange(vectorBearing, radarRange), symbol, y, x)
		print "Range Ctr = " + str(self.adjustRange(vectorBearing, radarRange))

	# ------------------------------------------------------------------------
	def applyVectorToMap(self, vectorBearing, radarRange, symbol, y, x):
		"""
		Applies a vector to the text map.  
		"""
		if radarRange > self.rangeMax:
			radarRangeTemp = self.rangeMax
			symbolTemp = "?"
		else:
			radarRangeTemp = radarRange
			symbolTemp = symbol
		
		vector = self.vector(vectorBearing, int(radarRangeTemp) + 1, y, x, True)

		for i in vector:
			if i < len(vector) - 2:
				self.setCell(vector[i][0], vector[i][1], symbol)
			else:
				self.setLastCell(vector[i][0], vector[i][1], radarRangeTemp, symbolTemp)

	# ------------------------------------------------------------------------
	def setLastCell(self, y, x, radarRange, symbol):
		"""
		Sets the final cell (detected object or end of range) in the text map
		"""	
		symbolTemp = "A"

		# Get the current symbol from the map
		yFlipped = self.yMax - y
		xFlipped = self.xMax - x
		currentSymbol = self.map[yFlipped - 1][xFlipped - 1]

		# Early return based on clipping
		self.map[yFlipped - 1][xFlipped - 1] = symbolTemp
		return

		# If we are already displaying our max certainty "K" go home
		if currentSymbol == "K":
			return
		else:
			if currentSymbol == "?" or currentSymbol == " " or currentSymbol == "-":
				currentSymbol = "A"

		# If we are at our max range we don't need to do anything
		if symbolTemp != "?" and symbolTemp != "@":
			tempRange = int(radarRange/10)*10
			if tempRange > 60:
				tempRange = 60
			symbolAdjust = self.rangeCertainty[tempRange]
			symbolTemp = chr(ord(currentSymbol) + symbolAdjust)
			if symbolTemp > "K":
				symbolTemp = "K"

		self.map[yFlipped - 1][xFlipped - 1] = symbolTemp

	# ------------------------------------------------------------------------
	def setCell(self, y, x, symbol):
		"""
		Sets a cell in the text map
		"""
		yFlipped = self.yMax - y
		xFlipped = self.xMax - x
		self.map[yFlipped - 1][xFlipped - 1] = symbol

	# ------------------------------------------------------------------------
	def saveVector(self, vectorCount, vectorBearing, radarRange, symbol, y, x):
		"""
		Saves a vector
		"""
		# Save to file
		self.mapVectorFile.write(str(vectorBearing) + "," + str(radarRange) + "," + str(y) + "," + str(x) + "\n")

		# Save to dictionary that will be sorted for application to the map
		if vectorCount == 0:
			self.cachedVectors = []		
		self.cachedVectors.append([radarRange, vectorBearing, symbol, y, x])
		
	# ------------------------------------------------------------------------
	def dumpTextMap(self):
		"""
		Dumps the current map as a text file.  
		"""
		self.utility.logger("debug", "Saving Text Map")
		textMap = ""
		for y in range(self.yMax):
			textRow = ""
			for x in range(self.xMax):
				textRow += self.map[y][x]
			textMap += textRow + "\n"
		# Return the text map to the caller
		return textMap

	# ------------------------------------------------------------------------
	def dumpImageMap(self):
		"""
		Dumps the current map as an image.  Uploads the image to Control.
		"""
		self.utility.logger("debug", "Saving Image Map")
		scale = 2
		im = Image.new('RGB', (self.xMax * scale, self.yMax * scale), "yellow")
		draw = ImageDraw.Draw(im)
		for y in range(self.yMax):
			for x in range(self.xMax):
				if self.map[y][x] != "-":
					color = self.certaintyColors[self.map[y][x]]
					draw.rectangle((x * scale, y * scale, x * scale + scale, y * scale + scale), \
						fill=color, outline=color)
		del draw
		im.save("maps/RobotMap.png", "PNG")
		# Upload the image map to Control
		self.utility.uploadMap()
		return

	# ------------------------------------------------------------------------
	def saveMap(self, map):
		"""
		Saves the text map to maps/RobotMap.txt
		"""
		mapFile = file( "maps/RobotMap.txt", "w" )
		mapFile.write(map)
		mapFile.close()

	# ------------------------------------------------------------------------
	def vector(self, bearing, radarRange, yVector, xVector, adjustRange):
		if bearing >= 0 and bearing < 90:
			xComponent = bearing
			yComponent = 90 - xComponent
	
		if bearing >= 90 and bearing < 180:
			yComponent = (bearing - 90) * -1
			xComponent = 90 + yComponent
	
		if bearing >= 180 and bearing < 270:
			xComponent = (bearing - 180) * -1
			yComponent = (90 + xComponent) * -1
	
		if bearing >= 270:
			yComponent = bearing - 270
			xComponent = (90 - yComponent) * -1
					
		if abs(xComponent) < 45:
			z = 1 - float((45 - xComponent)) / 45
		else:
			z = 1 - float((xComponent - 45)) / 45
	
		yIncrement = float(yComponent) / 90
		xIncrement = float(xComponent) / 90

		if adjustRange:
			radarRange = self.adjustRange(bearing, radarRange)

		returnVector={}
		for z in range(radarRange):
			yVector = yVector + yIncrement
			xVector = xVector + xIncrement
			if xVector <= self.xMax and yVector <= self.yMax and xVector >= 1 and yVector >= 1:
				returnVector[z] = [int(yVector), int(xVector)]			
		return returnVector
	
	# ------------------------------------------------------------------------
	def adjustRange(self, bearing, radarRange):
		if bearing >= 0 and bearing < 90:
			xComponent = bearing
			yComponent = 90 - xComponent
	
		if bearing >= 90 and bearing < 180:
			yComponent = (bearing - 90) * -1
			xComponent = 90 + yComponent
	
		if bearing >= 180 and bearing < 270:
			xComponent = (bearing - 180) * -1
			yComponent = (90 + xComponent) * -1
	
		if bearing >= 270:
			yComponent = bearing - 270
			xComponent = (90 - yComponent) * -1
					
		if abs(xComponent) < 45:
			z = 1 - float((45 - xComponent)) / 45
		else:
			z = 1 - float((xComponent - 45)) / 45
	
		yIncrement = float(yComponent) / 90
		xIncrement = float(xComponent) / 90

		smallestComponent = abs(xComponent)
		if abs(yComponent) < abs(xComponent):
			smallestComponent = abs(yComponent)
		factor = 1 - (float(smallestComponent) / 45) * 0.29
		radarRange = int(round(radarRange * factor))
		return radarRange
	
	# ------------------------------------------------------------------------
	def expand(self, axis, increment):
		"""
		Expand the map on the selected axis (t)op, (b)ottom, (r)ight, or
		(l)eft by the increment
		"""
		# Temporary map
		mapTemp = {}

		# Expand the map at the top
		if axis == "t":
			# Create a template empty map row
			mapRow={}
			for i in range(self.xMax):
				mapRow[i] = "-"
			# Add empty rows at the top of the temporary map
			for i in range(increment):
				mapTemp[i] = mapRow
			# Copy over the current map
			for i in range(self.yMax):
				mapTemp[i+increment] = self.map[i]
			# Update the map from the temp (and it's new size)
			self.map = mapTemp
			self.yMax = self.yMax + increment
			# Update our position to reflect rows added at top of map
			self.navigation.y = self.navigation.y + increment
			return
		
		# Expand the map at the bottom
		if axis == "b":
			# Create a template empty map row
			mapRow={}
			for i in range(self.xMax):
				mapRow[i] = "-"
			# Copy the current map to the temporary map
			for i in range(self.yMax):
				mapTemp[i] = self.map[i]
			# Add new rows at the bottom of the temporary map
			for i in range(increment):
				mapTemp[self.yMax + i] = mapRow
			# Update the map from the temp (and it's new size)
			self.map = mapTemp
			self.yMax = self.yMax + increment
			return

		# Expand the map at the left
		if axis == "l":
			mapRow={}
			for i in range(self.yMax):
				for j in range(self.xMax + increment):
					if j < increment:
						mapRow[j] = "-"
					else:
						mapRow[j] = self.map[i][j - increment]
				self.map[i] = mapRow
			# Update the map's new size
			self.xMax = self.xMax + increment
			# Update our position to reflect rows added at top of map
			self.navigation.x = self.navigation.x + increment
			return

		# Expand the map at the right
		if axis == "r":
			mapRow={}
			for i in range(self.yMax):
				for j in range(self.xMax + increment):
					if j > self.xMax - 1:
						mapRow[j] = "-"
					else:
						mapRow[j] = self.map[i][j]
				self.map[i] = mapRow
			# Update the map's new size
			self.xMax = self.xMax + increment
			return

