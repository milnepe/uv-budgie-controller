/*
  Arduino Nano 33 IoT client for Met Office DataPoint API
  3Hr / 5 day forecast

  A personal API key is required by creating a Met Office
  DataPoint account at https://register.metoffice.gov.uk/

  Create a file named "arduino_secrets.h" and add the following
  macros to store your Wifi SSID and password plus Met Office API key:

  #define SECRET_SSID "YOURSSID"
  #define SECRET_PASS "YOUR_WIFI_PASSWORD"
  #define API_KEY "YOUR_MET_OFFICE_API_KEY"

  Set your local UK weather station by editing the STATION_ID macro:
  #define STATION_ID "354272"  // Witney, UK
  consult the DataPoint documentation:
  https://www.metoffice.gov.uk/services/data/datapoint/getting-started

  Set the interval between looks-ups by adjusting:
  #define FCST_INTERVAL 60 * 60000  // 60 mins

  Set the demo mode interval by adjusting:
  #define DEMO_DELAY 10 * 1000  // 10 sec

  Servo settings will need to be adjusted for your Budgie - they are
  found in "UVBudgie,h"

  Audio clips are in ./audio - connect your sound board to your PC
  and copy the clips to the mounted drive

  Hold down demo button to enter Demo Mode durring reset

  Author: Peter Milne
  Date: 13 July 2022

  Copyright 2022 Peter Milne
  Released under GNU GENERAL PUBLIC LICENSE
  Version 3, 29 June 2007
*/

#include <SPI.h>
#include <WiFiNINA.h>
#include <ArduinoJson.h>
#include <EasyButton.h>
#include "arduino_secrets.h"
#include "UVBudgie.h"
#include "UVBudgieDisplay.h"

const char* soft_version = "1.0.4";

#define FCST_INTERVAL 60 * 60000  // 60 mins
#define DEMO_DELAY 10 * 1000  // 10 sec

#define SERVO 0  // Flapping servo
#define SERVO_FREQ 50 // Analog servos run at ~50 Hz

#define SFX_RST 4 // Sound board RST pin

// 5 day forcast of uv and temperature data
static uvData forecast[5];

WiFiClient client;

// Servo board - default address 0x40
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

// Sound board connected to Serial1 - must be set to 9600 baud
Adafruit_Soundboard sfx = Adafruit_Soundboard(&Serial1, NULL, SFX_RST);

UVBudgie myBudgie = UVBudgie(&sfx, &pwm, forecast);

UVBudgieDisplay epd = UVBudgieDisplay(&myBudgie);

int status = WL_IDLE_STATUS;
boolean updateDisplayFlag = false;
unsigned long lastReconnectAttempt = 0;
boolean playBackFlag = false;

// Define pins
const int wifiLed = 10;  // Optional debug LED (Green - On if connected)
const int rButton = 2;
const int lButton = 3;
const int dButton = 9;

EasyButton rightButton(rButton);  // RH button
EasyButton leftButton(lButton);  // LH button
EasyButton demoButton(dButton);  // External demo button

void setup() {
  pinMode(wifiLed, OUTPUT);
  digitalWrite(wifiLed, LOW);

  // Initialize Serial Port
  Serial.begin(115200);
  //  while (!Serial) {
  //    ; // wait for serial port to connect. Needed for native USB port only
  //  }
  delay(2000);

  // Serial 1 used for sound board at 9600 baud
  Serial1.begin(9600);
  while (!Serial1) {
    ; // wait for serial port to connect
  }
  Serial.println("Serial1 attached");

  // Initialize buttons
  rightButton.begin();
  rightButton.onPressed(playback);  // Short press triggers playback
  rightButton.onPressedFor(2000, audio);  // Long press toggles audio
  leftButton.begin();
  leftButton.onPressed(demo);  // Short press for demo
  demoButton.begin();
  demoButton.onPressed(demo); // Short press external button for demo
  // Press reset button (middle) to exit demo 

  epd.initDisplay();
  epd.showGreeting();

  // Init servo
  pwm.begin();
  pwm.setOscillatorFrequency(27000000);
  pwm.setPWMFreq(SERVO_FREQ);  // Analog servos run at ~50 Hz updates
  delay(1000);

  myBudgie.init(SERVO, WINGS_DOWN);
  myBudgie.doAction(epd.audioOn);

  leftButton.read();
  demoButton.read();
  if (leftButton.isPressed() || demoButton.isPressed()) {  
    epd.demoOn = true;
  }

  Serial.print("Starting Metoffice client version: ");
  Serial.println(soft_version);
  delay(12000);
}

void loop() {
  // Continuously update the button states
  rightButton.read();
  leftButton.read();
  demoButton.read();

  if (!epd.demoOn) {  // Standard mode
    if (WiFi.status() != WL_CONNECTED) {
      // Connect wifi if it's not connected
      digitalWrite(wifiLed, LOW);
      epd.wifiOn = false;
      reconnectWiFi();
      delay(2000);
      if (WiFi.status() == WL_CONNECTED) {
        digitalWrite(wifiLed, HIGH);
        epd.wifiOn = true;
        Serial.println("Wifi connected...");
      }
      doUpdate();  // Initial update
    }
    unsigned long now = millis();
    if ((now - lastReconnectAttempt > FCST_INTERVAL) || (updateDisplayFlag) || (playBackFlag)) {
      updateDisplayFlag = false;
      playBackFlag = false;
      lastReconnectAttempt = now;
      doUpdate();
    }
  } else {  // Demo mode - reset to exit so everything re-initialises
    doDemo();
  }
}

void doUpdate() {
  getData();
  myBudgie.updateState();
  epd.updateDisplay();
  myBudgie.doAction(epd.audioOn);
  printData();
}

void doDemo() {
  // Load demo data
  memcpy(forecast[0].datestr, "2022-06-01Z", DATESTR_LEN - 1);
  forecast[0].uv[0] = 0;
  forecast[0].temp[0] = 20;
  memcpy(forecast[1].datestr, "2022-06-02Z", DATESTR_LEN - 1);
  forecast[1].uv[0] = 2;
  forecast[1].temp[0] = 21;
  memcpy(forecast[2].datestr, "2022-06-03Z", DATESTR_LEN - 1);
  forecast[2].uv[0] = 4;
  forecast[2].temp[0] = 22;
  memcpy(forecast[3].datestr, "2022-06-04Z", DATESTR_LEN - 1);
  forecast[3].uv[0] = 6;
  forecast[3].temp[0] = 23;
  memcpy(forecast[4].datestr, "2022-06-05Z", DATESTR_LEN - 1);
  forecast[4].uv[0] = 8;
  forecast[4].temp[0] = 24;

  // Cycle through all states using uv value as trigger
  for (int i = 0; i < 10; i += 2) {  // Set uv level
    for (int j = 0; j < 8; ++j) {  // Set all fcst periods to uv level
      forecast[0].uv[j] = i;
    }
    myBudgie.updateState();
    epd.updateDisplay();
    myBudgie.doAction(epd.audioOn);
    delay(DEMO_DELAY);  // Delay between state change
  }
}

int reconnectWiFi() {
  // WL_IDLE_STATUS     = 0
  // WL_NO_SSID_AVAIL   = 1
  // WL_SCAN_COMPLETED  = 2
  // WL_CONNECTED       = 3
  // WL_CONNECT_FAILED  = 4
  // WL_CONNECTION_LOST = 5
  // WL_DISCONNECTED    = 6

  WiFi.disconnect();  // Force a disconnect
  delay(1000);
  // Creds from arduino_secrets.h
  WiFi.begin(SECRET_SSID, SECRET_PASS);
  return WiFi.status();
}

void getData() {
  // Connect to host
  Serial.println("Connecting to datapoint.metoffice.gov.uk");
  if (!client.connect("datapoint.metoffice.gov.uk", 80)) {
    Serial.println("Failed to connect to server");
    return;
  }

  // Send HTTP request
  client.println("GET /public/data/val/wxfcs/all/json/" STATION_ID
                 "?res=3hourly&key=" API_KEY " HTTP/1.0");
  client.println("Host: datapoint.metoffice.gov.uk");
  client.println("Connection: close");
  client.println();

  // Check status code
  char status[32] = {0};
  client.readBytesUntil('\r', status, sizeof(status));
  // should be "HTTP/1.0 200 OK"
  if (memcmp(status + 9, "200 OK", 6) != 0) {
    Serial.print("Unexpected HTTP status");
    Serial.println(status);
    client.stop();
    return;
  }

  // Skip response headers
  client.find("\r\n\r\n");

  // Create a filter for just UV data
  StaticJsonDocument<208> filter;

  JsonObject filter_SiteRep = filter.createNestedObject("SiteRep");

  JsonObject filter_SiteRep_DV_Location_Period = filter_SiteRep["DV"]["Location"]["Period"].createNestedObject();
  filter_SiteRep_DV_Location_Period["value"] = true;
  filter_SiteRep_DV_Location_Period["Rep"][0]["U"] = true;
  filter_SiteRep_DV_Location_Period["Rep"][0]["T"] = true;

  // Get the complete document (Dynamic)
  DynamicJsonDocument doc(3072);

  DeserializationError error = deserializeJson(doc, client, DeserializationOption::Filter(filter));

  if (error) {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.c_str());
    return;
  }

  // Close the connection to the server
  client.stop();

  JsonArray periods = doc["SiteRep"]["DV"]["Location"]["Period"];

  for (JsonObject fcst_period : periods) {
    int i, j = 0;
    memcpy(forecast[i].datestr, fcst_period["value"].as<const char *>(), DATESTR_LEN - 1); // "2022-06-15Z"
    for (JsonObject item : fcst_period["Rep"].as<JsonArray>()) {
      forecast[i].uv[j] = item["U"].as<char>();  // UV index
      forecast[i].temp[j] = item["T"].as<char>();  // Temperature C
      ++j;
    }
    ++i;
  }
}

// Button callbacks
void playback() {
  Serial.println("Playback button pressed!");
  playBackFlag = true;
}

void audio() {
  Serial.println("Audio button pressed!");
  epd.audioOn = ! epd.audioOn;
  updateDisplayFlag = true;
}

void demo() {
  Serial.println("Demo button pressed!");
  epd.demoOn = true;
}

void printData() {
  for (int i = 0; i < 5; ++i) {
    Serial.println(myBudgie.dow(i));
    Serial.print("Max U: ");
    Serial.print(myBudgie.getUVMax(i));
    Serial.print(" Max T: ");
    Serial.println(myBudgie.getTempMax(i));
    for (int j = 0; j < 8; ++j) {
      Serial.print("U: ");
      Serial.print((int)forecast[i].uv[j]);
      Serial.print(" T: ");
      Serial.print((int)forecast[i].temp[j]);
      Serial.println(" °C");
    }
  }
}
