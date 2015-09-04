<?php
session_start();
require_once("include/global.php");
//
// v0.4 - best view with Google Chrome
//
// Initialize Web Control -> send get.defaultSettings over WebSocket to receive default Settings. 
// If default Settings received -> load ropi.php , else continue getting!
//
?>
<!DOCTYPE html>
<html>
 <head>
  <meta http-equiv="Content-Type" content="text/html; charset=utf-8">
  <meta name="robots" content="DISALLOW">
  <meta charset="utf-8">
  <title>RoPi-Control-Init</title>
  <link rel="stylesheet" type="text/css" href="css/style.css">
  <script>
  	var DEBUG = <?php echo $DEBUG; ?>;
  </script>
  <script src="js/websocket.js"></script>
  <script src="js/control.js"></script>
 </head>
 <body onLoad='WebSocket_Open();'>
  <center>
   <div class="divtable">
     <div id="Connection" class="divtablecellborder">
       <b>Connection:</b> 
       <input type="button" value="open" onClick="WebSocket_Open();" />
       <input type="button" value="close" onClick="WebSocket_Close();" />
       <br/> <b>Status:</b> <span id="connectionStatus">closed</span>
     </div>
   </div>
   <?php if (isset($DEBUG) AND $DEBUG == 1) { ?>
   <div id="Log" style="text-align:left; width:100%;">&nbsp;</div>
   <?php } ?>
  </center>
 </body>
</html>