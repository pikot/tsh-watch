#include "sensors.hpp"
#include "esp_adc_cal.h"
#include "ulp_main.h"

//============================================================================================
void Sensor::init_direct(){
}

void Sensor::init(){
    inited = true;
    return;
}
//============================================================================================
GyroscopeLSM6DS3::GyroscopeLSM6DS3() {
    steps = 0;
}

GyroscopeLSM6DS3::~GyroscopeLSM6DS3(){
}

void GyroscopeLSM6DS3::init() {   
    Sensor::init();
}

void GyroscopeLSM6DS3::read() {
    // test for errcode from ulp and set value from rtc memory 
    printf_w("LSM6DS3 driver signature  %d, error_cnt %d\n", ulp_lsm6ds3_driver_sign, ulp_lsm6ds3_error_cnt);

    if (ulp_lsm6ds3_error)
        return;
    printUlpData();
    
    steps = ulp_lsm6ds3_step_count; //
}

uint16_t GyroscopeLSM6DS3::getStepCount(){
    return  steps;
}

void GyroscopeLSM6DS3::wake(){
    ulp_sensors_switch_extern |= SENSOR_LSM6DS3;
}

void GyroscopeLSM6DS3::sleep(){
    ulp_sensors_switch_extern &= ~SENSOR_LSM6DS3;
}

void GyroscopeLSM6DS3::printUlpData(){
    printf_w("LSM6DS3 status: step_count %d\n",  ulp_lsm6ds3_step_count);
}

//============================================================================================
ThermoMeter::ThermoMeter() {
    AmbientTempC = 0;
    ObjectTempC  = 0;
}

void ThermoMeter::init(){
    Sensor::init();
}

void ThermoMeter::wake(){
    ulp_sensors_switch_extern |= SENSOR_MLX90615;
}

void ThermoMeter::sleep(){
    ulp_sensors_switch_extern &= ~SENSOR_MLX90615;
}

float ThermoMeter::raw_temp_to_C(uint16_t _t) { 
    if (0 == _t)
        return 0; 
    return ( (float)_t * 0.02 ) - 273.15;
}

void ThermoMeter::read() {
    // test for errcode from ulp and set value from rtc memory 
    if (ulp_mlx90615_error)
        return;
        
    printUlpData();
    AmbientTempC = raw_temp_to_C(ulp_mlx90615_amb_temperature);
    ObjectTempC  = raw_temp_to_C(ulp_mlx90615_obj_temperature);
}

float ThermoMeter::getAmbientC(){
    return AmbientTempC;
}

float ThermoMeter::getObjectC() {
    return ObjectTempC;
}
void ThermoMeter::printUlpData(){
    printf_w("mlx90615 data: "
                  "obj_temperature %f, amb_temperature %f,ulp_mlx90615_obj_temperature %d, amb_temperature raw %d"
                  "\n", 
                   raw_temp_to_C(ulp_mlx90615_obj_temperature), raw_temp_to_C(ulp_mlx90615_amb_temperature), ulp_mlx90615_obj_temperature, ulp_mlx90615_amb_temperature
                   );

//    printf_w("mlx90615 data: b1 %d, b2 %d, pec %d, last val %d\n",
   //         ulp_mlx90615_b1, ulp_mlx90615_b2, ulp_mlx90615_pec, (ulp_mlx90615_b2 << 8) | ulp_mlx90615_b1
  //                 );
}
//============================================================================================
PulseMeter::PulseMeter(){
    heartRate = 0;
    spO2 = 0;
}

void PulseMeter::init(){
    Sensor::init();
}

void PulseMeter::wake(){
    ulp_sensors_switch_extern |=  SENSOR_MAX30100;
}

void PulseMeter::sleep(){
    ulp_sensors_switch_extern &= ~SENSOR_MAX30100;
}   

void PulseMeter::read() {
    // test for errcode from ulp and set value from rtc memory 
    heartRate = 0;
    spO2  = 0;
}

float PulseMeter::getHeartRate(){
    return heartRate;
}

float PulseMeter::getSpO2(){
    return spO2;
}

//============================================================================================

//  https://www.jackogrady.me/battery-management-system/state-of-charge
CurrentMeter::CurrentMeter() {
    vcc = 0;
    current_since_reboot = 0;
    //aggr_current      = 0;
    //aggr_current_prev = 0;
}

void CurrentMeter::init(float *current){
    Sensor::init();
    
    battery = new Battery(3000, 4200, A0);
    battery->begin(180, 1.0, &sigmoidal);

    aggr_current = current;
}


float CurrentMeter::calc_aggr_accum(float mA) {
    float aggr = (float) mA/(float)ulp_ina219_currentDivider_mA;
    return  tm * aggr / tb;
}

void CurrentMeter::read() {
    if (ulp_ina219_error )
        return; 

    // test for errcode from ulp and set value from rtc memory 
    float _current = calc_aggr_accum( (float) ulp_ina219_aggr_current);
    ulp_ina219_aggr_current = 0;
    
    *aggr_current += _current;
    /*
    printf_w("current list\n");
    for (uint32_t i = 0; i< 60; i++){
        printf_w("%d.\t %fma\n", i, (float) ( (int16_t)ulp_ina219_current_table[i]) / (float)ulp_ina219_currentDivider_mA);
    }
*/
    vcc      = (float) (ulp_ina219_voltage * 4 ) * 0.001;
    current  = (float) ulp_ina219_current/(float) ulp_ina219_currentDivider_mA;
    batLevel = battery->level( vcc *1000 );
    
    printf_w("ina219 :2: --- c %f, p %f, bat level %d\n", _current, *aggr_current, battery->level( vcc *1000 ) );

    printUlpData();
}

void CurrentMeter::wake(){
    ulp_sensors_switch_extern |= SENSOR_INA219;
}

void CurrentMeter::sleep(){
    ulp_sensors_switch_extern &= ~SENSOR_INA219;
}

float CurrentMeter::getVcc(){
    return vcc;
}

uint8_t CurrentMeter::getBatLevel(){
    return batLevel;
}

void CurrentMeter::printUlpData(){
    printf_w("ina219 data: config %x, current %fmA, current_aggr_raw %u, current_aggr %fmA, current_aggr_sr %fmA, "
                      "voltage %fV, "
                      "currentDivider_mA = %d, calValue = %u"
                      "\n", 
                      (uint16_t)ulp_ina219_config,
                      (float) ulp_ina219_current/(float)ulp_ina219_currentDivider_mA ,
                      ulp_ina219_aggr_current,
                      *aggr_current,    current_since_reboot,
                      (float)(ulp_ina219_voltage *4)*0.001,
                      (uint16_t) ulp_ina219_currentDivider_mA,
                      (uint16_t) ulp_ina219_calValue
                      
                      );
}
//============================================================================================
TimeMeter::TimeMeter(){
  
}

void TimeMeter::init(){
    Sensor::init();
}

time_t TimeMeter::currentTime(){
    return now();
}

uint8_t TimeMeter::bcd2dec(uint8_t bcd)
{
    return ((bcd / 16) * 10) + (bcd % 16);
}


uint8_t TimeMeter::dec2bcd(uint8_t n)
{
    uint16_t a = n;
    byte b = (a*103) >> 10;
    return  n + b*6;
}

bool TimeMeter::read_data_to_tm(tmElements_t &tm) {
    if (ulp_ds3231_error )
        return false; 
        
    printUlpData();
                   
    uint32_t yr = bcd2dec(ulp_ds3231_year);
    if( yr > 99)
        yr = yr - 1970;
    else
        yr += 30;  
      
    tm.Year = yr;
    tm.Month = bcd2dec(ulp_ds3231_month);
    tm.Day = bcd2dec(ulp_ds3231_day);
    tm.Hour = bcd2dec(ulp_ds3231_hour);
    tm.Minute = bcd2dec(ulp_ds3231_minute);
    tm.Second = bcd2dec(ulp_ds3231_second);
    return true;
}

void TimeMeter::read() {
    tmElements_t tm;
    if ( true == read_data_to_tm(tm) )
        setTime(makeTime(tm));
}

void TimeMeter::printUlpData(){
  
    printf_w("ds3231 data: "
                  "year %d, month %d, day %d, dayOfWeek %d, "
                  "hour %d (raw hour %d), minute %d, second %d (year raw %d)"
                  ", reseted_steps %d\n",
                   bcd2dec(ulp_ds3231_year) + 2000, bcd2dec(ulp_ds3231_month), bcd2dec(ulp_ds3231_day), bcd2dec(ulp_ds3231_dayOfWeek), 
                   bcd2dec(ulp_ds3231_hour), ulp_ds3231_hour, bcd2dec(ulp_ds3231_minute), bcd2dec(ulp_ds3231_second),  ulp_ds3231_year,
                   ulp_lsm6ds3_reseted_steps);
                   /*
    printf_w("ds3231 SET DATA: %02d:%02d:%02d  %02d.%02d.%02d (%d dow)  update_flag %d\n",
                   bcd2dec(ulp_ds3231_set_hour), bcd2dec(ulp_ds3231_set_minute),  bcd2dec(ulp_ds3231_set_second),
                   bcd2dec(ulp_ds3231_set_day),  bcd2dec(ulp_ds3231_set_month),  bcd2dec(ulp_ds3231_set_year) + 2000,  
                   bcd2dec(ulp_ds3231_set_dayOfWeek) , (uint16_t) ulp_ds3231_update_flag);
*/
}

void TimeMeter::updateUlpTime(tmElements_t &tm){
    ulp_ds3231_set_second = dec2bcd(tm.Second);
    ulp_ds3231_set_minute = dec2bcd(tm.Minute);
    ulp_ds3231_set_hour   = dec2bcd(tm.Hour);
    ulp_ds3231_set_dayOfWeek = dec2bcd(tm.Wday);
    ulp_ds3231_set_day       = dec2bcd(tm.Day);
    ulp_ds3231_set_month     = dec2bcd(tm.Month);
    ulp_ds3231_set_year      = dec2bcd(tm.Year - 30);
    printf_w("year %d\n", ulp_ds3231_set_year);
    ulp_ds3231_update_flag = 1;
}

RTC_DATA_ATTR uint32_t lastNtpUpdate = 0;

time_t TimeMeter::updateNtpTime(){
    ulp_ds3231_update_flag = 0;
    if (  (lastNtpUpdate) && (  ((now() - lastNtpUpdate) ) < ntpUpdateInterval ) ) {
        printf_w("updateNtpTime -- %d : %d\n", lastNtpUpdate, ntpUpdateInterval);
        return 0;
    }
    printf_w("updateNtpTime: lastNtpUpdate  %d, ntpUpdateInterval %d, status %d/%d\n",
              lastNtpUpdate, ntpUpdateInterval, WiFi.status(), WL_CONNECTED);

    if (WiFi.status() != WL_CONNECTED) 
        return 0;
    
    Udp.begin(localPort);
 //   printf_w("Local port: %d", Udp.localPort() );
   // printf_w("waiting for sync\n");
  //  setSyncProvider( TimeMeter::getNtpTime );
  //  setSyncInterval(300);
    time_t net_time = getNtpTime();
    printf_w("updateNtpTime:   net_time %d\n", net_time);
    if (0 == net_time)
        return 0;

    tmElements_t tm; 
    breakTime(net_time, tm);
    
    updateUlpTime(tm);
    lastNtpUpdate = now();
}

time_t TimeMeter::getNtpTime()
{
    if (WiFi.status() != WL_CONNECTED) 
        return 0;
        
    IPAddress ntpServerIP; // NTP server's ip address

    while (Udp.parsePacket() > 0) ; // discard any previously received packets
    printf_w("Transmit NTP Request\n");
    // get a random server from the pool
    WiFi.hostByName(ntpServerName, ntpServerIP);

    printf_w("%s : %s\n", ntpServerName, ntpServerIP);
    sendNtpPacket(ntpServerIP);
    uint32_t beginWait = millis();
    while (millis() - beginWait < 1500) {
        int size = Udp.parsePacket();
        if (size >= NTP_PACKET_SIZE) {
            printf_w("Receive NTP Response\n");
            Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
            unsigned long secsSince1900;
            // convert four bytes starting at location 40 to a long integer
            secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
            secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
            secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
            secsSince1900 |= (unsigned long)packetBuffer[43];
            return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
        } 
    }
    printf_w("No NTP Response :-(\n");
    return 0; // return 0 if unable to get the time
}

void TimeMeter::sendNtpPacket(IPAddress &address)
{
    // set all bytes in the buffer to 0
    memset(packetBuffer, 0, NTP_PACKET_SIZE);
    // Initialize values needed to form NTP request
    // (see URL above for details on the packets)
    packetBuffer[0] = 0b11100011;   // LI, Version, Mode
    packetBuffer[1] = 0;     // Stratum, or type of clock
    packetBuffer[2] = 6;     // Polling Interval
    packetBuffer[3] = 0xEC;  // Peer Clock Precision
    // 8 bytes of zero for Root Delay & Root Dispersion
    packetBuffer[12] = 49;
    packetBuffer[13] = 0x4E;
    packetBuffer[14] = 49;
    packetBuffer[15] = 52;
    // all NTP fields have been given values, now
    // you can send a packet requesting a timestamp:
    Udp.beginPacket(address, 123); //NTP requests are to port 123
    Udp.write(packetBuffer, NTP_PACKET_SIZE);
    Udp.endPacket();
}


void TimeMeter::updateTime(int h,  int m, int s){  
    tmElements_t tm; 
    if ( true == read_data_to_tm(tm) ) {
        tm.Hour   = h;
        tm.Minute = m;
        tm.Second = s;
        updateUlpTime(tm);
        setTime(makeTime(tm));
    }
}

void TimeMeter::updateDate(int d,  int m, int y){
    tmElements_t tm; 
    if ( true == read_data_to_tm(tm) ) {
      
        tm.Year   = y;
        tm.Month  = m;
        tm.Day    = d;

        updateUlpTime(tm);
        setTime(makeTime(tm));
    }
}

//============================================================================================
