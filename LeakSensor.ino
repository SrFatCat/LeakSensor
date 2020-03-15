/*********************************************************************
 Name:		LeakSensor.ino
 Created:	13.02.2020
 Modified:  15.03.2020
 Author:	Alex aka Sr.FatCat

LeakSensorsController up to 20 chanels (16 ADC + 4 extended ADS)
*********************************************************************/
#include <SPI.h>
#include <Ethernet.h>
#define BLYNK_PRINT Serial
#define BLYNK_INFO_CONNECTION "W5500"
//#define W5100_CS  53
#include <BlynkSimpleEthernet.h>

#include <Adafruit_NeoPixel.h>

#include "LeakSensor.h"

#include "UniversalKeyboardDriver.h"

const uint8_t PIN_BUTTON = 43;
const uint8_t PIN_BUZZER = 39;
const uint8_t PIN_LED_STRIP = 41;

const uint8_t LEAK_CHANELS = 16;

const byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
const char auth[] = "CpXaKWnOab8-DRUxwMlyeYhVUyotEiEM";
bool isBlynking = false;
WidgetLCD lcdBlynk(V0);
WidgetLED ledBlynk(V1);

Adafruit_NeoPixel stripLED(LEAK_CHANELS, PIN_LED_STRIP, NEO_GRB);

enum EColors {BLACK = 0, RED = 0xFF0000, GREEN = 0x00A000, WHITE = 0xFFFFFF, YELLOW = 0x5F5000, DARK_YELLOW=0x131000};

CBeeper beeper(PIN_BUZZER);

CKeyboardDriver keyDriver(1);
const int16_t keyCodeArray[] = { PIN_BUTTON };

//template <typename T> inline Print & operator << (Print &s, T n) { s.print(n); return s; }

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
    CLeakSensor(A0),
    CLeakSensorADS(0),
    CLeakSensorADS(1),
    CLeakSensorADS(2),
    CLeakSensorADS(3)
    };

ISR(TIMER1_COMPA_vect){
    beeper.tick();
    keyDriver.listenKeys();
}

void(* resetFunc) (void) = 0;

void updateLCD(){
    if (!isBlynking) return;
    String sNotReady, sError, sAlarm;
    for (int j = 0; j < LEAK_CHANELS; j++) {
        if (leakSensor[j].State() == CLeakSensor::EState::STATE_ERR){
            sError += String(sError.length() == 0 ? "" : ",") + String(j+1);
        }
        else if (leakSensor[j].State() == CLeakSensor::EState::STATE_ALARM){
            sAlarm += String(sAlarm.length() == 0 ? "" : ",") + String(j+1);
        }
        else if (leakSensor[j].State() == CLeakSensor::EState::STATE_START_ERR){
            sNotReady += String(sNotReady.length() == 0 ? "" : ",") + String(j+1);
        }
    }
    String out = (sAlarm.length() > 0 ? "Протеч." + sAlarm + "! " : "") +  (sError.length() > 0 ? "Авар." + sError + " " : "") + (sNotReady.length() > 0 ? "Откл." + sNotReady : "");
    lcdBlynk.clear();
    lcdBlynk.print(0,0, out.c_str());
}

void setup(){    
    Serial.begin(115200);
    Serial.println("*************** START ************\n\n");
    
    keyDriver.addDevice((CKeybdDevice *)new CKeybdDigital((int16_t *)keyCodeArray, 1));
    
    beeper.init();

    stripLED.begin();
    stripLED.fill(WHITE);
    stripLED.show();
    delay(1000);

    for (int j = 0; j < LEAK_CHANELS; j++) {
        beeper.beep();
        if (leakSensor[j].begin() == CLeakSensor::EState::STATE_OK) stripLED.setPixelColor(j, GREEN); else stripLED.setPixelColor(j, YELLOW);
        stripLED.show();
        leakSensor[j].print();
    }

    for (int i = 0; i < 9; i++){
        if (i%2 == 0) beeper.beep();
        for (int j = 0; j < LEAK_CHANELS; j++) {
            if (stripLED.getPixelColor(j) == YELLOW) stripLED.setPixelColor(j, 0);
            else if (stripLED.getPixelColor(j) == 0) stripLED.setPixelColor(j, YELLOW);
        }
        stripLED.show();
        delay(600);
    }

    // инициализация Timer1
    cli();  // отключить глобальные прерывания
    TCCR1A = 0;   // установить регистры в 0
    TCCR1B = 0;
    OCR1A = 2000;//15624; // установка регистра совпадения ~0.125c
    TCCR1B |= (1 << WGM12);  // включить CTC режим 
    TCCR1B |= (1 << CS10); // Установить биты на коэффициент деления 1024
    TCCR1B |= (1 << CS12);
    TIMSK1 |= (1 << OCIE1A);  // включить прерывание по совпадению таймера 
    sei(); // включить глобальные прерывания
    
    Ethernet.init(53);
    Ethernet.begin(mac, 10000, 10000);
    if (Ethernet.hardwareStatus() == EthernetNoHardware) Serial.println("Ethernet shield was not found.");
    else{
        Serial.println("Ethernet shield was FOUND!!!!!!!!!!!!!!!!");
        if (Ethernet.linkStatus() == LinkOFF) Serial.println("Ethernet cable is not connected.");
        else {
            Serial.println("Ethernet cable is CONNECTED!!!!!!!!!!!!!!!!!!!!!");
            Blynk.begin(auth);
            isBlynking = true;
            updateLCD();
        }    
    }

    Serial.println("-------------------------------------------");
}

void loop(){
    static uint8_t sensorCounter = 0;
    static bool isSendNotify = false;
    const uint32_t t = millis();
    static uint32_t prev_t = t;


    if (t - prev_t > 500) {
        bool isAlarm = false;
        bool isShow = false;
        for (int j = 0; j < LEAK_CHANELS; j++) {
            if (leakSensor[j].State() == CLeakSensor::EState::STATE_OK){
                if (stripLED.getPixelColor(j) != GREEN){
                    stripLED.setPixelColor(j, GREEN);   
                    isShow = true; 
                } 
            }
            else if (leakSensor[j].State() == CLeakSensor::EState::STATE_ERR){
                if (stripLED.getPixelColor(j) != YELLOW) stripLED.setPixelColor(j, YELLOW); else stripLED.setPixelColor(j, DARK_YELLOW);
                isShow = true;
            }
            else if (leakSensor[j].State() == CLeakSensor::EState::STATE_ALARM){
                if (stripLED.getPixelColor(j) != RED) stripLED.setPixelColor(j, RED); else stripLED.setPixelColor(j, 0);    
                isAlarm = isShow = true;
                if (isBlynking && isSendNotify){
                    Blynk.notify("ПРОТЕЧКА!!!");
                    isSendNotify = false;
                }
            }
        }
        static bool isAlarmLed = false;
        if (isBlynking && (isAlarm || isAlarmLed)){
            isAlarmLed = !isAlarmLed;
            if (isAlarmLed) ledBlynk.on(); else ledBlynk.off();
        }
        if (isShow) stripLED.show();      
        prev_t = t;
    }
    
    leakSensor[sensorCounter].getState();
    if (leakSensor[sensorCounter].getChange()){
        leakSensor[sensorCounter].print();
        updateLCD();
        isSendNotify = true;
        beeper.addEvent(leakSensor[sensorCounter]);
    } 
    if (++sensorCounter == LEAK_CHANELS) sensorCounter = 0;

    if (isBlynking) Blynk.run();

    if (keyDriver.isKeyPressed()) {
        bool longPressKey;
        keyDriver.getKeyCode(&longPressKey);
		//Serial << " LPK=" << longPressKey << "\n";
        if (longPressKey) resetFunc(); else beeper.stop();
	}
}

BLYNK_CONNECTED() {
    updateLCD(); 
}
