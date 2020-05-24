#ifndef _SENSOR_
#define _SENSOR_

#include "utils.h"
#include "MAX30100_PulseOximeter.h"
#include <BMI160Gen.h>
#include <Adafruit_INA219.h>

#define _USE_MLX90615_ 1


#if defined(_USE_MLX90614_)
#include <SparkFunMLX90614.h>
#elif defined(_USE_MLX90615_)
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

class Gyroscope : public Sensor {
    private:
        const int      i2c_addr = 0x69;
        BMI160GenClass gyro;
        
        int32_t  steps;
    public:
        Gyroscope();
        void init(bool is_after_deepsleep);
        void update();
        void sleep();
        void wake();
        
        void read_data();
        uint16_t StepCount();
};

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

class CurrentMeter: public Sensor {
    private:
        Adafruit_INA219 ina219;
        
        float vcc;
        
    public:
        CurrentMeter();
        void init();
        void sleep();
        void wake();
        
        void read_data();
        float Vcc();
};

#endif
