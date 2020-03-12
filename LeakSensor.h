/***********************************************





***********************************************/

#pragma once

class CLeakSensor {
public:    
    enum EState {STATE_OK, STATE_ERR, STATE_ALARM, STATE_START_ERR};

private:
    const uint16_t okAverageDiv = 50;
    const uint16_t okStDiffDiv = 90;
    const uint16_t sampleSize = 1000;
    
    const uint8_t pin;
    int16_t average;
    uint16_t standartDif;
    
    CLeakSensor::EState state;
    bool isChange = false;

    // переменные для калмана
    //float varVolt = standartDif;  // среднее отклонение (ищем в begin)
    float varProcess = 0.001; // скорость реакции на изменение (подбирается вручную)
    float Pc = 0.0;
    float G = 0.0;
    float P = 1.0;
    float Xp = 0.0;
    float Zp = 0.0;
    float Xe = 0.0;
    // переменные для калмана

    virtual uint16_t valueRead() {return analogRead(pin);}
    int16_t filter(float val);
public:
    CLeakSensor(const uint16_t pin_) :  pin(pin_){}
    virtual void init() {pinMode(pin, INPUT);}
    CLeakSensor::EState begin();
    void print() {Serial.print("--- Pin A"); Serial.print(pin-54); Serial.print(" avr "); Serial.print(average); Serial.print(" state "); Serial.print(state); Serial.print(" STDIF "); Serial.println(standartDif);}
    CLeakSensor::EState getState();
    bool getChange(){return isChange;}
};

CLeakSensor::EState CLeakSensor::getState(){
    int val = filter(valueRead());
    if (state != STATE_START_ERR && val < 200 || val > 650 ){
        isChange = (state != STATE_ERR);
        return state = STATE_ERR;
    }
    else if (state == STATE_START_ERR) return state;
    else if (abs(average - val) > max(150, standartDif * 3 / 2)){
        isChange = (state != STATE_ALARM);
        return state = STATE_ALARM;         
    }
    else {
        isChange = (state != STATE_OK);
        return state = STATE_OK;         
       
    }

}

int16_t CLeakSensor::filter(float val) {  //функция фильтрации
    Pc = P + varProcess;
    G = Pc/(Pc + standartDif);
    P = (1-G)*Pc;
    Xp = Xe;
    Zp = Xp;
    Xe = G*(val-Zp)+Xp; // "фильтрованное" значение
    return int(Xe)/10*10;
}

CLeakSensor::EState CLeakSensor::begin(){
    uint16_t val[sampleSize];
    uint32_t averageTmp = 0;
    for (int i=0; i<sampleSize; i++){
        val[i] = valueRead();
        averageTmp += val[i];
    }
    average = averageTmp / sampleSize;
    averageTmp = 0;
    for (int i=0; i<sampleSize; i++){
        averageTmp += (val[i] - average)^2;
    }
    standartDif = sqrt(averageTmp/sampleSize);

    if (abs(average - 510) > okAverageDiv || standartDif > okStDiffDiv) { 
        Serial.print("!!!!!!!!!!!! ");
        return  state = STATE_START_ERR; 
    }    
    else{
        return  state = STATE_OK;
    } 
}


