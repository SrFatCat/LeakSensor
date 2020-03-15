/***********************************************
Name:		LeakSensor.h
 Created:	12.03.2020
 Modified:  15.03.2020
 Author:	Alex aka Sr.FatCat

Classes for LeakSensor sketch
***********************************************/

#pragma once

#include <Wire.h>
#include <Adafruit_ADS1015.h>
class CLeakSensor {
public:    
    enum EState {STATE_OK, STATE_ERR, STATE_ALARM, STATE_START_ERR};
    enum EstateTrend {TREND_UNDEFINED, TREND_FROM_ERR_TO_OK, TREND_FROM_ERR_TO_ALARM, TREND_FROM_ALARM_TO_ERR}; 
private:
    const uint16_t sampleSize = 1000;

    const uint8_t pin;
    int16_t average;
    uint32_t sample = 0;
    uint16_t sampleCounter = 0;
    
    CLeakSensor::EState state;
    bool isChange = false;
    CLeakSensor::EstateTrend stateTrend = TREND_UNDEFINED;

    virtual uint16_t valueRead() {return analogRead(pin);}
    int16_t filter(float val);
public:
    CLeakSensor(const uint16_t pin_) :  pin(pin_){}
    virtual void init() {pinMode(pin, INPUT);}
    CLeakSensor::EState begin();
    void print() {Serial.print("--- Pin A"); Serial.print(pin-54); Serial.print(" avr "); Serial.print(average); Serial.print(" state "); Serial.println(state);}
    CLeakSensor::EState getState();
    inline CLeakSensor::EState State() const {return state;}
    inline bool getChange() const {return isChange;}
    inline CLeakSensor::EstateTrend getStatusTrend() const { return stateTrend; }
    
    friend class CLeakSensorADS;
};

class CLeakSensorADS : public CLeakSensor {
    static bool isInit;
    static Adafruit_ADS1115 &ads;
    uint16_t valueRead() final {return ads.readADC_SingleEnded(pin) / 32;}
public: 
    CLeakSensorADS(const uint16_t pin_) :  CLeakSensor(pin_){}
    void init() final { if (!isInit) {ads.setGain(GAIN_ONE); ads.begin(); isInit = true;} }
};

bool CLeakSensorADS::isInit = false;
Adafruit_ADS1115 & CLeakSensorADS::ads = *new Adafruit_ADS1115;
class CBeeper {
    const uint8_t pin;

    const int8_t alarmBeeperDelay = 6;
    const int8_t errorBeeperOnDelay = 1;
    const int8_t errorBeeperOffDelay = 36;

    uint8_t alarmCounter = 0;
    uint8_t errorCounter = 0;
    uint8_t tickCounter = 0;
    bool isRun = false;
    bool isStop = false;
public:
    CBeeper(const uint8_t pin_) : pin(pin_) {};
    inline void init() { pinMode(pin, OUTPUT); }
    inline void beep() { tone(pin, 100, 50); } // немедленый бип
    inline void stop() { isStop = true; }
    void addEvent(CLeakSensor& );
    void tick();
};

void CBeeper::addEvent(CLeakSensor& thisLeakSensor){
    isStop = false;
    if (thisLeakSensor.State() == CLeakSensor::EState::STATE_ALARM) {
        alarmCounter++;
        if (thisLeakSensor.getStatusTrend() == CLeakSensor::EstateTrend::TREND_FROM_ERR_TO_ALARM) errorCounter--;
        if (!isRun) digitalWrite(pin, isRun = true);
        else if (alarmCounter == 1) { // если пищали про ошибку, а тут подвалила протечка
            tickCounter = 0;
            digitalWrite(pin, LOW);
        }
    }
    else if (thisLeakSensor.State() == CLeakSensor::EState::STATE_ERR) {
        errorCounter++;
        if (thisLeakSensor.getStatusTrend() == CLeakSensor::EstateTrend::TREND_FROM_ALARM_TO_ERR) alarmCounter--;
        if (!isRun && alarmCounter == 0) digitalWrite(pin, isRun = true);
    }
    else if (thisLeakSensor.State() == CLeakSensor::EState::STATE_OK){
        if (thisLeakSensor.getStatusTrend() == CLeakSensor::EstateTrend::TREND_FROM_ERR_TO_OK) {
            errorCounter--;
        }
        else {
            alarmCounter--;
        }
        if (errorCounter + alarmCounter == 0) digitalWrite(pin, isRun = false);
    }    

}

void CBeeper::tick(){
    if ( !isRun ){
        tickCounter = 0;
        return;
    }
    if (isStop) {
        digitalWrite(pin, isRun = isStop = false);
    }
    tickCounter++;
    if (alarmCounter > 0){ // пищим тревогу
        if (tickCounter == alarmBeeperDelay) {
            digitalWrite(pin, !digitalRead(pin));
            tickCounter = 0;
        }
    }
    else { // пищим ошибку
        if (digitalRead(pin)) {
           if (tickCounter == errorBeeperOnDelay){
               digitalWrite(pin, LOW);
               tickCounter = 0;
           }
        }
        else {
           if (tickCounter == errorBeeperOffDelay){
               digitalWrite(pin, HIGH);
               tickCounter = 0;
           }
        }
    }
}

CLeakSensor::EState CLeakSensor::getState(){
    sample += valueRead();
    if (++sampleCounter < sampleSize){
        isChange = false;
        return state;
    }
    int val = sample / sampleSize;
    sample = 0;
    sampleCounter = 0;
    if (state == STATE_START_ERR) return state;
    else if (state != STATE_START_ERR && val < 200 || val > 650 ){
        isChange = (state != STATE_ERR);
        if (isChange && state == STATE_ALARM) stateTrend = TREND_FROM_ALARM_TO_ERR; else stateTrend = TREND_UNDEFINED;
        return state = STATE_ERR;
    }
    else if (abs(average - val) > 150){
        isChange = (state != STATE_ALARM);
        if (isChange && state == STATE_ERR) stateTrend = TREND_FROM_ERR_TO_ALARM; else stateTrend = TREND_UNDEFINED;
        return state = STATE_ALARM;         
    }
    else {
        isChange = (state != STATE_OK);
        if (isChange) 
            if (state == STATE_ERR) stateTrend = TREND_FROM_ERR_TO_OK; else stateTrend = TREND_UNDEFINED;
        return state = STATE_OK;         
       
    }

}

CLeakSensor::EState CLeakSensor::begin(){
    uint16_t val[sampleSize];
    uint32_t averageTmp = 0;
    for (int i=0; i<sampleSize; i++){
        val[i] = valueRead();
        averageTmp += val[i];
    }
    average = averageTmp / sampleSize;

    if (abs(average - 510) > okAverageDiv) { 
        Serial.print("!!!!!!!!!!!! ");
        return  state = STATE_START_ERR; 
    }    
    else{
        return  state = STATE_OK;
    } 
}


