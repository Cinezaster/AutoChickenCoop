/**
 * PROJECT: Autochickencoop
 * DESCRIPTION:  Chicken Coop controller. Closes the door at the evening and opens it up in the morning
 *               also prevents that the coop gets to hot.
 * AUTHOR: Toon Nelissen
 * GITHUB: https://github.com/Cinezaster/AutoChickenCoop
 */

#include <OneWire.h> // dependency for DallasTemperature lib
#include <DallasTemperature.h> // lib for Dallas OneWire sensor
#include <Time.h> // library taking care of time (doesn't realy need it but it but it is a dependency for TimeAlarms
#include <TimeAlarms.h> // library taking care of intervals 
#include <Bounce2.h> // library taking care of debouncing of button 

#define LDR1_PIN A0 // LDR 
#define LDR2_PIN A1 // LDR
#define DOOR_THRESHOLD_PIN A2 //potentiometer to set the door open en door close threshold
#define TEMPERATURE_THRESHOLD_PIN A3 //potentiometer to set the ventilation threshold
#define VENTILATION_PIN 2 // Ventilator
#define LIGHT_PIN 3 // Led string
#define DOOR_THRESHOLD_LED_PIN 4 //TODO change back to 4 later
#define ONE_WIRE_BUS 5 // pin to connect the Dallas oneWire temperature sensor
#define ENDSTOP_BOTTOM_PIN 8// switch HIGH when active (closed position)
#define ENDSTOP_TOP_PIN 7 // switch HIGH when active (closed position)
#define MOTOR_PWM_PIN 6 // PowerFet BUK456 gate to control speed of motor
#define MOTOR_1A_PIN 9 // 1A of SN754410 
#define MOTOR_2A_PIN 10 // 2A of SN754410
#define MOTOR_EN_PIN 11 // EN of SN754410
#define TEST_BUTTON_PIN 12 // pushButton

#define DEBUG true // turn on Serial debug info

int doorThresholdValue;
int temperatureThresholdValue;
int ldrValue;
volatile int doorThresholdState;
volatile int endstopBottomState;
volatile int endstopTopState;
int ticker;
const int transitionTime = 5; // minutes check
const int motorSpeed = 180; // 

OneWire oneWire(ONE_WIRE_BUS); // initiate  one wire
DallasTemperature sensors(&oneWire); // make a dallas sensor of the one wire bus
DeviceAddress insideThermometer; 

Bounce testButton = Bounce();  // initiate debounced button
Bounce endstopBottom = Bounce();
Bounce endstopTop = Bounce();


/**
 * initiate all input ports
 */
void init_input(){
  pinMode(LDR1_PIN, INPUT);
  pinMode(LDR2_PIN, INPUT);
  pinMode(DOOR_THRESHOLD_PIN, INPUT);
  pinMode(TEMPERATURE_THRESHOLD_PIN, INPUT);
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

/**
 * initiate all output ports
 */
void init_output(){
  pinMode(VENTILATION_PIN, OUTPUT);
  digitalWrite(VENTILATION_PIN, LOW);
  pinMode(LIGHT_PIN, OUTPUT);
  digitalWrite(LIGHT_PIN, LOW);
  pinMode(DOOR_THRESHOLD_LED_PIN, OUTPUT);
  digitalWrite(DOOR_THRESHOLD_LED_PIN, LOW);
  pinMode(MOTOR_1A_PIN,OUTPUT);
  digitalWrite(MOTOR_1A_PIN,LOW);
  pinMode(MOTOR_2A_PIN,OUTPUT);
  digitalWrite(MOTOR_2A_PIN,LOW);
  pinMode(MOTOR_EN_PIN,OUTPUT);
  digitalWrite(MOTOR_EN_PIN,LOW);
  pinMode(MOTOR_PWM_PIN,OUTPUT);
  analogWrite(MOTOR_PWM_PIN,0);
}

/**
 * Check check the light and the temperature situation
 */
void check(){
   check_light();
   check_temp();
}
  
/**
 * checking light conditions and acting if needed
 */
void check_light(){
  // when light intensity is below the threshold and door is open, ...
  if (ldrValue < doorThresholdValue && endstopTopState == HIGH) {
    // when ticker is smaller then the delay add 1 to ticker 
    // and put light on to conditionate the chickens that the door is about to close :-)
    if (ticker < transitionTime*2) {
      ticker++;
      light_on();
    } else {
      door_close();
      light_on();
      ticker = 0;
    }
  } else if (ldrValue > doorThresholdValue && endstopBottomState == HIGH) {
    if (ticker < transitionTime*2) {
      ticker++;
    } else {
      door_open();
      light_off();
      ticker = 0;
    }
  }
  
  // this prevents that a temporary shadow on the sensor would trigger the doors and leave the ticker in a half way state
  if (ticker > 0 && ldrValue > doorThresholdValue && endstopTopState == HIGH || ticker > 0 && ldrValue < doorThresholdValue && endstopBottomState == HIGH) {
    ticker--; // or 0?? check beheaviour 
  }
}

/**
 * checking temperature and turns on the ventilation
 */
void check_temp(){
  if (sensors.hasAlarm(insideThermometer) && analogRead(VENTILATION_PIN) == HIGH)
  {
    digitalWrite(VENTILATION_PIN, HIGH);
  } else {
    digitalWrite(VENTILATION_PIN, LOW);
  }
  
  sensors.requestTemperatures();
  
  float tempC = sensors.getTempC(insideThermometer);

  if (DEBUG) {
    Serial.print("Temp C: ");
    Serial.println(tempC);
  }
}

void test(){
  test_light();
  test_temp();
}

/**
 * Read temperature threshold
 * If threshold has changed set the alarm on the temperature sensor
 */
void test_temp(){
  int prevTempCalV = temperatureThresholdValue;
  temperatureThresholdValue = map(analogRead(TEMPERATURE_THRESHOLD_PIN), 0, 1023, 20, 40);

  if (prevTempCalV != temperatureThresholdValue) {
    sensors.setHighAlarmTemp(insideThermometer, temperatureThresholdValue);
  }

  if (DEBUG) {
    Serial.print("Temp threshold C: ");
    Serial.println(temperatureThresholdValue);
  }
}

/**
 * Read LDR and threshold
 * If LDR is lower then threshold turn doorThresholdState HIGH if already HIGH go low (blinks the LED);
 */
void test_light(){
  set_ldr_value();
  doorThresholdValue = analogRead(DOOR_THRESHOLD_PIN);
  
  if (ldrValue < doorThresholdValue && doorThresholdState == LOW) {
    doorThresholdState = HIGH;
  } else {
    doorThresholdState = LOW;
  }

  if (DEBUG) {
    Serial.print("LDR: ");
    Serial.println(ldrValue);
    Serial.print("thresholdV: ");
    Serial.println(doorThresholdValue);
  }
}

void set_ldr_value(){
  // ldrValue = averaging 2 LDR sensor pointed at a different hemisphere Souht-East Souht-West
  ldrValue = (analogRead(LDR1_PIN) + analogRead(LDR2_PIN))/2;
}

/**
 * Light in coop ON
 */
void light_on(){
  digitalWrite(LIGHT_PIN,HIGH);
}

/**
 * Light in coop OFF
 */
void light_off(){
  digitalWrite(LIGHT_PIN,LOW);
}

/**
 * Open the Door
 */
void door_open(){
  start_motor("open");
  // wait while door is going to the top
  while(endstopTopState == LOW) {
    // When door has left endstop
    if (endstopBottomState == LOW) {
      set_motor_speed(motorSpeed);
    }
    set_endstop_states();
    delay(100);
  }
  stop_motor();
}

/**
 * Open the Door
 */
void door_close(){
  start_motor("close");
  // wait while door is going to the top
  while(endstopTopState == LOW) {
    // When door has left endstop
    if (endstopBottomState == LOW) {
      set_motor_speed(motorSpeed);
    }
    set_endstop_states();
    delay(100);
  }
  stop_motor();
}

/**
 * start the motor
 * @variable : direction "open"/"close"
 */
void start_motor(const char* direction){
  
  if (DEBUG) {
    Serial.print("Going to start the motor. Direction: ");
    Serial.println(direction);
  }

  enable_motor();
  set_motor_speed(255);
  set_motor_direction(direction);
}

/**
 * stop the motor
 */
void stop_motor() {
  disable_motor();
  set_motor_speed(0);
}

/**
 * set motor speed 
 * @variable : speed (0 to 255)
 */
void set_motor_speed(int speed) {
  analogWrite(MOTOR_PWM_PIN, speed);
}

/**
 * set motor direction
 * @variable : direction "open"/"close"
 */
void set_motor_direction(const char* direction) {
  if (direction == "open") {
    digitalWrite(MOTOR_1A_PIN, HIGH);
  } else if (direction == "close") {
    digitalWrite(MOTOR_2A_PIN, HIGH);
  }
}

/**
 * What could this be?
 */
void enable_motor(){
  digitalWrite(MOTOR_EN_PIN, HIGH);
}

/**
 * disable the motor
 */
void disable_motor(){
  digitalWrite(MOTOR_EN_PIN, LOW);
  digitalWrite(MOTOR_1A_PIN,LOW);
  digitalWrite(MOTOR_2A_PIN,LOW);
}

/**
 * what is says 
 */
void set_endstop_states(){
  endstopBottomState = endstopBottom.read();
  endstopTopState = endstopTop.read();
}

/**
 * toggles the door if open it closes visa versa
 */
void toggle_door(){
  // prevents that you toggle the door when it is moving and also takes care of the correct direction
  if (endstopTopState == LOW) {
    door_close();
  } else if (endstopBottomState == LOW) {
    door_open();
  }
}

/**
 * If you don't know why this is here start with the blink sketch
 */
void setup(){
  Serial.begin(57600);
  
  setTime(1,1,1,1,1,1);
  // Start up the library
  sensors.begin();

  if (DEBUG) {
    Serial.print("Found ");
    Serial.print(sensors.getDeviceCount(), DEC);
    Serial.println(" devices.");
  }
  
  
  if (!sensors.getAddress(insideThermometer, 0) && DEBUG) Serial.println("Unable to find address for Device 0"); 
  
  
  init_input();
  init_output();
  
  Alarm.timerRepeat(30, check);
}

/**
 * Lets do this rollercoaster
 * Hopefully I'm not going to kill some chickens
 */
void loop(){
  test();

  set_endstop_states();
  
  if (testButton.update() && testButton.read() == LOW) {
    toggle_door();
  }
  
  digitalWrite(DOOR_THRESHOLD_LED_PIN,doorThresholdState);
  
  Alarm.delay(500);
}
