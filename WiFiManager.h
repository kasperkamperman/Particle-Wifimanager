#ifndef __WiFiManager_H
#define __WiFiManager_H

#include "application.h"

/*

Class to manage WiFi.
It always falls back to WiFi direct mode (listen).
For example if the network is not available, or if the password is wrong.

It automatically will try to reconnect of the network has been found again.

Power cycle is not implemented, don't know if this is necessary?
// https://github.com/kennethlimcp/particle-examples/blob/master/wifi-auto-reconnect/wifi-auto-reconnect.ino

*/

#define WDEBUG 1

#define WIFI_DIRECT_SSID_ATTEMPT_CALL_TIMEOUT 60000
#define CHECK_SSID_AVAILABLE_TIMEOUT 					15000
#define ATTEMPT_CONNECTING_TIMEOUT   					15000

#define ONE_DAY_MILLIS (24 * 60 * 60 * 1000)

class WiFiManager {

	public:

		boolean failedConnectionAttemptFlag = false;
		boolean weakRSSIFlag 								= false;

		WiFiManager();
		virtual ~WiFiManager();

		void setup();
		void loop();
		bool connected();
		void resetWiFiRetryConnectTimer();

		bool isFirstScan;


	private:

		// global because accessible from wifi_scan_callback and isStoredSSIDAvailable functions.
		// we always set this, because maybe on the road WAPs will be added (without a reset)
		WiFiAccessPoint storedWAPs[5];
		int             storedWAPsAmount = 0;
		bool            isStoredSSIDFound = false;
		int							foundSSID_RSSI;

		enum WiFiManagerState { WIFI_DIRECT, WIFI_CONNECTED, WIFI_CHECK_SSID_AVAILABLE, WIFI_ATTEMPT_CONNECTING, WIFI_OFF };

		enum WiFiManagerState state 				= WIFI_CONNECTED;
		enum WiFiManagerState previousState = WIFI_OFF;

		unsigned long currentStateStartTime;
		unsigned long lastWiFiStateUpdateTime;
		unsigned long lastConnectAttemptTime;
		unsigned long lastRtcUpdateTime;

		void updateWiFiStates();

		static void handle_ap(WiFiAccessPoint* wap, WiFiManager* self);
		void next(WiFiAccessPoint& ap);
		bool isStoredSSIDAvailable();

};

#endif
