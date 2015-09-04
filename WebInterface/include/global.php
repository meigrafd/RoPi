<?php
require("config.php");

error_reporting(E_ALL);
ini_set('track_errors', 1);
ini_set('display_errors', 1);
ini_set('log_errors', 1);
ini_set("session.gc_maxlifetime",$WEBIF_TIMEOUT); // default: 60 min
define('TIMEOUT',$WEBIF_TIMEOUT); // default: 60 min
define("TEMP_PATH","/tmp/");

session_cache_limiter('public');
session_cache_expire($WEBIF_TIMEOUT);
ini_set("session.gc_maxlifetime",$WEBIF_TIMEOUT); // default: 60 min

ini_set('default_socket_timeout',$WEBIF_SOCKETTIMEOUT);
/*
//needed to fix STRICT warnings about timezone issues
$tz = exec('date +%Z');
@date_default_timezone_set(timezone_name_from_abbr($tz));
ini_set('date.timezone',@date_default_timezone_get());
*/
$S="&nbsp;";
$s="&#160;";
$_SELF=$_SERVER['PHP_SELF'];
ob_implicit_flush(true);
@ob_end_flush();
?>