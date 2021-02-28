//============================================================================================
// Display
#include "display.hpp"

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

void Display::init() 
{
   // U8G2_SSD1306_128X64_NONAME_F_HW_I2C _display1(U8G2_R0, /* reset=*/ U8X8_PIN_NONE, /* clock=*/  22, /* data=*/ 21); 
    U8G2_SH1106_128X64_NONAME_F_HW_I2C  _display1(U8G2_R0, U8X8_PIN_NONE, /* clock=*/ 22, /* data=*/ 21 );

    _display = _display1;
    _display.begin(/*Select=*/ GPIO_NUM_27, /*Right/Next=*/ GPIO_NUM_33, /*Left/Prev=*/ GPIO_NUM_14, /*Up=*/ U8X8_PIN_NONE, /*Down=*/ U8X8_PIN_NONE, /*Home/Cancel=*/ U8X8_PIN_NONE); 
    _display.setBusClock(400000);

   // _display.setPowerSave(true);// kkkkkkK oatoff after test
    tsLastDisplayUpdate = millis();
    currentActivity = WATCH_ACTICITY;
    bitmapInitialize();

   // main_menu.init(_display);
    setupMenu(&_display);
}

void Display::bitmapInitialize(){
    batteryPic.set(batteryPicArray,16, 9);
}

void Display::showDigitalClock(int16_t x, int16_t y) 
{
    String timenow = String(hour()) + ":" + twoDigits(minute());//+":"+twoDigits(second());
    _display.setFont(u8g2_font_freedoomr25_tn);//(ArialMT_Plain_24);
    _display.drawStr(x, y , timenow.c_str());
}

void Display::showTemperatureC(int16_t x, int16_t y, float therm, float pulse) 
{
    String temp_str1 =  String(therm) + "*C ";
    String temp_str2 =  String(pulse) + "bpm";

    _display.setFont(u8g2_font_helvR08_te);
    _display.drawStr(x, 62, temp_str1.c_str());
  //  _display.drawStr(x, 32, temp_str2.c_str());
}

void Display::showSteps(int16_t x, int16_t y, uint16_t steps) 
{
    String steps_str = "steps: " + String (steps) ;

    _display.setFont(u8g2_font_helvR08_te);
    _display.drawUTF8(x, y, steps_str.c_str());
}

void Display::showBatteryLevel(uint8_t batLevel){
//      oled.display_pic(battery_pic.pic, battery_pic.width, battery_pic.height,  true);
  //  int color = light_up? SSD1306_WHITE : SSD1306_BLACK;
  //  _display.setDrawColor(color);

    _display.drawXBMP(105, 0, batteryPic.width, batteryPic.height, batteryPic.pic);
    uint32_t _size = 14;
    int w = (batLevel * _size) / 100;
    _display.drawBox(106, 2, w, 5);
    
     printf_w("-showBatteryLevel- %d %, %d\n", batLevel, w);

  //  oled.display_rect(107,2,last_battery_volt_level,5,lightup);
}

void Display::showWatchActivity(float therm, float pulse, uint16_t steps, uint8_t batLevel) 
{
    showDigitalClock(0, 32);
    showTemperatureC(90, 64, therm, pulse);
    showSteps(0, 64, steps);
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

void Display::update(float therm, float pulse, uint16_t steps, uint8_t batLevel, Graph *graph)
{
   // if (millis() - tsLastDisplayUpdate <= REPORTING_PERIOD_MS) 
   //     return;

    tsLastDisplayUpdate = millis();
    
    if (WATCH_ACTICITY == currentActivity) {
        _display.clearBuffer();         // clear the internal memory
        showWatchActivity( therm,  pulse,  steps, batLevel);
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
