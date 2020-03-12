/***********************************************





***********************************************/

#pragma once

class CLeakSensor {
public:    
    enum EState {STATE_OK, STATE_ERR, STATE_ALARM, STATE_START_ERR};

private:
    const uint16_t okAverageDiv = 50;
    const uint16_t sampleSize = 1000;
    
    const uint8_t pin;
    int16_t average;
    uint32_t sample = 0;
    uint16_t sampleCounter = 0;
    
    CLeakSensor::EState state;
    bool isChange = false;

    virtual uint16_t valueRead() {return analogRead(pin);}
    int16_t filter(float val);
public:
    CLeakSensor(const uint16_t pin_) :  pin(pin_){}
    virtual void init() {pinMode(pin, INPUT);}
    CLeakSensor::EState begin();
    void print() {Serial.print("--- Pin A"); Serial.print(pin-54); Serial.print(" avr "); Serial.print(average); Serial.print(" state "); Serial.println(state);}
    CLeakSensor::EState getState();
    bool getChange(){return isChange;}
};

CLeakSensor::EState CLeakSensor::getState(){
    sample += valueRead();
    if (++sampleCounter < sampleSize){
        isChange = false;
        return state;
    }
    int val = sample / sampleSize;
    sample = 0;
    sampleCounter = 0;
    if (state != STATE_START_ERR && val < 200 || val > 650 ){
        isChange = (state != STATE_ERR);
        return state = STATE_ERR;
    }
    else if (state == STATE_START_ERR) return state;
    else if (abs(average - val) > 150){
        isChange = (state != STATE_ALARM);
        return state = STATE_ALARM;         
    }
    else {
        isChange = (state != STATE_OK);
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


