#include <Wire.h>
#include <Adafruit_LPS28.h>
#include <avr/sleep.h>

#define HORN_PIN PIN_PA4
//#define HORN_PIN 1


// Tunable thresholds
#define TRIGGER_ON  1.5
#define TRIGGER_OFF 0.8

Adafruit_LPS28 lps28;

float baselinePressure = 0;
float filteredPressure = 0;

bool alarmActive = false;

unsigned long lastToggle = 0;
bool hornState = false;
bool firstTrigger = false;

unsigned long startTime;

// ------------------ SLEEP ------------------

void sleepWarning(){
  // distinctive pattern
  digitalWrite(HORN_PIN, HIGH);
  delay(300);
  digitalWrite(HORN_PIN, LOW);
  delay(150);
  digitalWrite(HORN_PIN, HIGH);
  delay(300);
  digitalWrite(HORN_PIN, LOW);
}

void goToSleep(){

  sleepWarning();

  digitalWrite(HORN_PIN, LOW);
  pinMode(HORN_PIN, INPUT); // reduce leakage

  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();
  sleep_cpu();
}

// ------------------ SETUP ------------------

void setup() {
  pinMode(HORN_PIN, OUTPUT);
  digitalWrite(HORN_PIN, LOW);

  Wire.begin();

  if(!lps28.begin()){
    while(1){
      digitalWrite(HORN_PIN, HIGH);
      delay(500);
      digitalWrite(HORN_PIN, LOW);
      delay(500);
    }
  }

  lps28.setDataRate(LPS28_ODR_200_HZ);
  lps28.setAveraging(LPS28_AVG_4);
  lps28.setFullScaleMode(true);

  delay(1000); // sensor settle

  // Initial baseline
  float sum = 0;
  for(int i=0;i<20;i++){
    sum += lps28.getPressure();
    delay(50);
  }

  baselinePressure = sum / 20.0;
  filteredPressure = baselinePressure;

  // Startup beep
  for(int i=0;i<3;i++){
    digitalWrite(HORN_PIN, HIGH);
    delay(80);
    digitalWrite(HORN_PIN, LOW);
    delay(80);
  }

  startTime = millis();  // start timer
}

// ------------------ LOOP ------------------

void loop() {

  float pressureNow = lps28.getPressure();

  // Fast filter
  filteredPressure = filteredPressure * 0.7 + pressureNow * 0.3;

  float diff = filteredPressure - baselinePressure;

  // Baseline tracking
  if (!alarmActive) {
    baselinePressure = baselinePressure * 0.99 + filteredPressure * 0.01;
  }

  // Hysteresis
  if (!alarmActive && diff > TRIGGER_ON) {
    alarmActive = true;
    firstTrigger = true;
  }

  if (alarmActive && diff < TRIGGER_OFF) {
    alarmActive = false;
    digitalWrite(HORN_PIN, LOW);
  }

  // Alarm behavior
  if (alarmActive) {

    // First chirp
    if (firstTrigger) {
      digitalWrite(HORN_PIN, HIGH);
      delay(80);
      digitalWrite(HORN_PIN, LOW);
      delay(80);
      firstTrigger = false;
    }

    float d = diff;
    if(d > 20) d = 20;
    if(d < 2) d = 2;

    int interval = 300 - (d * 10);
    if(interval < 60) interval = 60;

    if (millis() - lastToggle > interval) {
      hornState = !hornState;
      lastToggle = millis();
      digitalWrite(HORN_PIN, hornState);
    }

  } else {
    digitalWrite(HORN_PIN, LOW);
  }

  // -------- AUTO SLEEP AFTER 5 MIN --------
  if(millis() - startTime > 300000UL){
    goToSleep();
  }

  delay(5);
}