// Constants used in program


#define SERIAL_BAUD 38400     // Serial Baudrate to communicate with RaspberryPI, max 115200

// for Grid-Map and Path-Finding
const int GridSize   =  10;
const int RobotWidth = 640;
// Wave Front Variables
int nothing =   0;
int goal    =   1;
int robot   = 254;
int wall    = 255;
//starting robot/goal locations
int robot_x =     8; //half of GridSize
int robot_y =     8;
int goal_x  =     2;
int goal_y  =     2;
int cell_size = 330; //the size of the robot
//Map locations
int x = 0;
int y = 0;


// Ultrasonic HC-SR04
#define Distance_Safe    10   // Safe distance between robot and objects (in centimeters)
#define Distance_Max    500   // Maximum distance before an object is considered "out of range"
#define Distance_Best    50   // Best distance to be from object so tracking won't be lost if object moves suddenly
#define Ping_Interval   330   // 33 Milliseconds between sensor pings (29ms is about the min to avoid cross-sensor echo)

#define PAN_Center     1400   // Center position of pan servo in uS
#define PAN_Min         700   // Pan servo lower limit in uS
#define PAN_Max        2100   // Pan servo upper limit in uS
#define PAN_Scalefact     4   // Scale factor to prevent pan servo overcorrecting

#define TILT_Center    1500   // Center position of tilt servo in uS
#define TILT_Min        700   // Tilt servo lower limit in uS
#define TILT_Max       1750   // Tilt servo upper limit in uS
#define TILT_Scalefact    4   // Scale factor to prevent tilt servo overcorrecting

#define explorespeed    150   // travel speed when robot is exploring
#define patience       4000   // milliseconds without stimulation in playmode required to become bored
#define reversetime    2000   // time to reverse/backward in milliseconds

#define MotorSpeed      127   // default Motor Speed on startup
#define minMotorSpeed    30   // minimal Motor Speed to prevent stall

//CMPS10 Compass
// needed: á 1k8 or 4k7 resistors between:
//  Arduino-UNO pin A4 and 5V -> CMPS10 SDA
//  Arduino-UNO pin A5 and 5V -> CMPS10 SCL
//  Only A4 and A5 can be used for I2C! http://playground.arduino.cc/Learning/Pins
//  Arduino-Mega pin 20 and 5V -> CMPS10 SDA
//  Arduino-Mega pin 21 and 5V -> CMPS10 SCL
#define COMPASS_I2C_ADDRESS 0x60  // CMPS10 Compass I2C address (default: 0x60)

#define EEPROM_I2C_ADDRESS 0x50  // 24LC256 I2C address (default: 0x50)
#define RESET_EEPROM false       // true = Standardwerte im nichtflüchtigen Speicherbereich schreiben. false = nichts tun
const int EEPROM_MIN_ADDR = 0;
const int EEPROM_MAX_ADDR = 32768;  // 1024 bytes * 32
const int BUFSIZE=50; 
char buffer[BUFSIZE];

// Scannerservo lenkt den Ultrasonic
const int ScannerServoMaxPan = 170;       // Max Pan servo angle in degrees. Don't go to very end of servo travel which may not be all the way from 0 to 180.
const int ScannerServoMinPan = 0;         // Min Pan servo angle in degrees.
const int ScannerServoMaxTiltUP = 110;    // Max Tilt UP position
const int ScannerServoMaxTiltDOWN = 180;  // Max Tilt DOWN position
const int ScannerServoPanSteps = 30;
const int ScannerServoTiltSteps = 30;
const int ScannerServoWait = 330;         // Milliseconds between Step Changes
int pos_pan = 70;                         // variable to store the servo position
int pos_tilt = 150;                       // variable to store the servo position
int start_pos_pan = pos_pan;              // pan start pos
int start_pos_tilt = pos_tilt;            // tilt start pos
// dont change (x)!
#define UP (1)
#define DOWN (2)
#define RIGHT (3)
#define LEFT (4)
int dir_pan = RIGHT;                     // pan default direction
int dir_tilt = UP;                       // tilt default direction

// deadband values prevent the robot over reacting to small movements of the object

#define disdeadband    40     // Distance deadband allows the object to change distance a small amount without robot reacting
#define pandeadband    20     // Pan deadband allows head to pan a small amount before body begins tracking object in uS
