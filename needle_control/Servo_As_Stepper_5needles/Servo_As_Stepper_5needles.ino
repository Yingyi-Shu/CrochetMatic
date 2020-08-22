// Teensy pin 3 is step for main needle 
// Teensy pin 4 direction for main needle
// Teensy pin 29 is step for latch needle 
// Teensy pin 30 direction for latch needle
// Receive instructions on I2C0 (SCL0 -> 19 and SCA0 -> 18) or serial
#include "Encoder.h"
#include <Wire.h>
#include <Servo.h>

Servo hook_servo;
Servo latch_servo;

const int I2C_SLAVE = 42;
const int STEP_PER_MM = 128;
const int MAX_RANGE_MM = 60;
const int MAXIMUM_STEPS = MAX_RANGE_MM * STEP_PER_MM;
//const int MIN_NEEDLE_LATCH_DELTA_STEP = -10 * STEP_PER_MM;
//const int MAX_NEEDLE_LATCH_DELTA_STEP = 28*STEP_PER_MM;
const int MIN_NEEDLE_LATCH_DELTA_STEP = 0;
const int MAX_NEEDLE_LATCH_DELTA_STEP = 30*STEP_PER_MM;
int current_needle;

int prev_needle_value = 0;
int prev_latch_value = 0;

const int starting_low_pulse_width = 1500;
const int starting_high_pulse_width = 1500;
const int small_delta = 10;
const int large_delta = 50;
int needle_current_pulse_width =1000;
int latch_current_pulse_width =1000;
bool calibration_command = false;

class ServoParameters {
  public:
  int pin;
  int min_pulse_width;
  int max_pulse_width;
};


int32_t current_needle_pos;
int32_t current_latch_pos;

class Needle {
  public:
  int needle_steps;
  int latch_steps;
  float needle_pos;
  float latch_pos;
  bool inverted;
  
  void safe_update_servos();
  void set_as_active();
  void detach();
  void safe_update_needle(float transmitZ, float transmitE);

  ServoParameters hook;
  ServoParameters latch;
  
  // int needle_servo_pin;
  // int latch_servo_pin;
  // Servo needle_servo;
  // Servo latch_servo;
};


void Needle::safe_update_servos()
{  
  latch_steps = latch_stepper_position();
  needle_steps = needle_stepper_position();
  //provides constrain for needle steps so needle doesn't go beyond maximum range
  needle_steps = constrain(needle_steps, 0, MAXIMUM_STEPS);
  
  //Serial.print("Needle steps: "); Serial.println(needle_steps/STEP_PER_MM);
  int max_safe_latch_steps = needle_steps + MAX_NEEDLE_LATCH_DELTA_STEP;
  max_safe_latch_steps = constrain(max_safe_latch_steps, 0, MAXIMUM_STEPS);
  int min_safe_latch_steps = needle_steps + MIN_NEEDLE_LATCH_DELTA_STEP;
  min_safe_latch_steps = constrain(min_safe_latch_steps, 0, MAXIMUM_STEPS);
  
  latch_steps = constrain(latch_steps, min_safe_latch_steps, max_safe_latch_steps);
  //Serial.print("Latch steps: "); Serial.println(latch_steps/STEP_PER_MM);
  float needleAngle = map(needle_steps, 0, MAXIMUM_STEPS, 0, 180);
  float latchAngle = map(latch_steps, 0, MAXIMUM_STEPS, 0, 180);
  
  hook_servo.write(needleAngle);
  latch_servo.write(180 - latchAngle);

  if(this->inverted){
    hook_servo.write(180 - needleAngle);
    latch_servo.write(latchAngle);
    //Serial.print("Written Angles: "); Serial.print(180 - needleAngle); Serial.print("/"); Serial.println(latchAngle);
  }
  else{
    hook_servo.write(needleAngle);
    latch_servo.write(180 - latchAngle);
    //Serial.print("Written Angles: "); Serial.print(needleAngle); Serial.print("/"); Serial.println(180 - latchAngle);
  }
}

void Needle::safe_update_needle(float transmitZ, float transmitE)
{
  needle_pos = transmitZ;
  latch_pos = transmitE;
  
  needle_steps = transmitZ * STEP_PER_MM;
  needle_steps = constrain(needle_steps, 0, MAXIMUM_STEPS);
  
  int max_safe_latch_steps = needle_steps + MAX_NEEDLE_LATCH_DELTA_STEP;
  max_safe_latch_steps = constrain(max_safe_latch_steps, 0, MAXIMUM_STEPS);
  
  int min_safe_latch_steps = needle_steps + MIN_NEEDLE_LATCH_DELTA_STEP;
  min_safe_latch_steps = constrain(min_safe_latch_steps, 0, MAXIMUM_STEPS);
  
  latch_steps = transmitE * STEP_PER_MM;
  latch_steps = constrain(latch_steps, min_safe_latch_steps, max_safe_latch_steps);

  float needleAngle = map(needle_steps, 0, MAXIMUM_STEPS, 0, 180);
  float latchAngle = map(latch_steps, 0, MAXIMUM_STEPS, 0, 180);
 
  if(this->inverted){
    hook_servo.write(180 - needleAngle);
    Serial.print("Safe update Z:"); Serial.println(180 - needleAngle);
    latch_servo.write(latchAngle);
    Serial.print("Safe update E:"); Serial.println(latchAngle);
  }
  else{
    hook_servo.write(needleAngle);
    Serial.print("Safe update Z:"); Serial.println(needleAngle);
    latch_servo.write(180 - latchAngle);
    Serial.print("Safe update E:"); Serial.println(180 - latchAngle);
  }
}

void Needle::set_as_active()
{
  hook_servo.attach(hook.pin, hook.min_pulse_width, hook.max_pulse_width);
  latch_servo.attach(latch.pin, latch.min_pulse_width, latch.max_pulse_width);
  set_needle_stepper_position(current_needle_pos);
  set_latch_stepper_position(current_latch_pos);
}

void Needle::detach()
{
  hook_servo.detach();
  latch_servo.detach();
}

const int number_of_needles = 10;
Needle needles[number_of_needles];

//row 1
//needle 0 {main: 0, latch: 1}
//needle 1 {main: 2, latch: 5}
//needle 2 {main: 6, latch: 7}
//needle 3 {main: 8, latch: 9}
//needle 4 {main: 10, latch: 11}

//const int servo_pins[2*number_of_needles] = {0, 1, 2, 5, 6, 7, 8, 9, 10, 11};
const ServoParameters needle_parameters[number_of_needles][2] = {{{0,  500, 2500}, {1,  500, 2500}},
                                                                 {{2,  500, 2500}, {5,  500, 2500}},
                                                                 {{6,  500, 2500}, {7,  500, 2500}},
                                                                 {{8,  500, 2500}, {9,  500, 2500}},
                                                                 {{10, 500, 2500}, {11, 500, 2500}},
                                                                 {{12, 500, 2500}, {13, 500, 2500}},
                                                                 {{14, 500, 2500}, {15, 500, 2500}},
                                                                 {{16, 500, 2500}, {17, 500, 2500}},
                                                                 {{20, 500, 2500}, {21, 500, 2500}},
                                                                 {{22, 500, 2500}, {23, 500, 2500}}};

void switch_active_needle(int old_needle, int new_needle)
{
  if (old_needle != new_needle && new_needle < number_of_needles && new_needle >= 0)
  {
    needles[old_needle].detach();
    needles[new_needle].set_as_active();
    current_needle = new_needle;
  }
}

void setup()
{
 Serial.begin(9600);
 while (Serial == 0);
 encoder_setup();
  for (int i = 0; i < number_of_needles; i++)
  {
    // Here we initialize the different parameters for each servo of that needle
    needles[i].hook = needle_parameters[i][0];
    needles[i].latch = needle_parameters[i][1];
    Serial.println(needles[i].hook.pin);
    Serial.println(needles[i].latch.pin);
//    needles[i].hook_servo.attach(needle_parameters[i].needle_pin,
//                                   needle_parameters[i].needle_min_pulse_width,
//                                   needle_parameters[i].needle_max_pulse_width);
//    needles[i].latch_servo.attach(needle_parameters[i].latch_pin,
//                                  needle_parameters[i].latch_min_pulse_width,
//                                  needle_parameters[i].latch_max_pulse_width);
    if(i%2==0){
//      needles[i].hook_servo.write(0);
//      needles[i].latch_servo.write(180);
      needles[i].inverted = false;
    }
    else{
//      needles[i].hook_servo.write(180);
//      needles[i].latch_servo.write(0);
      needles[i].inverted = true;
    }
  }
  Serial.println("Needle 0 is Active:");
  current_needle = 0;
  needles[current_needle].set_as_active();

  current_needle_pos = 0;
  current_latch_pos = 0;
  Wire.begin(I2C_SLAVE);
  Wire.onReceive(on_I2C_event);
  Wire1.begin();
}

void on_I2C_event(int how_many)
{
  if (how_many >= 4)
  {
    if (how_many < 32)
    {
      //N1,60.00,60.00
      //  0   1   2   3   4   5   6   7   8   9   10  11 12  13
      //['N','1',' ','6','0','.','0','0',' ','6','0','.','0','0']
      //data array will be 14 long
      Serial.print("Processing a ");
      Serial.print(how_many);
      Serial.println(" characters command...");
      char command[32];
      for (int k = 0; k < how_many; k++)
      {
        command[k] = Wire.read();
      }
      command[how_many] = 0;
      Serial.print("Recieved: ");
      Serial.print(command);
      Serial.println(" ");
      if (command[0] == 'N')
      {
        String to_parse = (command + 1);
        to_parse = to_parse.substring(1);
        String needleNum, zCoord, eCoord;
        for(int i = 0; i < to_parse.length(); i++){
          if(to_parse.charAt(i) == ','){
            needleNum = to_parse.substring(0,i);
            zCoord = to_parse.substring(i+1);
            break;
          }
        }
        for(int i = 0; i < zCoord.length(); i++){
          if(zCoord.charAt(i) == ','){
            eCoord = zCoord.substring(i+1);
            zCoord = zCoord.substring(0,i);
            break;
          }
        }
        long new_needle = needleNum.toInt();
        float z = zCoord.toFloat();
        float e = eCoord.toFloat();
        Serial.print("N: "); Serial.print(new_needle); Serial.print(" Z: "); Serial.print(z); Serial.print(" E: "); Serial.println(e);
        switch_active_needle(current_needle, new_needle);
        Serial.print("Active needle is: "); Serial.println(current_needle);
        needles[current_needle].safe_update_needle(z, e);
      }
      else
      {
         Serial.println("Cannot parse the I2C command!");
      }
    }
    else
    {
      Serial.println("The I2C command is too long!");
    }
  }
  else
  {
    Serial.println("The I2C command is too short!");
  }
}

void loop()
{
    if (Serial.available() > 0)
  {
    switch(Serial.read())
    {
      case '1':
        switch_active_needle(current_needle, 0);
        current_needle = 0;
      break;
      case '2':
        switch_active_needle(current_needle, 1);
        current_needle = 1;
      break;
      case '3':
        switch_active_needle(current_needle, 2);
        current_needle = 2;
      break;
      case '4':
        switch_active_needle(current_needle, 3);
        current_needle = 3;
      break;
      case '5':
        switch_active_needle(current_needle, 4);
        current_needle = 4;
      break;
      case '6':
        switch_active_needle(current_needle, 5);
        current_needle = 5;
      break;
      case '7':
        switch_active_needle(current_needle, 6);
        current_needle = 6;
      break;
      default:
      break;
    }
  }
  if (calibration_command == true)
  {
    hook_servo.writeMicroseconds(needle_current_pulse_width);
    latch_servo.writeMicroseconds(latch_current_pulse_width);
    Serial.print("Needle #");
    Serial.print(current_needle);
    Serial.print(": Needle pulse width: ");
    Serial.print(needle_current_pulse_width);
    Serial.print(" (");
    Serial.print(hook_servo.read());
    Serial.print("deg); ");    
    Serial.print(" Latch pulse width: ");
    Serial.print(latch_current_pulse_width);
    Serial.print(" (");
    Serial.print(latch_servo.read());
    Serial.print("deg); ");  
    Serial.println("");  
    calibration_command = false;
  }
  else
  {
    if(current_needle_pos != needle_stepper_position() || current_latch_pos != latch_stepper_position()){
      needles[current_needle].safe_update_servos();
      Serial.print("Active needle is: "); Serial.println(current_needle);
      Serial.print("Needle/latch position is: "); Serial.print(needle_stepper_position());  Serial.print("/");  Serial.println(latch_stepper_position());
      Serial.print("Needle/latch degree is: "); Serial.print(hook_servo.read()); Serial.print(" deg / "); Serial.print(latch_servo.read()); Serial.println(" deg");
      current_needle_pos = needle_stepper_position();
      current_latch_pos = latch_stepper_position();
    }
  }
}

