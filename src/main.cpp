#include <App.hpp>
#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include "esp_adc_cal.h"            // Battery sensor
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

#define SDA_pin 19
#define SCL_pin 18
#define OLED_RESET     -1
#define SCREEN_ADDRESS 0x3C //0x3D ///
Adafruit_SSD1306 display;
int CursorX;                      // Cursor of display
int CursorY;                      // Cursor of display
int displayPtr = 1;               // pointer for display items
bool played = false;              // helper for display
int displaySeconds = 0;           // Display seconds
int progress = 0;                 // progress bar


void MqttCallback(char *topic, byte *payload, unsigned int length);

WiFiClient wifiClient;
PubSubClient client(MQTT_HOST,MQTT_PORT,MqttCallback,wifiClient);
time_t startTime = millis();

// Battery & Sond
#define BAT_ADC   2
#define SOND_ADC  4
#define SOND_VSS 10

float Voltage_bat = 0.0;
float Voltage_sond = 0.0;
uint32_t readADC_Cal(int ADC_Raw);
uint32_t sleepTime = TIME_TO_SLEEP;
uint32_t MqttInerval = 30;              // default MQTT interval in normal mode
uint32_t seconds = 0;         
uint32_t ZistMax = 6300;                // default max litre
uint32_t ZistUpper = 0; 
uint32_t ZistLower = 1000; 
uint32_t ZistPercent = 0;
uint32_t ZistLitre = 0;

bool gotRecall = false;
bool connectMQTT = false;
bool batteryMode = false;               // battery mode (true = deepSleepMode)

// Task definitions
struct task
{    
    unsigned long rate;
    unsigned long previous;
};

task taskA = { .rate = 1000, .previous = 0 };     // 1 second

void initScreen(){
  Wire.setPins(SDA_pin,SCL_pin);
  display = Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
  
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    //for(;;); // Don't proceed, loop forever
  }

  display.clearDisplay();
  display.display();
  delay(500);

  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE); // Draw white text
  //display.setTextColor(SSD1306_BLACK); // Draw black text
  display.cp437(true);         // Use full 256 char 'Code Page 437' font

  display.setCursor(10, 1);
  display.printf("Zisterne");
  display.setCursor(10, 28);
  display.printf("Ver. 0.1");

  //display.fillRect(0, 0, display.width()-1, display.height()-1, SSD1306_INVERSE);
  display.display();
}

void MqttCallback(char *topic, byte *payload, unsigned int length) {
char payloadvalue[10];
 
  if (strcmp(topic, "ESP32-PressureSond/cmnd/SleepTimeSeconds") == 0 ) {
    Serial.print("MQTT arrived [");
    Serial.print(topic);
    Serial.print("] ");
    for (int i=0;i<length;i++) {
      Serial.print((char)payload[i]);
      payloadvalue[i] = payload[i];
      payloadvalue[i+1] = '\0'; 
    }
    Serial.println(F(""));
    sleepTime = atoi(payloadvalue);
    if (sleepTime <= 1 ) {
      sleepTime = 120;
    }
  }

  if (strcmp(topic, "ESP32-PressureSond/cmnd/MqttIntervalSeconds") == 0 ) {
    Serial.print("MQTT arrived [");
    Serial.print(topic);
    Serial.print("] ");
    for (int i=0;i<length;i++) {
      Serial.print((char)payload[i]);
      payloadvalue[i] = payload[i];
      payloadvalue[i+1] = '\0'; 
    }
    Serial.println(F(""));
    MqttInerval = atoi(payloadvalue);
    if ((MqttInerval <= 1 ) || (MqttInerval == '\0' ))  {
      MqttInerval = 30;
      Serial.print(F("Set default MqttInerval to: "));
      Serial.println(MqttInerval);
    }
  }

  if (strcmp(topic, "ESP32-PressureSond/cmnd/BatteryMode") == 0 ) {
    Serial.print("MQTT arrived [");
    Serial.print(topic);
    Serial.print("] ");
    for (int i=0;i<length;i++) {
      Serial.print((char)payload[i]);
      payloadvalue[i] = payload[i];
      payloadvalue[i+1] = '\0'; 
    }
    batteryMode = atoi(payloadvalue);
    Serial.println(F(""));
    if (payloadvalue == "0" )  {
      batteryMode = false;
    }
    if (payloadvalue == "1")  {
      batteryMode = true;
    }
  }

  if (strcmp(topic, "ESP32-PressureSond/cmnd/ZistMaxLiter") == 0 ) {
    Serial.print("MQTT arrived [");
    Serial.print(topic);
    Serial.print("] ");
    for (int i=0;i<length;i++) {
      Serial.print((char)payload[i]);
      payloadvalue[i] = payload[i];
      payloadvalue[i+1] = '\0'; 
    }
    Serial.println(F(""));
    ZistMax = atoi(payloadvalue);
    if ((ZistMax <= 1 ) || (ZistMax == '\0' ))  {
      ZistMax = 6300;
      Serial.print(F("Set default ZistMaxLiter to: "));
      Serial.println(ZistMax);
    }
  }

  if (strcmp(topic, "ESP32-PressureSond/cmnd/ZistUpper") == 0 ) {
    Serial.print("MQTT arrived [");
    Serial.print(topic);
    Serial.print("] ");
    for (int i=0;i<length;i++) {
      Serial.print((char)payload[i]);
      payloadvalue[i] = payload[i];
      payloadvalue[i+1] = '\0'; 
    }
    Serial.println(F(""));
    uint32_t upper = atoi(payloadvalue);
    if (upper == '\0')  {
    } else {
      ZistUpper = upper;
      Serial.print(F("Set default ZistUpper to: "));
      Serial.println(ZistUpper);
    }
  }

  if (strcmp(topic, "ESP32-PressureSond/cmnd/ZistLower") == 0 ) {
    Serial.print("MQTT arrived [");
    Serial.print(topic);
    Serial.print("] ");
    for (int i=0;i<length;i++) {
      Serial.print((char)payload[i]);
      payloadvalue[i] = payload[i];
      payloadvalue[i+1] = '\0'; 
    }
    Serial.println(F(""));
    uint32_t lower = atoi(payloadvalue);
    if (lower == '\0')  {
    } else {
      ZistLower = lower;
      Serial.print(F("Set default ZistLower to: "));
      Serial.println(ZistLower);
    } 
  }

  gotRecall = true;
}

void MqttSubscribe(void) {
    client.setCallback(MqttCallback);
    client.subscribe("ESP32-PressureSond/cmnd/BatteryMode");
    client.subscribe("ESP32-PressureSond/cmnd/SleepTimeSeconds");
    client.subscribe("ESP32-PressureSond/cmnd/MqttIntervalSeconds");
    client.subscribe("ESP32-PressureSond/cmnd/ZistMaxLiter");
    client.subscribe("ESP32-PressureSond/cmnd/ZistUpper");
    client.subscribe("ESP32-PressureSond/cmnd/ZistLower");
}

void printIP(void) {
    // clear display
    display.clearDisplay();
    display.setCursor(0,0);
    display.setTextSize(2);
    display.print("IP");
    display.setCursor(0, 19);
    display.setTextSize(1);
    IPAddress myIP = WiFi.localIP();
    display.print(myIP); 
    display.display();
}

void printSensor(void) {
    progress = Voltage_sond;
    
    display.clearDisplay();
    display.setCursor(0,0);

    display.setTextSize(2);
    display.print("Zisterne");

    display.fillRect(2, 20, ZistPercent, 10, SSD1306_WHITE);
    display.drawLine(0,18,0,31, SSD1306_WHITE);
    display.drawLine(0,31,104,31, SSD1306_WHITE);
    display.drawLine(104,31,104,18, SSD1306_WHITE);
    display.drawLine(104,18,0,18, SSD1306_WHITE);
    display.setTextSize(1);               // Normal 1:1 pixel scale
    display.setTextColor(SSD1306_WHITE);  // Draw white text
    display.setCursor(110,21);
    display.println("%");

    display.setCursor(0, 42);
    display.setTextSize(3);
    display.print(ZistLitre);
    display.setTextSize(1);
    display.print(" Liter"); 
    display.display();
}

void DisplayValues (void) { 
  played = false;
  displaySeconds = 0;
  
    if (displayPtr == 1) {
          printIP();
          played = true;
        } else if (displayPtr == 2) {
           printSensor();
           played = true;
        } else {
          displayPtr++;
        }             
        
    if (displayPtr >=3) {
      displayPtr = 1;
    }

    if (played) {
      displayPtr++;
    }

  }

void MqttPublish(void) {
    char buffer[64];
    char Buf[50];
    // Publish Battery voltage
    dtostrf(Voltage_bat / 1000.0, 4, 2, buffer);  //float to char
    String  MQTT_OUTTOPIC = "ESP32-PressureSond/";
            MQTT_OUTTOPIC += "Battery";
            MQTT_OUTTOPIC.toCharArray(Buf, 50);
      client.publish(Buf, buffer, true);             

    #ifdef DEBUG_MSG
    Serial.printf("%s : Battery.\n", buffer);
    #endif 

    // Publish Sonde Litre
    dtostrf(ZistLitre, 5, 0, buffer);  //float to char
            MQTT_OUTTOPIC = "ESP32-PressureSond/";
            MQTT_OUTTOPIC += "Liter";
            MQTT_OUTTOPIC.toCharArray(Buf, 50);
      client.publish(Buf, buffer, true);             

    #ifdef DEBUG_MSG
    Serial.printf("%s : Litre.\n", buffer);
    #endif 

    // Publish Sonde Percent
    dtostrf(ZistPercent, 3, 0, buffer);  //float to char
            MQTT_OUTTOPIC = "ESP32-PressureSond/";
            MQTT_OUTTOPIC += "Percent";
            MQTT_OUTTOPIC.toCharArray(Buf, 50);
      client.publish(Buf, buffer, true); 

    #ifdef DEBUG_MSG
    Serial.printf("%s : Percent.\n", buffer);
    #endif      

    // Publish Sonde ZistUpperValue
    dtostrf(ZistUpper, 5, 0, buffer);  //float to char
            MQTT_OUTTOPIC = "ESP32-PressureSond/";
            MQTT_OUTTOPIC += "ZistUpperValue";
            MQTT_OUTTOPIC.toCharArray(Buf, 50);
      client.publish(Buf, buffer, true); 

    #ifdef DEBUG_MSG
    Serial.printf("%s : ZistUpperValue.\n", buffer);
    #endif             

    // Publish Sonde ZistLowerValue
    dtostrf(ZistLower, 5, 0, buffer);  //float to char
            MQTT_OUTTOPIC = "ESP32-PressureSond/";
            MQTT_OUTTOPIC += "ZistLowerValue";
            MQTT_OUTTOPIC.toCharArray(Buf, 50);
      client.publish(Buf, buffer, true); 

    #ifdef DEBUG_MSG
    Serial.printf("%s : ZistLowerValue.\n", buffer);
    #endif      
}

void sendValues (void) {
    client.disconnect();
    Serial.println(F("\n*** Sleep now. Chrrrr ***\n"));
    ESP.deepSleep(sleepTime * uS_TO_S_FACTOR);
}

void setup() {
  Serial.begin(115200);
  initScreen();

  pinMode(3, OUTPUT);         // bild in LED
  pinMode(SOND_VSS, OUTPUT);        // define PORT as output VCC sensor

  // Set Sensor & LED ON
  digitalWrite(3, LOW);    // enable Status LED

// Read battery value
Voltage_bat = (readADC_Cal(analogRead(BAT_ADC))) * 2;
Serial.print(F("Battery "));
Serial.printf("%.2fV", Voltage_bat / 1000.0); // Print Voltage (in V)
Serial.println(F(""));
Serial.print(F("Battery RAW: "));
Serial.print(Voltage_bat); // Print raw Voltage 
Serial.println(F(""));



if (Voltage_bat >= 3600) {        // no Wifi if battery is bad
//if (Voltage_bat >= 4550) {

#ifdef DEBUG_MSG
  Serial.println("WiFi config.");
#endif

  WiFi.mode(WIFI_OFF);
  WiFi.disconnect(true);

  WiFi.config(
    IPAddress(IP_HOST), 
    IPAddress(IP_GATEWAY), 
    IPAddress(IP_NETMASK),
    IPAddress(IP_DNS1)
  );

#ifdef DEBUG_MSG
  Serial.println("WiFi begin.");
#endif

  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED)
  {
#ifdef DEBUG_MSG
    Serial.print('.');
#endif
    delay(500);
    if ((millis() - startTime) > CONNECTION_TIMEOUT)
    {
      Serial.println("\n\n*** WiFi connection timeout ***\n\n");
      ESP.restart();
    }
  }

#ifdef DEBUG_MSG
  Serial.println("\nWiFi connected.");
#endif

  
  client.setServer(MQTT_HOST, MQTT_PORT);
#ifdef DEBUG_MSG
  Serial.println("Connecting to MQTT server.");
#endif
  } else {
    Serial.print(F("!!! Battery has to load before starting Wifi !!!"));
    ESP.deepSleep(sleepTime * uS_TO_S_FACTOR);
  }
}

void loop()
{
  if (gotRecall == true) {
    if (batteryMode == 1) {
    sendValues();
    }
  }
  
  if (connectMQTT == false) {
  // Read sond value
      digitalWrite(SOND_VSS, HIGH);     // enable VCC to sensor
      delay(500);                 // give sond some time to boot
      Voltage_sond = (readADC_Cal(analogRead(SOND_ADC))) * 2;
      Serial.print(F("Sond "));
      Serial.printf("%.2fV", Voltage_sond / 1000.0); // Print Voltage (in V)
      Serial.println(F(""));

      Serial.print(F("Sond RAW "));
      Serial.print(Voltage_sond); // Print Voltage (RAW)
      Serial.println(F(""));

      // Calculating percent, litre, maxValue, minValue (ZistUpper, ZistLower ZistPercent, ZistLitre)
        if (ZistUpper < Voltage_sond) {
            ZistUpper = Voltage_sond;
            Serial.print(F("New upper value = "));
            Serial.println(ZistUpper);
        }
        if (ZistLower > Voltage_sond) {
            ZistLower = Voltage_sond;
            Serial.print(F("New lower value = "));
            Serial.println(ZistLower);

        }
        
        ZistLitre = ( (ZistMax / (ZistUpper - ZistLower)) * (Voltage_sond - ZistLower) );
          Serial.print(F("Calculated litre = "));
          Serial.println(ZistLitre);

        ZistPercent = ( (100.0 / ZistMax) * ZistLitre);
          Serial.print(F("Calculated % = "));
          Serial.println(ZistPercent);

    client.setSocketTimeout(2);
    #ifdef MQTT_USEAUTH
      client.connect(MQTT_CLIENTID, MQTT_USER, MQTT_PASSWORD);
    #else
      client.connect(MQTT_CLIENTID);
    #endif

  if (client.connected())
    {
    #ifdef DEBUG_MSG
      Serial.println(F("MQTT server connected."));
      Serial.println(F("Values:"));
    #endif   
      // Subscribe and publish MQTT
      MqttSubscribe();
      MqttPublish();
      digitalWrite(SOND_VSS, LOW);     // disable VCC to sensor
      digitalWrite(3, LOW);      // Status LED OFF
    }
    else
    {
      if ((millis() - startTime) > CONNECTION_TIMEOUT)
      {
        Serial.println("\n\n*** MQTT connection timeout ***\n\n");
        ESP.restart();
      }
    }
    connectMQTT = true;
  }

  //tasks
    if (taskA.previous == 0 || (millis() - taskA.previous > taskA.rate)) {
        taskA.previous = millis();

        seconds++;
        displaySeconds++;

        if (seconds >= MqttInerval) {
          seconds = 0;
          connectMQTT = false;
        }

        // To roll display to show more informations, uncomment this
        // if (displaySeconds >= 3) {
        //    DisplayValues();
        //}
        // comment this
        printSensor();

    }

  client.loop();
}

uint32_t readADC_Cal(int ADC_Raw)
{
    esp_adc_cal_characteristics_t adc_chars;

    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 1100, &adc_chars);
    return (esp_adc_cal_raw_to_voltage(ADC_Raw, &adc_chars));
}
