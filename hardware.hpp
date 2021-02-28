#ifndef _HARDWARE_
#define _HARDWARE_

#include "sensors.hpp"
#include <TimeLib.h>

//#include "icon_menu.h"
#include <ArduinoJson.h>

#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
#include "sensors.hpp"
#include "stat_records.hpp"
#include "stat_server.hpp"

#include "display.hpp"
#include "ulp_main.h"

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
    CONTROL_WIFI_INIT,
};
static const char * control_state_name[] = {"CONTROL_GOTOSLEEP", "CONTROL_SLEEPING", "CONTROL_AWAKE", "CONTROL_WIFI_INIT"};

static const char * espSleepWake[] = {
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

#define  IS_GRAPH_ACTIVITY(act)  ( GRAPH_BODY_TEMP_ACTIVITY == act || GRAPH_STEPS_ACTIVITY == act ) 

class Control {
    private:        
        uint64_t _Button_State      = 0;
        uint64_t _Button_PrevState  = 0;
        gpio_num_t LEFT_BUTTON          = GPIO_NUM_14;
        gpio_num_t RIGHT_BUTTON         = GPIO_NUM_33; // in schema 33 return after test
        gpio_num_t OK_BUTTON            = GPIO_NUM_27;       


        void printButtonState (const char *func_name, uint32_t line_number, int l_State, int r_State, int o_State );
    public:

        void init();
        bool buttonPressed();
        void initWakeup();

        gpio_num_t pinOk( ) { return OK_BUTTON;};
        gpio_num_t pinLeft( ) { return LEFT_BUTTON;};
        gpio_num_t pinRight( ) { return RIGHT_BUTTON;};

        uint64_t buttons() { return _Button_State;};
};

#define UID_SIZE 32
#define TOKEN_SIZE 64
#define SERVER_ADDR_SIZE 64

class Config {
    private:
         bool temp_sensor_switch;
         bool step_sensor_switch;
         int8_t timezone;
         bool auto_update_time_over_wifi;
         
         char server_uid[UID_SIZE];
         char server_token[TOKEN_SIZE];
         char server_addr[SERVER_ADDR_SIZE];


         char *filename = "/config.txt";
         bool need_save = false;
         void updateUlpConfig();
    public:
        void init();
        void load();
        void save();

        void setSaveFlag(bool _save) { need_save = _save;}

        void setToMenu();
        void cast_from_menu();
        void setCurrenDayTimeToMenu();


        void setTempSensorSwitch(bool val) {  temp_sensor_switch = val;}
        void setStepSensorSwitch(bool val) {  step_sensor_switch = val;}
        void setServerUid(const char* u) ;
        void setServerToken(const char * t);
        void setServerAddr(const char * a);
        void setTimezone(int8_t val) { timezone = val;}

        bool getTempSensorSwitch(){ return temp_sensor_switch;}
        bool getStepSensorSwitch(){ return step_sensor_switch;}
        char *getServerUid();
        char *getServerToken();
        char *getServerAddr();
        int   getTimezone() { return timezone;}         
};

void saveWifiConfigCallback();

class Hardware {
    private:
        bool            use_wifi = false;

        //int32_t         wifi_stage;
        Config          cfg;

        Gyroscope       sGyro;
        ThermoMeter     sTerm;
        PulseMeter      sPulse;
        CurrentMeter    sCurrent;

        TimeMeter       sTime;
        Display         display;
        Control         control;
        FileSystem      log_file;
        WiFiManager     wm;

        WiFiManagerParameter *api_uid_server = NULL;
        WiFiManagerParameter *api_key_server = NULL;
        WiFiManagerParameter *api_addr_server = NULL;

        time_t          currentTime;

        bool isAfterDeepsleep = false;
        esp_sleep_wakeup_cause_t wakeupReason;
     
        uint8_t  pulse_threshold   = 3; 
        uint32_t displaySleepDelay = 4000; //
        uint32_t displaySleepTimer = 0;
        Graph  graph;

        bool debugTraceFsm = false;
        bool powerSave = false;

        void initSensors();
        
        void timeInit();
        void serialInit();

        int  nextWakeTime();
        void goToSleep() ;

        void          readSensors();
        statRecord_t *getCurrentSensorsData(statRecord_t *record);
        statRecord_t *getLastSensorsData ();

        void  printAllStat();
        void  printStat(statRecord_t *record);
        void  printFsmState(const char *func_name, uint32_t line_number);

        bool isWakeByDeepsleep(esp_sleep_wakeup_cause_t wakeupReason);
        bool isNewDay();

        void runActivity(uint64_t buttons);
        void updateFromConfig();

        void prepareGraphData(displayActivity_t act, int32_t _day,  int32_t _month,  int32_t _year );

        struct tm *  changeDayFile(int32_t _day);
        struct tm *  loadNextDayFile();
        struct tm *  loadPrevDayFile();
        
        void getTimeOverWifi();
        
        void updateSkimlog();
        struct tm *  takeLogDayFile(unsigned int curValue);
    public:
        void init();
        void update();
        void runPowerSafe();

// menu logic
        void setTime( TimeStorage tm);
        void setDate( DateStorage dt);
        void setTimezone( int tz , bool need_save);
        
        void setTempSwitch(bool val, bool need_save); 
        void setPedoSwitch(bool val, bool need_save);
        
        void showActivity(displayActivity_t act);
        
        void updateWebApiConfig();
        void showWifiPortal();
        void syncTimeViaWifi();
        void syncStatViaWifi();
  
        void start_graph_logic(displayActivity_t _activity, unsigned int curValue);

        void setGraphStarting(bool val) { graph.starting = val; };
        bool getGraphStarting() { return graph.starting; };
        void showGraph();
};


#endif
