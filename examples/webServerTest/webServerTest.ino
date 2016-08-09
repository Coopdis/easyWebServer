#include <Arduino.h>
#include <ArduinoJson.h>
#include <easyMesh.h>

easyMesh  mesh;  // mesh global

void setup() {
  Serial.begin( 115200 );
  mesh.init();
}

void loop() {
  // put your main code here, to run repeatedly:

}
