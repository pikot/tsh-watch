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

 unsigned char humidityPicArray[] = {
 0x40,0xe0,0xe0,0xe0,0x10,0xe1,0x08,0xe2,0x04,0xe4,0x02,0xe8,
 0x02,0xe8,0x01,0xf0,0x01,0xf0,0x01,0xf0,0x05,0xf0,0x09,0xf0,
 0x3a,0xf8,0xe6,0xec,0x1c,0xe6,0xf0,0xe1};

void Display::init() 
{
     init(false);
}

void Display::init(bool isAfterDeepsleep) 
{
#ifdef IS_V1_4
    print_w("Display::SSD1681 \n"); // test to see slow  init

    U8G2_SSD1681_200X200_F_4W_SW_SPI _display1(U8G2_R3, /* clock=*/ ELINK_CL, /* data=*/ ELINK_MOSI, /* cs=*/ ELINK_SS, /* dc=*/ ELINK_DC, /* reset=*/ ELINK_RESET, ELINK_BUSY);    
#else
    print_w("Display::SH1106 \n"); // test to see slow  init

    U8G2_SH1106_128X64_NONAME_F_HW_I2C  _display1(U8G2_R0, U8X8_PIN_NONE, /* clock=*/ 22, /* data=*/ 21 );
    _display1.setBusClock(400000);
#endif
    _display = _display1;

    print_w("Display::init -- before begin  \n"); // test to see slow  init
#ifdef IS_V1_4
    if (isAfterDeepsleep) {
        _display.initPartialDisplay();
        _display.setPowerSave(0);
    } else {
        _display.begin(); 
    }
#else
    _display.begin(); 
#endif
    print_w("Display::init -- after begin  \n"); // test to see sloe init end

   // _display.setPowerSave(true);// kkkkkkK oatoff after test
    tsLastDisplayUpdate = millis();
    currentActivity = WATCH_ACTICITY;

    bitmapInitialize();
#ifdef IS_V1_4
    mScreen = new mainScreen200x200(&_display);
#else 
    mScreen = new mainScreen128x64(&_display);
#endif
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


screenEntity::screenEntity(U8G2 *_display, int _x, int _y, const uint8_t *_font,  
                           unsigned char *_picArray, uint8_t _picW, uint8_t _picH): 
                           display(_display), x(_x), y(_y), font(_font){
    if ( _picArray ) {
        bmp = new Bitmap();
        bmp->set(_picArray, _picW, _picH);
    }
    else 
        bmp = NULL;
}
                          
screenEntity::~screenEntity(){
    if (bmp)
        delete(bmp);
}

void screenEntity::draw(char *str) {
    if (bmp) {
        display->drawXBMP(x, y - (bmp->height - bmp->height/4) , bmp->width, bmp->height, bmp->pic);
    }
    if (str) {
        display->setFont(font);
        uint16_t shift = 0;
        if (bmp) {
            shift = bmp->width + 2;
        }
        display->drawStr(x + shift, y, str);
    }
}

void screenEntity::drawBmp(int x, int y) {
    if (bmp) {
        display->drawXBMP(x, y, bmp->width, bmp->height, bmp->pic);
    }
}


void dateScreenEntity::draw() {
    char                buf[32];
    snprintf(buf, sizeof(buf), "%02d/%02d %s", month(), day(), weekDays[weekday()] );  
    screenEntity::draw(buf);
}

void timeScreenEntity::draw() {
    char                buf[16];
    snprintf(buf, sizeof(buf), "%d:%02d", hour(), minute() );  
    screenEntity::draw(buf);
}

void batteryScreenEntity::draw(uint8_t batLevel) {   
      display->drawXBMP(x, y, bmp->width, bmp->height, bmp->pic);
 
    uint32_t _size = 14;
    int w = (batLevel * _size) / 100;
    display->drawBox(x + 1, y + 2, w, 5);
    printf_w("-showBatteryLevel- %d %, %d\n", batLevel, w);
}

void stepScreenEntity::draw(uint16_t steps) {
    char                buf[32];
    snprintf(buf, sizeof(buf), "%d", steps );
    screenEntity::draw(buf);
}

void tempScreenEntity::draw(float therm) {
    char                buf[32];
    snprintf(buf, sizeof(buf), "%.1f\xB0""C", therm );  
    screenEntity::draw(buf);
}

void presScreenEntity::draw(float pressure) {
    char                buf[32];
    snprintf(buf, sizeof(buf), "%.0f", pressure);  
    screenEntity::draw(buf);
}

void humScreenEntity::draw(float humidity) {
    char                buf[32];
    snprintf(buf, sizeof(buf), "%.0f%%", humidity);  
    screenEntity::draw(buf);
}

void graphScreenEntity::draw(uint16_t w,  uint16_t h ) {
  
    float buf[CACHE_DISPLAY_CNT];
    uint8_t bufSize = getDisplayStat(buf, CACHE_DISPLAY_CNT);
    
    //printf_w("DDDDDDDDDDDDDDDDDDisplay::showGraph -- bufSize %d \n", bufSize); // test to see sloe init end
    if (bufSize <= 0)
        return;
  //  _display.drawFrame(x, y, w, h);
    uint32_t digit_shift = 15;
    display->drawLine(x, y+h, x + w - digit_shift, y+h);
    if (bmp) {
        display->drawXBMP(x + w - bmp->width, y - bmp->height - 5, bmp->width, bmp->height, bmp->pic);
    }

    float maxV = 0, minV = FLT_MAX;
    for (int32_t i = 0; i < bufSize; i++) {
       //   printf_w("Display::showGraph buf[i] %f,  maxV %f\n",  buf[i],  maxV );
          maxV = max(maxV, buf[i]);
          minV = min (minV, buf[i]);
    }
    if (maxV <= 0) {
          return;
    }

    float maxLen = maxV - minV;
    float graphEndY = y + h;
    float  _pos_x = x + 3, _step = w / bufSize;

    display->setFont(u8g2_font_blipfest_07_tn);
    display->setDrawColor( 1 );
    int countColumnY = 2;
    float step_V =  maxLen / countColumnY; 
    int step_y =  h / countColumnY; 

    float dig = (float) maxV, count = 0 ;
    int i = 0;
    for (count = y, i = 0; i <= countColumnY; count += step_y, dig -= (float)step_V  , i += 1) {
     //   display->drawLine(_pos_x + w , count,   x + w - 2, count);
        display->drawLine(x, count, x + 2, count);
        display->setCursor(x + w - digit_shift , count + 3 );
        char buf[16];
        snprintf(buf, sizeof(buf), "%.1f", dig);
        display->print(buf);
    } 

    
    for (int32_t i = 0; i < bufSize; i++) {
           float lenLine = ( (buf[i] - minV )* h) / maxLen;
           float _pos_y_start  = (graphEndY - lenLine);
           float _pos_y_end = graphEndY;
        //   printf_w("Display::showGraph -- i %d, lenLine %f, _pos_y_start %f, _pos_y_end %f, _pos_x %f \n", i, lenLine, _pos_y_start, _pos_y_end, _pos_x ); // test to see sloe init end

           display->drawLine(_pos_x, _pos_y_start, _pos_x, _pos_y_end);
           _pos_x += _step;
    }    
}

mainScreen128x64::mainScreen128x64(U8G2 *_display) {
    display = _display;
    
    graphW = 63; 
    graphH = 36;
    Date    = new dateScreenEntity(_display, 64, 6, u8g2_font_profont10_tr, NULL, 0, 0) ;
    Time    = new timeScreenEntity(_display, 0, 21, u8g2_font_logisoso20_tn, NULL, 0, 0);
    Battery = new batteryScreenEntity(_display, 112, 0, NULL, batteryPicArray, 16, 9);
    Steps   = new stepScreenEntity(_display, 0, 60, u8g2_font_courR08_tf, footprintsPicArray, 13, 16);
    Temperature = new tempScreenEntity(_display, 0, 45, u8g2_font_courR08_tf, temperaturePicArray, 12, 14);
    Pressure = new presScreenEntity(_display, 75, 60, u8g2_font_courR08_tf, pressurePicArray, 13, 16);
    Graph    = new graphScreenEntity(_display, 65, 11, NULL, 0, 0);
}

mainScreen200x200::mainScreen200x200(U8G2 *_display) {
    display = _display;

    graphW = 199; 
    graphH = 100;
    //TODO create bmp separately 
    Date    = new dateScreenEntity(_display, 105, 10, u8g2_font_DigitalDisco_tr, NULL, 0, 0) ;
    Time    = new timeScreenEntity(_display, 0, 35, u8g2_font_logisoso34_tn, NULL, 0, 0);
    Battery = new batteryScreenEntity(_display, 180, 0, NULL, batteryPicArray, 16, 9);
    uint16_t personDataY = 186;
    Steps   = new stepScreenEntity(_display, 0, personDataY, u8g2_font_DigitalDisco_tu, footprintsPicArray, 13, 16);
    Temperature = new tempScreenEntity(_display, 65, personDataY, u8g2_font_DigitalDisco_te, temperaturePicArray, 12, 14);
    
    Pressure = new presScreenEntity(_display, 130, 197, u8g2_font_DigitalDisco_tu, pressurePicArray, 13, 16);
    Humidity = new humScreenEntity(_display, 130, 180, u8g2_font_DigitalDisco_tu, humidityPicArray, 13, 16);

    Graph    = new graphScreenEntity(_display, 0, 60, temperaturePicArray, 12, 14);
}

mainScreen::~mainScreen(){
    delete(Date);
    delete(Time);
    delete(Battery);
    delete(Steps);
    delete(Temperature);
    delete(Pressure);
    delete(Graph);
    if (Humidity) {
        delete(Humidity);
    }
}

void mainScreen::draw(float therm, float pulse, uint16_t steps, float pressure, uint8_t humidity, uint8_t batLevel){
    Date->draw();
    Time->draw();
    Battery->draw(batLevel);
    Steps->draw(steps);
    Temperature->draw(therm);
    Pressure->draw(pressure);
    if (Humidity) {
          Humidity->draw(humidity);
    }
    Graph->draw(graphW, graphH);
}
/*
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
    _display.drawXBMP(112, 0, batteryPic.width, batteryPic.height, batteryPic.pic);
    uint32_t _size = 14;
    int w = (batLevel * _size) / 100;
    _display.drawBox(113, 2, w, 5);
    
     printf_w("-showBatteryLevel- %d %, %d\n", batLevel, w);

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
       //   printf_w("Display::showGraph buf[i] %f,  maxV %f\n",  buf[i],  maxV );
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
        //   printf_w("Display::showGraph -- i %d, lenLine %f, _pos_y_start %f, _pos_y_end %f, _pos_x %f \n", i, lenLine, _pos_y_start, _pos_y_end, _pos_x ); // test to see sloe init end

           _display.drawLine(_pos_x, _pos_y_start, _pos_x, _pos_y_end);
           _pos_x += _step;
    }    
}
*/

void Display::showWatchActivity(float therm, float pulse, uint16_t steps, float pressure, uint8_t humidity, uint8_t batLevel) 
{
  /*
    showDigitalClock(0, 21);
    showDate(64, 6);
    showTemperatureC(0, 45, therm, pulse);
    showSteps(0, 60, steps);
    showPressure(75, 60, pressure);
    showGraph(65, 11);
    
    showBatteryLevel(batLevel);
*/
    if (mScreen) {
         mScreen->draw(therm,  pulse,  steps,  pressure, humidity, batLevel);
    }
    
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

void Display::update(float therm, float pulse, uint16_t steps, float pressure, uint8_t humidity, uint8_t batLevel, Graph *graph)
{
   // if (millis() - tsLastDisplayUpdate <= REPORTING_PERIOD_MS) 
   //     return;

    tsLastDisplayUpdate = millis();
    
    if (WATCH_ACTICITY == currentActivity) {
      /*
        _display.clearBuffer();         // clear the internal memory
        showWatchActivity( therm,  pulse,  steps, pressure, batLevel);
        _display.sendBuffer();          // transfer internal memory to the display
*/
        
        _display.firstPage();
        do {
              showWatchActivity( therm,  pulse,  steps, pressure, humidity, batLevel);
        } while ( _display.nextPage() );
        delay(1000);
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
