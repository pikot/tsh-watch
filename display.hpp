#ifndef _DISPLAY_
#define _DISPLAY_

#include <U8g2lib.h>
#include "tsh-watch_menu.h"
#include <TimeLib.h>
#include "utils.h"
#include "stat_records.hpp"

static char *wifi_ap_name = "TshwatchAP";

enum displayActivity_t {
    WATCH_ACTICITY = 1 ,
    ICON_MENU_MAIN_ACTIVITY,
    SETTINGS_ACTIVITY,
    GRAPH_BODY_TEMP_ACTIVITY,
    GRAPH_STEPS_ACTIVITY,
    CONNECT_WIFI_ACTIVITY,
    START_WIFI_AP_ACTIVITY,
};

enum displayAsixType_t {
    SHOW_INT_TYPE,
    SHOW_FLOAT_TYPE,
    SHOW_MAX_TYPE
};

class Graph {
        float   max_v;
        float   min_v;  
        String  title;
        
        time_t            currentDate;
        displayAsixType_t asixType;
        
    public:

        float  graphData[24];
        int    graphDataLen;

        void   process();

        time_t getDate()               { return currentDate; };
        void   setDate(time_t _date)   { currentDate = _date;};
        void   setDate( int _day, int _month, int _year, int _weekday);

        
        void   setData( struct hourStat_t *skim_log,  skimrecord_idx_t idxm, displayAsixType_t _asixType);
        void   setTitle(String _title) { title = _title;};
        String getTitle()              { return title ;};

        float max() {return max_v;};
        float min() {return min_v;};  
        displayAsixType_t  getAsixType() {return asixType;};
        
        bool          starting;
        unsigned int  encoderVal = 100;
};

class Bitmap{
  public:
    int height;
    int width;
    unsigned char *pic;
    void set(unsigned char *pic, int width, int height){
      this->pic = pic;
      this->width = width;
      this->height = height;
    }
};

class Display {
    private:
        const int REPORTING_PERIOD_MS = 1000;
        U8G2 _display;
        
        int screenSizeX = 128;
        int screenSizeY = 64;
        
        int graphStartX = 0;
        int graphStartY = 14;
        
        int graphEndX = screenSizeX - 22;
        int graphEndY = screenSizeY - 6;
        
        int countColumnX = 24;
        int countColumnY = 3;

        displayActivity_t currentActivity;
        Bitmap batteryPic;

        uint32_t tsLastDisplayUpdate = 0;

        void  showBatteryLevel(uint8_t batLevel);
        void  showDigitalClock(int16_t x, int16_t y);
        void  showTemperatureC(int16_t x, int16_t y, float therm, float pulse);
        void  showSteps(int16_t x, int16_t y, uint16_t steps);
        
        void  showWatchActivity(float therm, float pulse, uint16_t steps, uint8_t batLevel);
    public:
        void init();
        void bitmapInitialize();
        
        void update(float therm, float pulse, uint16_t steps, uint8_t batLevel, Graph *graph);
        void setPowerSave(bool powerSave);
        
        displayActivity_t getCurrentActivity()              { return currentActivity;     };
        void setCurrentActivity(displayActivity_t activity) ;//{ currentActivity = activity; };

        void drawCGraph( Graph *graph, boolean Redraw);
        void drawAxises(float _min, float _max, int _column_x, int _column_y, int step_x, int step_y, displayAsixType_t asixType) ;
        void graphDraw(Graph *graph);

        void showSyncTimeMessage(CompletedHandlerFn completedHandler) ;
        void showWifiApMessage(CompletedHandlerFn completedHandler) ;
        void showWifiPortalMessage(CompletedHandlerFn completedHandler) ;
        void showSyncStatDataMessage(CompletedHandlerFn completedHandler);
};

#endif
