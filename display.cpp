//  SPDX-FileCopyrightText: 2020-2021 Ivan Ivanov 
//  SPDX-License-Identifier: GPL-3.0-or-later

// Display
#include "display.hpp"
#include <float.h>

String twoDigits(int digits) 
{
    char str[16];
    int  res = snprintf(str, sizeof(str), "%02d", digits);
    if (res >= sizeof(str) || res <= 0) 
        res = snprintf (str, sizeof(str), "00");
            
    return String(str);
}

unsigned char batteryPicArray [] PROGMEM =  {
 0xff,0x3f,0x01,0xe0,0x49,0x92,0x49,0x92,0x49,0x92,0x49,0x92,
 0x49,0x92,0x01,0xe0,0xff,0x3f
};

//#define image_width  13
//#define image_height 16
unsigned char footprintsPicArray[] PROGMEM = {
 0x00,0xe0,0x30,0xe0,0x3c,0xe0,0x16,0xe0,0xba,0xe1,0xbd,0xe7,
 0x1e,0xed,0x9e,0xeb,0x8e,0xf7,0x1c,0xef,0x1c,0xee,0x38,0xef,
 0x38,0xe7,0x80,0xe3,0xc0,0xe3,0xc0,0xe1
};

unsigned char pressurePicArray[] = {
 0x04,0xe4,0x04,0xe4,0x04,0xe4,0x04,0xe4,0x04,0xe4,0x04,0xe4,
 0x04,0xe4,0x04,0xe4,0x15,0xf5,0x0e,0xee,0x04,0xe4,0x00,0xe0,
 0x38,0xf8,0x6c,0xec,0xc6,0xe6,0x83,0xe3};
 
unsigned char temperaturePicArray[] = {
 0x38,0xf3,0x74,0xf3,0xd4,0xf7,0x74,0xf3,0x54,0xf3,0x74,0xfb,
 0x54,0xff,0xba,0xf6,0x7d,0xf1,0x7d,0xf1,0x7d,0xf1,0xbb,0xf1,
 0xc6,0xf0,0x7c,0xf0};



void Display::init() 
{

   // U8G2_SSD1306_128X64_NONAME_F_HW_I2C _display1(U8G2_R0, /* reset=*/ U8X8_PIN_NONE, /* clock=*/  22, /* data=*/ 21); 
    U8G2_SH1106_128X64_NONAME_F_HW_I2C  _display1(U8G2_R0, U8X8_PIN_NONE, /* clock=*/ 22, /* data=*/ 21 );
    _display = _display1;
    _display.setBusClock(400000);
    print_w("Display::init -- before begin  \n"); // test to see slow  init
    _display.begin(); 
    print_w("Display::init -- after begin  \n"); // test to see sloe init end

   // _display.setPowerSave(true);// kkkkkkK oatoff after test
    tsLastDisplayUpdate = millis();
    currentActivity = WATCH_ACTICITY;

    bitmapInitialize();

   // main_menu.init(_display);
    setupMenu(&_display);

}

void Display::bitmapInitialize(){
    batteryPic.set(batteryPicArray,16, 9);
    footprintsPic.set(footprintsPicArray,13, 16);
    pressurePic.set(pressurePicArray, 13, 16);
    temperaturePic.set(temperaturePicArray, 12, 14);
}

void Display::showDigitalClock(int16_t x, int16_t y) 
{
    String timenow = String(hour()) + ":" + twoDigits(minute());//+":"+twoDigits(second());
    _display.setFont(u8g2_font_logisoso20_tn);//(ArialMT_Plain_24);
    _display.drawStr(x, y , timenow.c_str());
}

const char * weekDays[] = { "", "Sun", "Mon", "Tue", "Wed",
                                "Thu", "Fri", "Sat",
                              };
                              
void Display::showDate(int16_t x, int16_t y) {
    char                buf[32];
    snprintf(buf, sizeof(buf), "%02d/%02d %s", month(), day(), weekDays[weekday()] );  
   
    _display.setFont(u8g2_font_profont10_tr);
    _display.drawStr(x , y, buf);
}
void Display::showTemperatureC(int16_t x, int16_t y, float therm, float pulse) 
{
  
    _display.drawXBMP(x, y - 11, temperaturePic.width, temperaturePic.height, temperaturePic.pic);
    char                buf[32];
    snprintf(buf, sizeof(buf), "%.1f\xB0""C", therm );  
    
    _display.setFont(u8g2_font_courR08_tf);
    _display.drawStr(x + 16, y, buf);
}

void Display::showPressure(int16_t x, int16_t y, float pressure) 
{
    char                buf[32];
    snprintf(buf, sizeof(buf), "%.0f", pressure );  
    _display.drawXBMP(x, y - 12, pressurePic.width, pressurePic.height, pressurePic.pic);

    _display.setFont(u8g2_font_courR08_tf);
    _display.drawStr(x + 16, y, buf);
}

void Display::showSteps(int16_t x, int16_t y, uint16_t steps) 
{
    _display.drawXBMP(x, y - 12, footprintsPic.width, footprintsPic.height, footprintsPic.pic);
    char                buf[32];
    snprintf(buf, sizeof(buf), "%d", steps );

    _display.setFont(u8g2_font_courR08_tf);
    _display.drawUTF8(x + 16, y, buf);
}

void Display::showBatteryLevel(uint8_t batLevel){
//      oled.display_pic(battery_pic.pic, battery_pic.width, battery_pic.height,  true);
  //  int color = light_up? SSD1306_WHITE : SSD1306_BLACK;
  //  _display.setDrawColor(color);

    _display.drawXBMP(112, 0, batteryPic.width, batteryPic.height, batteryPic.pic);
    uint32_t _size = 14;
    int w = (batLevel * _size) / 100;
    _display.drawBox(113, 2, w, 5);
    
   //  printf_w("-showBatteryLevel- %d %, %d\n", batLevel, w);

  //  oled.display_rect(107,2,last_battery_volt_level,5,lightup);
}


void Display::showGraph(int16_t x, int16_t y) 
{    
    uint32_t w =  63, h = 36;
    float buf[CACHE_DISPLAY_CNT];
    uint8_t bufSize = getDisplayStat(buf, CACHE_DISPLAY_CNT);
    
    printf_w("DDDDDDDDDDDDDDDDDDisplay::showGraph -- bufSize %d \n", bufSize); // test to see sloe init end
    if (bufSize <= 0)
        return;
  //  _display.drawFrame(x, y, w, h);
    _display.drawLine(x, y+h, x+w, y+h);


    float maxV = 0, minV = FLT_MAX;
    for (int32_t i = 0; i < bufSize; i++) {
          printf_w("Display::showGraph buf[i] %f,  maxV %f\n",  buf[i],  maxV );
          maxV = max(maxV, buf[i]);
          minV = min (minV, buf[i]);
    }
    if (maxV <= 0) {
          return;
    }
    float maxLen = maxV - minV;
    float graphEndY = y + h;
    float  _pos_x = x + 3, _step = w / bufSize;
    for (int32_t i = 0; i < bufSize; i++) {
           float lenLine = ( (buf[i] - minV )* h) / maxLen;
           float _pos_y_start  = (graphEndY - lenLine);
           float _pos_y_end = graphEndY;
           printf_w("Display::showGraph -- i %d, lenLine %f, _pos_y_start %f, _pos_y_end %f, _pos_x %f \n", i, lenLine, _pos_y_start, _pos_y_end, _pos_x ); // test to see sloe init end

           _display.drawLine(_pos_x, _pos_y_start, _pos_x, _pos_y_end);
           _pos_x += _step;
    }
    
}

void Display::showWatchActivity(float therm, float pulse, uint16_t steps, float pressure, uint8_t batLevel) 
{
    showDigitalClock(0, 21);
    showDate(64, 6);
    showTemperatureC(0, 45, therm, pulse);
    showSteps(0, 60, steps);
    showPressure(75, 60, pressure);
    showGraph(65, 11);
    
    showBatteryLevel(batLevel);
}

const char pgmInfoHeader[] PROGMEM = "Sync watch time ";
const char wifiPortalHeader[] PROGMEM = "Start Wifi Ap ";
const char syncStatHeader[] PROGMEM = "Sync stat data";

char dialogMessage[255];

void Display::showSyncTimeMessage(CompletedHandlerFn completedHandler) 
{
    snprintf(dialogMessage, sizeof(dialogMessage), "Try to get data\nfrom internet..");
    showDialog(pgmInfoHeader, dialogMessage,  completedHandler);
    _display.clear();
    renderer.exec();
}

void Display::showWifiApMessage(CompletedHandlerFn completedHandler) 
{
    snprintf(dialogMessage,sizeof(dialogMessage), "Start WiFi portal\nAp name: %s\nPortal ip: 192.168.4.1", wifi_ap_name);
    showDialog(pgmInfoHeader, dialogMessage, completedHandler );
    _display.clear();
    renderer.exec();
}

void Display::showWifiPortalMessage(CompletedHandlerFn completedHandler) 
{
    snprintf(dialogMessage,sizeof(dialogMessage), "Ap name %s,\nPortal ip: 192.168.4.1", wifi_ap_name );
    showDialog(wifiPortalHeader, dialogMessage, completedHandler );
    _display.clear();
    renderer.exec();
}

void Display::showSyncStatDataMessage(CompletedHandlerFn completedHandler) 
{
    snprintf(dialogMessage,sizeof(dialogMessage), "Send statistics data\nto server..");
    showDialog(syncStatHeader, dialogMessage,  completedHandler);
    _display.clear();
    renderer.exec();
}

void Display::drawAxises(float _min, float _max, int _column_x, int _column_y, int step_x, int step_y, displayAsixType_t asixType)  
{
    float _step = ((float)_max - (float) _min) / (float) countColumnY ;
    float dig = (float) _max;
    int i = 0;
    int _hour = 0;
    int count;

    _display.drawLine(graphStartX, graphStartY, graphStartX, graphEndY);
    _display.drawLine(graphEndX - 1, graphStartY, graphEndX-1, screenSizeY);

    _display.setFont(u8g2_font_tom_thumb_4x6_mn);
    _display.setDrawColor( 1 );

    for (count = graphStartY + step_y/2 , i = 0; i < _column_y; count += step_y, dig -= (float)_step, i += 1) {
        _display.drawLine(graphEndX, count,   graphEndX - 2, count);
        _display.drawLine(graphStartX, count, graphStartX + 2, count);
        char buf[16];
        _display.setCursor(graphEndX + 1, count + 3 );
        if (SHOW_INT_TYPE == asixType) 
            snprintf(buf, sizeof(buf), "%d", (int) dig);
        else 
            snprintf(buf, sizeof(buf), "%.1f", dig);
        
        _display.print(buf);
    //    _display.drawStr(5, count - 3, dig);
    } 
    // time axis
    for (count = graphStartX + step_x/2, _hour = 0; _hour < _column_x; count += step_x, _hour += 1) {
        _display.drawPixel(count, graphStartY + 1);
        _display.drawPixel(count, graphEndY - 1);

        _display.setCursor(count-2, graphEndY + 7  );
        if (_hour % 2 )
            _display.print(_hour+1);
    }
}

void Display::drawCGraph(Graph *graph, boolean Redraw) 
{
    if (!graph)
        return;

    double i;
    double temp;
    int _i = 0; 
    int count = 0;
    int _column_x = graph->graphDataLen,  _column_y = countColumnY;
    int step_x =  (graphEndX - graphStartX) / (_column_x);
    int step_y =  (graphEndY - graphStartY) / (_column_y); 
    float graph_v = graph->max() - graph->min();
    
    printf_w("-- graph->graphDataLen %d, min %f,  max %f\n\n", graph->graphDataLen, graph->min() , graph->max());

    Serial.flush();

    _display.drawBox(0, 0,  127 , 14);
    _display.setFont(u8g2_font_t0_11b_tf);
    _display.setDrawColor(0);
    _display.drawStr(3, 13, graph->getTitle().c_str());

    _display.setDrawColor(1);

    for (count = graphStartX + step_x/2, _i = 0; _i < graph->graphDataLen; count += step_x, _i++) {
           float len_line = ((graphEndY - 5 - graphStartY) *  (graph->graphData[_i] - graph->min()))  / graph_v;
           float _pos_y_start  = (graphEndY - len_line - 5);
           float _pos_y_end = screenSizeY - 5;

           _display.drawLine(count, _pos_y_start, count, _pos_y_end);
    }
    
    drawAxises(graph->min() , graph->max(), graph->graphDataLen, countColumnY, step_x, step_y, graph->getAsixType());
}

void Display::setCurrentActivity(displayActivity_t activity) 
{
     currentActivity = activity; 
//     printf_w("---  setCurrentActivity %d\n", activity);
}

void Display::update(float therm, float pulse, uint16_t steps, float pressure, uint8_t batLevel, Graph *graph)
{
   // if (millis() - tsLastDisplayUpdate <= REPORTING_PERIOD_MS) 
   //     return;

    tsLastDisplayUpdate = millis();
    
    if (WATCH_ACTICITY == currentActivity) {
        _display.clearBuffer();         // clear the internal memory
        showWatchActivity( therm,  pulse,  steps, pressure, batLevel);
        _display.sendBuffer();          // transfer internal memory to the display
    }
    else
    if (ICON_MENU_MAIN_ACTIVITY == currentActivity ||
       GRAPH_STEPS_ACTIVITY == currentActivity ||
       GRAPH_BODY_TEMP_ACTIVITY == currentActivity) {
      //MENUDRAW_COMPLETE_REDRAW;

       // redrawMode = MENUDRAW_COMPLETE_REDRAW;
        taskManager.runLoop();
    } 
}

void  Display::graphDraw(Graph *graph)
{
    _display.clearBuffer();         // clear the internal memory
    drawCGraph(graph, true);
    _display.sendBuffer();          // transfer internal memory to the display
}

void Display::setPowerSave(bool powerSave)
{
    _display.setPowerSave(powerSave);// kkkkkkK oatoff after test
}

//-----------


void Graph::process() 
{
    printf_w("Graph::process start\n\n");

    max_v = 0;

    for (int i = 0; i < graphDataLen; i++) {
        max_v = graphData[i] > max_v ? graphData[i] : max_v;
    }
    min_v = max_v;
    for (int i = 0; i < graphDataLen; i++) {
        min_v = graphData[i] < min_v ? ((graphData[i] == 0)? min_v : graphData[i]) : min_v;
    }
    printf_w("Graph::process end\n");
}

void  Graph::setDate(int _day, int _month, int _year, int _weekday)
{
    tmElements_t tme;
    tme.Second = 0;
    tme.Minute = 0; 
    tme.Hour  = 0; 
    tme.Wday  = _weekday;   // day of week, sunday is day 1
    tme.Day   = _day;
    tme.Month = _month;  // Jan = 1 in tme, 0 in tm
    tme.Year  = (uint8_t) _year - 1970 ; // offset from 1970; 
  
    currentDate = makeTime(tme);

    printf_w("---  setDate %d, _day %d,  _month %d,  _year %d  \n", currentDate,   _day,  _month,  _year);
}

void  Graph::setData(struct hourStat_t *skim_log, skimrecord_idx_t idx, displayAsixType_t _asixType)
{
    if (skim_log->hoursInTotal <= 0) {
        graphDataLen = 0;
        return;
    }
    asixType = _asixType;
    graphDataLen = 0;
    memset(graphData, 0, sizeof(graphData));
    printf_w("Graph::setData ----  hoursInTotal %d\n", skim_log->hoursInTotal);

    for (int i = 0; i < skim_log->hoursInTotal; i++) {
       //   skim_log->byHours[i].print();
          graphData[i]  = skim_log->byHours[i].getField(idx);
          printf_w("idx %d, i %d, data %f\n", idx, i,  graphData[i]);
          graphDataLen += 1;
    }
    printf_w("Graph::setData ----  graphDataLen %d\n", graphDataLen );
    
    process();
}
