
// read squarewave on pin 2 and calc ignition timing
//
#include <digitalWriteFast.h>

const byte interruptPin = 2;
const byte SPARKPLUG_1_4 = 10;
// volatile byte state = LOW;
volatile unsigned long timeSinceMissingTooth = 0;
volatile unsigned long timeToFire = 0;
volatile unsigned long toothGap = 0;
volatile unsigned long lastTime = 0;
volatile unsigned long lastGap = 0 ;
volatile byte missingTooth = 0 ;
volatile float timePerDegree = 0;
byte signalToothOffSet = 90; // in degrees - the sensor being fitted 90 degrees before TDC
byte defaultAdvance = 10;    // in degrees
float frequency = 0;
volatile unsigned int sparkDelayTime_1_4 = 10;   // microseconds

volatile unsigned int sparkOnTime = 2000;     // microseconds

// allow for time taken to enter ISR (determine empirically)
const unsigned int isrDelayFactor = 200;        // microseconds

// is spark currently on?
volatile boolean sparkOn_1_4;
void setup()
{
  Serial.begin(9600);
  pinMode(interruptPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(interruptPin), ISR_missing_Tooth, FALLING);
  noInterrupts();           // disable all interrupts
  TCCR1A = 0;  //  Timer 1 normal mode
  TCCR1B = 0;  //  Timer 1 stop timer
  TIMSK1 = 0;  //  Timer 1 cancel timer interrupt
  
  interrupts();           // enable all interrupts
  pinMode (SPARKPLUG_1_4, OUTPUT);
}

void loop()
{
   defaultAdvance = map(analogRead(A0), 0, 1023, 0, 45);
  if (missingTooth = 1) {
    // if timesincemissing tooth eq or greater than ignition and off set
    // fire sparkplug
     timeToFire = (signalToothOffSet - defaultAdvance) * timePerDegree;
    if (timeSinceMissingTooth >= timeToFire) {
      timeSinceMissingTooth = 0;      
      // fire 1,4 sparkplugs
     sparkDelayTime_1_4 = timeToFire;
     timeToFire = (signalToothOffSet - defaultAdvance + 180) * timePerDegree;
     fireSparkPlugs_1_4(); // fire now on cylinders 1 and 4  
    }
    timePerDegree = lastGap / 10; // lastGap period covers 10 degrees
    frequency = 1/((timePerDegree / 1000000) * 360 ) * 60;   /// need to rename this RPM
    Serial.print("rpm:");
    Serial.print(frequency);
    Serial.print("\n");
    missingTooth = 0 ;
  }
}


void ISR_missing_Tooth() {

  toothGap = micros() - lastTime;
  if (toothGap > 1.8 * lastGap) {
  // missing tooth detected
  Serial.print("missing tooth\n");
    timeSinceMissingTooth = micros();
    missingTooth = 1;
    }
  lastGap = toothGap;
  lastTime = micros();
}

void fireSparkPlugs_1_4() // pass parameter of offset time
 {
  sparkOn_1_4 = false;                  // make sure flag off just in case
  // set up Timer 1
  noInterrupts();
  TCCR1A = 0;                       // normal mode
  TCNT1 = 0;                        // count back to zero
  TCCR1B = bit(WGM12) | bit(CS11);  // CTC, scale to clock / 8
  // time before timer fires - zero relative
  // multiply by two because we are on a prescaler of 8
  OCR1A = (sparkDelayTime_1_4 * 2) - (isrDelayFactor * 2) - 1;
  TIMSK1 = bit (OCIE1A);            // interrupt on Compare A Match
  interrupts();
  }

  // interrupt for when time to turn spark on then off again
ISR (TIMER1_COMPA_vect)
  {
  // if currently on, turn off
  if (sparkOn_1_4)
    {
    digitalWriteFast (SPARKPLUG_1_4, LOW);  // spark off
    TCCR1B = 0;                         // stop timer
    TIMSK1 = 0;                         // cancel timer interrupt
    }
  else
    // hold-off time must be up
    {
    digitalWriteFast (SPARKPLUG_1_4, HIGH); // spark on
    TCCR1B = 0;                         // stop timer
    TCNT1 = 0;                          // count back to zero
    TCCR1B = bit(WGM12) | bit(CS11);    // CTC, scale to clock / 8
    // time before timer fires (zero relative)
    // multiply by two because we are on a prescaler of 8
    OCR1A = (sparkOnTime * 2) - (isrDelayFactor * 2) - 1;    
    }
    sparkOn_1_4 = !sparkOn_1_4;                  // toggle
  }  // end of TIMER1_COMPA_vect
