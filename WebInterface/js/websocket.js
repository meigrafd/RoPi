var ws;
var ws_status = "closed";
function set_ws_status(status) {
    ws_status = status;
    document.getElementById("connectionStatus").innerHTML = status;
}

function WebSocket_Close() {
    ws.close();
}

function WebSocket_Open() {
    ws = new WebSocket("ws://"+location.host+":7070");
    //mylog(ws);
    ws.onerror = function(evt) {
        mylog('Error detected: '+evt.data);
    }
    ws.onopen = function() {
        mylog('Connection opened!');
        set_ws_status("opened");
        GetDefaultSettings();
    }
    ws.onclose = function(evt) {
        if (isset(evt.reason)) {
        	mylog('Connection closed:'+evt.reason);
        } else {
        	mylog('Connection closed!');
        }
        set_ws_status("closed");
        receivedDefault = 'NONE';
    }
    ws.onmessage = function(evt) {
        var message = evt.data;
        mylog('Received message: >>>'+message+'<<<');
        parseResponse(message.split("\n"));
    }
}

function WebSocket_Send(data) {
	if (ws_status == "opened") {
		ws.send(data);
		mylog('Sent message: >>>'+data+'<<<');
	}
}
