//  SPDX-FileCopyrightText: 2020-2021 Ivan Ivanov 
//  SPDX-License-Identifier: GPL-3.0-or-later

#ifndef _SENSOR_
#define _SENSOR_

// best case scenario is one file for each sensor and rule by makefiles, but it is arduino ide =)
//#define _USE_MLX90615_ 1  // 
//#define _USE_LSM6DS3_ 1

#include "utils.h"
#include <TimeLib.h>
#include <WiFiUdp.h>
#include <WiFi.h>
#include <SoftWire.h>

#include "battery.h"
#include "version.h"
#include "i2c_bme280.h"
#include "i2c_lsm6ds3.h"
#include "i2c_ina219.h"
#include "i2c_mlx90615.h"
#include "i2c_max30100.h"

    
#define SENSOR_INITED4ULP(X,VAL) ulp_ ## X _inited = VAL
#define SENSOR_ERROR4ULP(X,VAL) ulp_ ## X _error = VAL
 
class Sensor {
    private:
     //   int i2c_addr;

       // virtual void read_data_direct();

    protected:
        int i2c_addr;
        char *name;
    //    bool inited  = false;
        SoftWire      *i2c;
        int16_t       *ulp_error;
        

        virtual void init_direct();
        bool testAddress();

        uint8_t readRegister(uint8_t address);
        void    writeRegister(uint8_t address, uint8_t data);
        void    writeRegister(uint8_t address, uint16_t data);
        uint8_t updateRegister(uint8_t address, uint8_t mask, uint8_t value);


        uint8_t burstRead(uint8_t baseAddress, uint8_t *buffer, uint8_t length);

    public:
        bool init();
        
        void sleep();
        void wake();

        void on();
        void off();
        
        void read();
        
};
//============================================================================================

class GyroscopeLSM6DS3 : public Sensor {
    private:
        int32_t  steps;

        void printUlpData();

    public:
        GyroscopeLSM6DS3();
        ~GyroscopeLSM6DS3();
        void init(SoftWire *i2c);
        void sleep();
        void wake();
        
        void read();
        uint16_t getStepCount();
};
typedef  GyroscopeLSM6DS3 Gyroscope;

//============================================================================================

class ThermoMeter: public Sensor {
    private:
        float AmbientTempC;
        float ObjectTempC;
 
        float raw_temp_to_C(uint16_t _t);
        void printUlpData();
        void wakeUp();

    public:
        ThermoMeter();
        void init(SoftWire *i2c);
        void sleep();
        void wake();

        void read();
        float getAmbientC();  
        float getObjectC();
};

//============================================================================================

#define ALPHA 0.85  //dc filter alpha value
#define MEAN_FILTER_SIZE        15  

struct dcFilter_t {
    float w;
    float result;
};

struct meanDiffFilter_t
{
    float values[MEAN_FILTER_SIZE];
    uint8_t index;
    float sum;
    uint8_t count;
};

struct butterworthFilter_t
{
  float v[2];
  float result;
};

class PulseMeter: public Sensor {
    private:
//        PulseOximeter   pulse;
        float    heartRate;
        float    spO2;


        dcFilter_t dcFilterIR;
        meanDiffFilter_t meanDiffIR;
    
        void process_Value(uint16_t rawIR);

        float meanDiff(float M, meanDiffFilter_t* filterValues);
        dcFilter_t dcRemoval(float x, float prev_w, float alpha);
        butterworthFilter_t lpbFilterIR;

        void lowPassButterworthFilter( float x, butterworthFilter_t * filterResult );

        void printUlpData();
        void printRawData();
    public:
        PulseMeter();
        
        void init(SoftWire *i2c);
        void sleep();
        void wake();

        void read();
        float getHeartRate();
        float getSpO2();

        bool dataIsReady();
        void dropData();
};

//============================================================================================

/*
#define BATTERY_ADC_CH  ADC1_CHANNEL_4  // GPIO 32
#define BATTERY_ADC_SAMPLE  33
#define BATTERY_ADC_DIV  1
#define ADC_VREF        1128            // ADC calibration data
*/
class CurrentMeter: public Sensor {
    private:
        Battery         *battery;
        float   vcc;
        float   current;
        uint8_t batLevel;
        
        int32_t tm = 1;
        int32_t tb = 60;

        float calc_aggr_accum(float mA);
        void setCalibration(uint16_t calValue);

        void printUlpData();
    public:
        CurrentMeter();
        void init(SoftWire *i2c);
        void sleep();
        void wake();
    //    uint32_t get_battery_voltage(void);
        
        void read();
        float getVcc();
    //    float getCurrent();
        uint8_t getBatLevel();
};

//============================================================================================

const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message

class TimeMeter: public Sensor {
    private:        
        uint8_t bcd2dec(uint8_t bcd);
        uint8_t dec2bcd(uint8_t n);

        void printUlpData();
        
        WiFiUDP Udp;
        char *ntpServerName    = "europe.pool.ntp.org";
        uint32_t localPort = 8888;  // local port to listen for UDP packets

        byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets

        uint32_t  ntpUpdateInterval = 3600;
        int timeZone          = 3;     // Moscow  Time

        void   updateUlpTime(tmElements_t &tm);
        void   sendNtpPacket(IPAddress &address); 
        time_t getNtpTime();
        bool   read_data_to_tm(tmElements_t &tm) ;
    public:
        TimeMeter();
        
        void init(SoftWire *i2c);
        void read();

        void sleep();
        void wake();

        time_t currentTime();

        time_t updateNtpTime();

        void updateTime(int h,  int m, int s);
        void updateDate(int d,  int m, int y);
        void setTimeZone(int8_t tz) { timeZone = tz; }

 //       DS3231M_Class dateNow() {return DS3231M;}
};

//============================================================================================

#define BME280_TEMP_PRESS_CALIB_DATA_LEN          UINT8_C(26)
#define BME280_HUMIDITY_CALIB_DATA_LEN            UINT8_C(7)
#define BME280_P_T_H_DATA_LEN                     UINT8_C(8)

struct bme280CalibData
{
    uint16_t dig_t1;
    int16_t  dig_t2;
    int16_t  dig_t3;
    uint16_t dig_p1;
    int16_t  dig_p2;
    int16_t  dig_p3;
    int16_t  dig_p4;
    int16_t  dig_p5;
    int16_t  dig_p6;
    int16_t  dig_p7;
    int16_t  dig_p8;
    int16_t  dig_p9;
    
    uint8_t  dig_h1;
    uint16_t dig_h2;
    uint8_t  dig_h3;
    int16_t  dig_h4;
    int16_t  dig_h5;
    int8_t   dig_h6;
    int32_t  t_fine;
};

// un-compensated data
struct bme280UncompData
{
    uint32_t pressure;
    uint32_t temperature;
    uint32_t humidity;
};

/**\name Macro to combine two 8 bit data's to form a 16 bit data */
#define BME280_SWAP_BYTES(num)             ((num>>8) | (num<<8))
#define BME280_CONCAT_BYTES(msb, lsb)      (((uint16_t)msb << 8) | (uint16_t)lsb)

class BME280Meter: public Sensor {
    private:
        bme280UncompData  uData;
    //    bme280CalibData   calibData;

        int32_t  t_fine;

        float  temperature;
        float  pressure;
        float  humidity;

        void printUlpData();
#ifdef MAIN_CORE_INIT_SENSORS
        void parseTempPressCalibData(const uint8_t *reg_data);
        void parseHumidityCalibData(const uint8_t *reg_data);
        void outCalibData();
#else
        void parseTempPressCalibDataUlp();
        void parseHumidityCalibDataUlp();
#endif
        void  parseSensorData();
        float readTemperature();
        float readPressure();
        float readHumidity();
    public:
        BME280Meter();
        
        void init(SoftWire *i2c);
        void sleep();
        void wake();

        void read();

        float getPressure();
        float getHumidity();
};

#endif
