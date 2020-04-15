#include "hardware.hpp"

#if defined(_USE_MLX90615_)
#include <MLX90615.h>
#endif

//ADC_MODE(ADC_VCC);

String twoDigits(int digits){
  if(digits < 10) {
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

void Hardware::thermometer_init(){
#if defined(_USE_MLX90614_)
    therm.begin(); // Initialize the MLX90614
    therm.setUnit(TEMP_C);   
    therm.wake();
#elif defined(_USE_MLX90615_)
    therm.begin();
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
    DateTime now = DS3231M.now();                                               // get the current time             //
    setTime(now.unixtime());

    sprintf(inputBuffer,"%04d-%02d-%02d %02d:%02d:%02d", now.year(),            // Use sprintf() to pretty print    //
          now.month(), now.day(), now.hour(), now.minute(), now.second());    // date/time with leading zeros     //
    println_w(inputBuffer);                                                // Display the current date/time    //
    //print_w(F("DS3231M chip temperature is "));                            //                                  //
    //print_w( DS3231M.temperature() / 100.0, 1);                                // Value is in 100ths of a degree   //
    println_w("\xC2\xB0""C");   
}


void Hardware::pulse_init(){
    pulse.begin();
    pulse.setIRLedCurrent(MAX30100_LED_CURR_7_6MA);
}

void Hardware::init() {
    serial_init();
    thermometer_init();
    time_init();
    pulse_init();
    log_file.init();
    display.init();

    control.init();
}

void Hardware::print_stat() {
    float t_ambient = 0;
    float t_object  = 0;
#if defined(_USE_MLX90614_)
    t_ambient = therm.ambient());
    t_object  = print_w( therm.object()); 
#elif defined(_USE_MLX90615_)
    t_ambient = therm.readTemp(MLX90615::MLX90615_SRCA, MLX90615::MLX90615_TC ); 
    t_object  =  therm.readTemp( ); 
#endif

    
    print_w("Heart rate: ");print_w( pulse.getHeartRate() ); print_w("bpm / ");
    print_w("SpO2: ");      print_w( pulse.getSpO2());       print_w("%\t");
    print_w("Ambient: ");   print_w( t_ambient ); print_w("*C\t");
    print_w("Object: ");    print_w( t_object );  print_w("*C\t");
    print_w("Vcc: ");       print_w( get_redable_vcc() ) ;
    println_w();
}

void Hardware::update() {
    pulse.update();
#if defined(_USE_MLX90614_)
    therm.read();
#endif
    float t;
#if defined(_USE_MLX90614_)
    t = therm.object() ;
#elif defined(_USE_MLX90615_)
    t = therm.readTemp(MLX90615::MLX90615_SRCO, MLX90615::MLX90615_TC);
#endif
    float h = pulse.getHeartRate() ;
    
    display.update(t, h);
}

int Hardware::next_wake_time(){
    int rest = minute() % 1; 
    
    return (60 * rest) + 60 - second();
}

void Hardware::WakeSensors() {
    powerSave = false;
    display.setPowerSave(powerSave); // off 4 test
    pulse.resume();
#if defined(_USE_MLX90614_)
    therm.wake();
#elif defined(_USE_MLX90615_)
     //Mlx90615_ExitSleep(v)
#endif
}

void Hardware::GoToSleep() {
    powerSave = true;

    display.setPowerSave(powerSave);
    pulse.shutdown();
#if defined(_USE_MLX90614_)
    therm.sleep();
#elif defined(_USE_MLX90615_)
//    Mlx90615_EnterSleep();
#endif
    log_file.close(); 

    int next_wake = next_wake_time();
    
#if defined(ESP8266)
    ESP.deepSleep( next_wake * 1e6, WAKE_RF_DISABLED);
#elif defined(ESP32) 
    esp_sleep_enable_timer_wakeup(next_wake * 1e6);
    esp_deep_sleep_start();
#endif
}

void Hardware::power_safe_logic() {
    if ( control.button_pressed() ) {  // Call code if button transitions from HIGH to LOW
        if (powerSave == true) {
            WakeSensors();
        } 
        displaySleepTimer = millis();
    }
    if ( (millis() - displaySleepTimer) > displaySleepDelay ) {
        float t_ambient = 0, t_object = 0;
#if defined(_USE_MLX90614_)
        t_ambient = therm.ambient(); 
        t_object  = therm.object();
#elif defined(_USE_MLX90615_)
        t_ambient = therm.readTemp(MLX90615::MLX90615_SRCA, MLX90615::MLX90615_TC);
        t_object  = therm.readTemp();
#endif
        log_file.write_log( pulse.getHeartRate(), t_ambient, t_object, get_redable_vcc() );
        print_stat();
        GoToSleep();
    }
}

float Hardware::get_redable_vcc() {
    float voltaje=0.00f;
    
#if defined(ESP8266)
    voltaje = ESP.getVcc();
#elif defined(ESP32) 

#endif
    return voltaje/1024.00f;
}


//============================================================================================
// File System

void FileSystem::init(){
#if defined(ESP8266)
    FSInfo fs_info;
    SPIFFS.begin();
    SPIFFS.info(fs_info);
 
    String temp_str = "totalBytes = " + String(fs_info.totalBytes) + ", usedBytes = " +  String( fs_info.usedBytes);
    println_w(temp_str);

    char fname[32];
    current_day_fname( fname, sizeof(fname), "/log", year(), month(), day() );
    scan_log_dir("/log");

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
#endif
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

void FileSystem::scan_log_dir(char* dir_name) {   
    char *files[MAX_FILES_CNT];
    int i = 0;
#if defined(ESP8266)

    Dir dir = SPIFFS.openDir(dir_name);
    while ( dir.next() ) {
        File f = dir.openFile("r");
        print_w(dir.fileName());
        print_w(" ");
        println_w(f.size());
        // cat_file(f);
       // if (MAX_FILES_CNT > i) 
         //    files[i] = strncpy( );
      //  else 
#endif
}

void FileSystem::write_log( float HeartRate, float AmbientTempC, float ObjectTempC, float vcc ){
    if ( !_can_write )
        return;
      
    time_t  t = now();  
    String temp_str = String(t) + "\t" + String(vcc) + "\t" + String(HeartRate) + "\t" +  String(AmbientTempC) + "\t" +  String(ObjectTempC) + "\n" ;
    _file.print(temp_str);
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
    _display.setBusClock( 100000 );
    _display.setPowerSave(false);
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

void Display::update(float therm, float pulse){
    if (millis() - ts_last_display_update <= REPORTING_PERIOD_MS) 
        return;
//    print_stat();

    ts_last_display_update = millis();

    _display.clearBuffer();         // clear the internal memory
    show_digitalClock( 0, 32);
    show_temperatureC( 90, 32, therm, pulse);
    _display.sendBuffer();          // transfer internal memory to the display
}

void Display::setPowerSave(bool powerSave){
    _display.setPowerSave(powerSave);
}


//============================================================================================
// Control

void Control::init(){
    Button_State = digitalRead(Input_1);
}

bool Control::button_pressed(){
    Button_State = digitalRead(Input_1);
    if ( Button_State != Button_PrevState){
        println_w(Button_State);
        Button_PrevState = Button_State;
        if (0 == Button_State) {
          return true;
        }
    }
    return false;
}
