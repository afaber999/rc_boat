#include <Arduino.h>

#include <ESP8266WebServer.h>
#include <FS.h>
#include <ESP8266WiFi.h>
#include <servo.h>

#define USE_SERIAL

//Set Wifi ssid and password
char ssid[] = "RCBOAT";
char pass[] = "blupblupblup";

static bool debug = true;

const int SERVO_PIN = 12;
const int MOTOR_PIN = 14;

const int HOME_SERVO_ANGLE = 112;

const int MIN_SERVO_ANGLE = HOME_SERVO_ANGLE - 40;
const int MAX_SERVO_ANGLE = HOME_SERVO_ANGLE + 40;


ESP8266WebServer server (80);
static int servoAngle = HOME_SERVO_ANGLE;
static int curServoAngle = servoAngle;

Servo myservo; // create servo object to control a servo

static int motorPwm = 0;

static void SetDefaultPosition( ) {
  motorPwm = 0;
  servoAngle = HOME_SERVO_ANGLE;
}

static void SetServoAngle( int pos_in_degrees)
{
  if ( pos_in_degrees < MIN_SERVO_ANGLE ) pos_in_degrees = MIN_SERVO_ANGLE; 
  if ( pos_in_degrees > MAX_SERVO_ANGLE ) pos_in_degrees = MAX_SERVO_ANGLE;

  if ( debug )
  {
    Serial.print("servoAngle to : ");
    Serial.print(pos_in_degrees);
    Serial.println();
    Serial.flush();
  }

  servoAngle = pos_in_degrees;
}

static void ChangeMotorPwm( int deltaPwm)
{
  motorPwm += deltaPwm;

  if ( motorPwm < 0 ) motorPwm = 0; 
  if ( motorPwm > PWMRANGE ) motorPwm = PWMRANGE;

  if ( debug )
  {
    Serial.print("ChangeMotorPwm to : ");
    Serial.print(motorPwm);
    Serial.println();
    Serial.flush();
  }
}

int updates = 0;

//This function takes the parameters passed in the URL(the x and y coordinates of the joystick)
//and sets the motor speed based on those parameters. 
void handleJSData(){

  ++updates;

  boolean yDir;
  int x = server.arg(0).toInt();
  int y = server.arg(1).toInt();

  if ( y > 0 ) {
    motorPwm = (y * PWMRANGE ) / 100;
  } else {
    motorPwm = 0;
  }

  SetServoAngle( HOME_SERVO_ANGLE + ((x * 40 ) / 100) );
  server.send(200, "text/plain", "");   

  if ( debug ) {
    Serial.printf("handleJSData update %d", updates);
    Serial.printf(" x = %d y = %d\n",x, y);
    Serial.flush();
  }
}


void setup()
{
  Serial.begin(115200);
  Serial.println();
  Serial.println("Starting RC Boat version 0.2");

  pinMode(SERVO_PIN, OUTPUT);
  myservo.attach(SERVO_PIN);

  pinMode(MOTOR_PIN, OUTPUT);
  analogWrite(MOTOR_PIN, motorPwm);

  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, pass);

  // WiFi.mode(WIFI_STA);

  // WiFi.begin(ssid, pass);
  // while (WiFi.status() != WL_CONNECTED) {
  //   delay(500);
  //   Serial.print(".");
  // }

  // Serial.println("");
  // Serial.println("WiFi connected");  
  // Serial.println("IP address: ");
  // Serial.println(WiFi.localIP());


  //initialize SPIFFS to be able to serve up the static HTML files. 
  if (!SPIFFS.begin()){
    Serial.println("SPIFFS Mount failed");
  } 
  else {
    Serial.println("SPIFFS Mount succesfull");
  }
  //set the static pages on SPIFFS for the html and js
  server.serveStatic("/", SPIFFS, "/rcboat.html"); 
  server.serveStatic("/virtualjoystick.js", SPIFFS, "/virtualjoystick.js");
  //call handleJSData function when this URL is accessed by the js in the html file
  server.on("/jsData.html", handleJSData);  
  server.begin();
}

static int loopCnt = 0;

void loop()
{
  // handle if connection gets lost
  // set rudder to HOME_SERVO_ANGLE and PWM to 0
  if ( ( !debug ) && ( WiFi.softAPgetStationNum() == 0 )) {
    SetDefaultPosition();
  }

  server.handleClient();  

  if (Serial.available() > 0)
  {
    auto inByte = Serial.read();

    switch ( inByte )
    {
      case '1': SetServoAngle( HOME_SERVO_ANGLE + 40); break;
      case '2': SetServoAngle( HOME_SERVO_ANGLE + 30); break;
      case '3': SetServoAngle( HOME_SERVO_ANGLE + 20); break;
      case '4': SetServoAngle( HOME_SERVO_ANGLE + 10); break;
      case '5': SetServoAngle( HOME_SERVO_ANGLE); break;
      case '6': SetServoAngle( HOME_SERVO_ANGLE - 10); break;
      case '7': SetServoAngle( HOME_SERVO_ANGLE - 20); break;
      case '8': SetServoAngle( HOME_SERVO_ANGLE - 30); break;
      case '9': SetServoAngle( HOME_SERVO_ANGLE - 40); break;

      case 'a': ChangeMotorPwm( -10 ); break;
      case 's': ChangeMotorPwm( -100 ); break;
      case 'd': ChangeMotorPwm( 10 ); break;
      case 'f': ChangeMotorPwm( 100 ); break;
      case '0':
        SetDefaultPosition();
      break;

      case '?':
        Serial.print("HELP .... ");
      break;
      case 'D':
      {
        debug = !debug;
      }
      break;
    }
  }

  if (debug)
  {
    if ( (loopCnt % 100)==0 )
    {
      Serial.printf("Stations connected = %d\n", WiFi.softAPgetStationNum());
      Serial.print("Debug => ");
      Serial.print("servoAngle : ");
      Serial.print(servoAngle);
      Serial.print(" curservoAngle : ");
      Serial.print(curServoAngle);
      Serial.print(" motorPwm : ");
      Serial.print(motorPwm);
      Serial.println();
    }
  }

  analogWrite(MOTOR_PIN, motorPwm);
  myservo.write(curServoAngle);

  if ( curServoAngle < servoAngle) ++curServoAngle; 
  if ( curServoAngle > servoAngle) --curServoAngle;

  delay(10);

  loopCnt = loopCnt + 1;
}