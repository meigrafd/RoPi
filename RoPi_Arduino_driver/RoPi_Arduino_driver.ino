/*

  Sketch for Arduino-Mega 2560 with RaspberryPI, and Rover5 Explorer PCB: http://letsmakerobots.com/content/understanding-explorer-pcb

  version 0.8
  (c) 2014 meigrafd


  GRID MAP Docus:
  http://www.robotc.net/blog/2011/08/08/robotc-advanced-training/

*/
//------------------------------------------------------------------------------

#include <stdint.h>
#include <Servo.h>      // https://code.google.com/p/arduino/source/browse/trunk/libraries/Servo/Servo.h
#include <Wire.h>       // needed for CMPS10 Compass
#include <EEPROM.h>     // needed for 24LC256 EEPROM
#include <CMPS10.h>
#include <MemoryFree.h> // http://playground.arduino.cc/Code/AvailableMemory

#include "IO_pins.h"
#include "Configuration.h"
#include "notes.h"

#define DEBUG 1

//------------------------------------------------------------------------------
//              define Global Variables

byte mode;             // mode 0 = PLAY : mode 1 = EXPLORE

// Radsensoren (Rotary Encoder)
int leftValue = 0; 
int rightValue = 0;
int left_last = 0; // alter Wert
int left_actual = 0; //jetztiger Wert
int right_last = 0; // alter Wert
int right_actual = 0; //jetztiger Wert
int LeftCounter = 0;
int RightCounter = 0;

// Ultrasonic Sensor
unsigned long pingTimer;
long duration, distance, distanceCM; // Duration used to calculate distance

// Scannerservo - dont change!
int newLine = 0;
int PanTilt_initializated = 0;

// Zeitmessung
long lastcmdtime = micros(); //Zeit des letzten Kommandos
String lastcmd; //letztes ausgeführte Kommando
long lastsend = micros(); //Zeit des letzten Kommandos
long actual = micros();  // aktuelle Zeit
int ValidCommand = 0;
String ActiveMode; // speichern des aktuellen Modus: STOP / Search / AutoDrive / ManualDrive

// Buffer to store incoming commands from serial port
String dataString;

// Save MotorSpeeds
int leftspeed = 0;      // speed of left motor (PWM 0-255). start with motor speeds set to 0
int rightspeed = 0;     // speed of right motor (PWM 0-255). start with motor speeds set to 0

// used for chasing the LEDs
int lightchase;

volatile int Lencoder;   // counts pulses from left  encoder, reverse = -count
volatile int Rencoder;   // counts pulses from right encoder, reverse = -count

// for GRID-Map and Path-Finding
//records current pointed direction of robot:
//1 is up, 2 is right, 3 is down, and 4 is left (clockwise)
int old_robotDirection = 1;
int new_robotDirection = 1;//required direction of robot

//X is vertical, Y is horizontal
//int Map[GridSize][GridSize];

// Only for Debugging: A preloaded Map Array
int Map[10][10] = 
  {
    {255, 255, 255, 255, 255, 255, 255, 255, 255, 255},
    {255,   0,   0,   0, 255,   0,   0,   0,   0, 255},
    {255,   0, 254,   0, 255,   0,   1,   0,   0, 255},    
    {255,   0,   0,   0, 255,   0,   0,   0,   0, 255},
    {255,   0,   0,   0, 255, 255, 255,   0,   0, 255},
    {255,   0, 255,   0, 255,   0,   0,   0,   0, 255},
    {255,   0, 255,   0, 255,   0,   0,   0,   0, 255},
    {255,   0, 255, 255, 255,   0, 255, 255, 255, 255},
    {255,   0,   0,   0,   0,   0,   0,   0,   0, 255},
    {255, 255, 255, 255, 255, 255, 255, 255, 255, 255}
  };

//------------------------------------------------------------------------------
//              init Classes

Servo pan_ScannerServo;
Servo tilt_ScannerServo;
CMPS10 compass;

//------------------------------------------------------------------------------

/* derzeit nicht in verwendung
// If ping received, set the sensor distance.
// Timer2 interrupt calls this function
void echoCheck() {
  if (sonar.check_timer()) { // This is how you check to see if the ping was received.
    // Ping returned, uS result in ping_result, convert to cm with US_ROUNDTRIP_CM
    distance = sonar.ping_result / US_ROUNDTRIP_CM;
  }
  if (distance >= MAX_DISTANCE || distance <= MIN_DISTANCE) {
    // Send a negative number to computer and Turn LED ON to indicate "out of range"
    digitalWrite(pingLED, HIGH);
    distance = -1;
  } else {
    // Turn LED OFF to indicate successful reading.
    digitalWrite(pingLED, LOW);
  }
  Serial.print("distance: ");
  Serial.println(distance);
}

void ScannerServo() { 
  int i=0;
  int j=0;
  while (j < ScannerServoMax) {
      j = ScannerServoMin + i*2;
      Scannerservo.write(j);
      delay(10);
      if(i>=0 && i<=69)ultrasonic[i] =(ultrasonic[i]+ ping())/2;
      i++;
      //counter(); // Die Odometrie muß so oft es geht abgefragt werden, da keine Interrupts möglich sind
  }
  while (j> ScannerServoMin) {
      j = ScannerServoMin + i*2;
      Scannerservo.write(j);
      delay(10);
      if(i>=0 && i<=69)ultrasonic[i] =(ultrasonic[i]+ ping())/2;
      i--;
      //counter(); // Die Odometrie muß so oft es geht abgefragt werden, da keine Interrupts möglich sind      
  }
}

// derzeit nicht in verwendung
void drive(byte Aone, byte Atwo, byte Bone, byte Btwo, int pwmWert) {
  digitalWrite(AIN1, Aone);
  digitalWrite(AIN2, Atwo);
  digitalWrite(BIN1, Bone);
  digitalWrite(BIN2, Btwo);
  analogWrite(PWMA, pwmWert);
  analogWrite(PWMB, pwmWert);  
}

boolean receive() {
  uint8_t lastByte;
  boolean timeout = false;
  while(!timeout) {
    while(Serial.available() > 0) {
      lastByte = Serial.read();
      if (lastByte == abord){
        flush();
      } else if (lastByte == ack){
        process();
        flush();
      } else if (bufferCount < ByteBufferLenght){
        buffer[bufferCount] = lastByte;
        bufferCount++;
      } else return false;
    }
    if (Serial.available() <= 0 && !timeout){
      if (waitTime > 0) delayMicroseconds(waitTime);
        if(Serial.available() <= 0) timeout = true;
      }
    }
    return timeout;
}

// Auswertung der empfangenen Befehle
void process() { 
  //verarbeiten der empfangenen Befehle...
   getString(string);
   lastcmd = micros();      
   LiesundTue(); 
}

void LiesundTue() {
   Serial.print(startFlag);
   Serial.print("DEBUG ARDUINO: Lies und Tue: ");
   Serial.print(string);
   Serial.print(ack);
}

void flush() {
  for(uint8_t a=0; a < ByteBufferLenght; a++){
    buffer[a] = 0;
  }
  bufferCount = 0;
  numberOfValues = 0;
}
void send(const char str[]) {
  Serial.print(startFlag);
  Serial.print(str);
  Serial.print(ack);
}
void getString(char string[]) {
  for (int a = 1;a < bufferCount;a++){
    string[a-1] = buffer[a];
  }
  string[bufferCount-1] = '\0';
}


//-unused-
String getValue(String data, char separator, int index) {
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length()-1;
  for(int i=0; i<=maxIndex && found<=index; i++){
    if(data.charAt(i)==separator || i==maxIndex){
        found++;
        strIndex[0] = strIndex[1]+1;
        strIndex[1] = (i == maxIndex) ? i+1 : i;
    }
  }
  return found>index ? data.substring(strIndex[0], strIndex[1]) : "";
}

void establishContact() {
   String us;
   us = "0,0,";
   for (int i=0; i<70; i++) {
     us += "0,";
   }
   for(int i =0; i<4;i++) { 
     us += "1";
   }
   char cmd[us.length() +1];
   us.toCharArray(cmd, us.length() +1);
   while (Serial.available() <= 0) {
     send(cmd);   // send an initial string
     fehler();
  }
  lastcmd = micros();      
}

// Auswertung der Radencoder um zu wissen wieviel Strecke zurueck gelegt wurde (muss bei jeder Bewegung ausgefuehrt werden)
// Die Odometrie muß so oft es geht abgefragt werden, da keine Interrupts möglich sind
void counter() {
  digitalWrite(OdoTrig, HIGH);
  leftValue = analogRead(leftEncoder);    // read the input pin (0 to 1023)
  rightValue = analogRead(rightEncoder);    // read the input pin (0 to 1023)  
  digitalWrite(OdoTrig, LOW);

  if (leftValue < totpunkt_low) left_actual = 0;
  if (leftValue > totpunkt_high) left_actual = 1;
  if (left_last != left_actual) {
    LeftCounter++;
    left_last = left_actual;
  }
  if (rightValue < totpunkt_low) right_actual = 0;
  if (rightValue > totpunkt_high) right_actual = 1;
  if (right_last != right_actual) {
    RightCounter++;
    right_last = right_actual;
  }
  Serial.print("LeftEncoder: ");
  Serial.println(LeftCounter);
  Serial.print("RightEncoder: ");
  Serial.println(RightCounter);
  //Radsensoren zuruecksetzen, nachdem die Werte uebertragen wurden.
  LeftCounter = 0;
  RightCounter = 0;
}
*/

//########################################################################


void FreeArduinoRAM() {
  Serial.print("freeArduinoRAM: ");
  Serial.println(freeMemory());
}


static void activityLed (byte pin, byte state, byte time = 0) {
  pinMode(pin, OUTPUT);
  if (time == 0) {
    digitalWrite(pin, state);
  } else {
    digitalWrite(pin, state);
    delay(time);
    digitalWrite(pin, !state);
  }
}

// blink led for n times
void blinkLED (byte pin, byte n = 3) {
  pinMode(pin, OUTPUT);
  for (byte i = 0; i < 2 * n; ++i) {
    delay(100);
    digitalWrite(pin, !digitalRead(pin));
  }
}

// Hilfsfunktionen für die Serielle Kommunikation
int stoi(String s) {
  char char_array[s.length() +1];
  s.toCharArray(char_array,sizeof(char_array));
  int number = atoi(char_array);
  return number;
}
String splitString(String s, char parser, int index) {
  String rs='\0';
  int parserIndex = index;
  int parserCnt=0;
  int rFromIndex=0, rToIndex=-1;
  while (index >= parserCnt) {
    rFromIndex = rToIndex+1;
    rToIndex = s.indexOf(parser,rFromIndex);
    if (index == parserCnt) {
      if (rToIndex == 0 || rToIndex == -1) {
        return '\0';
      }
      return s.substring(rFromIndex,rToIndex);
    } else {
      parserCnt++;
    }
  }
  return rs;
}

// used with CMPS10 lib
void Compass() {
  Serial.print("Bearing: ");  // Peilung
  Serial.println(compass.bearing());
  Serial.print("Pitch: "); // Gefaelle/Steigung
  Serial.println(compass.pitch());
  Serial.print("Roll: "); // Neigung
  Serial.println(compass.roll());
}
/*
270 --> Osten
180 --> Norden
90 --> Westen
0 or 360 --> Süden

// used without CMPS10 lib
void Compass() {
  byte byteHigh, byteLow, fine;  // highByte and lowByte store high and low bytes of the bearing and fine stores decimal place of bearing
  char pitch, roll;              // Stores pitch and roll values of CMPS10, chars are used because they support signed value
  int bearing;                   // Stores full bearing

  Wire.beginTransmission(COMPASS_I2C_ADDRESS); //starts communication with CMPS10
  Wire.write(2);                               //Sends the register we wish to start reading from
  Wire.endTransmission();

  Wire.requestFrom(COMPASS_I2C_ADDRESS, 4);    // Request 4 bytes from CMPS10
  while(Wire.available() < 4);                 // Wait for bytes to become available
  byteHigh = Wire.read();
  byteLow = Wire.read();
  pitch = Wire.read();
  roll = Wire.read();
   
  bearing = ((byteHigh << 8) + byteLow) / 10;  // Calculate full bearing
  fine = ((byteHigh << 8) + byteLow) % 10;     // Calculate decimal place of bearing
 
  Serial.print("Bearing: "); Serial.println(bearing); // Peilung
  Serial.print("Bearing decimal: "); Serial.println(fine); // Peilung
  Serial.print("Pitch: "); Serial.println(pitch); // Gefaelle/Steigung
  Serial.print("Roll: "); Serial.println(roll); // Neigung
}
*/
int Compass_soft_ver() {
  int data;                        // Software version of  CMPS10 is read into data and then returned
  Wire.beginTransmission(COMPASS_I2C_ADDRESS);
  // Values of 0 being sent with write need to be masked as a byte so they are not misinterpreted as NULL this is a bug in arduino 1.0
  Wire.write((byte)0);             // Sends the register we wish to start reading from
  Wire.endTransmission();
  Wire.requestFrom(COMPASS_I2C_ADDRESS, 1); // Request byte from CMPS10
  while(Wire.available() < 1);
  data = Wire.read();             
  return(data);
}


// Ultrasonic Ping
long ping() {
  // The PING))) is triggered by a HIGH pulse of 2 or more microseconds.
  // Give a short LOW pulse beforehand to ensure a clean HIGH pulse:
  digitalWrite(ULTRASONIC_TRIG,LOW);
  delayMicroseconds(2);
  digitalWrite(ULTRASONIC_TRIG,HIGH);
  delayMicroseconds(10);
  digitalWrite(ULTRASONIC_TRIG,LOW);
  // The ECHO pin is used to read the signal from the PING))): a HIGH
  // pulse whose duration is the time (in microseconds) from the sending
  // of the ping to the reception of its echo off of an object.
  duration = pulseIn(ULTRASONIC_ECHO, HIGH);
  // convert the time into a distance
  //distance = microsecondsToInches(duration);
  distance = microsecondsToCentimeters(duration);
  if (distance >= Distance_Max || distance <= Distance_Safe){
    // Turn LED ON to indicate "out of range"
    distance = -1;
    digitalWrite(pingLED, HIGH); 
  } else {
  // Turn LED OFF to indicate successful reading.
    digitalWrite(pingLED, LOW); 
  }
  Serial.print("distance: ");
  Serial.println(distance);
}
long microsecondsToInches(long microseconds) {
  // According to Parallax's datasheet for the PING))), there are
  // 73.746 microseconds per inch (i.e. sound travels at 1130 feet per
  // second).  This gives the distance travelled by the ping, outbound
  // and return, so we divide by 2 to get the distance of the obstacle.
  return microseconds / 74 / 2;
}
long microsecondsToCentimeters(long microseconds) {
  // The speed of sound is 340 m/s or 29 microseconds per centimeter.
  // The ping travels out and back, so to find the distance of the
  // object we take half of the distance travelled.
  return microseconds / 29 / 2;
}

//schrittweise
void servo_pan(int pos) {
   static int old_pos=0;
   int difference = pos - old_pos;
   for (int i=0; i < (abs(difference) + 1); i++) {
     if (difference > 0)
       pan_ScannerServo.write(old_pos++);
     else if (difference<0)
       pan_ScannerServo.write(old_pos--);
     else {
       pan_ScannerServo.write(pos);
       break;
     }
     delay(15); //give the servos some time to get in position
   }
   old_pos=pos;
}
//schrittweise
void servo_tilt(int pos) {
   static int old_pos=0;
   int difference = pos - old_pos;
   for(int i=0; i < (abs(difference) + 1); i++) {
     if (difference > 0)
       tilt_ScannerServo.write(old_pos++);
     else if (difference < 0)
       tilt_ScannerServo.write(old_pos--);
     else {
       tilt_ScannerServo.write(pos);
       break;
     }
     delay(15); //give the servos some time to get in position
   }
   old_pos=pos;
}

void PanTilt(void) {
  if (PanTilt_initializated == 0) { InitPanTilt(); }
  servo_tilt(pos_tilt); 
  servo_pan(pos_pan);
  delay(ScannerServoWait);
  #ifdef DEBUG
    Serial.print("Pan: ");
    Serial.print(pos_pan, DEC);
    Serial.print(", Tilt: ");
    Serial.print(pos_tilt, DEC);
  #endif
  if (dir_pan == RIGHT) {
    pos_pan = pos_pan + ScannerServoPanSteps; 
    if (pos_pan > ScannerServoMaxPan) {
      pos_pan = pos_pan - ScannerServoPanSteps;
      dir_pan = LEFT;
      newLine = 1;
    }
  } else {
    pos_pan = pos_pan - ScannerServoPanSteps;   
    if (pos_pan < ScannerServoMinPan) {
      pos_pan = pos_pan + ScannerServoPanSteps;
      dir_pan = RIGHT;
      newLine = 1;
    }
  }
  if (newLine) {
    #ifdef DEBUG
      Serial.print(", newLine: ");
      Serial.print(newLine, DEC);
    #endif
    newLine = 0;
    if (dir_tilt == UP) {
      pos_tilt = pos_tilt - ScannerServoTiltSteps;
      if (pos_tilt < ScannerServoMaxTiltUP) {
        pos_tilt = pos_tilt + (ScannerServoTiltSteps * 2);
        dir_tilt = DOWN;
      }
    } else {
      pos_tilt = pos_tilt + ScannerServoTiltSteps;
      if (pos_tilt > ScannerServoMaxTiltDOWN) {
        pos_tilt = pos_tilt - (ScannerServoTiltSteps * 2);
        dir_tilt = UP;
      }
    }
  }
  #ifdef DEBUG
    Serial.println("");
  #endif
  ping();
}

void InitPanTilt() {
  pan_ScannerServo.attach(ScannerServoPan);
  tilt_ScannerServo.attach(ScannerServoTilt);
  tilt_ScannerServo.write(pos_tilt);
  pan_ScannerServo.write(pos_pan);
  PanTilt_initializated = 1;
  delay(15); //give the servos some time to get in position
}
void ReleasePanTilt() {
  tilt_ScannerServo.write(start_pos_tilt);
  pan_ScannerServo.write(start_pos_pan);
  delay(200); //give the servos some time to get in position
  pan_ScannerServo.detach();
  tilt_ScannerServo.detach();
  PanTilt_initializated = 0;
}

// manual move pan servo and detach after reaching position
//note: the read() function of Servo.h does not give the realy actual position, its only the last write() value!
//      to fix this issue: http://www.mafu-foto.de/elektronik/servos/104-servos-um-positionsrueckmeldung-erweitern
void move_servo_pan(int pos) {
  pan_ScannerServo.attach(ScannerServoPan);
  pan_ScannerServo.write(pos);
  volatile int cn = 0;
  do {
    cn++;
    delay(20);
  } while (cn < pos);
  delay(200);
  pan_ScannerServo.detach();
}
// manual move tilt servo and detach after reaching position
void move_servo_tilt(int pos) {
  tilt_ScannerServo.attach(ScannerServoTilt);
  tilt_ScannerServo.write(pos);
  volatile int cn = 0;
  do {
    cn++;
    delay(20);
  } while (cn < pos);
  delay(200);
  tilt_ScannerServo.detach();
}

/*
void drive(char direct, int distance) {
 switch (direct) {
  case 'F': //Forward
    digitalWrite(dirA, HIGH);
    digitalWrite(dirB, HIGH);
    for(int i=0; i<distance; i++) {
      analogWrite(motorA, defSpeed);
      analogWrite(motorB, defSpeed);
      ampCheck();
      delay(50);
      obstacleCheck();
    }
    break;
  case 'B': //back
  
 }
}
*/

// Control Motors
/*
The direction control pin, DIR connects to a digital output of your controller.
When your controller output is high, the motor will turn in one direction, when the output is low the motor will turn in the other direction.
If the digital pin of your controller is in the high impedance state (open circuit or input) then the motor controller will treat this as a low input.

The speed control pin, PWM connects to a digital output of your controller that is capable of generating Pulse Width Modulated control signals. 
These signals control the amount of power supplied to the motors by controlling the percentage of time that the motor has power.

For example if your PWM output is at 60% then the motor will have power 60% of the time and no power for 40% of the time. 
This cycle of ON and OFF repeats very quickly (at least 100 times a second) so that the motor's momentum keeps it spinning during the time that the power is OFF. 

The current sensing pin, CUR is an analog output. 
The voltage on this pin is proportional to the current drawn by the motor and is calibrated so that 1V is aproximately equal to 1A of current. 
This pin should be connected to an analog input of your controller. Your software can monitor this voltage to determine if the robot gets stuck.
*/
void Motors() {
  digitalWrite(LMDpin, (leftspeed < 0));   // set Left Motor Direction to forward if speed < 0 . if speed > 0 Motor Direction = backward
  analogWrite(LMSpin, abs(leftspeed));     // set Left Motor Speed

  digitalWrite(RMDpin, (rightspeed < 0));  // set Right Motor Direction to forward if speed < 0
  analogWrite(RMSpin, abs(rightspeed));    // set Left  Motor Speed
}

// forward
void goFWD(int pwmWert) {
  digitalWrite(LMDpin, LOW);
  digitalWrite(RMDpin, LOW);
  analogWrite(LMSpin, abs(pwmWert));
  analogWrite(RMSpin, abs(pwmWert));
}

// backward / reverse
void goREV(int pwmWert) {
  digitalWrite(LMDpin, HIGH);
  digitalWrite(RMDpin, HIGH);
  analogWrite(LMSpin, abs(pwmWert));
  analogWrite(RMSpin, abs(pwmWert));
}

// rotate left
void rotateLeft(int pwmWert) {
  digitalWrite(LMDpin, HIGH);
  digitalWrite(RMDpin, LOW);
  analogWrite(LMSpin, abs(pwmWert));
  analogWrite(RMSpin, abs(pwmWert));
}

// rotate right
void rotateRight(int pwmWert) {
  digitalWrite(LMDpin, LOW);
  digitalWrite(RMDpin, HIGH);
  analogWrite(LMSpin, abs(pwmWert));
  analogWrite(RMSpin, abs(pwmWert));
}

// stop
void Stop() {
  ActiveMode = "STOP";
  analogWrite(LMSpin, 0);
  analogWrite(RMSpin, 0);
  ReleasePanTilt();
}

void fehler() {
  digitalWrite(ErrorLED, HIGH);
  Stop();
}

void kein_fehler() {
  digitalWrite(ErrorLED, LOW);
}


// Avoid hitting objects with corner IR Sensors
void IR_ObjectDetection() {
  int frontrightsen;
  int frontleftsen;
  int rearleftsen;
  int rearrightsen;

  //turn on edge detection LEDs
  digitalWrite(FRLpin, 1);
  digitalWrite(FLLpin, 1);
  digitalWrite(RLLpin, 1);
  digitalWrite(RRLpin, 1);
  delayMicroseconds(50);
  
  // read total IR values
  frontrightsen = analogRead(FRSpin);
  frontleftsen = analogRead(FLSpin);
  rearleftsen = analogRead(RLSpin);
  rearrightsen = analogRead(RRSpin);

  // turn off edge detection LEDs
  digitalWrite(FRLpin, 0);
  digitalWrite(FLLpin, 0);
  digitalWrite(RLLpin, 0);
  digitalWrite(RRLpin, 0);
  delayMicroseconds(50);

  // subtract ambient IR from total IR to give reflected IR values
  frontrightsen = frontrightsen - analogRead(FRSpin);
  frontleftsen = frontleftsen - analogRead(FLSpin);
  rearleftsen = rearleftsen - analogRead(RLSpin);
  rearrightsen = rearrightsen - analogRead(RRSpin);

  Serial.print("IRfrontright: "); Serial.println(frontrightsen);
  Serial.print("IRfrontleft: "); Serial.println(frontleftsen);
  Serial.print("IRrearleft: "); Serial.println(rearleftsen);
  Serial.print("IRrearright: "); Serial.println(rearrightsen);

  // turn on indicator LED if object closer than safe distance otherwise chase LEDs
  digitalWrite(FRLpin, (lightchase==4 || frontrightsen > Distance_Safe));
  digitalWrite(FLLpin, (lightchase==1 || frontleftsen > Distance_Safe));
  digitalWrite(RLLpin, (lightchase==2 || rearleftsen > Distance_Safe));
  digitalWrite(RRLpin, (lightchase==3 || rearrightsen > Distance_Safe));

  // Adjust motor speeds to avoid collision
  if (frontrightsen > Distance_Safe && rightspeed > 0) rightspeed=0;
  if (frontleftsen > Distance_Safe && leftspeed > 0) leftspeed=0;
  if (rearleftsen > Distance_Safe && leftspeed < 0) leftspeed=0;
  if (rearrightsen > Distance_Safe && rightspeed < 0) rightspeed=0;
}

/*
// Explore Room - if nothing is detected then adjust speed to explore speed
void Explore() {
  static byte timeflag;
  // no active timer - explore
  if(timeflag==0) {
    if(leftspeed<explorespeed) leftspeed+=(explorespeed-leftspeed)/20+1;
    if(leftspeed>explorespeed) leftspeed-=(leftspeed-explorespeed)/20+1;
    if(rightspeed<explorespeed) rightspeed+=(explorespeed-rightspeed)/20+1;
    if(rightspeed>explorespeed) rightspeed-=(rightspeed-explorespeed)/20+1;
    // no object detected with eye
    if(distance < Distance_Safe*2) {
      pan+=panspeed;
      tilt+=tiltspeed;
      if(pan<panmin || pan>panmax) panspeed=-panspeed;                 // reverse pan  direction when limits reached
      if(tilt<tiltmin || tilt>tiltmax) tiltspeed=-tiltspeed;           // reverse tilt direction when limits reached
    }
    else                                                               // detected object with eye
    {
      IRtrack();                                                       // lock onto object with eye

      IRfollow();                                                      // adjust motor speeds to turn towards object
      leftspeed=-leftspeed;                                            // invert leftspeed  to turn away from object
      rightspeed=-rightspeed;                                          // invert rightspeed to turn away from object   
    }
  }
  
  if(frontrightsen>Distance_Safe) rightreverse=millis();                // if front right sensor detects object, reverse and turn left
  if(frontleftsen >Distance_Safe) leftreverse =millis();                // if front left  sensor detects object, reverse and turn right
  
  if(millis()-rightreverse<reversetime) {
    leftspeed=leftspeed-1;
    rightspeed=rightspeed-3;
    bitSet(timeflag,0);
  }
  else bitClear(timeflag,0);

  if(millis()-leftreverse<reversetime) {
    leftspeed=leftspeed-3;
    rightspeed=rightspeed-1;
    bitSet(timeflag,1);
  }
  else bitClear(timeflag,1);

  if(leftspeed<-explorespeed) leftspeed=-explorespeed;  
  if(rightspeed<-explorespeed) rightspeed=-explorespeed;

  if(abs(leftspeed)>50 || abs(rightspeed)>50) stoptime=millis();   // reset stop timer while robot is moving
  if(millis()-stoptime>2000) leftreverse =millis();                // if robot stops moving for more than 2 seconds, reverse and turn to the right
}
*/

/* Encoder
  Below is a diagram of the signal generated by the Rover 5 quadrature encoders:
          ___     ___         A | B
   A: ___|   |___|   |___     0 | 0
        ___     ___     _     0 | 1
   B: _|   |___|   |___|      1 | 1
                              1 | 0
  When one encoder pin changes states, the other encoder pin state is checked to determine direction
  Output A is switches between high and low 1000 times for every 6 revolutions of the output shaft. 
  Output B is Identical to output A except that it is 90° out of phase.
*/
void LeftEncoderA() {
  if (digitalRead(LMBpin)) {
    Lencoder++;
  } else {
    Lencoder--;
  }
  Serial.print("LeftEncoder: ");
  Serial.println(Lencoder);
  //Radsensoren zuruecksetzen, nachdem die Werte uebertragen wurden.
  //Lencoder = 0;
}
void LeftEncoderB() {
  if (digitalRead(LMApin)) {
    Lencoder++;
  } else {
    Lencoder--;
  }
  Serial.print("LeftEncoder: ");
  Serial.println(Lencoder);
  //Radsensoren zuruecksetzen, nachdem die Werte uebertragen wurden.
  //Lencoder = 0;
}
void RightEncoderA() {
  if (digitalRead(RMBpin)) {
    Rencoder++;
  } else {
    Rencoder--;
  }
  Serial.print("RightEncoder: ");
  Serial.println(Rencoder);
  //Radsensoren zuruecksetzen, nachdem die Werte uebertragen wurden.
  //Rencoder = 0;
}
void RightEncoderB() {
  if (digitalRead(RMApin)) {
    Rencoder++;
  } else {
    Rencoder--;
  }
  Serial.print("RightEncoder: ");
  Serial.println(Rencoder);
  //Radsensoren zuruecksetzen, nachdem die Werte uebertragen wurden.
  //Rencoder = 0;
}

void clearMap() {
  for (int X=0; X < GridSize; X++) {
    for (int Y=0; Y < GridSize; Y++) {
      Map[X][Y]=0;
    }
  }
}

void printMap() {
  Serial.println("STARTMAP");
  for (int X = 0; X < GridSize; X++) {
    Serial.print("MAP: ");
    for (int Y = 0; Y < GridSize; Y++) {
      Serial.print(Map[X][Y]);
      Serial.print(",");
    }
    Serial.println();
  }
  //Serial.println("ENDMAP");
}

// http://www.forum-raspberrypi.de/Thread-arduino-arduino-reboot
void reboot() {
  pinMode(PIN2RESET, OUTPUT);
  digitalWrite(PIN2RESET, LOW);
  delay(100);
}

//------------------------------------------------------------------------------

// Send default Settings once at Startup
void SendDefaultSettings() {
  Serial.print("DEFAULT: MotorSpeed "); Serial.println(MotorSpeed);
  Serial.print("DEFAULT: minMotorSpeed "); Serial.println(minMotorSpeed);
  Serial.print("DEFAULT: ScannerServoMaxPan "); Serial.println(ScannerServoMaxPan);
  Serial.print("DEFAULT: ScannerServoMinPan "); Serial.println(ScannerServoMinPan);
  Serial.print("DEFAULT: ScannerServoMaxTiltUP "); Serial.println(ScannerServoMaxTiltUP);
  Serial.print("DEFAULT: ScannerServoMaxTiltDOWN "); Serial.println(ScannerServoMaxTiltDOWN);
  Serial.print("DEFAULT: start_pos_pan "); Serial.println(start_pos_pan);
  Serial.print("DEFAULT: start_pos_tilt "); Serial.println(start_pos_tilt);
  Serial.print("DEFAULT: dir_pan "); Serial.println(dir_pan);
  Serial.print("DEFAULT: dir_tilt "); Serial.println(dir_tilt);
  Serial.print("DEFAULT: GridSize "); Serial.println(GridSize);
  Serial.print("DEFAULT: RobotWidth "); Serial.println(RobotWidth);
  #ifdef DEBUG
    Serial.print("DEFAULT: DEBUG "); Serial.println("1");
  #endif
  Serial.flush();
}


// initialize servos and configure pins
void setup() {
  Serial.begin(SERIAL_BAUD);
  Serial.println("Serial connection started, waiting for instructions...");
  Serial.flush();
  //establishContact();  // send a byte to establish contact until receiver responds
  //flush();

  Wire.begin();  // Connects I2C for CMPS10 Compass

  // reboot arduino
  pinMode(PIN2RESET, INPUT);
  digitalWrite(PIN2RESET, HIGH);
  

  // only 2 Encoders but with 2x Outputs A+B!
  // http://arduino.cc/en/pmwiki.php?n=Reference/AttachInterrupt
  // Syntax: attachInterrupt(<int>, <function>, RISING);
  attachInterrupt(0, LeftEncoderA, RISING);
  attachInterrupt(1, LeftEncoderB, RISING);
  attachInterrupt(5, RightEncoderA, RISING);
  attachInterrupt(4, RightEncoderB, RISING);

  pinMode(LMDpin, OUTPUT);      // configure Left Motor Direction pin as output
  pinMode(RMDpin, OUTPUT);      // configure Right Motor Direction pin as output

  pinMode(FRLpin, OUTPUT);      // configure Front Right LEDs pin for output
  pinMode(FLLpin, OUTPUT);      // configure Front Left  LEDs pin for output
  pinMode(RLLpin, OUTPUT);      // configure Rear  Left  LEDs pin for output
  pinMode(RRLpin, OUTPUT);      // configure Rear  Right LEDs pin for output

  pinMode(ULTRASONIC_TRIG, OUTPUT);
  pinMode(ULTRASONIC_ECHO, INPUT);

  pinMode(pingLED, OUTPUT);
  pinMode(motorLED, OUTPUT);
  pinMode(ErrorLED, OUTPUT);
  digitalWrite(pingLED, 1);  // 1 indicates off
  digitalWrite(motorLED, 1);
  digitalWrite(ErrorLED, 1);

  pinMode(Speaker, OUTPUT);     // configure speaker pin for output

  pinMode(ScannerServoPan, OUTPUT);
  pinMode(ScannerServoTilt, OUTPUT);

  leftspeed, rightspeed = MotorSpeed;  // set default speed on startup

  Stop();

  delay(1000); // give it time to init..

/*
  // init Grid Map
  for (int X=0; X < GridSize; X++) {
    for (int Y=0; Y < GridSize; Y++) {
      Map[X][Y]=0;
    }
  }
  //store robot location in Map
  Map[robot_x][robot_y] = robot;
  //store goal location in Map
  Map[goal_x][goal_y] = goal;
  // Drive around until a button or unsafe condition is detected
*/

  SendDefaultSettings();


/*
  // play tune on powerup / reset
  int melody[] = {NOTE_C4,NOTE_G3,NOTE_G3,NOTE_A3,NOTE_G3,0,NOTE_B3,NOTE_C4};
  int noteDurations[] = {4,8,8,4,4,4,4,4};
  for (byte Note = 0; Note < 8; Note++)        // Play eight notes
  {
    int noteDuration = 1000/noteDurations[Note];
    tone(Speaker,melody[Note],noteDuration);
    int pauseBetweenNotes = noteDuration * 1.30;
    delay(pauseBetweenNotes);
  }
*/

  pingTimer = millis() + Ping_Interval;  // ping starts at >=29 ms, gives time for the Arduino to chill before starting.
  #ifdef DEBUG
    Serial.println("DEBUG: Setup finished");
  #endif
}

// e.g. cmd: forward:255  --> vorwaerts mit pwm 255
void serialReceive() {
  if (Serial.available() > 0) {
    dataString = Serial.readStringUntil('\n');
    Serial.flush();
    dataString = dataString + ":"; //needed so below works
    String command = splitString(dataString, ':', 0);
    String value = splitString(dataString, ':', 1);
    #ifdef DEBUG
      Serial.println("command: " + command + " value: " + value);
    #endif

    if (command == "forward") {
      ValidCommand = 1;
      goFWD(value.toInt());
      blinkLED(motorLED, 2);
    } else if (command == "backward") {
      ValidCommand = 1;
      goREV(value.toInt());
      blinkLED(motorLED, 2);
    } else if (command == "left") {
      ValidCommand = 1;
      leftspeed = value.toInt();
      rotateLeft(leftspeed);
      blinkLED(motorLED, 2);
    } else if (command == "right") {
      ValidCommand = 1;
      rightspeed = value.toInt();
      rotateRight(rightspeed);
      blinkLED(motorLED, 2);
    } else if (command == "stop") {
      ValidCommand = 1;
      Stop();
      blinkLED(motorLED, 2);
    } else if (command == "ping") {
      ValidCommand = 1;
      ping();
      blinkLED(motorLED, 2);
    } else if (command == "compass") {
      ValidCommand = 1;
      Compass();
      blinkLED(motorLED, 2);
    } else if (command == "explore") {
      ValidCommand = 1;
      ActiveMode = "Explore";
      blinkLED(motorLED, 2);
    } else if (command == "auto") {
      ValidCommand = 1;
      ActiveMode = "AutoDrive";
      blinkLED(motorLED, 2);
    } else if (command == "init") {
      ValidCommand = 1;
      setup();
      blinkLED(motorLED, 2);
    } else if (command == "pan") {
      ValidCommand = 1;
      move_servo_pan(value.toInt());
      blinkLED(motorLED, 2);
    } else if (command == "tilt") {
      ValidCommand = 1;
      move_servo_tilt(value.toInt());
      blinkLED(motorLED, 2);
    } else if (command == "getMap") {
      ValidCommand = 1;
      printMap();
      blinkLED(motorLED, 2);
    } else if (command == "getFreeRAM") {
      ValidCommand = 1;
      FreeArduinoRAM();
      blinkLED(motorLED, 2);
    } else if (command == "reboot") {
      ValidCommand = 1;
      reboot();
      blinkLED(motorLED, 2);

    } else {
      ValidCommand = 0;
      Serial.print("INVALID COMMAND: ");
      Serial.println(dataString);
    }
    
    if (ValidCommand == 1) {
      lastcmdtime = micros(); //Zeit des letzten Kommandos
      lastcmd = command;  //letztes ausgeführte Kommando
      kein_fehler();
    }
  }
}

// Explore: Umgebung erkunden und Map erstellen
void ExploreMode() {
  IR_ObjectDetection();    // check corner sensors, drive indicator LEDs, override motor speeds if required
  //Motors();             // update left and right motor speed
  //Explore();   // use US and corner sensors to navigate and generate grid map
  PanTilt();
  Compass();
  printMap();
}
// AutoDrive: Selbstständig durch Umgebung fahren um Ziel zu finden
//mapping_inRobot_working.pde
void AutoDriveMode() {
/*
  while(Map[robot_x][robot_y] != goal) // program ends when robot reaches goal
  {
    find_walls();
  }
*/
  IR_ObjectDetection();    // check corner sensors, drive indicator LEDs, override motor speeds if required
  Compass();
  printMap();
  if (millis() >= pingTimer) {     // Is it time to ping?
    pingTimer += Ping_Interval;    // Set next time this sensor will be pinged.
    ping();
  }
}

void loop() {
  serialReceive();
  // Explore: Umgebung erkunden und Map erstellen
  if (ActiveMode == "Explore") { ExploreMode(); }
  // AutoDrive: Selbstständig durch Umgebung fahren um Ziel zu finden
  if (ActiveMode == "AutoDrive") { AutoDriveMode(); }

/*
  actual = micros();
  //Timeout seit dem letzten empfangenen Befehl 2Sekunden
  if(actual - lastcmdtime >= 2000000) {
    Stop();
    fehler();
  }
*/
}

/*
// EEPROM stuff
// http://fluuux.de/2013/04/daten-im-eeprom-speichern-auslesen-und-variablen-zuweisen/
// http://playground.arduino.cc/Code/EepromUtil

// Speicherbereich löschen
void eeprom_erase_all(byte b = 0xFF) {
  int i;
  for (i = EEPROM_MIN_ADDR; i <= EEPROM_MAX_ADDR; i++) {
    EEPROM.write(i, b);
  }
}

// Speicherbereich auslesen. Gibt einen Dump des Speicherinhalts in tabellarischer Form über den seriellen Port aus.
void eeprom_serial_dump_table(int bytesPerRow = 16) {
  int i;
  int j;
  byte b;
  char buf[10];
  j = 0;
  for (i = EEPROM_MIN_ADDR; i <= EEPROM_MAX_ADDR; i++) {
    if (j == 0) {
      sprintf(buf, "%03X: ", i);
      Serial.print(buf);
    }
    b = EEPROM.read(i);
    sprintf(buf, "%02X ", b);
    j++;
    if (j == bytesPerRow) {
      j = 0;
      Serial.println(buf);
    } else Serial.print(buf);
  }
}

//Schreibt eine Folge von Bytes, beginnend an der angegebenen Adresse.
//Gibt True zurück wenn das gesamte Array geschrieben wurde,
//Gibt False zurück wenn Start- oder Endadresse nicht zwischen dem minimal und maximal zulässigen Bereich liegt.
//Wenn False zurückgegeben wurde, wurde nichts geschrieben.
boolean eeprom_write_bytes(int startAddr, const byte* array, int numBytes) {
  int i;
  if (!eeprom_is_addr_ok(startAddr) || !eeprom_is_addr_ok(startAddr + numBytes)) return false;
  for (i = 0; i < numBytes; i++) {
    EEPROM.write(startAddr + i, array[i]);
  }  return true;
}

//Schreibt einen int-Wert an der angegebenen Adresse
boolean eeprom_write_int(int addr, int value) {
  byte *ptr;
  ptr = (byte*)&value;
  return eeprom_write_bytes(addr, ptr, sizeof(value));
}

//Liest einen Integer Wert an der angegebenen Adresse
boolean eeprom_read_int(int addr, int* value) {
  return eeprom_read_bytes(addr, (byte*)value, sizeof(int));
}

//Liest die angegebene Anzahl von Bytes an der angegebenen Adresse
boolean eeprom_read_bytes(int startAddr, byte array[], int numBytes) {
  int i;
  if (!eeprom_is_addr_ok(startAddr) || !eeprom_is_addr_ok(startAddr + numBytes)) return false;
  for (i = 0; i < numBytes; i++) {
    array[i] = EEPROM.read(startAddr + i);
  } return true;
}

//Liefert True wenn die angegebene Adresse zwischen dem minimal und dem maximal zulässigen Bereich liegt.
//Wird durch andere übergeordnete Funktionen aufgerufen um Fehler zu vermeiden.
boolean eeprom_is_addr_ok(int addr) {
  return ((addr >= EEPROM_MIN_ADDR) && (addr <= EEPROM_MAX_ADDR));
}

//Schreibt einen String, beginnend bei der angegebenen Adresse
boolean eeprom_write_string(int addr, const char* string) {
  int numBytes;
  numBytes = strlen(string) + 1;
  return eeprom_write_bytes(addr, (const byte*)string, numBytes);
}

//Liest einen String ab der angegebenen Adresse
boolean eeprom_read_string(int addr, char* buffer, int bufSize) {
  byte ch;
  int bytesRead;
  if (!eeprom_is_addr_ok(addr)) return false;
  if (bufSize == 0) return false;
  if (bufSize == 1) {
    buffer[0] = 0;
    return true;
  }
  bytesRead = 0;
  ch = EEPROM.read(addr + bytesRead);
  buffer[bytesRead] = ch;
  bytesRead++;
  while ((ch != 0x00) && (bytesRead < bufSize) && ((addr + bytesRead) <= EEPROM_MAX_ADDR)) {
    ch = EEPROM.read(addr + bytesRead);
    buffer[bytesRead] = ch;
    bytesRead++;
  }
  if ((ch != 0x00) && (bytesRead >= 1)) buffer[bytesRead - 1] = 0;
  return true;
}

// Wenn RESET_EEPROM = true oder statusFlag != 49 dann EEPROM löschen und Standardwerte schreiben
void initializeEEPROM() {
  int i;
  int statusFlag = EEPROM.read(0);
  Serial.print("EEPROM: StatusFlag: ");
  Serial.println(statusFlag);
 
  if (RESET_EEPROM || statusFlag != 49) {
    Serial.println("EEPROM: Loesche Speicherbereich...");
    eeprom_erase_all();
    delay(500);
    Serial.println("EEPROM: Lese EEPROM...");
    eeprom_serial_dump_table();
    delay(500);
    Serial.println("EEPROM: Schreibe Speicherbereich...");
    strcpy(buffer, "1"); //Status-FLAG (0-1) 1 Zeichen
    eeprom_write_string(0, buffer);
 
    // GRID-MAP
    //eeprom_write_bytes(2, &Map, sizeof(&Map));    

    delay(500);
    Serial.println("EEPROM: Lese Speicherbereich...");
    eeprom_serial_dump_table();

    // erase buffer
    for (i = 0; i < BUFSIZE; i++) {
      buffer[i] = 0;
    }
  }
}
*/


