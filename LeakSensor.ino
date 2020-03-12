/*********************************************************************
LeakSensor
by Alex.Y.B.

Created: 13.02.2020 modifyed
*********************************************************************/
#include <SPI.h>
#include <Ethernet.h>
#define BLYNK_PRINT Serial
//#define BLYNK_INFO_CONNECTION "W5500"
//#define W5100_CS  53
#include <BlynkSimpleEthernet.h>

#include <Adafruit_NeoPixel.h>

#include "LeakSensor.h"

const uint8_t PIN_BUTTON = 43;
const uint8_t PIN_BUZZER = 39;
const uint8_t PIN_LED_STRIP = 41;

const uint8_t LEAK_CHANELS = 16;

const byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
const char auth[] = "10e3db2a38084530bac9f105d87f7c42";

Adafruit_NeoPixel stripLED(LEAK_CHANELS, PIN_LED_STRIP, NEO_GRB);

enum EColors {BLACK = 0, RED = 0xFF0000, GREEN = 0x00FF00, WHITE = 0xFFFFFF, YELLOW = 0xFFFF00};

void beep(uint8_t duration, uint8_t cycles){

}

//CLeakSensor *leakSensor[LEAK_CHANELS];
CLeakSensor leakSensor[] = {
    CLeakSensor(A13), 
    CLeakSensor(A14), 
    CLeakSensor(A15), 
    CLeakSensor(A12), 
    CLeakSensor(A11), 
    CLeakSensor(A10), 
    CLeakSensor(A9), 
    CLeakSensor(A8),
    CLeakSensor(A7),
    CLeakSensor(A6),
    CLeakSensor(A5),
    CLeakSensor(A4),
    CLeakSensor(A3),
    CLeakSensor(A2),
    CLeakSensor(A1),
    CLeakSensor(A0)
    };

void setup(){
    pinMode(PIN_BUTTON, INPUT_PULLUP);
    pinMode(PIN_BUZZER, OUTPUT);
    
    Serial.begin(115200);
    Serial.println("*************** START ************\n\n");

    stripLED.begin();
    stripLED.fill(WHITE);
    stripLED.show();
    delay(1000);

    for (int j = 0; j < LEAK_CHANELS; j++) {
        tone(PIN_BUZZER, 100, 50);
        if (leakSensor[j].begin() == CLeakSensor::EState::STATE_OK) stripLED.setPixelColor(j, GREEN); else stripLED.setPixelColor(j, YELLOW);
        stripLED.show();
        leakSensor[j].print();
    }
    Serial.println("-------------------------------------------");
    /*Blynk.begin(auth);
    Ethernet.init(53);
    Ethernet.begin(mac, 10000, 10000);
    if (Ethernet.hardwareStatus() == EthernetNoHardware){
        Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
    }
    else Serial.println("Ethernet shield was FOUND!!!!!!!!!!!!!!!!");

    if (Ethernet.linkStatus() == LinkOFF){
        Serial.println("Ethernet cable is not connected.");
    } else Serial.println("Ethernet cable is CONNECTED!!!!!!!!!!!!!!!!!!!!!");*/
}

void loop(){
    const uint32_t t = millis();
    static uint32_t prev_t = t;
    static bool prev_but = true;
    const bool but = digitalRead(PIN_BUTTON);

    if (t - prev_t > 1000) {
        digitalWrite(PIN_BUZZER, !but);
        prev_t = t;
    }
    if (but != prev_but){
        prev_but = but;
        Serial.print(t);
        Serial.print(" BUT = ");
        Serial.println(but);
        
    }
    for (int i = 0; i < LEAK_CHANELS; i++){
        CLeakSensor::EState state = leakSensor[i].getState();
        if (leakSensor[i].getChange()) leakSensor[i].print();
    }
}
