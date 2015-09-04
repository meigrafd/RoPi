<?php
session_start();

$DEBUG=1;

$WEBIF_TIMEOUT = "3600";
$WEBIF_SOCKETTIMEOUT = "5";

// defaultSettings - variables must be named as in Arduino Sketch!
$_SESSION['MotorSpeed'] = "127";   // default Motor Speed
$_SESSION['minMotorSpeed'] = "30"; // Minimum Motor Speed (lower they get stall/stuck)
$_SESSION['ScannerServoMaxPan'] = "170";
$_SESSION['ScannerServoMinPan'] = "0";
$_SESSION['ScannerServoMaxTiltUP'] = "110";    // Max Up
$_SESSION['ScannerServoMaxTiltDOWN'] = "180";  // Min Down
$_SESSION['start_pos_pan'] = "70";
$_SESSION['start_pos_tilt'] = "150";
$_SESSION['dir_pan'] = "3";     // default Pan Direction: Right
$_SESSION['dir_tilt'] = "1";    // default Tilt Direction: Up
$_SESSION['GridSize'] = "15";   // default Size of GridMap: 15 (15x15)
$_SESSION['RobotWidth'] = "15"; // default Size of Robot: 330
?>