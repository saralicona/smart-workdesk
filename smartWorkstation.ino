#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// OLED DISPLAY CONFIGURATION
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// PIN DEFINITIONS 
const int PIN_BTN_UP     = 2; // Nav: UP
const int PIN_BTN_DOWN   = 4; // Nav: DOWN
const int PIN_BTN_SELECT = 3; // Nav: SELECT
const int PIN_SOUND_D5   = 5; // Digital Sound Sensor (CZN-1E)
const int PIN_LED_PWM    = 6; // Green Task Light PWM
const int PIN_LDR_A0     = A0;// Analog Light Sensor

//  MENU & SYSTEM STATES 
enum SystemMode { MODE_WELCOME, MODE_FOCUS, MODE_SOCIAL, MODE_RELAX };
SystemMode currentMode = MODE_WELCOME; // Starts at Welcome state

int menuIndex = 0; // 0=FOCUS, 1=SOCIAL, 2=RELAX
const int totalModes = 3;

//  TIMERS & DEBOUNCE VARIABLES 
unsigned long lastDebounceTime = 0;
const unsigned long DEBOUNCE_DELAY = 250;

unsigned long lastSoundPeakTime = 0;
const unsigned long LATCH_DURATION = 1500;
bool isAcousticMismatch = false;

// Count-Up Session Timer (45 Seconds simulates 45 minutes for prototype demo)
unsigned long sessionStartTime = 0;
const unsigned long FOCUS_SESSION_LIMIT_SEC = 45; 

void setup() {
  Serial.begin(115200);

  pinMode(PIN_BTN_UP, INPUT_PULLUP);
  pinMode(PIN_BTN_DOWN, INPUT_PULLUP);
  pinMode(PIN_BTN_SELECT, INPUT_PULLUP);
  pinMode(PIN_SOUND_D5, INPUT);
  pinMode(PIN_LED_PWM, OUTPUT);

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    for(;;);
  }
}

void loop() {
  unsigned long currentMillis = millis();

  
  // 1. GET DATA: NAVIGATION BUTTON INPUTS
 
  if (currentMillis - lastDebounceTime > DEBOUNCE_DELAY) {
    if (digitalRead(PIN_BTN_DOWN) == LOW) {
      menuIndex = (menuIndex + 1) % totalModes;
      lastDebounceTime = currentMillis;
    } 
    else if (digitalRead(PIN_BTN_UP) == LOW) {
      menuIndex = (menuIndex - 1 + totalModes) % totalModes;
      lastDebounceTime = currentMillis;
    } 
    else if (digitalRead(PIN_BTN_SELECT) == LOW) {
      // Map menuIndex (0, 1, 2) to selected mode
      if (menuIndex == 0) currentMode = MODE_FOCUS;
      else if (menuIndex == 1) currentMode = MODE_SOCIAL;
      else if (menuIndex == 2) currentMode = MODE_RELAX;

      sessionStartTime = currentMillis; // Reset timer every time a new mode is selected
      lastDebounceTime = currentMillis;
    }
  }

  // 2. GET DATA: SENSOR READINGS

  int rawAnalogLDR = analogRead(PIN_LDR_A0); // 0 (bright) - 1023 (dark)
  bool soundSpike = digitalRead(PIN_SOUND_D5);
  
  // Convert raw reading to direct Lux scale
  int mappedLux = map(rawAnalogLDR, 0, 1023, 0, 500); 


  // 3. UNDERSTAND DATA: RULE ENGINE

  int pwmBrightness = 0;
  String modeString = "";
  String statusString = "OK";
  unsigned long elapsedSec = 0;

  if (currentMode == MODE_WELCOME) {
    modeString = "WELCOME";
    statusString = "SELECT_STATE";
    pwmBrightness = 0;
    isAcousticMismatch = false;

  } else if (currentMode == MODE_FOCUS) {
    modeString = "FOCUS";
    elapsedSec = (currentMillis - sessionStartTime) / 1000;

    // Light Compensation Logic
    if (mappedLux < 300) {
      pwmBrightness = map(300 - mappedLux, 0, 300, 50, 255);
      statusString = "ADAPT_LIGHT";
    } else {
      pwmBrightness = 0;
    }

    // Acoustic Mismatch Logic
    if (soundSpike) {
      lastSoundPeakTime = currentMillis;
      isAcousticMismatch = true;
    }

    if (currentMillis - lastSoundPeakTime <= LATCH_DURATION) {
      statusString = "NOISE_ALERT";
    } else {
      isAcousticMismatch = false;
    }

    // 45-Second Break Suggestion Trigger
    if (elapsedSec >= FOCUS_SESSION_LIMIT_SEC) {
      statusString = "TAKE_A_BREAK";
    }

  } else if (currentMode == MODE_SOCIAL) {
    modeString = "SOCIAL";
    pwmBrightness = 80;
    statusString = "SOCIAL_ACTIVE";
    isAcousticMismatch = false;

  } else if (currentMode == MODE_RELAX) {
    modeString = "RELAX";
    pwmBrightness = 30;
    statusString = "RELAX_ACTIVE";
    isAcousticMismatch = false;
  }

  // Drive Hardware PWM Light Output
  analogWrite(PIN_LED_PWM, pwmBrightness);

  // 4. USE DATA: DRAW ON PHYSICAL OLED DISPLAY

  updateOLEDDisplay(modeString, mappedLux, pwmBrightness, elapsedSec, statusString, menuIndex);

  // 5. USE DATA: STREAM SERIAL DATA TO LAPTOP

  Serial.print("DATA:");
  Serial.print(modeString); Serial.print(",");
  Serial.print(mappedLux); Serial.print(",");
  Serial.print(pwmBrightness); Serial.print(",");
  Serial.print(isAcousticMismatch ? "1" : "0"); Serial.print(",");
  Serial.print(elapsedSec); Serial.print(",");
  Serial.println(statusString);

  delay(100);
}

void updateOLEDDisplay(String activeMode, int lux, int pwm, unsigned long elapsedSec, String status, int currentMenuIdx) {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

 //SCREEN SETTINGS  
  if (activeMode == "WELCOME") {
    // Welcome Greeting Screen
    display.setTextSize(1);
    display.setCursor(28, 4);
    display.print(F("HELLO USER!"));
    
    display.setCursor(8, 18);
    display.print(F("Select your state:"));
    display.drawLine(0, 28, 128, 28, SSD1306_WHITE);

    // Menu options
    display.setCursor(10, 32);
    display.print(currentMenuIdx == 0 ? "> FOCUS" : "  FOCUS");
    
    display.setCursor(10, 42);
    display.print(currentMenuIdx == 1 ? "> SOCIAL" : "  SOCIAL");
    
    display.setCursor(10, 52);
    display.print(currentMenuIdx == 2 ? "> RELAX" : "  RELAX");

  } else {
    // Main Active Mode Screen
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print(F("ACT:")); display.print(activeMode);
    
    if (activeMode == "FOCUS") {
       int mins = elapsedSec / 60;
       int secs = elapsedSec % 60;
       display.print(F(" "));
       if(mins < 10) display.print("0"); display.print(mins); display.print(":");
       if(secs < 10) display.print("0"); display.print(secs);
    }
    display.drawLine(0, 9, 128, 9, SSD1306_WHITE);

    // Menu Selection Items
    display.setCursor(0, 13);
    display.print(currentMenuIdx == 0 ? "> FOCUS" : "  FOCUS");
    
    display.setCursor(0, 23);
    display.print(currentMenuIdx == 1 ? "> SOCIAL" : "  SOCIAL");
    
    display.setCursor(0, 33);
    display.print(currentMenuIdx == 2 ? "> RELAX" : "  RELAX");

    // Telemetry & Status
    display.drawLine(0, 44, 128, 44, SSD1306_WHITE);
    display.setCursor(0, 47);
    display.print(F("Lx:")); display.print(lux);
    display.print(F(" PWM:")); display.print(map(pwm, 0, 255, 0, 100)); display.print(F("%"));
    display.setCursor(0, 56);
    
    if (status == "TAKE_A_BREAK") {
      display.print(F("!! TAKE A BREAK !!"));
    } else {
      display.print(F("St:")); display.print(status);
    }
  }

  display.display();
}
