#ifndef _SENSOR_
#define _SENSOR_

// best case scenario is one file for each sensor and rule by makefiles, but it is arduino ide =)
#define _USE_MLX90615_ 1  // 
//#define _USE_MLX90614_ 1
#define _USE_LSM6DS3_ 1
//define _USE_BMI160_ 1

#include "utils.h"
#include "MAX30100_PulseOximeter.h"
#include <Adafruit_INA219.h>

#ifdef _USE_LSM6DS3_
#include <SparkFunLSM6DS3.h>
#endif
#ifdef _USE_BMI160_
#include <BMI160Gen.h>
#endif

#if defined(_USE_MLX90614_)
#include <SparkFunMLX90614.h>
#endif
#if defined(_USE_MLX90615_)
#include <MLX90615.h>
#endif

class Sensor {
    private:
        int i2c_addr;
    public:
        bool inited = false;

        void init();
        void init(bool is_after_deepsleep);
        
        void update();
        void sleep();
        void wake();
        
        void read_data();
};

#ifdef _USE_BMI160_
class GyroscopeBMI160 : public Sensor {
    private:
        const int      i2c_addr = 0x69;
        BMI160GenClass gyro;
        
        int32_t  steps;
    public:
        GyroscopeBMI160();
        void init(bool is_after_deepsleep);
        void update();
        void sleep();
        void wake();
        
        void read_data();
        uint16_t StepCount();
};
typedef  GyroscopeBMI160 Gyroscope;
#endif

#ifdef _USE_LSM6DS3_
class GyroscopeLSM6DS3 : public Sensor {
    private:
        const int      i2c_addr = 0x6A;
        LSM6DS3Core  *gyro;
        
        int32_t  steps;
    public:
        GyroscopeLSM6DS3();
        ~GyroscopeLSM6DS3();
        void init(bool is_after_deepsleep);
        void update();
        void sleep();
        void wake();
        
        void read_data();
        uint16_t StepCount();
};
typedef  GyroscopeLSM6DS3 Gyroscope;
#endif

class ThermoMeter: public Sensor {
    private:
#if defined(_USE_MLX90614_)
        IRTherm         therm;
#elif defined(_USE_MLX90615_)
        MLX90615        therm;
#endif
        float AmbientTempC;
        float ObjectTempC;
        
    public:
        ThermoMeter();
        void init();
        void update();
        void sleep();
        void wake();

        void read_data();
        float AmbientC();  
        float ObjectC();
};

class PulseMeter: public Sensor {
    private:
        PulseOximeter   pulse;
        
        float    heartRate;
        float    spO2;

    public:
        PulseMeter();
        
        void init();
        void update();
        void sleep();
        void wake();

        void read_data();
        float HeartRate();
        float SpO2();
};

#define BATTERY_ADC_CH  ADC1_CHANNEL_4  // GPIO 32
#define BATTERY_ADC_SAMPLE  33
#define BATTERY_ADC_DIV  1
#define ADC_VREF        1128            // ADC calibration data

class CurrentMeter: public Sensor {
    private:
        Adafruit_INA219 ina219;
        
        float vcc;
        
    public:
        CurrentMeter();
        void init();
        void sleep();
        void wake();
        uint32_t get_battery_voltage(void);
        
        void read_data();
        float Vcc();
};

#endif
