#include "WiFiManager.h"

WiFiManager::WiFiManager() {
}

WiFiManager::~WiFiManager() {
}

void WiFiManager::setup() {

	#ifdef WDEBUG
		Serial.println("The WiFiManager says hi !!");
	#endif

	// we need to call WiFi.listen() once... otherwise
	// we get an SOS blink when trying WiFi.scan
	//WiFi.listen();
	isFirstScan = true;
	lastWiFiStateUpdateTime = 0;

	// We directly start to check if an SSID is available
	state = WIFI_CHECK_SSID_AVAILABLE;

	//updateWiFiStates();

}

void WiFiManager::loop() {

	if ( ( millis() - lastWiFiStateUpdateTime >= 1000 ) ||
       ( previousState != state )
		 ) {

			updateWiFiStates();
			lastWiFiStateUpdateTime = millis();
	}
}

bool WiFiManager::connected() {
	if(state == WIFI_CONNECTED) return true;
	else 												return false;
}



void WiFiManager::resetWiFiRetryConnectTimer() {

	// when we are active on the softap we don't want to move out of the WiFi Direct state
	// this make the time a bit longer indeed
	// we call this when someone asks for a page.
	// we need to improve this function later on, for example to check if
	// someone is really active.

	if(state == WIFI_DIRECT) currentStateStartTime = millis();

}

void WiFiManager::updateWiFiStates() {

	// we don't control the states completely by our selves.
	// however we use build in functions to check what's going on.

	// later (when softap is fully supported) more states can be introduced
	// for example if a client is actually actively doing something.

	Serial.print("Track WiFi.listening() status : ");
	Serial.println(WiFi.listening());

	switch(state) {

		case WIFI_DIRECT:

			if(WiFi.ready()) state = WIFI_CONNECTED;
			else {

				// check if we are really in WiFi Direct mode
				// if not we enter it.
				if(!WiFi.listening())	{
					WiFi.listen();

					#ifdef WDEBUG
						Serial.print("WiFi.listening() status after WiFi.listen() call: ");
						Serial.println(WiFi.listening());
					#endif
				}

				#ifdef WDEBUG
					Serial.print("time in WiFi direct state: ");
					Serial.println(millis() - currentStateStartTime);
				#endif

				// we check if the stored SSID is avaiable every n seconds
				if(millis() - currentStateStartTime >= WIFI_DIRECT_SSID_ATTEMPT_CALL_TIMEOUT) {

					state = WIFI_CHECK_SSID_AVAILABLE;

					weakRSSIFlag = false; // reset the flag here for a new attempt

				}

			}

		break;

		case WIFI_CONNECTED:

			if(WiFi.listening()) state = WIFI_DIRECT;
			else {
				if(!WiFi.ready()) {
					// we probably lost connection
					// we might be able to go directly to WIFI_ATTEMPT_CONNECTING
					// otherwise we have never ended up here. However this is more secure.
					// because SSID Available sends us to wifi direct mode.
					state = WIFI_CHECK_SSID_AVAILABLE;
				}
			}

			// manually connect to the cloud if we didn't do it yet
			if(!Particle.connected()) {
				Particle.connect();
			}
			else {
				Particle.process();

				// time sync

				if (millis() - lastRtcUpdateTime >= ONE_DAY_MILLIS) {
    			Particle.syncTime();
    			lastRtcUpdateTime = millis();
  			}
			}

			#ifdef WDEBUG
				//Serial.print("Current RSSI: ");
				//Serial.println(WiFi.RSSI());
			#endif

		break;

		case WIFI_CHECK_SSID_AVAILABLE:

			// We have to stop the listen mode when doing a connection attempt.
			// We only do this when we know that our stored SSID is available and that
			// The signal strength is sufficient enough to be stable.

			// Time out is to prevent that we get stuck in case the RSSI is weak.

			if(WiFi.hasCredentials()) {

				// always check if there is an update in credentials
				storedWAPsAmount = WiFi.getCredentials(storedWAPs, 5);

				if(isStoredSSIDAvailable()) {

					// >-50 dBm (excellent), -50 to -60 dBm (good), 60 to -75 dBm (fair), < -75 dBm (weak)

					if(foundSSID_RSSI > -75)
					{
						#ifdef WDEBUG
							Serial.println("SSID Found with signal strength higher then -75 dBm");
						#endif

						// exit listen mode if it was active
						if(WiFi.listening()) {
							WiFi.listen(false);

							// we reset it here, this seems the most logical
							failedConnectionAttemptFlag = false;
							weakRSSIFlag = false;
						}

						state = WIFI_ATTEMPT_CONNECTING;

					}
					else {

						// Weak RSSI
						weakRSSIFlag = true;

						// move to WIFI_DIRECT
						if(millis() - currentStateStartTime >= CHECK_SSID_AVAILABLE_TIMEOUT) {
							state = WIFI_DIRECT;
						}
					}
				}
				else {
					// credentials stored in Photon
					// however we can't find them with a scan
					if(millis() - currentStateStartTime >= CHECK_SSID_AVAILABLE_TIMEOUT) {
						state = WIFI_DIRECT;
					}

				}
			}
			else {
				// no credentials so we can go directly (back) to WiFI Direct
				state = WIFI_DIRECT;
			}

		break;


		case WIFI_ATTEMPT_CONNECTING:

			// in this state we already have found to SSID that we try to
			// connect to. So if this fails why probably have a wrong password entered
			// or the connection is not stable enough...

			if(millis() - currentStateStartTime <= ATTEMPT_CONNECTING_TIMEOUT) {

				if(WiFi.ready()) {
					state = WIFI_CONNECTED;
				}
				else {

					//WiFi.connecting will return false once the device has successfully
					//connected to the Wi-Fi network.
					if( !WiFi.connecting() ) {

						// we manage listen ourself
						WiFi.connect(WIFI_CONNECT_SKIP_LISTEN);
						//WiFi.connect();

						#ifdef WDEBUG
							Serial.println("Attempting to connect.");
						#endif
					}
				}

			}
			else {

				// we are attempting for more than n seconds now.
				// time to move on...
				state = WIFI_DIRECT;

				// set the failedConnectAttemptFlag
				// we don't use it to block this state. However we can give feedback
				// to the user that they probably have entered a wrong password.
				failedConnectionAttemptFlag = true;

				#ifdef WDEBUG
					Serial.println("Failed attempt!");
				#endif
			}

		break;

	};

	// we'd like to know how long we are in a certain state.
	if(previousState != state) {
		currentStateStartTime = millis();

		#ifdef WDEBUG
			Serial.print("Current state: ");
			Serial.print(state);
			Serial.print(" ");

			if(state == WIFI_DIRECT)								Serial.print(" WIFI_DIRECT");
			if(state == WIFI_CONNECTED)							Serial.print(" WIFI_CONNECTED");
		  if(state == WIFI_CHECK_SSID_AVAILABLE)	Serial.print(" WIFI_CHECK_SSID_AVAILABLE");
			if(state == WIFI_ATTEMPT_CONNECTING)		Serial.print(" WIFI_ATTEMPT_CONNECTING");
			if(state == WIFI_OFF)										Serial.print(" WIFI_OFF");

			Serial.println();

		#endif

		previousState = state;
	}

}

// This is the callback passed to WiFi.scan()
// It makes the call on the `self` instance - to go from a static
// member function to an instance member function.
void WiFiManager::handle_ap(WiFiAccessPoint* wap, WiFiManager* self)
{
	 self->next(*wap);
}

void WiFiManager::next(WiFiAccessPoint& ap)
{
	#ifdef WDEBUG
		Serial.print("SSID: ");
		Serial.println(ap.ssid);
	#endif

	for (int i = 0; i < storedWAPsAmount; i++) {
	  if (strcmp(ap.ssid, storedWAPs[i].ssid) == 0) {
	    isStoredSSIDFound = true;
			foundSSID_RSSI = ap.rssi;

			#ifdef WDEBUG
	    Serial.print("Found Stored SSID: ");
	    Serial.print(ap.ssid);
	    Serial.print(" RSSI: ");
	    Serial.println(ap.rssi);
			#endif

			// we move out of the loop when we found
			// the ssid
			break;
	  }
	}

}

bool WiFiManager::isStoredSSIDAvailable() {

  // in the callback we compare scanned WAPs with the stored WAPs
  // storedSSIDFound will be set to true if we have a match
  // this is global so we can access this after a call to this function
  isStoredSSIDFound = false;

	#ifdef WDEBUG
		Serial.println("Call WiFi.scan");
	#endif

	// safety measures againt firmware crash.
	// this only works when in a certain
	if(isFirstScan) {
		WiFi.on();
		WiFi.listen();
		waitUntil(WiFi.listening);
		isFirstScan = false;
	}

	WiFi.scan(handle_ap, this);

  return isStoredSSIDFound;
}
