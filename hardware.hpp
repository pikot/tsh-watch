#ifndef _HARDWARE_
#define _HARDWARE_

#include <TimeLib.h>
#include "MAX30100_PulseOximeter.h"
#include "FS.h"
#include <DS3231M.h>
#include <U8g2lib.h>

#define _SERIAL_DEBUG_ 1
#define _USE_MLX90615_ 1

#if defined(_USE_MLX90614_)
#include <SparkFunMLX90614.h>
#elif defined(_USE_MLX90615_)
#include <MLX90615.h>
#endif


#ifdef _SERIAL_DEBUG_
#define print_w(a) (Serial.print(a))
#define println_w(a) (Serial.println(a))
#define write_w(a) (Serial.write(a))
#else
#define print_w(a) 
#define println_w(a) 
#define write_w(a) 
#endif

class Display{
    private:
        const int REPORTING_PERIOD_MS = 2000;
        U8G2 _display;
 
        uint32_t ts_last_display_update = 0;
 
        void  show_digitalClock(int16_t x, int16_t y);
        void  show_temperatureC(int16_t x, int16_t y, float therm, float pulse);
    public:
        void init();
        void update(float therm, float pulse);
        void setPowerSave(bool powerSave);
};

class FileSystem {
    private:
        bool    _can_write = false;

        File    _file;
        
        char *current_day_fname(char *inputBuffer, int inputBuffer_size, char *dir, int16_t year, int8_t month, int8_t  day);

    public:
        void init();
        void cat_file(File f);
        void write_log(float HeartRate, float AmbientTempC, float ObjectTempC, float vcc);
        void scan_log_dir(char* dir_name);

        void close();
};

class Control {
    private:
        int Button_PrevState     = 0;
        int Button_State         = 0;
        int Input_1              = 2;
    public:
        void init();
        bool button_pressed();
};
 
class Hardware {
    private:
#if defined(_USE_MLX90614_)
        IRTherm         therm;
#elif defined(_USE_MLX90615_)
        MLX90615        therm;
#endif
        DS3231M_Class   DS3231M;
        Display         display;
        PulseOximeter   pulse;
        FileSystem      log_file;
        Control         control;

        uint32_t displaySleepDelay = 3000;
        uint32_t displaySleepTimer = 0;

        bool powerSave = false;
 
        void thermometer_init();
        void display_init();
        void time_init();
        void filesystem_init();
        void serial_init();
        void pulse_init();

        int  next_wake_time();
        void WakeSensors() ;
        void GoToSleep() ;
        
        
        float get_redable_vcc();
        void  print_stat();
        
 
    public:
        void init();
        void update();
        void power_safe_logic();
};

#endif
