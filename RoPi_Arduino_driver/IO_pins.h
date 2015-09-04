/*
 RoPi I/O pin definitions for Arduino Mega 2560

 http://arduino.cc/en/Hacking/PinMapping2560

 Rover 5 Explorer with Arduino Mega or DAGU Spider.
 This page tells the controller which pin is connected to which sensor, switch, motor etc.
 You can re-define the pins here to suit your robot. This is your wiring diagram!
 Pins are usually chosen by their function but in some cases by physical location (shorter wires).
 You may not require all functions listed here, ignore those you do not need.
*/

// http://www.forum-raspberrypi.de/Thread-arduino-arduino-reboot
#define PIN2RESET 33   // digital input       D33

// HC-SR04
//5V on Arduino to VCC of HC-SR04
//Pin 30 of Arduino to TRIG of HC-SR04
//Pin 31 of Arduino to ECHO of HC-SR04
//GND of Arduino to GND of HC-SR04
#define ULTRASONIC_TRIG 30  // Ultrasonic HC-SR04 Sensor TRIGGER   - digital output       D30
#define ULTRASONIC_ECHO 31  // Ultrasonic HC-SR04 Sensor ECHO      - digital input        D31

#define ScannerServoPan 40   // PAN  left  or  right               - digital output       D40
#define ScannerServoTilt 41  // TILT  up   or  down                - digital output       D41

#define LMDpin 53           // Left  Motor Direction               - digital output       D53
#define LMSpin 5            // Left  Motor Speed PWM               - digital output       D4
#define LMCpin A6           // Left  Motor Current                 - analog   input       A6
#define LMApin 2            // Left  Motor Encoder A               - external Interrupt0  D2
#define LMBpin 3            // Left  Motor Encoder B               - external Interrupt1  D3

#define RMDpin 52           // Right Motor Direction               - digital output       D52
#define RMSpin 4            // Right Motor Speed PWM               - digital output       D5
#define RMCpin A7           // Right Motor Current                 - analog   input       A7
#define RMApin 18           // Right Motor Encoder A               - external interrupt5  D18
#define RMBpin 19           // Right Motor Encoder B               - external interrupt4  D19

#define FRSpin A12          // Front Right IR Sensor               - analog   input       A12
#define FLSpin A13          // Front Left  IR Sensor               - analog   input       A13
#define RLSpin A14          // Rear  Left  IR Sensor               - analog   input       A14
#define RRSpin A15          // Rear  Right IR Sensor               - analog   input       A15

#define FRLpin  8           // Front Right IR LEDs                 - digital  output      D8
#define FLLpin  9           // Front Left  IR LEDs                 - digital  output      D9
#define RLLpin 10           // Rear  Left  IR LEDs                 - digital  output      D10
#define RRLpin 11           // Rear  Right IR LEDs                 - digital  output      D11

#define pingLED 12          // Ultrasonic activity LED             - digital output       D12
#define motorLED 49         // Motor activity LED                  - digital output       D49
#define ErrorLED 22         // Error LED                           - digital output       D22

// Note: optional speaker should have a series capacitor of between 10uF and 100uF
#define Speaker 24          // Speaker                             - digital output       D24

