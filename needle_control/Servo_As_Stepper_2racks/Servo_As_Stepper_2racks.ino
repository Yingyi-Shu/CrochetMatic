// Teensy pin 3 is step for main needle 
// Teensy pin 4 direction for main needle
// Teensy pin 29 is step for latch needle 
// Teensy pin 30 direction for latch needle
// Receive instructions on I2C0 (SCL0 -> 19 and SCA0 -> 18) or serial
#include "Encoder.h"
#include <Wire.h>
#include <Servo.h>
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
  void safe_update_needle(float transmitZ, float transmitE);

  // To be replaced by I2C data to reach a servo;
  Servo needle_servo;
  Servo latch_servo;
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
  
  needle_servo.write(needleAngle);
  latch_servo.write(180 - latchAngle);

  if(this->inverted){
    needle_servo.write(180 - needleAngle);
    latch_servo.write(latchAngle);
    //Serial.print("Written Angles: "); Serial.print(180 - needleAngle); Serial.print("/"); Serial.println(latchAngle);
  }
  else{
    needle_servo.write(needleAngle);
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
    needle_servo.write(180 - needleAngle);
    Serial.print("Safe update Z:"); Serial.println(180 - needleAngle);
    latch_servo.write(latchAngle);
    Serial.print("Safe update E:"); Serial.println(latchAngle);
  }
  else{
    needle_servo.write(needleAngle);
    Serial.print("Safe update Z:"); Serial.println(needleAngle);
    latch_servo.write(180 - latchAngle);
    Serial.print("Safe update E:"); Serial.println(180 - latchAngle);
  }
}

void Needle::set_as_active()
{
  set_needle_stepper_position(current_needle_pos);
  set_latch_stepper_position(current_latch_pos);
}


const int number_of_needles = 10;
Needle needles[number_of_needles];

//row 1
//needle 0 {main: 0, latch: 1}
//needle 1 {main: 2, latch: 5}
//needle 2 {main: 6, latch: 7}
//needle 3 {main: 8, latch: 9}
//needle 4 {main: 10, latch: 11}

//row 2
//needle 5 {main: 12, latch: 13}
//needle 6 {main: 14, latch: 15}
//needle 7 {main: 16, latch: 17}
//needle 8 {main: 20, latch: 21}  // figure something out 
//needle 9 {main: 22, latch: 23}  // for these two (no open pins)

const int servo_pins[2*number_of_needles] = {0, 1, 2, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 20, 21, 22, 23};

void switch_active_needle(int old_needle, int new_needle)
{
  if (old_needle != new_needle && new_needle < number_of_needles && new_needle >= 0)
  {
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
    Serial.println(servo_pins[i*2]);
    Serial.println(servo_pins[i*2 + 1]);
    needles[i].needle_servo.attach(servo_pins[i*2]);
    needles[i].latch_servo.attach(servo_pins[i*2 + 1]);
    if( (i%2==0 && i<5) || (i%2==1 && i>=5) ){ // forward facing servos
      needles[i].needle_servo.write(0);
      needles[i].latch_servo.write(180);
      needles[i].inverted = false;
    }
    else { // backwards facing servos
      needles[i].needle_servo.write(180);
      needles[i].latch_servo.write(0);
      needles[i].inverted = true;
    }
  }
  current_needle = 0;
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
      case '8':
        switch_active_needle(current_needle, 7);
        current_needle = 7;
      break;
      case '9':
        switch_active_needle(current_needle, 8);
        current_needle = 8;
      break;
      default:
      break;
    }
    Serial.print("Active needle is: ");
    Serial.println(current_needle);
  }

  
  if(current_needle_pos != needle_stepper_position() || current_latch_pos != latch_stepper_position()){
    needles[current_needle].safe_update_servos();
    Serial.print("Active needle is: "); Serial.println(current_needle);
    Serial.print("Needle/latch position is: "); Serial.print(needle_stepper_position());  Serial.print("/");  Serial.println(latch_stepper_position());
    Serial.print("Needle/latch degree is: "); Serial.print(needles[current_needle].needle_servo.read()); Serial.print(" deg / "); Serial.print(needles[current_needle].latch_servo.read()); Serial.println(" deg");
    current_needle_pos = needle_stepper_position();
    current_latch_pos = latch_stepper_position();
  }
}

