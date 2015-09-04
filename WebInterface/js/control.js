//
// control javascript for RoPi - v0.8
//

//----------------------------------------------------------------
//           Usefull functions
//----------------------------------------------------------------

if (typeof(String.prototype.strip) === "undefined") {
	String.prototype.strip = function() {
		return String(this).replace(/^\s+|\s+$/g, '');
	};
}

function isset(strVariableName) { 
	try { 
		eval( strVariableName );
	} catch( err ) { 
		if ( err instanceof ReferenceError ) 
			return false;
	}
	return true;
}

function sleep(millis, callback) {
  setTimeout(function() { callback(); } , millis);
}

//source of: http://www.html5tutorial.info/html5-range.php
function printValue(sliderID, textbox) {
	var x = document.getElementById(textbox);
	var y = document.getElementById(sliderID);
	x.value = y.value;
}

//----------------------------------------------------------------
//           Init functions
//----------------------------------------------------------------

function init() {
	// Initalize the various page elements here...
	WebSocket_Open();
}

function init_control() {
	// Initalize the various page elements here...
	WebSocket_Open();
	Init_Compass();
	Init_Map();
	// start Main Timers
	setInterval(PingWatchdog, 300);
	setInterval(Robot_ping, 300);
	setInterval(GetGridMap, 1000);
	setInterval(Robot_SystemStatusUpdate, 500);
	setInterval(Robot_DriveStatusUpdate, 500);
	setInterval(Robot_StatusUpdate, 500);
}

//----------------------------------------------------------------
//           Grid Map
//----------------------------------------------------------------

// canvas : http://www.peterkroener.de/eine-kleine-canvas-einfuehrung/

//http://www.tayloredmktg.com/rgb/
//es funktionieren aber auch html colors
var lightgrey = "rgb(184,184,184)"; // background/unknown
var black = "rgb(0,0,0)";           // wall/barrier
var red = "rgb(255,0,0)";           // barrier
var green = "rgb(0,255,0)";         // goal
var blue = "rgb(0,0,255)";          // robot
var white = "rgb(255,255,255)";     // nothing/free
var grid = "rgb(255,250,205)";      // grid lines

function Init_Map() {
	var canvas = document.getElementById('map');
	//Canvas supported?
	if (canvas.getContext){
		var context = canvas.getContext('2d');
		context.fillStyle = lightgrey;
		context.fillRect( 0, 0, canvas.width, canvas.height );
	}
}

function drawMap(GridMapArray) {
	var canvas = document.getElementById('map');
	//Y is vertical, X is horizontal
	for (var Y = 0; Y < GridMapArray.length; Y++) {
		var Coords = GridMapArray[Y].strip().split(",");
		for (var X = 0; X < Coords.length; X++) {
			if (Coords[X] == "") { continue; }
			if (Coords[X] == ",") { continue; }
			if (Coords[X] == 0) { // nothing, white
				var r = 255;
				var g = 255;
				var b = 255;
			} else if (Coords[X] == 1) { // goal, green
				var r = 0;
				var g = 255;
				var b = 0;
			} else if (Coords[X] == 254) { // robot, blue
				var r = 0;
				var g = 0;
				var b = 255;
			} else if (Coords[X] == 255) { // wall, black
				var r = 0;
				var g = 0;
				var b = 0;
			}
			drawPixel(canvas, X, Y, r, g, b);
		}
	}
}

function drawPixel(canvas, x, y, r, g, b) {
	if (canvas.getContext){
		var context = canvas.getContext('2d');
		var canvasData = context.getImageData(0, 0, canvas.width, canvas.height);
		var PixelSize = 4;
		x *= PixelSize;
		y *= PixelSize;
		var index;
		for (var i = 0; i < PixelSize; i++) {
			for (var j = 0; j < PixelSize; j++) {
				index = (x  + (y + j) * canvas.width) * 4;
				canvasData.data[index + 0] = r;
				canvasData.data[index + 1] = g;
				canvasData.data[index + 2] = b;
				canvasData.data[index + 3] = 255; //Alpha Channel
			}
			++x;
		}
		context.putImageData(canvasData, 0, 0);
	}
}

//----------------------------------------------------------------
//           Compass
//----------------------------------------------------------------

function Init_Compass() {
	var canvas = document.getElementById('compass');
	if (canvas.getContext){
		var context = canvas.getContext('2d');
		var w = canvas.width;
		var h = canvas.height;
		var FontSize = 15;
		var NorthPos = w / 2 - FontSize; // when w = 200 and FontSize = 15 -> NorthPos = 85
		var OstPos = h / 2 - FontSize;
		var SouthPos = w / 2 - FontSize;
		var WestPos = h / 2 - FontSize;
		context.font = FontSize+"px Georgia";
		context.fillStyle = 'black';
		// "text", horizontal, vertical
		context.fillText("N", NorthPos, FontSize);
		context.fillText("O", (w - FontSize), OstPos);
		context.fillText("S", SouthPos, h);
		context.fillText("W", 0, WestPos);
	}
}

function resetCompass() {
	// clear canvas
	var canvas = document.getElementById('compass');
	if (canvas.getContext){
		var context = canvas.getContext('2d');
		context.clearRect(0, 0, canvas.width, canvas.height);
		Init_Compass();
	}
}

// http://www.dhtmlgoodies.com/tutorials/canvas-clock/
function drawCompass(degrees) {
	var canvas = document.getElementById('compass');
	if (canvas.getContext){
		resetCompass();
		var context = canvas.getContext('2d');
		var w = canvas.width;
		var h = canvas.height;
		var FontSize = 15;
		var HandSize = w / 2 - FontSize;
		context.save();
		context.fillStyle = 'green';
		context.strokeStyle = '#555';
		// Move registration point to the center of the canvas
		context.translate(w/2-FontSize, h/2-FontSize);
		// Rotate degrees
		context.rotate(degreesToRadians(degrees));
		context.beginPath();
		context.moveTo(0, 0) // this will actually be (85,85) in relation to the upper left corner
		context.lineTo(0, HandSize) // (250,250)
		context.stroke();
		// Move registration point back to the top left corner of canvas
		//context.translate(-w/2, -h/2);
		context.restore();
	}
}

function degreesToRadians(degrees) {
	return (Math.PI / 180) * degrees
}

//----------------------------------------------------------------
//           Misc functions
//----------------------------------------------------------------

function GetGridMap() {
  WebSocket_Send('get.Map');
}
var GridMapArray = new Array();

// http://stackoverflow.com/questions/4539253/what-is-console-log
function mylog(message) {
	if (isset(DEBUG) && DEBUG == 1) {
		console.log(message);
		var logthingy;
		logthingy = document.getElementById("Log");
		if( logthingy.innerHTML.length > 5000 )
			logthingy.innerHTML = logthingy.innerHTML.slice(logthingy.innerHTML.length-5000);
		logthingy.innerHTML = logthingy.innerHTML+"<br/>"+message;
		logthingy.scrollTop = logthingy.scrollHeight*2;
	}
}

var pingCtr = 1;
function PingWatchdog() {
	if (ws_status == "opened") {
		pingCtr -= 1;
		if( pingCtr == 0) {
			pingCtr = 1;
			document.getElementById("pingSymbol").innerHTML = "Error";
		} else {
			document.getElementById("pingSymbol").innerHTML = "Ok";
		}
  }
}

function Robot_ping() {
	if (ws_status == "opened") {
		WebSocket_Send('localping');
	}
}

function Robot_SystemStatusUpdate() {
	if (ws_status == "opened") {
		WebSocket_Send('get.system.loadavg \n get.system.uptime \n get.system.ram');
	}
}

function Robot_DriveStatusUpdate() {
	if (ws_status == "opened") {
		WebSocket_Send('get.bearing \n get.pitch \n get.roll \n get.encoder.left \n get.encoder.right');
	}
}

function Robot_StatusUpdate() {
	if (ws_status == "opened") {
		WebSocket_Send('get.status.us1 \n get.status.ir1 \n get.status.ir2 \n get.status.ir3 \n get.status.ir4');
	}
}

function Robot_SetSpeed(direction) {
	if (direction == "left") { 
		speed = document.getElementById("leftspeed").value.toString();
		command = "set.motor.left:"+speed;
		WebSocket_Send(command);
	}
	if (direction == "right") {
		speed = document.getElementById("rightspeed").value.toString();
		command = "set.motor.right:"+speed;
		WebSocket_Send(command);
	}
	if (direction == "master") {
		speed = document.getElementById("masterspeed").value.toString();
		rightcommand = "set.motor.right:"+speed;
		leftcommand = "set.motor.left:"+speed;
		WebSocket_Send(rightcommand+" \n "+leftcommand);
	}
}

var receivedDefault = 'NONE';
function GetDefaultSettings() {
	// wait till default values from Arduino received
	if (ws_status == "opened" && receivedDefault == "NONE") {
		WebSocket_Send('get.defaultSettings');
		sleep(3000, GetDefaultSettings);
	} else if (ws_status == "closed") {
		sleep(3000, GetDefaultSettings);
	}
}

function LoadControlPage() {
	var check = window.location.pathname;
	//mylog(".............window.location.pathname: "+check);
	if (check == "/" || check == "/index.php") {
		document.location="http://"+location.host+"/ropi.php";
	}
}

//----------------------------------------------------------------
//           Parse Response from/over WebSocket
//----------------------------------------------------------------

function parseResponse(requestlist) {
	//mylog("Parsing: "+requestlist)
	for (var i=0; i<requestlist.length; i++) {
		var requestsplit = requestlist[i].strip().split(':')
		requestsplit[requestsplit.length] = "dummy";
		command = requestsplit[0];
		value = requestsplit[1];
		value2 = requestsplit[2];

		if( command == "get.defaultSettings" ) {
			if (value == "NONE" && value2 == "NONE") {
				sleep(500, WebSocket_Send('get.defaultSettings'));
			} else {
				if (value == "END" && value2 == "END") {
					receivedDefault = 'TRUE';
					sleep(2000, LoadControlPage);
				} else {
					img = new Image();
					img.src='include/defaultSettings.php?'+value+'='+value2;
				}
			}
		}
//
// Received message: >>> get.Map:0:255,255,255,255,255,255,255,255,255,255, get.Map:1:255,0,0,0,255,0,0,0,0,255, get.Map:2:255,0,254,0,255,0,1,0,0,255, get.Map:3:255,0,0,0,255,0,0,0,0,255, get.Map:4:255,0,0,0,255,255,255,0,0,255, get.Map:5:255,0,255,0,255,0,0,0,0,255, get.Map:6:255,0,255,0,255,0,0,0,0,255, get.Map:7:255,0,255,255,255,0,255,255,255,255, get.Map:8:255,0,0,0,0,0,0,0,0,255, get.Map:9:255,255,255,255,255,255,255,255,255,255, get.Map:END:END<<<
//
		if( command == "get.Map" ) {
			if (value == "NONE" && value2 == "NONE") {
				mylog("..Currently no Grid Map Data available!..");
			} else {
				if (value == "END" && value2 == "END") {
					drawMap(GridMapArray);
				} else {
					//add to array..
					GridMapArray[value] = value2;
				}
			}
		}

		if( command == "localping" )
			if( value == "ok" )
				pingCtr = 5;

		if( command == "get.bearing" ) {
			document.getElementById("DriveAnglenum").innerHTML = value+"&deg;";
			document.getElementById("DriveAngleperc").value=50+parseFloat(value)/2;
			drawCompass(value);
		}
		if( command == "get.pitch" ) {
			document.getElementById("PITCHnum").innerHTML = value;
			document.getElementById("PITCHperc").value=50+parseFloat(value)/2;
		}
		if( command == "get.roll" ) {
			document.getElementById("ROLLnum").innerHTML = value;
			document.getElementById("ROLLperc").value=50+parseFloat(value);
		}

		if( command == "get.status.us1" ) {
			document.getElementById("US1num").innerHTML = value;
		}
		if( command == "get.status.ir1" ) {
			document.getElementById("IR1num").innerHTML = value;
		}
		if( command == "get.status.ir2" ) {
			document.getElementById("IR2num").innerHTML = value;
		}
		if( command == "get.status.ir3" ) {
			document.getElementById("IR3num").innerHTML = value;
		}
		if( command == "get.status.ir4" ) {
			document.getElementById("IR4num").innerHTML = value;
		}

		if( command == "get.encoder.left" ) {
			document.getElementById("ELnum").innerHTML = value;
		}
		if( command == "get.encoder.right" ) {
			document.getElementById("ERnum").innerHTML = value;
		}
    
		if( command == "get.system.loadavg" ) {
			document.getElementById("LoadAVGnum").innerHTML = value;
			document.getElementById("LoadAVGperc").value=parseFloat(value)*100;
		}
		if( command == "get.system.ram" ) {
			document.getElementById("RAMnum").innerHTML = parseFloat(value)+"%";
			document.getElementById("RAMperc").value=parseFloat(value);
		}
		if( command == "get.system.uptime" ) {
			document.getElementById("uptime").innerHTML = requestsplit[1]+":"+requestsplit[2]+":"+requestsplit[3];
		}
	}
}
