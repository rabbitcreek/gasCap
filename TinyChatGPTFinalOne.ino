#include <Wire.h>
#include <Adafruit_LPS28.h>
#include <avr/sleep.h>

#define HORN_PIN PIN_PA4

Adafruit_LPS28 lps28;

float baselinePressure = 0;
float filteredPressure = 0;

unsigned long startTime;

uint8_t confirmCount = 0;
const uint8_t confirmNeeded = 5;   // 0.5 sec confirmation

// ------------------ BEEP FUNCTIONS ------------------

void beep(int t){
  digitalWrite(HORN_PIN, HIGH);
  delay(t);
  digitalWrite(HORN_PIN, LOW);
  delay(t);
}

void startupBeep(){
  for(int i=0;i<3;i++){
    beep(80);
  }
}

void sleepWarning(){
  beep(300);
  beep(150);
  beep(300);
}

// Pressure-modulated alarm
void ringDing(float diff){

  // Clamp diff to stable range
  if(diff > 20.0) diff = 20.0;
  if(diff < 2.0)  diff = 2.0;

  // Map pressure → beep speed
  int delayTime = 300 - (diff * 10);   // adjustable curve

  if(delayTime < 60) delayTime = 60;

  digitalWrite(HORN_PIN, HIGH);
  delay(delayTime);
  digitalWrite(HORN_PIN, LOW);
  delay(delayTime);
}

// ------------------ SLEEP ------------------

void goToSleep(){

  sleepWarning();

  pinMode(HORN_PIN, INPUT);  // reduce leakage

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
      beep(500);   // sensor error
    }
  }

  // Fast response settings
  lps28.setDataRate(LPS28_ODR_200_HZ);
  lps28.setAveraging(LPS28_AVG_4);
  lps28.setFullScaleMode(true);

  delay(100);

  // -------- Stable baseline (20 samples) --------
  float sum = 0;

  for(int i=0;i<20;i++){
    sum += lps28.getPressure();
    delay(50);
  }

  baselinePressure = sum / 20.0;
  filteredPressure = baselinePressure;
  // ---------------------------------------------

  startupBeep();

  startTime = millis();
}

// ------------------ LOOP ------------------

void loop() {

  float pressureNow = lps28.getPressure();

  // Smooth sloshing (3-line filter)
  filteredPressure = filteredPressure * 0.9 + pressureNow * 0.1;

  float diff = filteredPressure - baselinePressure;

  // Sustained detection logic
  if(diff > 2.0){
      confirmCount++;
      if(confirmCount >= confirmNeeded){
          ringDing(diff);   // <-- now modulated
      }
  } else {
      confirmCount = 0;
  }

  // Auto sleep after 5 minutes
  if(millis() - startTime > 300000UL){
      goToSleep();
  }

  delay(100);
}