<?php
session_start();
require_once("include/global.php");
?>
<!DOCTYPE html>
<html>
 <head>
  <meta http-equiv="Content-Type" content="text/html; charset=utf-8">
  <meta name="robots" content="DISALLOW">
  <meta charset="utf-8">
  <title>RoPi-Control</title>
  <link rel="stylesheet" type="text/css" href="css/style.css">
  <script>
  	var DEBUG = <?php echo $DEBUG; ?>;
  </script>
  <script src="js/websocket.js"></script>
  <script src="js/control.js"></script>
  <script>
    function outputLeftSpeed(speed) {
      document.querySelector('#leftMotorSpeed').value = speed;
    }
    function outputRightSpeed(speed) {
      document.querySelector('#rightMotorSpeed').value = speed;
    }
    function outputMasterSpeed(speed) {
      document.querySelector('#masterMotorSpeed').value = speed;
    }
    function outputPan(pos) {
      document.querySelector('#PanPos').value = pos;
    }
    function outputTilt(pos) {
      document.querySelector('#TiltPos').value = pos;
    }
  </script>
  <script src="js/swfobject.js"></script>
  <script>
    var parameters = {
      src: "rtmp://" + window.location.hostname + "/rtmp/live",
      width: 640, height: 480,
      autoPlay: true,
      controlBarAutoHide: true,
      controlBarMode: "floating",
      playButtonOverlay: false,
      showVideoInfoOverlayOnStartUp: false,
      optimizeBuffering : false,
      optimizeInitialIndex: false,
      liveBufferTime: 0.1,
      liveDynamicStreamingBufferTime: 0.1,
      initialBufferTime : 0.1,
      expandedBufferTime : 0.1,
      minContinuousPlayback : 0.1,
      posterScaleMode: "none",
      streamType: "live",
      scaleMode: "none",
      muted: true,
      verbose: false,
      enableStageVideo: false
    };

    swfobject.embedSWF("include/StrobeMediaPlayback.swf", "StrobeMediaPlayback", 
                   640, 480, "10.1.0", "expressInstall.swf", parameters, 
                   {allowFullScreen: "true", wmode: "opaque"},
                   {name: "StrobeMediaPlayback"});
  </script>
 </head>
 <body onload='init_control();'>
   <div style="text-align:center; width:100%; margin-top: 30px;">
     <div id="contentBox">
<!--- Top Table --->
       <div class="divtable">
         <div id="Connection" class="divtablecellborder">
           <b>Connection:</b>
           <input type="button" value="open" onclick="WebSocket_Open();" />
           <input type="button" value="close" onclick="WebSocket_Close();" />
           <br/> <b>Status:</b> <span id="connectionStatus" >closed</span>
           <br/> <b>Ping:</b> <span id="pingSymbol" >&#183;</span>
           <br/> <b>Power:</b> <input type="button" value="shutdown" onclick="WebSocket_Send('set.system.power:off')" />&nbsp;<input type="button" value="reboot" onclick="WebSocket_Send('set.system.power:reboot')" />
           <br/><br/><br/><br/>
           <b>System:</b> <br/>
           &nbsp; LoadAVG: <progress id="LoadAVGperc" value="0" max="100"></progress> <span id="LoadAVGnum" ></span> <br/>
           &nbsp; RAM: &nbsp; &nbsp; <progress id="RAMperc" value="0" max="100"></progress> <span id="RAMnum" >%</span> <br/>
           &nbsp; Uptime: &nbsp;<span id="uptime" >__:__:__</span> <br/>
         </div>

         <div id="View" class="divtablecellborder">
           <div class="divtable">
             <div id="PanTilt" class="divtablecell">
               &nbsp; &nbsp; <label for="Tilt">Tilt:</label> <output for="Tilt" id="TiltPos"><?php echo $_SESSION['start_pos_tilt']; ?></output>
               <br/><input type="range" id="Tilt" min="<?php echo $_SESSION['ScannerServoMaxTiltUP']; ?>" max="<?php echo $_SESSION['ScannerServoMaxTiltDOWN']; ?>" value="<?php echo $_SESSION['start_pos_tilt']; ?>" class="vertical" orient="vertical" onInput="outputTilt(value);" onChange="WebSocket_Send('tilt:'+value);">
               <br/><input type="range" id="Pan" min="<?php echo $_SESSION['ScannerServoMinPan']; ?>" max="<?php echo $_SESSION['ScannerServoMaxPan']; ?>" value="<?php echo $_SESSION['start_pos_pan']; ?>" onInput="outputPan(value);" onChange="WebSocket_Send('pan:'+value);">
               <br/><label for="Pan">Pan:</label> <output for="Pan" id="PanPos"><?php echo $_SESSION['start_pos_pan']; ?></output>
             </div>
             <div id="Camera" class="divtablecell">
               <div id="StrobeMediaPlayback">
                 <!-- fallback for devices without Flash is HLS streaming -->
                 <video src=/hls/live.m3u8 width=640 height=480 autoplay controls></video>
               </div>
             </div>
           </div>
         </div>

         <div id="Orientation" class="divtablecellborder">
           <div class="divtable">
             <div id="Comps" class="divtablecell">
     	         <canvas id="compass" width="200" height="200">Your browser does not support the HTML5 canvas tag.</canvas>
             </div>
           </div>
           <div class="divtable">
             <div id="Mapping" class="divtablecell">
     	         <canvas id="map" width="300" height="300">Your browser does not support the HTML5 canvas tag.</canvas>
             </div>
           </div>
         </div>

       </div>
<!--- Lower Table --->
       <div class="divtable">

         <div id="column1" class="divtablecellborder">
           <b>Sensor - Status:</b> <br/><br/>
           <table border="0">
           	<tr><td>&nbsp; US Distance: &nbsp;</td> <td><span id="US1num"></span>&nbsp;</td></tr>
             <tr><td>&nbsp; IR front right: &nbsp;</td> <td><span id="IR1num"></span>&nbsp;</td></tr>
             <tr><td>&nbsp; IR front left: &nbsp;</td> <td><span id="IR2num"></span>&nbsp;</td></tr>
             <tr><td>&nbsp; IR rear left: &nbsp;</td> <td><span id="IR3num"></span>&nbsp;</td></tr>
             <tr><td>&nbsp; IR rear right: &nbsp;</td> <td><span id="IR4num"></span>&nbsp;</td></tr>
           </table>
           <br/><br/><br/><br/>
           &nbsp;<input type="button" value="US Ping" onclick="WebSocket_Send('ping');" />&nbsp;
         </div>

         <div id="column2" class="divtablecellborder">
           <div class="divtable">
             <div id="MotorSpeedLeft" class="divtablecell">
               <label for="leftspeed">Left Speed:</label><br/>
               <input type="range" id="leftspeed" min="<?php echo $_SESSION['minMotorSpeed']; ?>" max="255" value="<?php echo $_SESSION['MotorSpeed']; ?>" class="vertical" orient="vertical" onInput="outputLeftSpeed(value);" onChange="Robot_SetSpeed('left');">
               <br/>&nbsp;<output for="leftspeed" id="leftMotorSpeed"><?php echo $_SESSION['MotorSpeed']; ?></output>
             </div>
             <div id="movement" class="divtablecell">
               &nbsp;
               <input type="button" value="&#8598; " style="height:60px; width:60px" onClick="WebSocket_Send('forwardleft')" />
               <input type="button" value="&#8593; " style="height:60px; width:60px" onClick="WebSocket_Send('forward')" />
               <input type="button" value="&#8599; " style="height:60px; width:60px" onClick="WebSocket_Send('forwardright')" />
               <br/>                &nbsp;
               <input type="button" value="&#8592; " style="height:60px; width:60px" onClick="WebSocket_Send('left')" />
               <input type="button" value="Stop"     style="height:60px; width:60px" onClick="WebSocket_Send('stop')" />
               <input type="button" value="&#8594; " style="height:60px; width:60px" onClick="WebSocket_Send('right')" />
               <br/>                &nbsp;
               <input type="button" value="&#8601; " style="height:60px; width:60px" onClick="WebSocket_Send('backwardleft')" />
               <input type="button" value="&#8595; " style="height:60px; width:60px" onClick="WebSocket_Send('backward')" />
               <input type="button" value="&#8600; " style="height:60px; width:60px" onClick="WebSocket_Send('backwardright')" />
             </div>
             <div id="MotorSpeedRight" class="divtablecell">
               <label for="rightspeed">Right Speed:</label><br/>
               <input type="range" id="rightspeed" min="<?php echo $_SESSION['minMotorSpeed']; ?>" max="255" value="<?php echo $_SESSION['MotorSpeed']; ?>" class="vertical" orient="vertical" onInput="outputRightSpeed(value);" onChange="Robot_SetSpeed('right');" />
               <br/>&nbsp;<output for="rightspeed" id="rightMotorSpeed"><?php echo $_SESSION['MotorSpeed']; ?></output>
             </div>
           </div>
           <div class="divtable">
             <div id="movekeyboard" class="divtablecell">
               &nbsp;
               <br/><br/>
               <b>Keyboard:</b><br/>
               &nbsp; <input type="text" style="height:15px; width:60px" onSubmit="return verifyKey(event)">
             </div>
             <div id="MotorSpeedMaster" class="divtablecell">
               <label for="masterspeed">Master Speed:</label>
               <input type="range" id="masterspeed" min="<?php echo $_SESSION['minMotorSpeed']; ?>" max="255" value="<?php echo $_SESSION['MotorSpeed']; ?>" onInput="outputMasterSpeed(value); outputRightSpeed(value); outputLeftSpeed(value);" onChange="Robot_SetSpeed('master');" />
               <br/>&nbsp;&nbsp;&nbsp;&nbsp;<output for="masterspeed" id="masterMotorSpeed"><?php echo $_SESSION['MotorSpeed']; ?></output>
             </div>
             <div id="DriveModes" class="divtablecell">
               &nbsp;
               <br/><br/>
               <input type="button" value="Explore" onClick="WebSocket_Send('explore')" />&nbsp;
               <input type="button" value="AutoDrive" onClick="WebSocket_Send('auto')" />
             </div>
           </div>
         </div>

         <div id="column3" class="divtablecellborder">
           <b>Drive - Status:</b> <br/><br/>
           &nbsp; Angle: <progress id="DriveAngleperc" value="0" max="360"></progress> <span id="DriveAnglenum">&deg;</span> <br/>
           <br/>
           &nbsp; Pitch: <progress id="PITCHperc" value="0" max="100"></progress> <span id="PITCHnum">&deg;</span> <br/>
           &nbsp; Roll: <progress id="ROLLperc" value="0" max="100"></progress> <span id="ROLLnum">&deg;</span> <br/>
           <br/>
           &nbsp; Encoder Left: </progress> <span id="ELnum"></span> <br/>
           &nbsp; Encoder Right: </progress> <span id="ERnum"></span> <br/>
         </div>

       </div>

    </div>
  </div>

  <div id="foot">
    <center><p>Copyright 2014 meigrafd <a href="http://goo.gl/I2e5bu" target="new">RoPi</a></p></center>
  </div>
  <?php if (isset($DEBUG) AND $DEBUG == 1) { ?>
  <div id="Log" style="text-align:left; width:100%;">&nbsp;</div>
  <?php } ?>
 </body>
</html>