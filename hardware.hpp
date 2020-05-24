#ifndef _HARDWARE_
#define _HARDWARE_

#include "sensors.hpp"
#include <TimeLib.h>
#include "FS.h"
#include "SPIFFS.h"
#include <DS3231M.h>
#include <U8g2lib.h>

#define  CACHE_RECORD_CNT 10

enum sensor_state_t{
    SENSOR_GOTOSLEEP = 0,
    SENSOR_SLEEPING,
    SENSOR_AWAKE,
    SENSOR_WRITE_RESULT
};
static const char * sensor_state_name[] = { "SENSOR_GOTOSLEEP", "SENSOR_SLEEPING", "SENSOR_AWAKE", "SENSOR_WRITE_RESULT"  };

enum control_state_t{
    CONTROL_GOTOSLEEP = 0,
    CONTROL_SLEEPING,
    CONTROL_AWAKE,
};
static const char * control_state_name[] = {"CONTROL_GOTOSLEEP", "CONTROL_SLEEPING", "CONTROL_AWAKE"};

static const char * esp_sleep_wake[] = {
    "ESP_SLEEP_WAKEUP_UNDEFINED",
    "ESP_SLEEP_WAKEUP_ALL",
    "ESP_SLEEP_WAKEUP_EXT0", 
    "ESP_SLEEP_WAKEUP_EXT1",
    "ESP_SLEEP_WAKEUP_TIMER",
    "ESP_SLEEP_WAKEUP_TOUCHPAD",
    "ESP_SLEEP_WAKEUP_ULP",
    "ESP_SLEEP_WAKEUP_GPIO",
    "ESP_SLEEP_WAKEUP_UART"
};
    
struct StatRecord_t{
    time_t   Time;
    uint32_t Steps;
    float    HeartRate;
    float    SpO2;
    float    AmbientTempC;
    float    ObjectTempC;
    float    Vcc;
};

class Display {
    private:
        const int REPORTING_PERIOD_MS = 1000;
        U8G2 _display;
 
        uint32_t ts_last_display_update = 0;
 
        void  show_digitalClock(int16_t x, int16_t y);
        void  show_temperatureC(int16_t x, int16_t y, float therm, float pulse);
        void  show_steps(int16_t x, int16_t y, uint16_t steps);

    public:
        void init();
        void update(float therm, float pulse, uint16_t);
        void setPowerSave(bool powerSave);
};

class FileSystem {
    private:
        bool    _can_write = true; //false;

        File    _file;
        
        char *current_day_fname(char *inputBuffer, int inputBuffer_size, char *dir, int16_t year, int8_t month, int8_t  day);
        void save_records_to_file();
        void list_dir(fs::FS &fs, const char * dirname);

    public:
        void init();
        void cat_file(File f);
        void write_log(StatRecord_t *record);
        void scan_log_dir(char* dir_name);

        void close();
};

class Control {
    private:        
        uint64_t _Button_State      = 0;
        uint64_t _Button_PrevState  = 0;
        
        gpio_num_t LEFT_BUTTON          = GPIO_NUM_14;
        gpio_num_t RIGHT_BUTTON         = GPIO_NUM_33;
        gpio_num_t OK_BUTTON            = GPIO_NUM_27;

        void print_button_state (const char *func_name, uint32_t line_number, int l_State, int r_State, int o_State );
    public:
        void init();
        bool button_pressed();
        void init_wakeup();
};
 
class Hardware {
    private:
        Gyroscope       sGyro;
        ThermoMeter     sTerm;
        PulseMeter      sPulse;
        CurrentMeter    sCurrent;
        
        DS3231M_Class   DS3231M;
        Display         display;
        Control         control;
        FileSystem      log_file;

        time_t          current_time;
         
        bool is_after_deepsleep = false;
     
        uint8_t  pulse_threshold   = 3; 
        uint32_t displaySleepDelay = 3000;
        uint32_t displaySleepTimer = 0;

        bool debug_trace_fsm = false;
        bool powerSave = false;

        void init_sensors();
        void time_init();
        void display_init();
        void filesystem_init();
        void serial_init();

        int  next_wake_time();
        void WakeSensors() ;
        void GoToSleep() ;

        void          read_sensors();
        StatRecord_t *current_sensor_data(StatRecord_t *record);
        StatRecord_t *get_last_sensor_data ();

        void  print_all_stat();
        void  print_stat(StatRecord_t *record);
        void  print_fsm_state(const char *func_name, uint32_t line_number);

        bool is_wake_by_deepsleep(esp_sleep_wakeup_cause_t wakeup_reason);
 
    public:
        void init();
        void update();
        void power_safe_logic();
};

#endif
