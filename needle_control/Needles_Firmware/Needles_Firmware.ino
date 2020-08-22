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
int MAX_NEEDLE_LATCH_DELTA_STEP = 35*STEP_PER_MM;
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

int32_t current_needle_pos = 0;
int32_t current_latch_pos = 0;

bool hook_inversion_map[] = {false, true, false, true, false,
                             false, true, false, true, false, 
                             false, true, false, true, false,
                             false, true, false, true, false};
                             
bool latch_inversion_map[] = {false, true, false, true, false,
                             false, true, false, true, false, 
                             false, true, false, true, false,
                             false, true, false, true, false};

class Needle {
  public:
  int needle_steps;
  int latch_steps;
  int latch_state;
  float needle_pos;
  float latch_pos;
  bool hook_inverted;
  bool latch_inverted;
  
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
  int lp = this->latch_state;
  if(lp == 0){
    latch_steps = needle_steps + 10.0 * STEP_PER_MM;
  }
  else if(lp == -1){
    latch_steps = needle_steps + 0 * STEP_PER_MM;
  }
  else if(lp == 1){
    latch_steps = needle_steps + 100 * STEP_PER_MM;
  }
  
  //Serial.print("Needle steps: "); Serial.println(needle_steps/STEP_PER_MM);
  int max_safe_latch_steps = needle_steps + MAX_NEEDLE_LATCH_DELTA_STEP;
  max_safe_latch_steps = constrain(max_safe_latch_steps, 0, MAXIMUM_STEPS);
  int min_safe_latch_steps = needle_steps + MIN_NEEDLE_LATCH_DELTA_STEP;
  min_safe_latch_steps = constrain(min_safe_latch_steps, 0, MAXIMUM_STEPS);
  
  latch_steps = constrain(latch_steps, min_safe_latch_steps, max_safe_latch_steps);
  //Serial.print("Latch steps: "); Serial.println(latch_steps/STEP_PER_MM);
  float needleAngle = map(needle_steps, 0, MAXIMUM_STEPS, 0, 180);
  float latchAngle = map(latch_steps, 0, MAXIMUM_STEPS, 0, 180);
  
//  hook_servo.write(needleAngle);
//  latch_servo.write(180 - latchAngle);

  // handle hook & latch inversion and write to servo. 
  if(this->hook_inverted){
    hook_servo.write(180 - needleAngle);
  }
  else{
    hook_servo.write(needleAngle);
  }
  if(this->latch_inverted){
    latch_servo.write(latchAngle);
  }
  else{
    latch_servo.write(180 - latchAngle);
  }
}

void Needle::safe_update_needle(float transmitZ, float transmitE)
{
    int lp = this->latch_state;
    if(lp == 0){
      transmitE = transmitZ + 10.0;
    }
    else if(lp == -1){
      transmitE = transmitZ + 0;
    }
    else if(lp == 1){
      transmitE = transmitZ + 100;
    }
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

  Serial.print("transmitZ: "); Serial.println(transmitZ);
  Serial.print("transmitE: "); Serial.println(transmitE);
  Serial.print("latch_steps: ");Serial.println(latch_steps);
  Serial.print("Min safe latch steps: "); Serial.println(min_safe_latch_steps);
  Serial.print("Max safe latch steps: "); Serial.println(max_safe_latch_steps);

  float needleAngle = map(needle_steps, 0, MAXIMUM_STEPS, 0, 180);
  float latchAngle = map(latch_steps, 0, MAXIMUM_STEPS, 0, 180);
 
  if(this->hook_inverted){
    hook_servo.write(180 - needleAngle);
    Serial.print("Safe update Z:"); Serial.println(180 - needleAngle);
  }
  else{
    hook_servo.write(needleAngle);
    Serial.print("Safe update Z:"); Serial.println(needleAngle);
  }
  if(this->latch_inverted){
    latch_servo.write(latchAngle);
    Serial.print("Safe update E:"); Serial.println(latchAngle);
  }
  else{
    latch_servo.write(180 - latchAngle);
    Serial.print("Safe update E:"); Serial.println(180 - latchAngle);
  }
}

void Needle::set_as_active()
{ 
  Serial.print("Attaching to the servo...");
  Serial.print("hook pin ");
  Serial.print(hook.pin);
  Serial.print(" ");
  hook_servo.attach(hook.pin, hook.min_pulse_width, hook.max_pulse_width);
  Serial.print(hook_servo.attached());
  Serial.print(" latch pin ");
  Serial.print(latch.pin);
  Serial.print(" ");
  latch_servo.attach(latch.pin, latch.min_pulse_width, latch.max_pulse_width);
  Serial.print(latch_servo.attached());
  Serial.println("");
  set_needle_stepper_position(current_needle_pos);
  set_latch_stepper_position(current_latch_pos);
}

void Needle::detach()
{
  hook_servo.detach();
  latch_servo.detach();
//  Serial.print("Detaching needle, with hook pin#:");
//  Serial.println(hook.pin);
  pinMode(hook.pin, OUTPUT);
  pinMode(latch.pin, OUTPUT);
  digitalWrite(hook.pin, LOW);
  digitalWrite(latch.pin, LOW);
}

const int number_of_needles = 20;
Needle needles[number_of_needles];

// servo calibration


// Latch min: 140mm
// Latch max: 92mm
// Needle min: 145mm
// Needle max: 97mm



// {needle}, {latch}L
// params pin #, upper bound, lower bound
const ServoParameters needle_parameters[number_of_needles][2] = {{{0,  250, 2800}, {1,  250, 2800}},  // needle 0
                                                                 {{2,  250, 2800}, {5, 525, 2580}},   // needle 1
                                                                 {{6,  250, 2800}, {7,  250, 2800}},  // needle 2
                                                                 {{8,  250, 2800}, {9,  250, 2800}},  // needle 3
                                                                 {{10, 250, 2800}, {11, 250, 2800}},  // needle 4
                                                                 
                                                                 {{12, 700, 2100}, {13, 650, 2050}},  // needle 5
                                                                 {{14, 680, 2240}, {15, 900, 2280}},  // needle 6
                                                                 {{16, 570, 1930}, {17, 820, 2230}},  // needle 7
                                                                 {{20, 650, 2250}, {21, 700, 2150}},  // needle 8
                                                                 
                                                                 {{22, 250, 2800}, {23, 250, 2800}},  // needle 9 <- BASE BROKEN
                                                                 
                                                                 {{25, 750, 2300}, {24, 500, 1850}},  // needle 10
                                                                 {{27, 450, 1600}, {26, 540, 1640}},  // needle 11
                                                                 {{29, 380, 1700}, {28, 560, 1700}},  // needle 12 - problematic
                                                                 {{31, 530, 1970}, {30, 580, 2070}},  // needle 13
                                                                 {{33, 420, 1750}, {32, 560, 1870}},  // needle 14
                                                                 {{35, 810, 2090}, {34, 550, 1550}},  // needle 15
                                                                 {{37, 850, 2160}, {36, 980, 2240}},  // needle 16
                                                                 {{39, 570, 1690}, {38, 800, 2140}},  // needle 17
                                                                 {{41, 790, 2060}, {40, 740, 2320}},  // needle 18
                                                                 {{43, 480, 1820}, {42, 630, 1860}}}; // needle 19
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
    Serial.print("Assignment for needle ");
    Serial.print(i);
    Serial.print(": ");
    needles[i].hook = needle_parameters[i][0];
    needles[i].latch = needle_parameters[i][1];
    Serial.print(needles[i].hook.pin);
    Serial.print(", ");
    Serial.println(needles[i].latch.pin);
    needles[i].hook_inverted = hook_inversion_map[i];
    needles[i].latch_inverted = latch_inversion_map[i];
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
        String needleNum, zCoord, eCoord, latchPos;
        for(int i = 0; i < to_parse.length(); i++){
          if(to_parse.charAt(i) == ','){
            needleNum = to_parse.substring(0,i);
            zCoord = to_parse.substring(i+1);
            break;
          }
        }
        Serial.println(to_parse);
        Serial.println(zCoord);
        int first_c = 0; 
        int second_c = 0;
        for(int i = 0; i < zCoord.length(); i++){
          if(zCoord.charAt(i) == ',' && first_c == 0){
            first_c = i;
            continue;
          }
          if(zCoord.charAt(i) == ',' && first_c != 0 && second_c == 0){
            second_c = i;
          }
          if (first_c != 0 && second_c != 0){
            latchPos = zCoord.substring(second_c+1);
            eCoord = zCoord.substring(first_c+1, second_c);
            zCoord = zCoord.substring(0,first_c);
            break;
          }
        }
        long new_needle = needleNum.toInt();
        float z = zCoord.toFloat();
        float e = eCoord.toFloat();
        int lp = latchPos.toInt();
        Serial.print("N: "); Serial.print(new_needle); Serial.print(" Z: "); Serial.print(z); Serial.print(" E: "); Serial.println(e);
        switch_active_needle(current_needle, new_needle);
        Serial.print("Active needle is: "); Serial.println(current_needle);
        if(lp == 0){
          e = 10.0;
        }
        else if(lp == -1){
          e = 0;
        }
        else if(lp == 1){
          e = 100;
        }
        needles[current_needle].latch_state = lp;
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
      case 'L':
        {
          int new_needle_number = Serial.parseInt();
          if (new_needle_number <= number_of_needles * 2)
          {
            switch_active_needle(current_needle,new_needle_number - 1);
            current_needle = new_needle_number - 1;
          }
          else
          {
            Serial.println("Cannot read needle number!");
          }
          needle_current_pulse_width = (needles[current_needle].hook_inverted == false)?starting_low_pulse_width:starting_high_pulse_width;
          latch_current_pulse_width = (needles[current_needle].latch_inverted == true)?starting_low_pulse_width:starting_high_pulse_width;
          Serial.print("Active needle is: ");
          Serial.println(current_needle);
        }
        break;
      case 'w':
        needle_current_pulse_width += (needles[current_needle].hook_inverted == false)?small_delta:-small_delta;
        calibration_command = true;
        break;
      case 'z':
        needle_current_pulse_width -= (needles[current_needle].hook_inverted == false)?small_delta:-small_delta;
        calibration_command = true;
        break;
      case 'W':
        needle_current_pulse_width += (needles[current_needle].hook_inverted == false)?large_delta:-large_delta; 
        calibration_command = true;
        break;
      case 'Z':
        needle_current_pulse_width -= (needles[current_needle].hook_inverted == false)?large_delta:-large_delta;
        calibration_command = true;
        break;
      case 'r':
        latch_current_pulse_width += (needles[current_needle].latch_inverted == true)?small_delta:-small_delta;
        calibration_command = true;
        break;
      case 'c':
        latch_current_pulse_width -= (needles[current_needle].latch_inverted == true)?small_delta:-small_delta;
        calibration_command = true;
        break;
      case 'R':
        latch_current_pulse_width += (needles[current_needle].latch_inverted == true)?large_delta:-large_delta;
        calibration_command = true;
        break;
      case 'C':
        latch_current_pulse_width -= (needles[current_needle].latch_inverted == true)?large_delta:-large_delta;
        calibration_command = true;
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

