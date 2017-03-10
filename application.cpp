#include "application.h"
#include "WifiManager.h"
//#include "softap_http.h"

#pragma SPARK_NO_PREPROCESSOR

SYSTEM_THREAD(ENABLED);
SYSTEM_MODE(MANUAL);

WiFiManager wifiManager;

boolean clearCredentials(String command) {
  return WiFi.clearCredentials();
}

void setup() {

  Serial.begin(57600);
  WiFi.off();

  // clear credentials too show the bug.
  //WiFi.clearCredentials();

  delay(2000);
  //Serial.println("Photon says hi!");



  //clearCredentials("");
  //Serial.print("WiFi.hasCredentials() : ");
  //Serial.println(WiFi.hasCredentials());
  //Serial.println("cleared credentials");

  // otherwise WiFi.scan will crash...
  //delay(1000);

  wifiManager.setup();

}

void loop() {
  wifiManager.loop();

  if(wifiManager.connected()) {

    // we can parse TCP/UDP etc.

  }
}
