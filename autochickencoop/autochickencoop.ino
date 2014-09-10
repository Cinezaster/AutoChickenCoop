#include <OneWire.h>
#include <DallasTemperature.h>
#include <Time.h>
#include <TimeAlarms.h>
#include <Bounce2.h>

#define LDR_PIN A0
#define CALIBRATION_PIN A1
#define TEMPERATURE_CALIBRATION_PIN A2
#define VENTILATION_PIN 2
#define LIGHT_PIN 3
#define CALIBRATION_LED_PIN 13 // change back to 4 later
#define ONE_WIRE_BUS 5
#define ENDSTOP_BOTTOM_PIN 6
#define ENDSTOP_TOP_PIN 7
#define MOTOR_PWM_PIN 8
#define MOTOR_1A_PIN 9
#define MOTOR_2A_PIN 10
#define MOTOR_EN_PIN 11
#define TEST_BUTTON_PIN 12

int calibrationValue;
int temperatureCalibrationValue;
int ldrValue;
volatile int calibrationState;
volatile int endstopBottomState;
volatile int endstopTopState;
int ticker;
const int transitionTime = 5; // minutes check
const int motorSpeed = 180;

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
DeviceAddress insideThermometer;

Bounce testButton = Bounce();
Bounce endstopBottom = Bounce();
Bounce endstopTop = Bounce();

void init_input(){
  pinMode(LDR_PIN, INPUT);
  pinMode(CALIBRATION_PIN, INPUT);
  pinMode(TEMPERATURE_CALIBRATION_PIN, INPUT);
  pinMode(ENDSTOP_BOTTOM_PIN, INPUT_PULLUP);
  endstopBottom.attach(ENDSTOP_BOTTOM_PIN);
  endstopBottom.interval(40);
  pinMode(ENDSTOP_TOP_PIN, INPUT_PULLUP);
  endstopTop.attach(ENDSTOP_TOP_PIN);
  endstopTop.interval(40);
  pinMode(TEST_BUTTON_PIN, INPUT_PULLUP);
  testButton.attach(TEST_BUTTON_PIN);
  testButton.interval(120);
}

void init_output(){
  pinMode(VENTILATION_PIN, OUTPUT);
  digitalWrite(VENTILATION_PIN, LOW);
  pinMode(LIGHT_PIN, OUTPUT);
  digitalWrite(LIGHT_PIN, LOW);
  pinMode(CALIBRATION_LED_PIN, OUTPUT);
  digitalWrite(CALIBRATION_LED_PIN, LOW);
}

void init_motor(){
  pinMode(MOTOR_1A_PIN,OUTPUT);
  digitalWrite(MOTOR_1A_PIN,LOW);
  pinMode(MOTOR_2A_PIN,OUTPUT);
  digitalWrite(MOTOR_2A_PIN,LOW);
  pinMode(MOTOR_EN_PIN,OUTPUT);
  digitalWrite(MOTOR_EN_PIN,LOW);
  pinMode(MOTOR_PWM_PIN,OUTPUT);
  analogWrite(MOTOR_PWM_PIN,0);
}

void check_light(){
  // als donker vastgesteld en deur is nog steeds open ticker ++
  if (ldrValue < calibrationValue && endstopTopState == LOW) {
    if (ticker > transitionTime*2) {
      // deur naar beneden
      door_close();
      light_on();
      ticker = 0;
      
    } else {
      ticker++;
      light_on();
    }
    
  } else if (ldrValue > calibrationValue && endstopBottomState == LOW) {
    if (ticker > transitionTime*2) {
      // deur naar boven
      door_open();
      light_off();
      ticker = 0;
    } else {
      ticker++;
    }
  }
  // als we al aan het tellen zijn en de voorwaarde voldoet niet meer ticker naar 0;
  if (ticker > 0 && ldrValue > calibrationValue && endstopTopState == LOW || ticker > 0 && ldrValue < calibrationValue && endstopBottomState == LOW) {
    ticker--; // or 0?? check beheaviour 
  }
}

void check_temp(){
  int prevTempCalV = temperatureCalibrationValue;
  temperatureCalibrationValue = map(analogRead(TEMPERATURE_CALIBRATION_PIN), 0, 1023, 20, 40);
  Serial.print("temp calibrationV: ");
  Serial.println(temperatureCalibrationValue);
  if (prevTempCalV != temperatureCalibrationValue) {
    sensors.setHighAlarmTemp(insideThermometer, temperatureCalibrationValue);
  }
  
  
  if (sensors.hasAlarm(insideThermometer) && analogRead(VENTILATION_PIN) == HIGH)
  {
    digitalWrite(VENTILATION_PIN, HIGH);
  } else {
    digitalWrite(VENTILATION_PIN, LOW);
  }
  
  Serial.print("Requesting temperatures...");
  sensors.requestTemperatures();
  Serial.println("DONE");
  
  float tempC = sensors.getTempC(insideThermometer);
  Serial.print("Temp C: ");
  Serial.println(tempC);
}

void test_light(){
  Serial.print("temp calibrationV: ");
  Serial.println(map(analogRead(TEMPERATURE_CALIBRATION_PIN), 0, 1023, 20, 40));
  
  
  
  ldrValue = analogRead(LDR_PIN);
  calibrationValue = analogRead(CALIBRATION_PIN);
  Serial.print("LDR: ");
  Serial.println(ldrValue);
  Serial.print("calibrationV: ");
  Serial.println(calibrationValue);
  
  if (ldrValue < calibrationValue && calibrationState == LOW) {
    calibrationState = HIGH;
  } else {
    calibrationState = LOW;
  }
}

void check(){
   check_light();
   check_temp();
}

void light_on(){
  digitalWrite(LIGHT_PIN,HIGH);
}

void light_off(){
  digitalWrite(LIGHT_PIN,LOW);
}

void door_open(){
  digitalWrite(MOTOR_EN_PIN, HIGH);
  analogWrite(MOTOR_PWM_PIN, 255);
  digitalWrite(MOTOR_1A_PIN,HIGH);
  // een while op de deur sensors
  while(digitalRead(ENDSTOP_TOP_PIN) == LOW) {
    // als de deur vertrokken is het tempo verlage
    if (digitalRead(ENDSTOP_BOTTOM_PIN) == LOW) {
      analogWrite(MOTOR_PWM_PIN, motorSpeed);
    }
    delay(400);
  }
  digitalWrite(MOTOR_EN_PIN, LOW);
  analogWrite(MOTOR_PWM_PIN, 0);
  digitalWrite(MOTOR_1A_PIN,LOW);
}

void door_close(){
  digitalWrite(MOTOR_EN_PIN, HIGH);
  analogWrite(MOTOR_PWM_PIN, 255);
  digitalWrite(MOTOR_2A_PIN,HIGH);
  // een while op de deur sensors
  while(digitalRead(ENDSTOP_TOP_PIN) == LOW) {
    // als de deur vertrokken is het tempo verlaoge
    if (digitalRead(ENDSTOP_BOTTOM_PIN) == LOW) {
      analogWrite(MOTOR_PWM_PIN, motorSpeed);
    }
    delay(400);
  }
  digitalWrite(MOTOR_EN_PIN, LOW);
  analogWrite(MOTOR_PWM_PIN, 0);
  digitalWrite(MOTOR_2A_PIN,LOW);
}

void toggle_door(){
  // zorgt er ook voor dat de deur open of gesloten moet zijn
  if (endstopTopState == LOW) {
    door_close();
  } else if (endstopBottomState == LOW) {
    door_open();
  }
}

void setup(){
  Serial.begin(57600);
  
  setTime(1,1,1,1,1,1);
  // Start up the library
  sensors.begin();
  Serial.print("Found ");
  Serial.print(sensors.getDeviceCount(), DEC);
  Serial.println(" devices.");
  
  if (!sensors.getAddress(insideThermometer, 0)) Serial.println("Unable to find address for Device 0"); 
  
  
  init_input();
  init_output();
  init_motor();
  
  Alarm.timerRepeat(30, check);
  
}

void loop(){
  test_light();
  
  endstopBottomState = endstopBottom.read();
  endstopTopState = endstopTop.read();
  
  if (testButton.update() && testButton.read() == LOW) {
    toggle_door();
  }
  
  digitalWrite(CALIBRATION_LED_PIN,calibrationState);
  
  Alarm.delay(500);
}

