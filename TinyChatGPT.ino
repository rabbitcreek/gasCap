#include <Wire.h>
#include <Adafruit_LPS28.h>
#include <avr/sleep.h>

#define HORN_PIN PIN_PA4

Adafruit_LPS28 lps28;

float baselinePressure = 0;
float filteredPressure = 0;

unsigned long startTime;

uint8_t confirmCount = 0;
const uint8_t confirmNeeded = 5;   // 5 readings = 0.5 seconds

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

void ringDing(){
  digitalWrite(HORN_PIN, HIGH);
  delay(80);
  digitalWrite(HORN_PIN, LOW);
  delay(80);
}

void goToSleep(){

  sleepWarning();

  pinMode(HORN_PIN, INPUT);  // reduce leakage current

  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();
  sleep_cpu();
}

void setup() {

  pinMode(HORN_PIN, OUTPUT);
  digitalWrite(HORN_PIN, LOW);

  Wire.begin();

  if(!lps28.begin()){
    while(1){
      beep(500);   // sensor failure alarm
    }
  }

  // Fast sensor response
  lps28.setDataRate(LPS28_ODR_200_HZ);
  lps28.setAveraging(LPS28_AVG_4);
  lps28.setFullScaleMode(true);

  delay(100);

  // -------- Stable baseline measurement --------
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

void loop() {

  float pressureNow = lps28.getPressure();

  // Smooth out sloshing
  filteredPressure = filteredPressure * 0.9 + pressureNow * 0.1;

  float diff = filteredPressure - baselinePressure;

  // Require sustained pressure rise
  if(diff > 2.0){
      confirmCount++;
      if(confirmCount >= confirmNeeded){
          ringDing();
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