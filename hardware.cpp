//  SPDX-FileCopyrightText: 2020-2021 Ivan Ivanov 
//  SPDX-License-Identifier: GPL-3.0-or-later

#include "hardware.hpp"

RTC_DATA_ATTR uint8_t SensorState = SENSOR_AWAKE;
RTC_DATA_ATTR uint8_t ControlState = CONTROL_AWAKE;

RTC_DATA_ATTR time_t nextScanTime = 0;

RTC_DATA_ATTR int8_t  today  = 0;

RTC_DATA_ATTR time_t   nextAttemptStatSyncTime = 0; 
RTC_DATA_ATTR uint8_t  lastSkimlogUpdateHour = 0;

void printPinStatus() 
{
      for (int i = 0; i < 20; i++) { 
           int _State = digitalRead(i);

           print_w("status: pin ");
           print_w(i);     print_w(" - "); print_w(_State);     println_w("");  
      }
}

void Hardware::serialInit()
{
#ifdef _SERIAL_DEBUG_
    Serial.begin(115200);
    Serial.flush();
#endif
}

void Hardware::timeRead()
{
    sTime.read();

    currentTime = sTime.currentTime();
}

bool Hardware::isWakeByDeepsleep(esp_sleep_wakeup_cause_t wakeupReason) 
{
    bool status = false;
    switch (wakeupReason) {
        case 1  : 
        case 2  : 
        case 3  : 
        case 4  : 
        case 5  :
            status = true;
            break;
        default : status = false; break;
    }
    return status;
}

bool Hardware::isNewDay()
{
    if ( today != day() ) {
         today = day();
         return true;
    }
    return false;
}

void Hardware::init() 
{
    setCpuFrequencyMhz(80);
    esp_deep_sleep_disable_rom_logging();

    displaySleepTimer = millis();

    wakeupReason      = esp_sleep_get_wakeup_cause();
    isAfterDeepsleep  = isWakeByDeepsleep(wakeupReason);

    serialInit();
    SPIFFS.begin(true); // need for logs or config 
    printf_w("fs stat: totalBytes %d,  usedBytes %d\n", SPIFFS.totalBytes(), SPIFFS.usedBytes());

    cfg.init(); // cfg -> settings

    updateFromConfig(); // cfg -> hardware///  yep, pretty sheety and best case scenario is to have one place to sync directly
    
    printFsmState( __func__, __LINE__ );
    if (false == isAfterDeepsleep) {
        memset(&RTC_RECORDS, 0, sizeof(statRecord_t));
        uploadUlpProgram();
        initSensors();
        initUlpProgram();
        updateFromConfig(); // cfg -> hardware///  yep, pretty sheety and best case scenario is to have one place to sync directly
        runUlp();
    }

   // print_all_stat(); //!!!!!!!!!!!!!!!!!!
    printf_w("wakeupReason %s, statRecord_cnt %d\n",  espSleepWake[wakeupReason], statRecord_cnt );

    if (CONTROL_SLEEPING == ControlState && ESP_SLEEP_WAKEUP_EXT0 == wakeupReason)
         ControlState = CONTROL_AWAKE;

    timeRead();

    if (SENSOR_SLEEPING == SensorState && nextScanTime <= currentTime) 
         SensorState = SENSOR_AWAKE;

    if (CONTROL_AWAKE == ControlState) {
#ifndef IS_V1_4
        display.init();
#endif
        control.init();
    }

#ifdef IS_V1_4
    if (CONTROL_AWAKE == ControlState || SENSOR_AWAKE == SensorState ) {
        display.init(isAfterDeepsleep);
    }
#endif

    printFsmState( __func__, __LINE__);    
}

char swTxBuffer[32];
char swRxBuffer[32];

void Hardware::initSensors() 
{
    ulpI2c = new SoftWire(ulpSdaPin, ulpSclPin);

    ulpI2c->setTxBuffer(swTxBuffer, sizeof(swTxBuffer));
    ulpI2c->setRxBuffer(swRxBuffer, sizeof(swRxBuffer));
    ulpI2c->setTimeout_ms(40);
    ulpI2c->begin();
    ulpI2c->setTimeout_ms(100);

    printf_w("initSensors: ulpSdaPin %d, ulpSclPin %d\n", ulpSdaPin, ulpSclPin );

    printUlpStatus();

    sTime.init(ulpI2c);
    sTerm.init(ulpI2c);
    sCurrent.init(ulpI2c); 
    sPulse.init(ulpI2c);
    sGyro.init(ulpI2c);  
    sBME280.init(ulpI2c);

    printUlpStatus();
    
  //  main_core_inited = 1;

    printFsmState( __func__, __LINE__);    
}

///   https://www.analog.com/media/en/analog-dialogue/volume-44/number-2/articles/pedometer-design-3-axis-digital-acceler.pdf
void Hardware::printFsmState( const char *funcName, uint32_t lineNumber) 
{
    if (!debugTraceFsm)
        return;
    print_w( funcName );       print_w(":");    print_w( lineNumber );     print_w("\t");
    print_w("SensorState: ");   print_w( sensor_state_name[SensorState] );   print_w("\t");
    print_w("ControlState: ");  print_w( control_state_name[ControlState] ); print_w("\t");
    println_w();
}

void Hardware::printStat(statRecord_t *record) 
{
    printf_w("Stat:  Time '%d' Steps '%d', Heart rate '%f', SpO2: '%f', Ambient: '%f', Object: '%f', Pressure %f, Humidity %f, Vcc: '%f'\n",
          record->Time,
          record->Steps, record->HeartRate, record->SpO2, 
          record->AmbientTempC, record->ObjectTempC, 
          record->Pressure, record->Humidity, 
          record->Vcc 
    );
}

void Hardware::printAllStat() 
{
    printf_w("printAllStat: statRecord_cnt %d\n", statRecord_cnt );
    for (int i = 0; i < CACHE_RECORD_CNT; i++) {
         print_w( i ); print_w( ". ");
         printStat(&RTC_RECORDS[i]);
    }
    println_w();
}

void Hardware::readSensors()
{
    printf_w("readSensors\n");
    sGyro.read();
    sTerm.read();
    sPulse.read();
    sCurrent.read();
    sBME280.read();
}

void Hardware::updateSkimlog() 
{
    printf_w("updateSkimlog hour %d, last_hour %d\n", hour(), lastSkimlogUpdateHour);

    if (hour() == lastSkimlogUpdateHour) 
          return;
    
    SkimData *skim_log = new  SkimData(  day(), month(), year() );
    skim_log->process();
    free(skim_log);
    lastSkimlogUpdateHour = hour();
}

void Hardware::processHrData() {
    if ( !sPulse.dataIsReady() ) 
        return;

    if (hr_stat.logSize() < HR_LOG_MAX_SIZE ) { //  data take from ulp  
         hr_stat.writeRecordToLog( now(), ulp_max30100_raw_data, MAX30100_ARRAY_SIZE) ;
    }
    else 
    if ( currentTime > nextAttemptStatSyncTime )  {
        if ( sCurrent.getVcc() < 4.0) { 
              printf_w("processHrData: battery is low, skip wifi sync\n");
              return ;
        }
        nextAttemptStatSyncTime = currentTime + HR_LOG_SYNC_STEP; // in case reboot during atempt to send data  
        
        syncStatHrViaWifi();
    }
    else 
        return;
    sPulse.dropData();
}

void Hardware::processStatData() {
    statRecord_t current_rec;
    memset(&current_rec, 0, sizeof(current_rec));
    getCurrentSensorsData(&current_rec);
    
    
    log_file.init();
    log_file.writeLog( &current_rec ); ///!!!!!!!!!!!!!! return this string
    log_file.close(); 

    addDisplayStat(current_rec.ObjectTempC);
}
   
void Hardware::update() 
{
    currentTime = now();

    if (CONTROL_WIFI_INIT == ControlState)
        wm.process();
   /* --- return after  test 
    if ( SENSOR_SLEEPING == SensorState && nextScanTime <= currentTime ) {
         SensorState = SENSOR_AWAKE;
         init_sensors();
    }
     */
    if (SENSOR_AWAKE == SensorState) 
         SensorState = SENSOR_WRITE_RESULT;
   
    printFsmState( __func__, __LINE__);

    if (CONTROL_AWAKE == ControlState) {
        sGyro.read(); // ugly, read data only for prdometer
        printf_w("gyro read stop \n");
        sCurrent.read();
        printf_w("current read stop \n");
        statRecord_t *r = getLastSensorsData();
        //TODO  put into update all statRecord_t
        display.update(r->ObjectTempC, r->HeartRate, sGyro.getStepCount(), r->Pressure, r->Humidity, sCurrent.getBatLevel(), &graph);  // return r->Step after debug
    }

    if (SENSOR_WRITE_RESULT == SensorState) {
        processStatData();
        processHrData();

     //   printAllStat();///

        int next_wake  = nextWakeTime();
        nextScanTime = currentTime + next_wake;
        SensorState    = SENSOR_GOTOSLEEP;
        updateSkimlog();
    }

#ifdef IS_V1_4
    if (SENSOR_GOTOSLEEP == SensorState) {
        statRecord_t *r = getLastSensorsData();

        sCurrent.read();
        printf_w("gyro read stop %d \n", r->Vcc);
        display.update(r->ObjectTempC, r->HeartRate, r->Steps, r->Pressure, r->Humidity, sCurrent.getBatLevel(), &graph);  // return r->Step after debug
        
    }
#endif
}

int Hardware::nextWakeTime()
{
    int rest = minute() % 1; 
    
    return (60 * rest) + 60 - second();
}

void Hardware::goToSleep() 
{
    if (CONTROL_GOTOSLEEP == ControlState) {
        powerSave = true; 
        display.setPowerSave(powerSave);
        ControlState = CONTROL_SLEEPING;
    }

    if (SENSOR_GOTOSLEEP == SensorState) {
  //      log_file.close(); 

        SensorState = SENSOR_SLEEPING;
    }

    if ((CONTROL_SLEEPING == ControlState) && (SENSOR_SLEEPING == SensorState)) {
        cfg.save();
        control.initWakeup();

        int next_wake = nextWakeTime();
        printf_w("next_wake %d\n", next_wake);

   // esp_wifi_stop();
  // esp_bt_controller_disable();
        ESP_ERROR_CHECK(esp_sleep_enable_timer_wakeup(next_wake * 1e6));
     //   ESP_ERROR_CHECK( esp_sleep_enable_ulp_wakeup() );

        esp_deep_sleep_start();
    }
}

statRecord_t *Hardware::getCurrentSensorsData(statRecord_t *record) 
{
    printf_w("getCurrentSensorsData\n");
    readSensors();

    //statRecord_t record;
    record->Time         = currentTime; 
    record->Steps        = sGyro.getStepCount();
    record->HeartRate    = sPulse.getHeartRate();
    record->SpO2         = sPulse.getSpO2();
    record->AmbientTempC = sTerm.getAmbientC(); 
    record->ObjectTempC  = sTerm.getObjectC();
    record->Pressure     = sBME280.getPressure();
    record->Humidity     = sBME280.getHumidity();

    
    record->Vcc          = sCurrent.getVcc();

    return record;
}

statRecord_t *Hardware::getLastSensorsData() 
{
    uint8_t used_item = 0;
    if (0 == statRecord_cnt)
        used_item = CACHE_RECORD_CNT - 1;
    else 
        used_item = statRecord_cnt - 1;
    
    if (false == isAfterDeepsleep)
        used_item = 0;

    printStat(&RTC_RECORDS[ used_item ]);
    return &RTC_RECORDS[used_item];
}

struct tm * Hardware::changeDayFile(int32_t _day) 
{
     time_t _temp_date = graph.getDate() + _day * 86400;
     
     struct tm *tmstruct = localtime(&_temp_date); 
     int32_t  y = (tmstruct->tm_year) + 1900;
     int32_t  m = (tmstruct->tm_mon) + 1;
     int32_t  d = tmstruct->tm_mday;
     char fname[64];
     int32_t fname_len = log_file.filenameDate(fname, sizeof(fname), y, m, d); 
     if (fname_len > 0) {
         bool exist =  SPIFFS.exists(fname);
         if (exist) {
               tmstruct->tm_hour = 0;
               tmstruct->tm_min = 0;
               tmstruct->tm_sec = 0;

               tmElements_t tme = toTmElement(tmstruct);
               time_t new_time =  makeTime(tme);
               graph.setDate( new_time) ;
               return tmstruct;
         }
     }
     return NULL;
}


struct tm * Hardware::loadNextDayFile() 
{ 
    return changeDayFile(+1);
}

struct tm * Hardware::loadPrevDayFile() 
{
    return changeDayFile(-1);
}

void Hardware::runActivity(uint64_t _buttons) 
{
    displayActivity_t _activity = display.getCurrentActivity();


    if (WATCH_ACTICITY == _activity) {
#ifdef IS_V1_4
         bool pressed_menu = !(_buttons & (1ULL << control.pinLeft())) ?  true : false;
#else
         bool pressed_menu = !(_buttons & (1ULL << control.pinRight())) ?  true : false;
#endif
         if (true == pressed_menu) {
              cfg.setCurrenDayTimeToMenu();
              display.setCurrentActivity(ICON_MENU_MAIN_ACTIVITY);
         }
    }
    else
    if (GRAPH_BODY_TEMP_ACTIVITY == _activity || 
        GRAPH_STEPS_ACTIVITY == _activity) {
         bool pressed_menu = !(_buttons & ( 1ULL << control.pinOk() ) ) ?  true : false;
         bool pressed_Left = !(_buttons & ( 1ULL << control.pinLeft() ) ) ?  true : false;
         bool pressed_Right= !(_buttons & ( 1ULL << control.pinRight() ) ) ?  true : false;

         if (true == pressed_menu) {
   //               renderer.giveBackDisplay();
              display.setCurrentActivity(ICON_MENU_MAIN_ACTIVITY);
              
             // menuMgr.setCurrentMenu(menuMgr.getParentAndReset());
              returnToMenu();

         }
         else if (true == pressed_Left) {
              struct tm *_day = loadNextDayFile();
              if (_day != NULL) {
                   int32_t  y = (_day->tm_year) + 1900;
                   int32_t  m = (_day->tm_mon) + 1;
                   int32_t  d = _day->tm_mday;
                   
                   prepareGraphData(_activity, d, m, y);
              }
         }
         else if (true == pressed_Right) {
              struct tm *_day = loadPrevDayFile();
              if (_day != NULL) {
                   int32_t  y = (_day->tm_year) + 1900;
                   int32_t  m = (_day->tm_mon) + 1;
                   int32_t  d = _day->tm_mday;
                  prepareGraphData(_activity, d, m, y);
              }
         }
    }
    else
    if(SETTINGS_ACTIVITY == _activity){ 
    }
}

void Hardware::prepareGraphData(displayActivity_t act, int32_t _day,  int32_t _month,  int32_t _year)
{
    if (!IS_GRAPH_ACTIVITY(act)) 
        return;
        
    skimrecord_idx_t        rec_idx;
    char                pattern_irl[32];
    displayAsixType_t     asixType;
    
    if (GRAPH_BODY_TEMP_ACTIVITY == act) {
         rec_idx   = SKIMREC_OBJT_IDX ;
         snprintf(pattern_irl, sizeof(pattern_irl), "my body t\xB0: %d.%02d.%02d", _day, _month, _year % 100 );  
         asixType = SHOW_FLOAT_TYPE;
    }
    if (GRAPH_STEPS_ACTIVITY == act) {
         rec_idx   = SKIMREC_STEPS_IDX;
         snprintf(pattern_irl, sizeof(pattern_irl), "my steps : %d.%02d.%02d", _day, _month, _year % 100 );  
         asixType = SHOW_INT_TYPE;
    }

    graph.setTitle(String(pattern_irl));

    SkimData *skim_log = new  SkimData(_day, _month, _year);
    skim_log->process();
    hourStat_t *stat = skim_log->getStat();
    
    graph.setData( stat, rec_idx, asixType);
    free(skim_log);
}

struct tm * Hardware::takeLogDayFile(unsigned int curValue) 
{
     time_t _temp_date = now() - curValue * 86400; //  can shit fappened in case new day 
     
     struct tm *tmstruct = localtime(&_temp_date); 
     int32_t  y = (tmstruct->tm_year) + 1900;
     int32_t  m = (tmstruct->tm_mon) + 1;
     int32_t  d = tmstruct->tm_mday;
     char fname[64];
     int32_t fname_len = log_file.filenameDate(fname, sizeof(fname), y, m, d); 
     if (fname_len > 0) {
         bool exist =  SPIFFS.exists(fname);
         if (exist) {
               tmstruct->tm_hour = 0;
               tmstruct->tm_min = 0;
               tmstruct->tm_sec = 0;
               
               tmElements_t tme = toTmElement(tmstruct);
               time_t new_time =  makeTime(tme);
               graph.setDate(new_time) ;
               return tmstruct;
         }
     }
     return NULL;
}

void Hardware::start_graph_logic(displayActivity_t _activity, unsigned int curValue)
{
    if (curValue == graph.encoderVal)
        return;
    graph.encoderVal = curValue;
    
    struct tm *_day = takeLogDayFile(curValue);
    if (_day != NULL) {
        int32_t  y = (_day->tm_year) + 1900;
        int32_t  m = (_day->tm_mon) + 1;
        int32_t  d = _day->tm_mday;
        prepareGraphData(_activity, d, m, y );
    }
}

void Hardware::showGraph() 
{
    display.graphDraw(&graph);
}

void Hardware::showActivity(displayActivity_t act) 
{
    if (GRAPH_BODY_TEMP_ACTIVITY == act || GRAPH_STEPS_ACTIVITY == act) {
        prepareGraphData(act, day(), month(), year());
        graph.setDate(day(), month(), year(), weekday()) ;
    }
     
    display.setCurrentActivity(act);
}

void Hardware::runPowerSafe() 
{
    printFsmState(__func__, __LINE__);
    printUlpStatus();
    if (control.buttonPressed()) {  // Call code  button transitions from HIGH to LOW
        if (CONTROL_SLEEPING == ControlState)  
            display.init();
        
        uint64_t _buttons = control.buttons();

        runActivity(_buttons);
        ControlState = CONTROL_AWAKE;

        displaySleepTimer = millis();
    }

    if ((CONTROL_AWAKE == ControlState) && 
         ((millis() - displaySleepTimer) > displaySleepDelay)) 
        ControlState = CONTROL_GOTOSLEEP;
    
   // sleep(1);
    goToSleep();
}

//---- settings => config logic
void Hardware::setTime(TimeStorage tm) 
{
    sTime.updateTime(tm.hours, tm.minutes, tm.seconds);
}

void Hardware::setDate(DateStorage dt) 
{
    sTime.updateDate(dt.day,  dt.month, dt.year);
}

void Hardware::setTimezone(int tz, bool need_save) 
{
    sTime.setTimeZone(enumIntTimeZone[tz]);
    if (true == need_save) {
        cfg.setTimezone(tz);
        cfg.setSaveFlag(need_save);
    } 
}

void Hardware::setTempSwitch(bool val, bool need_save)
{
    if (val) 
        sTerm.wake();
    else 
        sTerm.sleep();
    if (true == need_save) {
        cfg.setTempSensorSwitch(val);
        cfg.setSaveFlag(need_save);
    }
}

void Hardware::setPedoSwitch(bool val, bool need_save)
{
    if (val) 
        sGyro.wake();
    else 
        sGyro.sleep();
    if (true == need_save) {
        cfg.setStepSensorSwitch(val);
        cfg.setSaveFlag(need_save);
    }
}

void Hardware::setPulseSwitch(bool val, bool need_save)
{
    if (val) 
        sPulse.wake();
    else 
        sPulse.sleep();
    if (true == need_save) {
        cfg.setPulseSensorSwitch(val);
        cfg.setSaveFlag(need_save);
    }
}

void Hardware::setBaroSwitch(bool val, bool need_save)
{
    if (val) 
        sBME280.wake();
    else
        sBME280.sleep();
    if (true == need_save) {
        cfg.setBaroSensorSwitch(val);
        cfg.setSaveFlag(need_save);
    }
}

void Hardware::updateFromConfig()
{
    setTempSwitch(cfg.getTempSensorSwitch(), false);
    setPedoSwitch(cfg.getStepSensorSwitch(), false);
    setPulseSwitch(cfg.getPulseSensorSwitch(), false);
    setBaroSwitch(cfg.getBaroSensorSwitch(), false);
    
    setTimezone(cfg.getTimezone(), false);
}


void onDialogFinished(ButtonType btnPressed, void* /*userdata*/)
{        
    if (btnPressed == BTNTYPE_CLOSE) {
//             Serial.printf("\n\n\n---------------------------------- onDialogFinished BTNTYPE_CLOSE --------------------------------\n\n\n");
    }
  //  Serial.printf("\n\n\n---------------------------------- onDialogFinished  --------------------------------\n\n\n");
}

void Hardware::syncTimeViaWifi()
{
    display.showSyncTimeMessage(onDialogFinished);

    getTimeOverWifi();
}


void Hardware::updateWebApiConfig()
{
    cfg.setServerUid(api_uid_server->getValue());
    cfg.setServerToken(api_key_server->getValue());
    cfg.setServerAddr(api_addr_server->getValue());

    cfg.setSaveFlag(true);
}

void Hardware::showWifiPortal()
{
    display.showWifiPortalMessage(onDialogFinished);

    WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP        
    //reset settings - wipe credentials for testing
    //wm.resetSettings();
   // wm.setTimeout(120);
    wm.setConfigPortalBlocking(false);

    wm.setSaveConfigCallback(saveWifiConfigCallback);
    wm.setBreakAfterConfig(true);

    if (!api_uid_server) {
          char * uid = cfg.getServerUid();
          api_uid_server = new WiFiManagerParameter("API_uid", "API uid", uid, UID_SIZE);
    }
    if (!api_key_server) {
          char * key = cfg.getServerToken();
          api_key_server = new WiFiManagerParameter("API_key", "API key", key, TOKEN_SIZE);
    }
    if (!api_addr_server) {
          char * addr = cfg.getServerAddr();
          api_addr_server = new WiFiManagerParameter("STAT_srv", "Statistics server", addr, SERVER_ADDR_SIZE);
    }

    wm.addParameter(api_addr_server);
    wm.addParameter(api_uid_server);
    wm.addParameter(api_key_server);

    if (!wm.startConfigPortal(wifi_ap_name, NULL)) {
        ControlState = CONTROL_WIFI_INIT;
    }
}

void Hardware::getTimeOverWifi()
{
//    wm.setDebugOutput(false);

    WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP        
    //reset settings - wipe credentials for testing
    //wm.resetSettings();
   // wm.setTimeout(120);
    wm.setConfigPortalBlocking(false);

    
    if (wm.autoConnect(wifi_ap_name)) {
        sTime.updateNtpTime();
        
        display.setCurrentActivity(WATCH_ACTICITY);
        ControlState = CONTROL_AWAKE;
        displaySleepTimer = millis();
    }
    else {
        display.showWifiApMessage(onDialogFinished);
        ControlState = CONTROL_WIFI_INIT;
    }
}

void Hardware::syncStatViaWifi()
{
    display.showSyncStatDataMessage(onDialogFinished);

    WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP        

    wm.setConfigPortalBlocking(false);
    
    if (wm.autoConnect(wifi_ap_name)){
        String uid   = String(cfg.getServerUid());   
        String token = String(cfg.getServerToken()); 
        String addr  = String(cfg.getServerAddr());
         
        int32_t  lastKnownData = getLastDate(addr, uid, token);
        printf_w("lastKnownData -- %d\n", lastKnownData );
        int32_t data_to_send[20];
        int32_t data_to_send_len = getArrayLogfiles(lastKnownData, data_to_send, sizeof(data_to_send) );

        for (int i =0; i < data_to_send_len ; i++) {
            char pattern_url[64];
            int32_t y, m, d;
            int32_t date_to_send = data_to_send[i];

            d = date_to_send % 100;
            m = (date_to_send / 100)  % 100;
            y = date_to_send / 10000;
            
            int32_t fname_len = log_file.filenameDate(pattern_url, sizeof(pattern_url), y, m, d); 

            if (fname_len > 0) {
                 String urlDir = "/action/send_thsdata.php";

                sendStatFile(addr, uid, token, String(d) + "."+ String(m) +"." + String(y), String(pattern_url), urlDir);
            }
        }
  
        display.setCurrentActivity(WATCH_ACTICITY);
        ControlState = CONTROL_AWAKE;
        displaySleepTimer = millis();
    }
    else {
        display.showWifiApMessage(onDialogFinished);

        printf_w("Configportal running\n");
        ControlState = CONTROL_WIFI_INIT;
    }
}

void Hardware::syncStatHrViaWifi()
{
    WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP        

    wm.setConfigPortalBlocking(false);
    
    if ( !wm.autoConnect(wifi_ap_name) ){
        printf_w("syncStatHrViaWifi: not connected to wifi");
        return;
    }
    uint32_t t0 = millis();
    uint32_t timeout = 15000UL;
    
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() > t0 + timeout) {
            printf_w("syncStatHrViaWifi: no wifi\n");
            return;
        }
        delay(1000);
    }
    String uid   = String(cfg.getServerUid());   
    String token = String(cfg.getServerToken()); 
    String addr  = String(cfg.getServerAddr());
        
    String urlDir = "/action/send_hrdata.php";
    if ( true == sendStatFile(addr, uid, token, String("HRDATA"), String(hrFileName), urlDir) ){
        hr_stat.deleteBigFile();
    }
}

//----
//============================================================================================
// Control

void Control::init()
{

    pinMode(LEFT_BUTTON,INPUT);
    pinMode(RIGHT_BUTTON,INPUT);
    pinMode(OK_BUTTON,INPUT);

    int l_State = digitalRead(LEFT_BUTTON);
    int r_State = digitalRead(RIGHT_BUTTON);
    int o_State = digitalRead(OK_BUTTON);

    _Button_State = 0;
    if (l_State)
        _Button_State |= 1ULL << LEFT_BUTTON;
    if (r_State)
        _Button_State |= 1ULL << RIGHT_BUTTON;
    if (o_State)
        _Button_State |= 1ULL << OK_BUTTON;
        //Button_State = digitalRead(Input_1);
    /*
    esp_sleep_enable_ext0_wakeup(OK_BUTTON, 1); //1 = High, 0 = Low
    esp_sleep_enable_ext0_wakeup(RIGHT_BUTTON,1); //1 = High, 0 = Low
    esp_sleep_enable_ext0_wakeup(LEFT_BUTTON,1); //1 = High, 0 = Low
*/
}

void  Control::printButtonState(const char *func_name, uint32_t line_number, int l_State, int r_State, int o_State) 
{
    print_w("printButtonState --  ");
    print_w( func_name );       print_w(":");    print_w( line_number );     print_w("\t");
    print_w(LEFT_BUTTON);   print_w(" - ");print_w(l_State);     print_w(", ");    
    print_w(RIGHT_BUTTON);  print_w(" - ");print_w(r_State);     print_w(", ");     
    print_w(OK_BUTTON);     print_w(" - ");print_w(o_State);     println_w("");  
}

bool Control::buttonPressed()
{
    pinMode(LEFT_BUTTON,INPUT);
    pinMode(RIGHT_BUTTON,INPUT);
    pinMode(OK_BUTTON,INPUT);

    int l_State = digitalRead(LEFT_BUTTON);
    int r_State = digitalRead(RIGHT_BUTTON);
    int o_State = digitalRead(OK_BUTTON);
    _Button_State = 0;
    if (l_State)
        _Button_State |= 1ULL << LEFT_BUTTON;
    if (r_State)
        _Button_State |= 1ULL << RIGHT_BUTTON;
    if (o_State)
        _Button_State |= 1ULL << OK_BUTTON;

    if (_Button_State != _Button_PrevState) {
        _Button_PrevState = _Button_State;
        if (!(_Button_State & (1ULL << LEFT_BUTTON)) ||
              !(_Button_State & (1ULL << RIGHT_BUTTON)) || 
              !(_Button_State & (1ULL << OK_BUTTON))) {
            return true;
        }
    }
    return false;
}

void Control::initWakeup()
{
     esp_sleep_enable_ext0_wakeup(LEFT_BUTTON, 0);
/*
    const uint64_t ext_wakeup_pin_1_mask = 1ULL << LEFT_BUTTON;
    const uint64_t ext_wakeup_pin_2_mask = 1ULL << RIGHT_BUTTON;
    const uint64_t ext_wakeup_pin_3_mask = 1ULL << OK_BUTTON;
    const uint64_t ext_wakeup_pin_mask = ext_wakeup_pin_1_mask | ext_wakeup_pin_2_mask | ext_wakeup_pin_3_mask;

    esp_sleep_enable_ext1_wakeup(ext_wakeup_pin_mask, ESP_EXT1_WAKEUP_ANY_HIGH);
*/
}


//============================================================================================
// Config

void Config::init() 
{
    load();
    setToMenu();
}

void Config::initDefaultValues() 
{
    temp_sensor_switch = true;
    step_sensor_switch = true;
    timezone           = 6;
    auto_update_time_over_wifi   = false;
}

void Config::load() 
{
    StaticJsonDocument<512> doc;
    File file;

    SPIFFS.begin();
    file = SPIFFS.open(filename);
        
    printf_w("%s, size %d\n", filename, file.size());
/*
    while (file.available()) {
        write_w(file.read());
    }
    printf_w("\n");
*/
    // Allocate a temporary JsonDocument
    // Don't forget to change the capacity to match your requirements.
    // Use arduinojson.org/v6/assistant to compute the capacity.

    DeserializationError error = deserializeJson(doc, file);
    if (error) {
        printf_w("Failed to read file, using default configuration. %p", file);
        initDefaultValues();
        need_save = true;
        file.close();
        return;
    }

 //   "baro_switch":false,"pulse_switch":true,"temp_switch":true,"step_switch":true,"
    // Copy values from the JsonDocument to the Config
    baro_sensor_switch  = doc["baro_switch"];
    pulse_sensor_switch = doc["pulse_switch"]; 
    temp_sensor_switch = doc["temp_switch"];
    step_sensor_switch = doc["step_switch"];
    timezone           = doc["timezone"];
    auto_update_time_over_wifi = doc["auto_update_time"];
    strcpy(server_uid,   doc["server_uid"]);
    strcpy(server_token, doc["server_token"]);
    
    if ( doc["server_addr"]) 
        strcpy(server_addr, doc["server_addr"]);

    file.close();
 
    printf_w("Config load: server_uid %s, server_token %s\n", server_uid, server_token);
    serializeJson(doc, Serial); // {"hello":"world"}
    printf_w("\n");

    updateUlpConfig();
}

void Config::updateUlpConfig() 
{
    uint16_t switch_extern = 0;
    /*
    if (temp_sensor_switch) {
        switch_extern |= SENSOR_MLX90615;
    }
    if (step_sensor_switch) {
        switch_extern |= SENSOR_LSM6DS3;
    }
    if (baro_sensor_switch) {
        switch_extern |= SENSOR_BME280;
    }
    if (pulse_sensor_switch) {
        switch_extern |= SENSOR_MAX30100;
    }
    */
    switch_extern |= SENSOR_INA219;

    if (ulp_sensors_switch_extern != switch_extern) {
        ulp_sensors_switch_extern = switch_extern; 
    }
}

void Config::save() 
{
  // Delete existing file, otherwise the configuration is appended to the file
    printf_w("Config save, need_save - '%s'\n", need_save? "y" : "n" );
    if (!need_save)
        return;

    printf_w("Config save: '%s'\n", filename);
    SPIFFS.remove(filename);
    printf_w("Config save: remove '%s'\n", filename);

    File file = SPIFFS.open(filename, "w");
    if (!file) {
        print_w("Failed to create file");
        return;
    }
    printf_w("Config save: open ok '%s'\n", filename);

    StaticJsonDocument<512> doc;
    printf_w("Config save: StaticJsonDocument init ok '%s', temp_switch '%s', step_switch '%s', timezone '%d', auto_update_time '%s', uid %s, token %s\n", 
              filename, 
              temp_sensor_switch? "y":"n", step_sensor_switch?"y":"n", timezone,  auto_update_time_over_wifi?"y":"n",
              server_uid, server_token
              );
    doc["baro_switch"] = baro_sensor_switch;
    doc["pulse_switch"] = pulse_sensor_switch;
    doc["temp_switch"] = temp_sensor_switch;
    doc["step_switch"] = step_sensor_switch;
    doc["timezone"]    = timezone;
    doc["auto_update_time"] = auto_update_time_over_wifi;
    doc["server_uid"]   = server_uid;
    doc["server_token"] = server_token;
    doc["server_addr"] = server_addr;

    printf_w("Config save: set value ok '%s'\n", filename);

    if (serializeJson(doc, file) == 0) {
        print_w("Failed to write to file");
    }
    printf_w("Config save: start close '%s'\n", filename);

    serializeJson(doc, Serial); // {"hello":"world"}
    
    file.close();
}


// set data to external values, uuuuglyyyy 
void Config::setToMenu() 
{
    menuBaroMeter.setBoolean(baro_sensor_switch, true);
    menuPulseMeter.setBoolean(pulse_sensor_switch, true);
    menuPedoMeter.setBoolean(step_sensor_switch, true);
    menuTemperature.setBoolean(temp_sensor_switch, true);
    menuTimeZone.setCurrentValue(timezone, true); 
    //menuAutoUpdate.setBoolean(auto_update_time_over_wifi, true);
}


void Config::setCurrenDayTimeToMenu() 
{
    DateStorage _date(  day(), month(),  year() );
    TimeStorage _time( hour(), minute(), second(), 0);
    
    menuTime.setTime(_time);
    menuDate.setDate(_date);
}

void Config::cast_from_menu() 
{
    baro_sensor_switch         = menuBaroMeter.getBoolean();
    pulse_sensor_switch        = menuPulseMeter.getBoolean();
    step_sensor_switch         = menuPedoMeter.getBoolean();
    temp_sensor_switch         = menuTemperature.getBoolean();
    timezone                   = menuTimeZone.getCurrentValue(); 
  //  auto_update_time_over_wifi = menuAutoUpdate.getBoolean();
}

void Config::setServerUid(const char *_uid)
{ 
    strcpy(server_uid, _uid);
}

void Config::setServerToken(const char *_token)
{
    strcpy(server_token, _token);
}

void Config::setServerAddr(const char *_addr)
{
    strcpy(server_addr, _addr);
}

char * Config::getServerUid()
{
    return server_uid;
}

char * Config::getServerToken()
{
    return server_token;
}

char * Config::getServerAddr()
{
    return server_addr;
}
