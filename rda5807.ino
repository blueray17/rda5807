#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <radio.h>
#include <RDA5807M.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <EEPROM.h>

#define EEPROM_SIZE 512

//oled
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32 // OLED display height, in pixels
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define FIX_BAND RADIO_BAND_FM
#define FIX_VOLUME 3

int vol = 1, mode = 0 , idx, frekuensi = 9370, jmlstat;
uint8_t menu = D5 , up = D6 , down = D7;
int daftarstat[512][2];

//long press tombol mode
const int SHORT_PRESS_TIME = 1000; // 1000 milliseconds
const int LONG_PRESS_TIME  = 1000; // 1000 milliseconds
int lastState = LOW;  // the previous state from the input pin
int currentState;     // the current reading from the input pin
unsigned long pressedTime  = 0;
unsigned long releasedTime = 0;
bool isPressing = false;
bool isLongDetected = false;

RDA5807M radio;  // Create an instance of Class for RDA5807M Chip

void printDisplay() {
  display.display();
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);             // Normal 1:1 pixel scale
  display.setCursor(0, 0);            // Start at top-left corner
  float fr = float(frekuensi) / 100.00;
  if (mode == 0) {
    display.print(" Mode : "); display.println("Memory");
  }
  else if (mode == 1) {
    display.print(" Mode : "); display.println("Manual/0.1MHz");
  }
  else if (mode == 2) {
    display.print(" Mode : "); display.println("Manual/1 MHz");
  }
  display.setTextSize(2);
  if (frekuensi < 10000)display.print(" ");
  display.print(fr); display.println(" MHz");
  display.setTextSize(1);

  if (mode == 0) {
    display.print(" Channel : "); display.print(idx + 1); display.print("/"); display.println(jmlstat);

  } else {
    display.print(" Channel : "); display.print(jmlstat); display.println(" saved");

  }
  display.display();
  delay(100);
}

void printDisplayMessage(String message) {
  display.display();
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(3);             // Normal 1:1 pixel scale
  display.setCursor(0, 0);
  display.print(message);
  display.display();
  delay(2000);
  printDisplay();
}

int cekMemory() {
  int jml = 0;
  for (int a = 0; a < 512; a++) {
    if (EEPROM.read(a) > 0) {
      daftarstat[jml][0] = a;
      daftarstat[jml][1] = int(EEPROM.read(a));
      jml++;
    }
  }
  return jml;
}

int findMemoryEmpty() {
  int y = 0;
  for (int a = 0; a <= 512; a++) {
    if (EEPROM.read(a) == 0) {
      y = a;
      break;
    }
  }
  return y;
}

void setup() {
  Serial.begin(9600);
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;); // Don't proceed, loop forever
  }
  delay(200);
  EEPROM.begin(EEPROM_SIZE);
  jmlstat = cekMemory();
  if (jmlstat == 0) {
    mode = 1;
    frekuensi = 9370;
  } else {
    mode = 0;
    idx = 0;
    frekuensi = daftarstat[idx][1] * 10 + 8500;
  }
  pinMode(menu, INPUT_PULLUP);//menu
  pinMode(up, INPUT);//up
  pinMode(down, INPUT);//down
  radio.debugEnable(true);
  radio._wireDebug(false);

  radio.setup(RADIO_FMSPACING, RADIO_FMSPACING_100);   // for EUROPE
  radio.setup(RADIO_DEEMPHASIS, RADIO_DEEMPHASIS_50);  // for EUROPE

  if (!radio.initWire(Wire)) {
    Serial.println("no radio chip found.");
    delay(200);
    ESP.restart();
  };
  printDisplay();

  radio.setBandFrequency(FIX_BAND, frekuensi);
  radio.setVolume(FIX_VOLUME);
  radio.setMono(true);
  radio.setMute(false);

  // clear memory
  for (int a = 0; a < 512; a++) {
    if (EEPROM.read(a) > 0) {
      Serial.print("eprom "); Serial.print(a); Serial.print(" = ");
      Serial.println(EEPROM.read(a));
      //       EEPROM.write(a, 0);
    }
  }
  // EEPROM.end();

}

void loop() {
  if (digitalRead(up) == HIGH) {  ///tombol up ditekan
    Serial.println("tombol up ");
    delay(200);
    if (mode == 0) {
      idx = idx + 1;
      if (idx == jmlstat)idx = 0;
      radio.setBandFrequency(FIX_BAND, daftarstat[idx][1] * 10 + 8500);
      frekuensi = daftarstat[idx][1] * 10 + 8500;
    } else if (mode == 1) {
      frekuensi = frekuensi + 10;
      if (frekuensi > 11000)frekuensi = 7000;
      radio.setBandFrequency(FIX_BAND, frekuensi);
    } else if (mode == 2) {
      frekuensi = frekuensi + 100;
      if (frekuensi > 11000)frekuensi = 7000;
      radio.setBandFrequency(FIX_BAND, frekuensi);
    }

    printDisplay();

  }

  if (digitalRead(down) == HIGH) {
    Serial.println("tombol down ");
    delay(200);
    if (mode == 0) {
      idx = idx - 1;
      if (idx < 0)idx = jmlstat - 1;
      radio.setBandFrequency(FIX_BAND, daftarstat[idx][1] * 10 + 8500);
      frekuensi = daftarstat[idx][1] * 10 + 8500;
    } else if (mode == 1) {
      frekuensi = frekuensi - 10;
      if (frekuensi < 7000)frekuensi = 11000;
      radio.setBandFrequency(FIX_BAND, frekuensi);
    } else if (mode == 2) {
      frekuensi = frekuensi - 100;
      if (frekuensi < 7000)frekuensi = 11000;
      radio.setBandFrequency(FIX_BAND, frekuensi);
    }
    printDisplay();

  }

  currentState = digitalRead(menu);
  if (lastState == LOW && currentState == HIGH) {       // button is pressed
    pressedTime = millis();
    isPressing = true;
    isLongDetected = false;
  } else if (lastState == HIGH && currentState == LOW) { // button is released
    isPressing = false;
    releasedTime = millis();
    long pressDuration = releasedTime - pressedTime;
    if ( pressDuration < SHORT_PRESS_TIME ) {
      Serial.println("A short press is detected");
      mode = mode + 1;
      if (mode == 3) {
        if (jmlstat > 0) {
          mode = 0;
          radio.setBandFrequency(FIX_BAND, daftarstat[idx][1] * 10 + 8500);
          frekuensi = daftarstat[idx][1] * 10 + 8500;
        } else {
          mode = 1;
        }
      }
      printDisplay();
      delay(200);
    }
  }
  if (isPressing == true && isLongDetected == false) {
    long pressDuration = millis() - pressedTime;
    if ( pressDuration > LONG_PRESS_TIME ) {
      Serial.println("A long press is detected");
      isLongDetected = true;
      if (mode == 0) {
        //Serial.println("deleted");
        EEPROM.write(daftarstat[idx][0], 0);
        EEPROM.commit();
        jmlstat = cekMemory();
        idx = idx-1;
        if (jmlstat == 0) {
          mode = 1;
          frekuensi = 9370;
          radio.setBandFrequency(FIX_BAND, frekuensi);
        } else {
          if (idx <0 ) {
            idx = 0;
            radio.setBandFrequency(FIX_BAND, daftarstat[idx][1] * 10 + 8500);
            frekuensi = daftarstat[idx][1] * 10 + 8500;
          } else {
            radio.setBandFrequency(FIX_BAND, daftarstat[idx][1] * 10 + 8500);
            frekuensi = daftarstat[idx][1] * 10 + 8500;
          }
        }
        printDisplayMessage("DELETED");

      } else if (mode >= 1) {
        if (frekuensi >= 10800 || frekuensi <= 8500) {
          printDisplayMessage(" ERROR");
        } else {
          int ind = findMemoryEmpty();
          Serial.print("indeks kosong = "); Serial.println(ind);
          if (ind < 512) {
            int savfrek = (frekuensi - 8500) / 10;
            EEPROM.write(ind, savfrek);
            Serial.print("frekuensi tersimpan = "); Serial.println(savfrek);
            EEPROM.commit();
            jmlstat = cekMemory();
            printDisplayMessage(" SAVED");
          } else {
            printDisplayMessage("NOT SAVED");
          }
        }

        Serial.print("save"); Serial.println(frekuensi);


      }
      delay(200);
    }
  }  // save the the last state
  lastState = currentState;
}
