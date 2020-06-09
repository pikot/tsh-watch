#include "hardware.hpp"

#if defined(_USE_MLX90615_)
#include <MLX90615.h>
#endif

#include <esp_wifi.h>
#include <esp_bt.h>

RTC_DATA_ATTR uint8_t SensorState = SENSOR_AWAKE;
RTC_DATA_ATTR uint8_t ControlState = CONTROL_AWAKE;

RTC_DATA_ATTR time_t next_scan_time = 0;
RTC_DATA_ATTR time_t next_scan_time_stop = 0;
RTC_DATA_ATTR StatRecord_t RTC_RECORDS[CACHE_RECORD_CNT];
RTC_DATA_ATTR uint8_t StatRecord_cnt = 0;

String twoDigits(int digits) {
    if (digits < 10) {
        String i = '0'+String(digits);
        return i;
    }
    else {
        return String(digits);
    }
}

void Hardware::serial_init(){
#ifdef _SERIAL_DEBUG_
    Serial.begin(115200);
    Serial.flush();
#endif
}

void Hardware::time_init(){
    const uint8_t  SPRINTF_BUFFER_SIZE =     32;                                  // Buffer size for sprintf()        //
    char          inputBuffer[SPRINTF_BUFFER_SIZE];                               // Buffer for sprintf()/sscanf()    //
    while (!DS3231M.begin()) {                                                 
        println_w(F("Unable to find DS3231MM. Checking again in 3s."));      
        delay(3000);
    }
    println_w(F("DS3231M initialized."));                                  //                                  //
  //  DS3231M.adjust();                                                           // Set to library compile Date/Time //
    DateTime _now = DS3231M.now();                                               // get the current time             //
    setTime(_now.unixtime());

    sprintf(inputBuffer,"%04d-%02d-%02d %02d:%02d:%02d", _now.year(),            // Use sprintf() to pretty print    //
          _now.month(), _now.day(), _now.hour(), _now.minute(), _now.second());    // date/time with leading zeros     //
    println_w(inputBuffer);                                                // Display the current date/time    //
    //print_w(F("DS3231M chip temperature is "));                            //                                  //
    //print_w( DS3231M.temperature() / 100.0, 1);                                // Value is in 100ths of a degree   //
  //  println_w("\xC2\xB0""C");   
    current_time = now();
}

bool Hardware::is_wake_by_deepsleep(esp_sleep_wakeup_cause_t wakeup_reason) {
  
    bool status = false;
    switch(wakeup_reason)
    {
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

void Hardware::init() {
    setCpuFrequencyMhz(80);
    displaySleepTimer = millis();

    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
    
    serial_init();
    print_fsm_state( __func__, __LINE__ );

    is_after_deepsleep = is_wake_by_deepsleep(wakeup_reason);
    if (false == is_after_deepsleep) {
        memset(&RTC_RECORDS, 0, sizeof(StatRecord_t));
    }
    print_all_stat(); //!!!!!!!!!!!!!!!!!!
    print_w("wakeup_reason ");  println_w( esp_sleep_wake[wakeup_reason] );
    print_w("StatRecord_cnt "); println_w( StatRecord_cnt );

    if ( CONTROL_SLEEPING == ControlState && ESP_SLEEP_WAKEUP_EXT0 == wakeup_reason ){
         ControlState = CONTROL_AWAKE;
    }
    
    sTerm.init(); // MLX9061* demands magic with  wire pins, so - it should run first, alltime :/
    time_init();  // timer should work alltime
    sCurrent.init(); // anyway i want to trace battery
    sGyro.init(is_after_deepsleep);  // Pedometer consume small energy, can wake alltime (convinient for steps observation)
    if ( SENSOR_SLEEPING == SensorState && next_scan_time <= current_time ) {
         SensorState = SENSOR_AWAKE;
    }
    
    if ( SENSOR_AWAKE == SensorState  ) {
        init_sensors(); 
    }
    if ( CONTROL_AWAKE == ControlState ) {
        display.init();
        control.init();   
    }

    print_fsm_state( __func__, __LINE__);
}

void Hardware::init_sensors() {
    sPulse.init();
    log_file.init();
}

void Hardware::print_fsm_state( const char *func_name, uint32_t line_number) {
    if (!debug_trace_fsm)
        return;
    print_w( func_name );       print_w(":");    print_w( line_number );     print_w("\t");
    print_w("SensorState: ");   print_w( sensor_state_name[SensorState] );   print_w("\t");
    print_w("ControlState: ");  print_w( control_state_name[ControlState] ); print_w("\t");
    println_w();
}

void Hardware::print_stat(StatRecord_t *record) {
    print_w("Steps: ");     print_w( record->Steps );  print_w("\t");
    print_w("Heart rate: ");print_w( record->HeartRate ); print_w("bpm / ");
    print_w("SpO2: ");      print_w( record->SpO2);       print_w("%\t");
    print_w("Ambient: ");   print_w( record->AmbientTempC ); print_w("*C\t");
    print_w("Object: ");    print_w( record->ObjectTempC );  print_w("*C\t");
    print_w("Vcc: ");       print_w( record->Vcc ) ;
    println_w();
}
void Hardware::print_all_stat() {
    println_w("print_all_stat");
    for (int i = 0; i < CACHE_RECORD_CNT; i++){
        print_w( i ); print_w( ". ");
         print_stat(&RTC_RECORDS[i]);
    }
    println_w();
}

void Hardware::read_sensors(){
    sGyro.read_data();
    sTerm.read_data();
    sPulse.read_data();
    sCurrent.read_data();
}

void Hardware::update() {
    current_time = now();
   
    if ( SENSOR_SLEEPING == SensorState && next_scan_time <= current_time ) {
         SensorState = SENSOR_AWAKE;
         init_sensors();
    }
  //  print_fsm_state( __func__, __LINE__);

    if ( SENSOR_AWAKE == SensorState && (next_scan_time + pulse_threshold) <= current_time ) {
         SensorState = SENSOR_WRITE_RESULT;
    }
  //  print_fsm_state( __func__, __LINE__);

    if ( SENSOR_AWAKE == SensorState ) { // start collect and process data 
        sPulse.update();    
        sTerm.update();
    }
 //   print_fsm_state( __func__, __LINE__);
 
    if ( CONTROL_AWAKE == ControlState ){
        sGyro.read_data(); // ugly, read data only for prdometer debug
        StatRecord_t * r  = get_last_sensor_data();
        display.update(r->ObjectTempC, r->HeartRate, sGyro.StepCount());  // return r->Step after debug
     }
    if ( SENSOR_WRITE_RESULT == SensorState ) {
        StatRecord_t current_rec;
        memset(&current_rec, 0, sizeof(current_rec));
        current_sensor_data(&current_rec);
        
        log_file.write_log( &current_rec );
        print_stat(&current_rec);
        int next_wake  = next_wake_time();
        next_scan_time = current_time + next_wake;
        SensorState    = SENSOR_GOTOSLEEP;
    }
  //  print_fsm_state( __func__, __LINE__);
}

int Hardware::next_wake_time(){
    int rest = minute() % 1; 
    
    return (60 * rest) + 60 - second();
}

void Hardware::WakeSensors() {
    powerSave = false;
    display.setPowerSave(powerSave); // off 4 test
    sPulse.wake();
    sCurrent.wake();
}

void Hardware::GoToSleep() {
    if ( CONTROL_GOTOSLEEP == ControlState ) {
        powerSave = true;
        display.setPowerSave(powerSave);
        ControlState = CONTROL_SLEEPING;
    }
    
    if ( SENSOR_GOTOSLEEP == SensorState ) {
        sPulse.sleep();
        // term should go in sleep last, because 2 sleep need manipulation with scl and sda 
        log_file.close(); 
        SensorState = SENSOR_SLEEPING;
    }

    if ( (CONTROL_SLEEPING == ControlState) && (SENSOR_SLEEPING == SensorState) ) {
        sCurrent.sleep();
        sTerm.sleep(); 

        control.init_wakeup();

        int next_wake = next_wake_time();
        print_w("next_wake ");  println_w(next_wake);  

   // esp_wifi_stop();
  // esp_bt_controller_disable();
        esp_sleep_enable_timer_wakeup(next_wake * 1e6);
        esp_deep_sleep_start();

    }
}

StatRecord_t *Hardware::current_sensor_data(StatRecord_t *record) {
    read_sensors();

    //StatRecord_t record;
    record->Time         = current_time; 
    record->Steps        = sGyro.StepCount();
    record->HeartRate    = sPulse.HeartRate();
    record->SpO2         = sPulse.SpO2();
    record->AmbientTempC = sTerm.AmbientC(); 
    record->ObjectTempC  = sTerm.ObjectC();
    record->Vcc          = sCurrent.Vcc();

    return record;
}

StatRecord_t *Hardware::get_last_sensor_data() {
    uint8_t used_item = 0;
    if (0 == StatRecord_cnt)
        used_item = CACHE_RECORD_CNT - 1;
    else 
        used_item = StatRecord_cnt - 1;
    
    if ( false == is_after_deepsleep )
        used_item = 0;

    print_fsm_state( __func__, __LINE__);
 //   print_w("used_item "); println_w( used_item ); //print_w(", ");
    print_stat(  &RTC_RECORDS[ used_item ] );
    return &RTC_RECORDS[used_item];
}

void Hardware::power_safe_logic() {
    print_fsm_state( __func__, __LINE__);

    if ( control.button_pressed() ) {  // Call code  button transitions from HIGH to LOW

        if (CONTROL_SLEEPING == ControlState) { 
                    display.init();
        } 
        ControlState = CONTROL_AWAKE;

        displaySleepTimer = millis();
    }
/*
    print_w("millis ");print_w(millis());          print_w("\t");
    print_w("displaySleepTimer ");print_w(displaySleepTimer); print_w("\t");
    print_w("diff ");print_w((millis() - displaySleepTimer)); print_w("\t");
    print_w("displaySleepDelay ");print_w(displaySleepDelay); println_w("");
*/

    if ( (CONTROL_AWAKE == ControlState) && 
         ((millis() - displaySleepTimer) > displaySleepDelay) ) {
        ControlState = CONTROL_GOTOSLEEP;
    }
    GoToSleep();
}


//============================================================================================
// File System

void FileSystem::init(){
    SPIFFS.begin(true);
    
    size_t totalBytes = 0, usedBytes = 0;
    totalBytes = SPIFFS.totalBytes();
    usedBytes  = SPIFFS.usedBytes();

    String temp_str = "totalBytes = " + String(totalBytes) + ", usedBytes = " +  String(usedBytes);
    println_w(temp_str);

    char fname[32];
    current_day_fname( fname, sizeof(fname), "/log", year(), month(), day() );
    // scan_log_dir("/log");

    char *modifier = "w";
    if (SPIFFS.exists(fname) )
        modifier = "a";
        
    if ( !_can_write )      
        return;
    _file = SPIFFS.open(fname, modifier);
    if ( !_file ) {
        println_w("file open failed");  //  "открыть файл не удалось"
    }
    if ("w" == modifier)
        _file.print("V:1\n");

}

char * FileSystem::current_day_fname(char *inputBuffer, int inputBuffer_size, char *dir, int16_t year, int8_t month, int8_t  day) {
    snprintf(inputBuffer, inputBuffer_size,"%s/%04d-%02d-%02d.txt", dir, year, month, day); 
}

#define MAX_FILES_CNT 6
void FileSystem::cat_file(File f){
    while (f.available()) {
        write_w(f.read());
    }
}

void FileSystem::list_dir(fs::FS &fs, const char * dirname){
    File root = fs.open(dirname);
    if(!root){
        return;
    }
    if(!root.isDirectory()){
        return;
    }

    File file = root.openNextFile();
    while(file){
        if(file.isDirectory()){
            continue;
        } else {
            print_w("  FILE: ");
            print_w(file.name());
            print_w("\tSIZE: ");
            println_w(file.size());
            cat_file(file);
            
        }
        file = root.openNextFile();
    }
}

void FileSystem::scan_log_dir(char* dir_name) {
    list_dir(SPIFFS, dir_name);
}

void FileSystem::save_records_to_file(){
    if ( !_can_write )
        return;

    StatRecord_t *record = &RTC_RECORDS[0];
    for (int i = 0; i < StatRecord_cnt; i++, record++) {
        String temp_str = String(record->Time) + "\t" + String(record->Vcc) + String(record->Steps) + "\t" +
                           String(record->HeartRate) + "\t" +  String(record->AmbientTempC) + "\t" +  String(record->ObjectTempC) + "\n" ;
        println_w(temp_str);
        _file.print(temp_str);
    }
}

void FileSystem::write_log( StatRecord_t *record ){
  
    memcpy( (void*) &RTC_RECORDS[StatRecord_cnt], (void*) record, sizeof(StatRecord_t) );
    
    if (CACHE_RECORD_CNT == (StatRecord_cnt + 1)){
        save_records_to_file();
        StatRecord_cnt = 0;
    } else {
        StatRecord_cnt++;
    }
}

void FileSystem::close(){
    if ( !_can_write )
        return;
    _file.close();
}

//============================================================================================
// Display

void Display::init() {
//    U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C _display1(U8G2_R0, /* reset=*/ U8X8_PIN_NONE, /* clock=*/ 5, /* data=*/ 4); 
    U8G2_SH1106_128X64_NONAME_F_HW_I2C  _display1(U8G2_R0, U8X8_PIN_NONE  );

    _display = _display1;
    _display.begin();        
    _display.setBusClock( 400000 );
    //_display.setPowerSave(false);
    ts_last_display_update = millis();
}

void Display::show_digitalClock(int16_t x, int16_t y) {
    String timenow = String(hour()) + ":" + twoDigits(minute());//+":"+twoDigits(second());
    _display.setFont(u8g2_font_freedoomr25_tn);//(ArialMT_Plain_24);
    _display.drawStr( x, y , timenow.c_str() );
}

void Display::show_temperatureC(int16_t x, int16_t y, float therm, float pulse) {
    String temp_str1 =  String (therm) + "*C ";
    String temp_str2 =  String (pulse) + "bpm";

    _display.setFont(u8g2_font_helvR08_te);
    _display.drawStr(x, 14, temp_str1.c_str());
    _display.drawStr(x, 32, temp_str2.c_str());
}

void Display::show_steps(int16_t x, int16_t y, uint16_t steps) {
    String steps_str = "steps: " + String (steps) ;

    _display.setFont(u8g2_font_helvR08_te);
    _display.drawUTF8(x, y, steps_str.c_str());
}

void Display::update(float therm, float pulse, uint16_t steps){
   // if (millis() - ts_last_display_update <= REPORTING_PERIOD_MS) 
   //     return;
//    print_stat();

    ts_last_display_update = millis();

    _display.clearBuffer();         // clear the internal memory
    show_digitalClock( 0, 32);
    show_temperatureC( 90, 32, therm, pulse);
    show_steps(0, 50, steps);
    
    _display.sendBuffer();          // transfer internal memory to the display
}

void Display::setPowerSave(bool powerSave){
    _display.setPowerSave(powerSave);
}


//============================================================================================
// Control

void Control::init(){
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

void  Control::print_button_state (const char *func_name, uint32_t line_number, int l_State, int r_State, int o_State ) {
    print_w("print_button_state --  ");
    print_w( func_name );       print_w(":");    print_w( line_number );     print_w("\t");
    print_w(LEFT_BUTTON);   print_w(" - ");print_w(l_State);     print_w(", ");    
    print_w(RIGHT_BUTTON);  print_w(" - ");print_w(r_State);     print_w(", ");     
    print_w(OK_BUTTON);     print_w(" - ");print_w(o_State);     println_w("");  
}

bool Control::button_pressed(){
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
        
    if ( _Button_State != _Button_PrevState){
        _Button_PrevState = _Button_State;
        if (  !(_Button_State & (1ULL << LEFT_BUTTON) ) ||
              !(_Button_State & (1ULL << RIGHT_BUTTON) ) || 
              !(_Button_State & (1ULL << OK_BUTTON)) ) {
          return true;
        }
    }
    return false;
}


void Control::init_wakeup(){
     esp_sleep_enable_ext0_wakeup(LEFT_BUTTON, 0);

/*
    const uint64_t ext_wakeup_pin_1_mask = 1ULL << LEFT_BUTTON;
    const uint64_t ext_wakeup_pin_2_mask = 1ULL << RIGHT_BUTTON;
    const uint64_t ext_wakeup_pin_3_mask = 1ULL << OK_BUTTON;
    const uint64_t ext_wakeup_pin_mask = ext_wakeup_pin_1_mask | ext_wakeup_pin_2_mask | ext_wakeup_pin_3_mask;

    esp_sleep_enable_ext1_wakeup(ext_wakeup_pin_mask, ESP_EXT1_WAKEUP_ANY_HIGH);
*/

}
