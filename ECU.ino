
/************************************************************************
 *  INJ-CPU --------> B01 | B02 -> PLUG-CPU
 *  STEPER-CPU1 ----> B03 | B04 -> STEPER-CPU3
 *  STEPER-CPU2 ----> B05 | B06 -> STEPER-CPU4
 *  O2-CPU ---------> B07 | B08 -> GND
 *  GND ------------> B09 | B10 -> BAT-CPU
 *  AIT ------------> B11 | B12 -> TPS
 *  MAP ------------> B13 | B14 -> OIL Sensor
 *  CAM ------------> B15 | B16 -> SPARK
 *  +5V ------------> B17 | B18 -> TEMP
 *                    
 *  INPUT:                           | OUTPUT:
 *    DIGITAL:                       |   INJ-CPU ------> B01 -> D22 
 *      OIL Sensor -> B14 -> D28     |   PLUG-CPU -----> B02 -> D23
 *    INTERRUPT:                     |   STEPER-CPU1 --> B03 -> D24
 *      CAM --------> B15 -> D02     |   STEPER-CPU2 --> B04 -> D25
 *      SPARK ------> B16 -> D03     |   STEPER-CPU3 --> B05 -> D26
 *    ANALOG:                        |   STEPER-CPU4 --> B06 -> D27
 *      O2-CPU -----> B07 -> A0(D54) |
 *      BAT-CPU ----> B10 -> A1(D55) |
 *      AIT --------> B11 -> A2(D56) |
 *      TPS --------> B12 -> A3(D57) |
 *      MAP --------> B13 -> A4(D58) |
 *      TEMP -------> B18 -> A5(D59) |
 */
#include <Stepper.h>
#include <digitalWriteFast.h>

const short OILSENSOR = 28;
const short CAM = 2;
const short SPARK = 3;
const short INJ = 22;
const short PLUG = 23;
const short STEPER1 = 24;
const short STEPER2 = 25;
const short STEPER3 = 26;
const short STEPER4 = 27;
const short O2 = A0;
const short BAT = A1;
const short AIT = A2;
const short TPS = A3;
//map(sensorValue, 431, 762, 0, 100);
const short MAP = A4;
const short TEMP = A5;

const int stepsPerRevolution = 200;
volatile unsigned int injOnTime = 20000;     // microseconds
volatile unsigned int sparkOnTime = 4000;     // microseconds
bool firstsig = true;
bool intakestart = true;
bool firestart = true;


Stepper myStepper(stepsPerRevolution, STEPER1, STEPER2, STEPER3, STEPER4);

void setup() {
  Serial.begin(9600);
  pinMode(INJ, OUTPUT);
  pinMode(PLUG, OUTPUT);
  pinMode(CAM, INPUT_PULLUP);
  pinMode(SPARK, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(CAM), intake, FALLING);
  attachInterrupt(digitalPinToInterrupt(SPARK), fire, FALLING);
  noInterrupts(); // disable all interrupts
  TCCR1A = 0;  //  Timer 1 normal mode
  TCCR1B = 0;  //  Timer 1 stop timer
  TIMSK1 = 0;  //  Timer 1 cancel timer interrupt
  interrupts(); // enable all interrupts
  Serial.begin(9600);
  Serial.print("hello\n");
}

void loop() {
  map(analogRead(O2), 0, 1023, 0, 100);
  map(analogRead(BAT), 0, 1023, 0, 100);
  (((analogRead(AIT) * 5) / 1023) - 0.5) * 100;
  map(analogRead(TPS), 431, 762, 0, 100);
  map(analogRead(MAP), 0, 1023, 0, 105);
 (((analogRead(TEMP) * 5) / 1023) - 0.5) * 100;
}

void steper() {
  // read the sensor value:
  int sensorReading = analogRead(A0);
  // map it to a range from 0 to 100:
  int motorSpeed = map(sensorReading, 0, 1023, 0, 100);
  // set the motor speed:
  if (motorSpeed > 0) {
    myStepper.setSpeed(motorSpeed);
    // step 1/100 of a revolution:
    myStepper.step(stepsPerRevolution / 100);
  }
}



void intake() {
  Serial.print("intake\n");
  digitalWriteFast(INJ, HIGH); // inj on
  noInterrupts();
  TCCR1B = 0;                         // stop timer
  TCNT1 = 0;                          // count back to zero
  TCCR1B = bit(WGM12) | bit(CS11);    // CTC, scale to clock / 8
  // multiply by two because we are on a prescaler of 8
  OCR1A = (injOnTime * 2) - 1;
  TIMSK1 |= (1 << OCIE1A);
  interrupts();
}

void fire() {
  Serial.print("fire\n");
  digitalWriteFast(PLUG, HIGH);
  noInterrupts();
  TCCR1B = 0;                         // stop timer
  TCNT1 = 0;                          // count back to zero
  TCCR1B = bit(WGM12) | bit(CS11);    // CTC, scale to clock / 8
  // multiply by two because we are on a prescaler of 8
  OCR1A = (sparkOnTime * 2) - 1;
  TIMSK1 |= (1 << OCIE1A);
  firstsig = false;
  interrupts();
}

ISR (TIMER1_COMPA_vect) {
  Serial.print("off\n");
  digitalWriteFast (INJ, LOW);
  digitalWriteFast(PLUG, LOW);
  TCCR1B = 0;                         // stop timer
  TIMSK1 = 0;                         // cancel timer interrupt
}
