#ifndef _SENSOR_
#define _SENSOR_

// best case scenario is one file for each sensor and rule by makefiles, but it is arduino ide =)
#define _USE_MLX90615_ 1  // 
//#define _USE_MLX90614_ 1
#define _USE_LSM6DS3_ 1
//define _USE_BMI160_ 1

#include "utils.h"
#include <TimeLib.h>
#include <WiFiUdp.h>
#include <WiFi.h>

#include "battery.h"

class Sensor {
    private:
        int i2c_addr;

       // virtual void read_data_direct();

    protected:
        bool inited  = false;
        virtual void init_direct();

    public:
        void init();
        
        void sleep();
        void wake();

        void on();
        void off();
        
        void read();
        
};


class GyroscopeLSM6DS3 : public Sensor {
    private:
        const int      i2c_addr = 0x6A;
        
        int32_t  steps;

        void printUlpData();

    public:
        GyroscopeLSM6DS3();
        ~GyroscopeLSM6DS3();
        void init();
        void sleep();
        void wake();
        
        void read();
        uint16_t getStepCount();
};
typedef  GyroscopeLSM6DS3 Gyroscope;


class ThermoMeter: public Sensor {
    private:
        float AmbientTempC;
        float ObjectTempC;
        
 
        float raw_temp_to_C(uint16_t _t);
        void printUlpData();
   
    public:
        ThermoMeter();
        void init();
        void sleep();
        void wake();

        void read();
        float getAmbientC();  
        float getObjectC();
};

class PulseMeter: public Sensor {
    private:
//        PulseOximeter   pulse;
        
        float    heartRate;
        float    spO2;

    public:
        PulseMeter();
        
        void init();
        void sleep();
        void wake();

        void read();
        float getHeartRate();
        float getSpO2();
};

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
        
        float current_since_reboot;
        float *aggr_current;
        float aggr_current_prev;

        int32_t tm = 1;
        int32_t tb = 60;
       // prew_aggr_current;

        float calc_aggr_accum(float mA);
        void printUlpData();
    public:
        CurrentMeter();
        void init(float *current);
        void sleep();
        void wake();
    //    uint32_t get_battery_voltage(void);
        
        void read();
        float getVcc();
    //    float getCurrent();
        uint8_t getBatLevel();
};

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
        
        void init();
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

#endif
