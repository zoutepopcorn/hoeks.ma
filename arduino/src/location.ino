/*******************************************************************************
 * Copyright (c) 2015 Matthijs Kooijman and Thomas Telkamp
 *
 * HLC (Hoeks.ma Location) .ino file edited by Johan Hoeksma
 * You will need a esp8266
 *
 * Permission is hereby granted, free of charge, to anyone
 * obtaining a copy of this document and accompanying files,
 * to do whatever they want with them without any restriction,
 * including, but not limited to, copying, modification and redistribution.
 * NO WARRANTY OF ANY KIND IS PROVIDED.
 *
 * This example sends a valid LoRaWAN packet with payload "Hello, world!", that
 * will be processed by The Things Network server.
 *
 * Change DEVADDR to a unique address!
 *
 * Do not forget to define the radio type correctly in config.h, default is:
 *   #define CFG_sx1272_radio 1
 * for SX1272 and RFM92, but change to:
 *   #define CFG_sx1276_radio 1
 * for SX1276 and RFM95.
 *
 *
 *******************************************************************************/
#include "ESP8266WiFi.h"
#include <lmic.h>
#include <hal/hal.h>
#include <SPI.h>
#include <Arduino.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>

#define USE_SERIAL Serial

// use ttn dashboard to get this keys
static const PROGMEM u1_t NWKSKEY[16] = { 0x47, 0xAE, 0x21, 0xA9, 0xDF, 0xDA, 0x5C, 0x27, 0xB1, 0xEF, 0x99, 0xAB, 0x53, 0x81, 0x4A, 0xE5 };
static const u1_t PROGMEM APPSKEY[16] = { 0x22, 0x9D, 0x2A, 0x58, 0x1D, 0xA7, 0xC7, 0x32, 0x4E, 0xA9, 0x4C, 0x9D, 0xE8, 0x77, 0xF1, 0x34 };
static const u4_t DEVADDR = 0x26011B0A; // <-- Change this address for every node!

// WiFi vars
static const int NR_MACS = 3;
static uint8_t mydata[] = "test";
uint8_t macs[NR_MACS * 7];    // Array to store the 4 mac for  // (nrMacs * 7) + nrMacs
char buffer[64];

void os_getArtEui (u1_t* buf) { }
void os_getDevEui (u1_t* buf) { }
void os_getDevKey (u1_t* buf) { }

ESP8266WiFiMulti WiFiMulti;

static osjob_t sendjob;

// Schedule TX every this many seconds (might become longer due to duty
// cycle limitations).
const unsigned TX_INTERVAL = 60;

// Pin mapping ESP8266   D8 -> Slave select
const lmic_pinmap lmic_pins = {
    .nss = 15,
    .rxtx = LMIC_UNUSED_PIN,
    .rst = LMIC_UNUSED_PIN,
    .dio = {LMIC_UNUSED_PIN, LMIC_UNUSED_PIN, LMIC_UNUSED_PIN},
};

void ota() {
    USE_SERIAL.println();
    USE_SERIAL.println();
    Serial.println("ota");
    WiFiMulti.addAP("ssid", "12345678");
    for(uint8_t t = 5; t > 0; t--) {
        USE_SERIAL.printf("[SETUP] WAIT %d...\n", t);
        USE_SERIAL.flush();
        delay(1000);
    }

    Serial.println("setup complete");
    delay(5000);
    if(WiFiMulti.run() != WL_CONNECTED)  {
        Serial.println("No wifi update shutting down wifi");
    } else {
        Serial.println("updating");
        t_httpUpdate_return ret = ESPhttpUpdate.update("http://server.nl/ota.bin");

        switch(ret) {
            case HTTP_UPDATE_FAILED:
                USE_SERIAL.printf("HTTP_UPDATE_FAILD Error (%d): %s", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
                break;

            case HTTP_UPDATE_NO_UPDATES:
                USE_SERIAL.println("HTTP_UPDATE_NO_UPDATES");
                break;

            case HTTP_UPDATE_OK:
                USE_SERIAL.println("HTTP_UPDATE_OK");
                break;
        }
    }
}

boolean hasLocation() {
  int n = WiFi.scanNetworks(); // number of acces points found
  if (n == 0) {
    Serial.println("Nope. No spots, are you lost or what :(");
    return false;
  } else {
    Serial.print("I found ");
    Serial.print(n);
    Serial.println(" spots!");
    if(n > NR_MACS) {
      n = NR_MACS;
    }
    // TODO: send only strongest RSSI??
    for (int i = 0; i < n; ++i) {
        Serial.println();
        Serial.print("rssi");
        Serial.println(WiFi.RSSI(i));
        for(int c = 0; c < 6; c++) {
            macs[(i * 7) + c] = (uint8_t)WiFi.BSSID(i)[c];
            Serial.print(WiFi.BSSID(i)[c]);
            Serial.print(":");
        }
        Serial.println();
        macs[(i*7)+6] =  (uint8_t)((-1) * WiFi.RSSI(i));
      delay(10);
    }
  }
  Serial.println("");
  return true;
}

void do_send(osjob_t* j){
    if (LMIC.opmode & OP_TXRXPEND) {
        Serial.println(F("OP_TXRXPEND, not sending"));
    } else {
        if(hasLocation()) {
          LMIC_setTxData2(1, macs, sizeof(macs), 0);
        } else {
          LMIC_setTxData2(1, mydata, sizeof(mydata), 0);
        }
        Serial.println(F("Packet queued"));
    }
    // Next TX is scheduled after TX_COMPLETE event.
}

void onEvent (ev_t ev) {
    Serial.print(os_getTime());
    Serial.print(": ");
    switch(ev) {
        case EV_SCAN_TIMEOUT:
            Serial.println(F("EV_SCAN_TIMEOUT"));
            break;
        case EV_BEACON_FOUND:
            Serial.println(F("EV_BEACON_FOUND"));
            break;
        case EV_BEACON_MISSED:
            Serial.println(F("EV_BEACON_MISSED"));
            break;
        case EV_BEACON_TRACKED:
            Serial.println(F("EV_BEACON_TRACKED"));
            break;
        case EV_JOINING:
            Serial.println(F("EV_JOINING"));
            break;
        case EV_JOINED:
            Serial.println(F("EV_JOINED"));
            break;
        case EV_RFU1:
            Serial.println(F("EV_RFU1"));
            break;
        case EV_JOIN_FAILED:
            Serial.println(F("EV_JOIN_FAILED"));
            break;
        case EV_REJOIN_FAILED:
            Serial.println(F("EV_REJOIN_FAILED"));
            break;
        case EV_TXCOMPLETE:
            Serial.println(F("EV_TXCOMPLETE (includes waiting for RX windows)"));
            if (LMIC.txrxFlags & TXRX_ACK)
              Serial.println(F("Received ack"));
            if (LMIC.dataLen) {
              Serial.println(F("Received "));
              Serial.println(LMIC.dataLen);
              Serial.println(F(" bytes of payload"));
            }
            // Schedule next transmission
            os_setTimedCallback(&sendjob, os_getTime()+sec2osticks(TX_INTERVAL), do_send);
            break;
        case EV_LOST_TSYNC:
            Serial.println(F("EV_LOST_TSYNC"));
            break;
        case EV_RESET:
            Serial.println(F("EV_RESET"));
            break;
        case EV_RXCOMPLETE:
            // data received in ping slot
            Serial.println(F("EV_RXCOMPLETE"));
            break;
        case EV_LINK_DEAD:
            Serial.println(F("EV_LINK_DEAD"));
            break;
        case EV_LINK_ALIVE:
            Serial.println(F("EV_LINK_ALIVE"));
            break;
         default:
            Serial.println(F("Unknown event"));
            break;
    }
}

void setup() {
    Serial.begin(115200);
    Serial.println(F("Starting"));
    WiFi.mode(WIFI_STA);
    // check if it needs OTA update..
    ota();
    WiFi.mode(WIFI_OFF);
    // LMIC init
    os_init();
    // Reset the MAC state. Session and pending data transfers will be discarded.
    LMIC_reset();
    #ifdef PROGMEM
    uint8_t appskey[sizeof(APPSKEY)];
    uint8_t nwkskey[sizeof(NWKSKEY)];
    memcpy_P(appskey, APPSKEY, sizeof(APPSKEY));
    memcpy_P(nwkskey, NWKSKEY, sizeof(NWKSKEY));
    LMIC_setSession (0x1, DEVADDR, nwkskey, appskey);
    #else
    // If not running an AVR with PROGMEM, just use the arrays directly
    LMIC_setSession (0x1, DEVADDR, NWKSKEY, APPSKEY);
    #endif

    #if defined(CFG_eu868)
    LMIC_setupChannel(0, 868100000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    LMIC_setupChannel(1, 868100000, DR_RANGE_MAP(DR_SF12, DR_SF7B), BAND_CENTI);      // g-band
    LMIC_setupChannel(2, 868100000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    LMIC_setupChannel(3, 868100000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    LMIC_setupChannel(4, 868100000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    LMIC_setupChannel(5, 868100000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    LMIC_setupChannel(6, 868100000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    LMIC_setupChannel(7, 868100000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    LMIC_setupChannel(8, 868100000, DR_RANGE_MAP(DR_FSK,  DR_FSK),  BAND_MILLI);      // g2-band
    #elif defined(CFG_us915)
    LMIC_selectSubBand(1);
    #endif
    // Disable link check validation
    LMIC_setLinkCheckMode(0);
    // TTN uses SF9 for its RX2 window.
    LMIC.dn2Dr = DR_SF9;
    // Set data rate and transmit power for uplink (note: txpow seems to be ignored by the library)
    LMIC_setDrTxpow(DR_SF7,14);
    // Start job
    do_send(&sendjob);
}

void loop() {
    os_runloop_once();
}
