//  SPDX-FileCopyrightText: 2020-2021 Ivan Ivanov 
//  SPDX-License-Identifier: GPL-3.0-or-later

#ifndef _DISPLAY_
#define _DISPLAY_

#include <U8g2lib.h>
#include "tsh-watch_menu.h"
#include <TimeLib.h>
#include "utils.h"
#include "version.h"
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


#define ELINK_SS 5
#define ELINK_BUSY 4
#define ELINK_RESET 10
#define ELINK_DC 9
#define ELINK_CL 18
#define ELINK_MOSI 23
/*
class icoBmp {
    private:
        uint8_t *Array;
        uint8_t W;
        uint8_t H;
        Bitmap  bmp;
   public:
        icoBmp(uint8_t *_Array, uint8_t _W, uint8_t _H);
        Bitmap *getBmp() {return &bmp;}
};
*/
class screenEntity {
    protected:
        U8G2 *display;
        int x; 
        int y;
        const uint8_t *font;
        Bitmap  *bmp;
    public:
        screenEntity(U8G2 *_display, int _x, int _y, const uint8_t *_font,  unsigned char *_picArray, uint8_t _picW, uint8_t _picH);
        ~screenEntity();
        void draw(char *str);
        void drawBmp(int x, int y);
};


class dateScreenEntity : public screenEntity {
    public:
        dateScreenEntity(U8G2 *_display, int _x, int _y, const uint8_t *_font,  unsigned char *_picArray, uint8_t _picW, uint8_t _picH):
                       screenEntity(_display, _x, _y, _font, _picArray, _picW,  _picH){}
                         
                                 
        void draw();
};

class timeScreenEntity : public screenEntity {
    public:
        timeScreenEntity(U8G2 *_display, int _x, int _y, const uint8_t *_font,  unsigned char *_picArray, uint8_t _picW, uint8_t _picH):
                       screenEntity(_display, _x, _y, _font, _picArray, _picW,  _picH){}
        void draw();
};

class batteryScreenEntity : public screenEntity {
    public:
        batteryScreenEntity(U8G2 *_display, int _x, int _y, const uint8_t *_font,  unsigned char *_picArray, uint8_t _picW, uint8_t _picH):
                       screenEntity(_display, _x, _y, _font, _picArray, _picW,  _picH){}
        void draw(uint8_t batLevel);
};

class stepScreenEntity : public screenEntity {
    public:
        stepScreenEntity(U8G2 *_display, int _x, int _y, const uint8_t *_font,  unsigned char *_picArray, uint8_t _picW, uint8_t _picH):
                       screenEntity(_display, _x, _y, _font, _picArray, _picW,  _picH){}
        void draw(uint16_t steps);
};

class tempScreenEntity : public screenEntity {
    public:
        tempScreenEntity(U8G2 *_display, int _x, int _y, const uint8_t *_font,  unsigned char *_picArray, uint8_t _picW, uint8_t _picH):
                       screenEntity(_display, _x, _y, _font, _picArray, _picW,  _picH){}
        void draw(float therm);
};

class presScreenEntity : public screenEntity {
    public:
        presScreenEntity(U8G2 *_display, int _x, int _y, const uint8_t *_font,  unsigned char *_picArray, uint8_t _picW, uint8_t _picH):
                       screenEntity(_display, _x, _y, _font, _picArray, _picW,  _picH){}
        void draw(float pressure);
};

class graphScreenEntity : public screenEntity {
    public:
        graphScreenEntity(U8G2 *_display, int _x, int _y,  unsigned char *_picArray, uint8_t _picW, uint8_t _picH):
                       screenEntity(_display, _x, _y, NULL, _picArray, _picW,  _picH){}
        void draw(uint16_t w,  uint16_t h );
};

class humScreenEntity : public screenEntity {
    public:
        humScreenEntity(U8G2 *_display, int _x, int _y, const uint8_t *_font,  unsigned char *_picArray, uint8_t _picW, uint8_t _picH):
                       screenEntity(_display, _x, _y, _font, _picArray, _picW,  _picH){}
        void draw(float pressure);
};

class mainScreen {
    protected:
        U8G2 *display;
        uint16_t graphW;
        uint16_t graphH;
    
        dateScreenEntity *Date;
        timeScreenEntity *Time;
        batteryScreenEntity *Battery;

        stepScreenEntity *Steps;
        tempScreenEntity *Temperature;
        presScreenEntity *Pressure;
        humScreenEntity  *Humidity;

        graphScreenEntity *Graph;
    public:
      //  mainScreen(U8G2 *_display){}
        ~mainScreen();
        
        void draw(float therm, float pulse, uint16_t steps, float pressure, uint8_t humidity, uint8_t batLevel);

};


class mainScreen128x64 : public mainScreen {
    public:
        mainScreen128x64(U8G2 *_display);

};

class mainScreen200x200 : public mainScreen {
    public:
        mainScreen200x200(U8G2 *_display);

};

class Display {
    private:
        const int REPORTING_PERIOD_MS = 1000;
        U8G2 _display;
#ifdef IS_V1_4
        int screenSizeX = 200;
        int screenSizeY = 200;
#else
        int screenSizeX = 128;
        int screenSizeY = 64;
#endif   
        int graphStartX = 0;
        int graphStartY = 14;
        
        int graphEndX = screenSizeX - 22;
        int graphEndY = screenSizeY - 6;
        
        int countColumnX = 24;
        int countColumnY = 3;

        displayActivity_t currentActivity;
        Bitmap batteryPic;
        Bitmap footprintsPic;
        Bitmap pressurePic;
        Bitmap temperaturePic;

        mainScreen *mScreen;

        uint32_t tsLastDisplayUpdate = 0;

        void  showBatteryLevel(uint8_t batLevel);
        void  showDigitalClock(int16_t x, int16_t y);
        void  showDate(int16_t x, int16_t y);
        void  showTemperatureC(int16_t x, int16_t y, float therm, float pulse);
        void  showSteps(int16_t x, int16_t y, uint16_t steps);
        void  showPressure(int16_t x, int16_t y, float pressure);
        void  showGraph(int16_t x, int16_t y);
        
        void  showWatchActivity(float therm, float pulse, uint16_t steps, float pressure, uint8_t humidity, uint8_t batLevel);
    public:
        void init();
        void init(bool isAfterDeepsleep);
        
        void bitmapInitialize();
        
        void update(float therm, float pulse, uint16_t steps, float pressure, uint8_t humidity, uint8_t batLevel, Graph *graph);
        void setPowerSave(bool powerSave);
        
        displayActivity_t getCurrentActivity()              { return currentActivity; };
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
