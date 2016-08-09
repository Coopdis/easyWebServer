#include <Arduino.h>
#include <easyWebServer.h>

extern "C" {
#include "user_interface.h"
#include "espconn.h"
}

#define   WIFI_SSID       "ssid"
#define   WIFI_PASSWORD   "password"

void setup() {
  Serial.begin( 115200 );

  // log on to WiFi
  wifi_set_opmode_current(STATION_MODE);
  struct station_config stationConfig;
  strncpy((char*)stationConfig.ssid, WIFI_SSID, 32);
  strncpy((char*)stationConfig.password, WIFI_PASSWORD, 64);

  wifi_station_set_config(&stationConfig);
  wifi_set_event_handler_cb(eventHandler);

  wifi_station_connect();
}

void loop() {
  // put your main code here, to run repeatedly:

}

void eventHandler(System_Event_t *event) {
  switch (event->event) {
    case EVENT_STAMODE_CONNECTED:
      Serial.printf("Event: EVENT_STAMODE_CONNECTED\n");
      break;
    case EVENT_STAMODE_DISCONNECTED:
      Serial.printf("Event: EVENT_STAMODE_DISCONNECTED\n");
      break;
    case EVENT_STAMODE_AUTHMODE_CHANGE:
      Serial.printf("Event: EVENT_STAMODE_AUTHMODE_CHANGE\n");
      break;
    case EVENT_STAMODE_GOT_IP:
      Serial.printf("Event: EVENT_STAMODE_GOT_IP\n");

      // report IP and initize webServer
      struct ip_info info;
      wifi_get_ip_info(STATION_IF, &info);
      Serial.printf("IP=%u.%u.%u.%u\n", IP2STR(&info.ip) );
      webServerInit();

      break;
    case EVENT_SOFTAPMODE_STACONNECTED:
      Serial.printf("Event: EVENT_SOFTAPMODE_STACONNECTED\n");
      break;
    case EVENT_SOFTAPMODE_STADISCONNECTED:
      Serial.printf("Event: EVENT_SOFTAPMODE_STADISCONNECTED\n");
      break;
    default:
      Serial.printf("Unexpected event: %d\n", event->event);
      break;
  }
}


