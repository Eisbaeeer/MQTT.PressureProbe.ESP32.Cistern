#ifndef __APP_HPP__
#define __APP_HPP__

#undef DEBUG_MSG
#define DEBUG_MSG

#define WIFI_SSID "TuxNet"
#define WIFI_PASS "sueddeutscheq"

// your static ip configuration
#define IP_HOST    { 192, 168,   1, 211 }
#define IP_GATEWAY { 192, 168,   1,  1 }
#define IP_NETMASK { 255, 255, 255,  0 }
#define IP_DNS1    { 192, 168,   1,  1 }

#define MQTT_CLIENTID "esp32-mqtt-deepsleep-sonde"
#define MQTT_HOST "192.168.1.200"
#define MQTT_PORT 1883


#undef MQTT_USEAUTH
#define MQTT_USER "<your mqtt user>"
#define MQTT_PASSWORD "<your mqtt password>"

#define CONNECTION_TIMEOUT 5000

#define uS_TO_S_FACTOR 1000000
#define TIME_TO_SLEEP 120       // seconds

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

#define SDA_pin 19
#define SCL_pin 18
#define OLED_RESET     -1
#define SCREEN_ADDRESS 0x3C //0x3D ///

// Battery & Probe
#define BAT_ADC   2
#define SOND_ADC  4
#define SOND_VSS 10

#endif
