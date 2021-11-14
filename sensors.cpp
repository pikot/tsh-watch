//  SPDX-FileCopyrightText: 2020-2021 Ivan Ivanov 
//  SPDX-License-Identifier: GPL-3.0-or-later

#include "sensors.hpp"
#include "esp_adc_cal.h"
#include "ulp_main.h"


//============================================================================================
void Sensor::init_direct(){
     
}

bool Sensor::testAddress(){
    i2c->beginTransmission(i2c_addr);
    uint8_t error =   i2c->endTransmission(true);
    if (0 == error) {
        return true;
    }
    return false;
}

bool Sensor::init(){
    if (!ulp_error) {
          printf_w("error: sensor %s, not inited ulp_error pointer\n", name );
          return false;
    }
    *ulp_error =  SENSOR_STATUS_HW_ERROR;
    if (!i2c_addr) {
          printf_w("error: sensor %s, i2c_addr is 0\n", name);
          return false;
    }
    if (!i2c) {
          printf_w("error: sensor %s, i2c connection not set\n", name);
          return false;
    }
    int attempt = 0; 
    int max_attempt = 3;
    for ( attempt = 0; attempt < max_attempt; attempt++) {
        if ( false  == testAddress() ) {
            printf_w("error: sensor %s not found on address 0x%x, attempt %d \n", name, i2c_addr,  attempt);
            delay(300);
 //         return false;
        }
        else 
            break;
    }
    if ( max_attempt == attempt ) {
        printf_w("error: sensor %s not found - off it\n", name);
        return false;
    }
    
    *ulp_error = SENSOR_STATUS_OK;
    printf_w("init: sensor %s - found\n", name);
    return true;
}

void Sensor::writeRegister(uint8_t address, uint8_t data)
{
    if (!i2c) {
          return;
    }
    i2c->beginTransmission(i2c_addr);
    i2c->write(address);
    i2c->write(data);
    i2c->endTransmission();
}

void Sensor::writeRegister(uint8_t address, uint16_t data)
{
    if (!i2c) {
          return;
    }
    i2c->beginTransmission(i2c_addr);
    i2c->write(address);
    i2c->write( (const uint8_t *) &data, 2);
    i2c->endTransmission();
}

uint8_t Sensor::readRegister(uint8_t address)
{
    if (!i2c) { 
          return 0;
    }
    i2c->beginTransmission(i2c_addr);
    i2c->write(address);
    i2c->endTransmission();
    int numBytes = i2c->requestFrom( (uint8_t) i2c_addr, (uint8_t) 1);

    if (numBytes != 1) {
        printf_w("error: readRegister numBytes %d\n", numBytes );
        return 0;
    }
    return i2c->read();
}

uint8_t Sensor::updateRegister(uint8_t address,  uint8_t mask, uint8_t value)
{
    uint8_t reg;
    reg = readRegister(address);
    reg &= mask;
    reg |= value;
    writeRegister(address, reg);
}

uint8_t Sensor::burstRead(uint8_t baseAddress, uint8_t *buffer, uint8_t length)
{
    if (!i2c) {
          return 0;
    }
    i2c->beginTransmission(i2c_addr);
    i2c->write(baseAddress);
    i2c->endTransmission();
    
    int numBytes = i2c->requestFrom( (uint8_t) i2c_addr, (uint8_t) length );
    if (numBytes != length) {
        printf_w("error: burstRead i2c_addr %d,baseAddress %d, length %d, numBytes %d\n", i2c_addr,baseAddress, length, numBytes );

        return 0;
    }
    uint8_t idx;
    for (idx = 0; idx < numBytes; idx++) {
        buffer[idx] = i2c->read();
    }
    return idx;
}

//============================================================================================
GyroscopeLSM6DS3::GyroscopeLSM6DS3() {
    i2c_addr = LSM6DS3_ADDR;
    name     = "lsm6ds3";
    ulp_error = &ulp_lsm6ds3_error;

    steps    = 0;
}

GyroscopeLSM6DS3::~GyroscopeLSM6DS3(){
  
}

void GyroscopeLSM6DS3::init(SoftWire *_i2c) { 
    i2c =  _i2c;

    if ( false == Sensor::init() ) {
      
          return;
    }
    
   // delayMicroseconds(150); 

#ifdef MAIN_CORE_INIT_SENSORS
    uint8_t reg_id;
    
    uint8_t res =  burstRead(LSM6DS3_ACC_GYRO_WHO_AM_I_REG, &reg_id, 1);
    printf_w("LSM6DS3::init: reg_id = %d, res %d \n ", reg_id, res );
    if ( !( (LSM6DS3_ACC_GYRO_WHO_AM_I == reg_id) ||  (LSM6DS3_C_ACC_GYRO_WHO_AM_I == reg_id)) ) {
        *ulp_error = SENSOR_STATUS_HW_ERROR;
        return;
    }
    ulp_lsm6ds3_driver_sign = reg_id;
   
    uint8_t reg;
    reg = readRegister(LSM6DS3_ACC_GYRO_CTRL6_C);
    reg |= LSM6DS3_ACC_GYRO_XL_HM_MODE;
    writeRegister(LSM6DS3_ACC_GYRO_CTRL6_C, reg);

    reg = readRegister(LSM6DS3_ACC_GYRO_CTRL7_G);
    reg |= LSM6DS3_ACC_GYRO_G_HM_MODE;
    writeRegister(LSM6DS3_ACC_GYRO_CTRL7_G, reg);

  //  update_i2c_register LSM6DS3_ADDR LSM6DS3_ACC_GYRO_CTRL6_C fail_lsm6ds3_lowpower 0xef LSM6DS3_ACC_GYRO_XL_HM_MODE
  // update_i2c_register LSM6DS3_ADDR LSM6DS3_ACC_GYRO_CTRL7_G fail_lsm6ds3_lowpower 0x7f LSM6DS3_ACC_GYRO_G_HM_MODE

    writeRegister(LSM6DS3_ACC_GYRO_CTRL1_XL, (uint8_t) (LSM6DS3_ACC_GYRO_FS_XL_2g | LSM6DS3_ACC_GYRO_ODR_XL_26Hz) );
    writeRegister(LSM6DS3_ACC_GYRO_CTRL10_C, (uint8_t) 0x3C);   // Step 2: Set bit Zen_G, Yen_G, Xen_G, FUNC_EN, PEDO_RST_STEP(1 or 0)
    writeRegister(LSM6DS3_ACC_GYRO_TAP_CFG1, (uint8_t) 0x40);   // Step 3:  Enable pedometer algorithm
    writeRegister(LSM6DS3_ACC_GYRO_INT1_CTRL, (uint8_t) 0x00);   // Step 4: Step Detector interrupt driven to INT1 pin, set bit INT1_FIFO_OVR
    ulp_lsm6ds3_inited = 1;

#endif

}

void GyroscopeLSM6DS3::read() {
    // test for errcode from ulp and set value from rtc memory 
    printf_w("LSM6DS3 driver signature  %d, error_cnt %d, ulp_lsm6ds3_error %d, ulp_lsm6ds3_step_count %d, sm6ds3_inited %d\n", 
              ulp_lsm6ds3_driver_sign, ulp_lsm6ds3_error_cnt, ulp_lsm6ds3_error, ulp_lsm6ds3_step_count, ulp_lsm6ds3_inited );

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
    i2c_addr = MLX90615_ADDR;
    name = "mlx90615";
    ulp_error = &ulp_mlx90615_error;

    AmbientTempC = 0;
    ObjectTempC  = 0;
}

void ThermoMeter::wakeUp() {
    pinMode(GPIO_SCL, OUTPUT);    // sets the digital pin 13 as output
    digitalWrite(GPIO_SCL, LOW); // sets the digital pin 13 on
    delay(56);            // waits for a second
    digitalWrite(GPIO_SCL, HIGH);  // sets the digital pin 13 off
    //delay(100);            // waits for a second
}

void ThermoMeter::init(SoftWire *_i2c){
    i2c =  _i2c;
    
    wakeUp();
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
    printf_w("getObjectC -- %f\n", ObjectTempC);
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
    i2c_addr = MAX30100_ADDR;
    name = "max30100";
    ulp_error = &ulp_max30100_error ;
    
    heartRate = 0;
    spO2 = 0;
}

bool PulseMeter::dataIsReady(){
      
      return (ulp_max30100_flags & 0x2) ;
}

void PulseMeter::dropData(){
//     ulp_max30100_data_pointer    = ;
 //    ulp_max30100_raw_data        = ; 
  //   ulp_max30100_data_size_after = ;

     printf_w("dropData: ulp_max30100_flags 0x%x\n", ulp_max30100_flags);
     printUlpData();
     ulp_max30100_flags &= ~0x2;

     printf_w("dropData: ulp_max30100_flags 0x%x\n", ulp_max30100_flags);
     
}

void PulseMeter::init(SoftWire *_i2c){
    i2c =  _i2c;

    Sensor::init();

    uint8_t reg_id;
    
    uint8_t res =  burstRead(MAX30100_REG_PART_ID, &reg_id, 1);
    printf_w("max30100::init: reg_id = %d, res %d \n ", reg_id, res );
    
    if ( MAX30100_EXPECTED_PART_ID != reg_id ) {
        *ulp_error = SENSOR_STATUS_HW_ERROR;
        return;
    }
    uint16_t flags = ulp_max30100_flags;
    
    writeRegister(MAX30100_REG_MODE_CONFIGURATION, (uint8_t) 0x40);  // reset 
    updateRegister(MAX30100_REG_MODE_CONFIGURATION, 0x7f, MAX30100_MC_SHDN );

    writeRegister(MAX30100_REG_SPO2_CONFIGURATION, (uint8_t)  0x43);// (MAX30100_SPC_PW_1600US_16BITS & MAX30100_SAMPRATE_50HZ & MAX30100_SPC_SPO2_HI_RES_EN) ); 

    uint8_t irLedCurrent = MAX30100_LED_CURR_50MA,  redLedCurrent = MAX30100_LED_CURR_50MA;
    writeRegister(MAX30100_REG_LED_CONFIGURATION, (uint8_t) (redLedCurrent << 4 | irLedCurrent) ); 
    writeRegister(MAX30100_REG_MODE_CONFIGURATION, (uint8_t) MAX30100_MODE_HRONLY ); 
    
    writeRegister(MAX30100_REG_FIFO_WRITE_POINTER, (uint8_t) 0x00); 
    writeRegister(MAX30100_REG_FIFO_OVERFLOW_COUNTER, (uint8_t) 0x00); 
    writeRegister(MAX30100_REG_FIFO_READ_POINTER, (uint8_t) 0x00); 

   // updateRegister(MAX30100_REG_MODE_CONFIGURATION,  0x7f, 0 ); // 
    ulp_max30100_flags = ulp_max30100_flags | 1 ; // sensor is inited #1, data is full #2
  //  delay(50);  
   /*
    write_i2c MAX30100_ADDR MAX30100_REG_MODE_CONFIGURATION 0x40 fail_max30100_init // reset
    update_i2c_register MAX30100_ADDR MAX30100_REG_MODE_CONFIGURATION fail_max30100_init 0x7f MAX30100_MC_SHDN

    // MAX30100_SPC_PW_200US_13BITS
    move r0, 0x43 // = MAX30100_SPC_PW_1600US_16BITS (0x03) & MAX30100_SAMPRATE_50HZ (0x0 << 2) & MAX30100_SPC_SPO2_HI_RES_EN (0x40)
    write_i2c MAX30100_ADDR MAX30100_REG_SPO2_CONFIGURATION r0 fail_max30100_init

    move r0, 0xff // = MAX30100_LED_CURR_50MA(0xf) & MAX30100_LED_CURR_50MA (0xf)
    write_i2c MAX30100_ADDR MAX30100_REG_LED_CONFIGURATION r0 fail_max30100_init
    write_i2c MAX30100_ADDR MAX30100_REG_MODE_CONFIGURATION MAX30100_MODE_HRONLY fail_max30100_read_1

    write_i2c MAX30100_ADDR MAX30100_REG_FIFO_WRITE_POINTER 0x0 fail_max30100_read_1
    write_i2c MAX30100_ADDR MAX30100_REG_FIFO_OVERFLOW_COUNTER 0x0 fail_max30100_read_1
    write_i2c MAX30100_ADDR MAX30100_REG_FIFO_READ_POINTER 0x0 fail_max30100_read_1
    update_i2c_register MAX30100_ADDR MAX30100_REG_MODE_CONFIGURATION fail_max30100_read_1 0x7f 0
    set_flag max30100_flags 1 // sensor is inited #1, data is full #2
    
    move r2, 50    // wait some time 
    psr
    jump waitMs

 
//   jump buffer_is_full
    jump max30100_readFifoData  // test
       */
}

void PulseMeter::wake(){
    ulp_sensors_switch_extern |=  SENSOR_MAX30100;

}

void PulseMeter::sleep(){
    ulp_sensors_switch_extern &= ~SENSOR_MAX30100;

}   

void PulseMeter::read() {
    // test for errcode from ulp and set value from rtc memory 
    printUlpData();
  //  printRawData();

    heartRate = 0;
    spO2  = 0;
}


dcFilter_t PulseMeter::dcRemoval(float x, float prev_w, float alpha)
{
    dcFilter_t filtered;
    filtered.w = x + alpha * prev_w;
    filtered.result = filtered.w - prev_w;

    return filtered;
}

float PulseMeter::meanDiff(float M, meanDiffFilter_t* filterValues)
{
    float avg = 0;

    filterValues->sum -= filterValues->values[filterValues->index];
    filterValues->values[filterValues->index] = M;
    filterValues->sum += filterValues->values[filterValues->index];
  
    filterValues->index++;
    filterValues->index = filterValues->index % MEAN_FILTER_SIZE;

    if  (filterValues->count < MEAN_FILTER_SIZE)
        filterValues->count++;

    avg = filterValues->sum / filterValues->count;
    return avg - M;
}

class  FilterBuBp1
{
  public:
    FilterBuBp1()
    {
      v[0]=0.0;
      v[1]=0.0;
      v[2]=0.0;
    }
  private:
    float v[3];
  public:
    float step(float x) //class II 
    {
      v[0] = v[1];
      v[1] = v[2];
      v[2] = (1.066593595520557081e+1 * x)
         + (-22.19978474622116948467 * v[0])
         + (-9.42332950633080024261 * v[1]);
      return 
         (v[2] - v[0]);
    }
};


void PulseMeter::lowPassButterworthFilter( float x, butterworthFilter_t * filterResult )
{  
  filterResult->v[0] = filterResult->v[1];

  //Fs = 100Hz and Fc = 10Hz
  // filterResult->v[1] = (2.452372752527856026e-1 * x) + (0.50952544949442879485 * filterResult->v[0]);

  //Fs = 100Hz and Fc = 4Hz
  //filterResult->v[1] = (1.367287359973195227e-1 * x) + (0.72654252800536101020 * filterResult->v[0]); //Very precise butterworth filter 
// http://www.schwietering.com/jayduino/filtuino/index.php?characteristic=bu&passmode=lp&order=1&usesr=usesr&sr=25&frequencyLow=10&noteLow=&noteHigh=&pw=pw&calctype=float&run=Send
  //Fs = 25Hz and Fc = 10Hz -- lowpass  1 order
  filterResult->v[1] = (7.547627247472143974e-1 * x)
         + (-0.50952544949442879485 * filterResult->v[0]);


  //Fs = 50Hz and Fc = 10Hz -- lowpass  1 order
   //filterResult->v[1] = (4.208077798377318768e-1 * x)
     //    + (0.15838444032453627419 * filterResult->v[0]);
         
         
  filterResult->result = filterResult->v[0] + filterResult->v[1];
}


void PulseMeter::process_Value(uint16_t rawIR) {
  
        dcFilterIR = dcRemoval( (float) rawIR, dcFilterIR.w, ALPHA );
//        
        float meanDiffResIR = meanDiff( dcFilterIR.result, &meanDiffIR);
        lowPassButterworthFilter( meanDiffResIR/*-dcFilterIR.result*/, &lpbFilterIR );
        printf_w("%d\t%f\t%f\t%f\r\n", rawIR , dcFilterIR.result,  meanDiffResIR, lpbFilterIR.result  );
}

void PulseMeter::printRawData () {
    dcFilterIR.w = 0;
    dcFilterIR.result = 0;

    meanDiffIR.index = 0;
    meanDiffIR.sum = 0;
    meanDiffIR.count = 0;
    memset( meanDiffIR.values, 0, MEAN_FILTER_SIZE * sizeof(float) );

    lpbFilterIR.v[0] = 0;
    lpbFilterIR.v[1] = 0;
    lpbFilterIR.result = 0;
  
    printf_w("max30100_raw_data :\n");

    for (int i = 0; i < 124; i++) {
     //   uint16_t rawIR = (uint16_t) *( ulp_max30100_raw_data + i);
        uint16_t rawIR =   (uint16_t) *(ulp_max30100_raw_data +i) ;
        process_Value((rawIR ));
//       process_Value((rawIR >> 8));
  //     process_Value((rawIR & 0xff));
    }
    printf_w("\n");
}

float PulseMeter::getHeartRate(){
    return heartRate;
}

float PulseMeter::getSpO2(){
    return spO2;
}
void PulseMeter::printUlpData(){
    printf_w("max30100 data: loop_counter %d, "
                      "get_all_fifo_counter %d, "
                      "fifo_filled %d, "
                      "toRead %d, "
                      "fifo_num_available_samples %d, "
                      "fifo_overflow_counter %d, "
                      "flags %d"
                      "\n", 
                      (uint16_t)ulp_max30100_loop_counter,
                      (uint16_t) ulp_max30100_get_all_fifo_counter,
                      (uint16_t)ulp_max30100_fifo_filled,
                      (uint16_t)ulp_max30100_toRead,
                      (uint16_t)ulp_max30100_fifo_num_available_samples,
                      (uint16_t)ulp_max30100_fifo_overflow_counter,
                      (uint16_t)ulp_max30100_flags
                      
                      
                      );



   
    printf_w("max30100 data: max30100_data_size %d, data_size_filled %d, free_data_size %d\n", 
            ulp_max30100_data_size, ulp_max30100_data_size_after, ulp_max30100_free_data_size
          );

}
//============================================================================================

//  https://www.jackogrady.me/battery-management-system/state-of-charge
CurrentMeter::CurrentMeter() {
    i2c_addr = INA219_ADDR;
  
    name = "ina219";
    ulp_error = &ulp_ina219_error ;

    vcc = 0;

    battery = new Battery(3200, 4200, A0);
#ifdef IS_V1_3
    battery->begin(200, 1.0, &sigmoidal);
#else
    battery->begin(130, 1.0, &sigmoidal);
#endif
}


void CurrentMeter::setCalibration(uint16_t calValue){
    
    writeRegister(INA219_REG_CALIBRATION, calValue);
}


void CurrentMeter::init(SoftWire *_i2c){
    i2c =  _i2c;

    if ( false == Sensor::init() ) {
        return;
    }

#ifdef MAIN_CORE_INIT_SENSORS
   ulp_ina219_calValue = 4096;
   ulp_ina219_currentDivider_mA = 10;
   setCalibration(ulp_ina219_calValue);

   uint16_t reg_conf = INA219_CONFIG_BVOLTAGERANGE_32V | INA219_CONFIG_GAIN_8_320MV | INA219_CONFIG_BADCRES_12BIT |
                       INA219_CONFIG_SADCRES_12BIT_1S_532US; // | INA219_CONFIG_MODE_SANDBVOLT_CONTINUOUS;
   ulp_ina219_config  = reg_conf;
   writeRegister(INA219_REG_CONFIG, reg_conf);
   
#endif
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
/*
    printf_w("current list\n");
    for (uint32_t i = 0; i< 60; i++){
        printf_w("%d.\t %fma\n", i, (float) ( (int16_t)ulp_ina219_current_table[i]) / (float)ulp_ina219_currentDivider_mA);
    }
*/
    vcc      = (float) (ulp_ina219_voltage * 4 ) * 0.001;
    current  = (float) ulp_ina219_current / (float) ulp_ina219_currentDivider_mA;
    batLevel = battery->level( vcc *1000 );
    printf_w("ina219 : vcc %f, batLevel %d, currentDivider_mA %f  \n", vcc, batLevel, (float) ulp_ina219_currentDivider_mA );

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

void CurrentMeter::printUlpData() {
    printf_w("ina219 data: config %x, current %fmA, "
                      "voltage %fV, "
                      "currentDivider_mA = %d, calValue = %u"
                      "\n", 
                      
                      (uint16_t)ulp_ina219_config,
                      (float) ulp_ina219_current/(float)ulp_ina219_currentDivider_mA ,
                      (float)(ulp_ina219_voltage *4)*0.001,
                      (uint16_t) ulp_ina219_currentDivider_mA,
                      (uint16_t) ulp_ina219_calValue
                      );
}
//============================================================================================
TimeMeter::TimeMeter(){
    i2c_addr = 0x68;
    name = "ds3231";
    ulp_error = &ulp_ds3231_error;
}

void TimeMeter::init(SoftWire *_i2c){
    i2c =  _i2c;

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

void TimeMeter::printUlpData() {

    printf_w("ds3231: ulp data %d/%d   %02d:%02d:%02d\n", ulp_ds3231_month, ulp_ds3231_day, ulp_ds3231_hour, ulp_ds3231_minute, ulp_ds3231_second);  
    printf_w("ds3231 SET DATA: %02d:%02d:%02d  %02d.%02d.%02d (%d dow)  update_flag %d\n",
                   bcd2dec(ulp_ds3231_set_hour), bcd2dec(ulp_ds3231_set_minute),  bcd2dec(ulp_ds3231_set_second),
                   bcd2dec(ulp_ds3231_set_day),  bcd2dec(ulp_ds3231_set_month),  bcd2dec(ulp_ds3231_set_year) + 2000,  
                   bcd2dec(ulp_ds3231_set_dayOfWeek) , (uint16_t) ulp_ds3231_update_flag);

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
RTC_DATA_ATTR bme280CalibData   calibData;

BME280Meter::BME280Meter(){
    i2c_addr = BME280_ADDR;
    name     = "bme280";
    ulp_error = &ulp_bme280_error;
}

void BME280Meter::init(SoftWire *_i2c) {
    i2c =  _i2c;
    if ( false == Sensor::init() ) {
        return;
    }
#ifdef MAIN_CORE_INIT_SENSORS
    uint8_t reg_id = readRegister(BMP280_REG_ID);
    if ( reg_id != BME280_CHIP_ID ) {
        return;
    }

    uint8_t calib_data[BME280_TEMP_PRESS_CALIB_DATA_LEN] = { 0 };
    if (0 == burstRead(BME280_REG_T_CAL, calib_data, BME280_TEMP_PRESS_CALIB_DATA_LEN)) {
        return;
    }
    parseTempPressCalibData(calib_data);

    if (0 == burstRead(BME280_REG_H_CAL, calib_data, BME280_HUMIDITY_CALIB_DATA_LEN) ) {
        return;
    }
    parseHumidityCalibData(calib_data);

    outCalibData();
    
    writeRegister(BME280_REG_CTRL_MEAS, (uint8_t) BME280_MODE_SLEEP);   // off all
    
    uint8_t standbyTime = 0x00; // 0x00 - 0.5ms
    uint8_t filter      = 0x00; // 0x00 - filter off
    uint8_t spiEnable   = 0x00; // 1 - disable
    // config register. (config[7:5] = standby time, config[4:2] = filter, ctrl_meas[0] = spi enable.)
    uint8_t config = ((uint8_t) standbyTime << 5) | ((uint8_t) filter << 2) | (uint8_t) spiEnable;
    writeRegister(BMP280_REG_CONFIG, (uint8_t) config);
    
    // ctrl_hum register. (ctrl_hum[2:0] = Humidity oversampling rate.)
    uint8_t ctrlHum = (uint8_t)0x01;  // 1 * oversampling
    writeRegister(BME280_REG_CTRL_HUMIDITY, (uint8_t) ctrlHum);  

    uint8_t tempOSR = 0x01; //  1 * oversampling
    uint8_t presOSR = 0x01; //  1 * oversampling
    uint8_t mode    = BME280_MODE_SLEEP; // BME280_MODE_NORMAL;//  BME280_MODE_SLEEP;    
    // ctrl_meas register. (ctrl_meas[7:5] = temperature oversampling rate, ctrl_meas[4:2] = pressure oversampling rate, ctrl_meas[1:0] = mode.)
    uint8_t ctrlMeas = ((uint8_t) tempOSR << 5) | ((uint8_t) presOSR << 2) | (uint8_t) mode;
    writeRegister(BME280_REG_CTRL_MEAS, (uint8_t) ctrlMeas);
    outCalibData();
    printf_w("BME280: ALL INIT OK\n");
#endif
}

void BME280Meter::printUlpData(){
    printf_w("bme280 data: ulp_bme280_DEBUG %d\n", ulp_bme280_DEBUG );
 //   outCalibData();
/*
    printf_w("BME280 ulp_calib: dig_t1 0x%x, dig_t2 0x%x, dig_t3 0x%x, \n"
              "BME280 ulp_calib: dig_t1 %d, dig_t2 %d, dig_t3 %d, \n"
              "BME280 ulp_calib: dig_p1 0x%x, dig_p2 0x%x, dig_p3 0x%x, dig_p4 0x%x, dig_p5 0x%x, dig_p6 0x%x, dig_p7 0x%x, dig_p8 0x%x, dig_p9 0x%x\n"
              "BME280 ulp_calib: dig_h1 0x%x, dig_h2 0x%x, dig_h3 0x%x, dig_h4 0x%x, dig_h5 0x%x, dig_h6 0x%x\n",
              ulp_bme280_calib_dig_t1, ulp_bme280_calib_dig_t2, ulp_bme280_calib_dig_t3, 
              ulp_bme280_calib_dig_t1, ulp_bme280_calib_dig_t2, ulp_bme280_calib_dig_t3,  
 
              ulp_bme280_calib_dig_p1, ulp_bme280_calib_dig_p2, ulp_bme280_calib_dig_p3, ulp_bme280_calib_dig_p4,  ulp_bme280_calib_dig_p5,
              ulp_bme280_calib_dig_p6, ulp_bme280_calib_dig_p7, ulp_bme280_calib_dig_p8, ulp_bme280_calib_dig_p9,
              ulp_bme280_calib_dig_h1, ulp_bme280_calib_dig_h2, ulp_bme280_calib_dig_h3, ulp_bme280_calib_dig_h4,  ulp_bme280_calib_dig_h5, ulp_bme280_calib_dig_h6
              );
*/
}

void BME280Meter::parseSensorData()
{
    uint32_t data_xlsb;
    uint32_t data_lsb;
    uint32_t data_msb;

    data_msb  = (uint32_t)ulp_bme280_data_press_MSB_LSB << 4;
    data_xlsb = (uint32_t)ulp_bme280_data_press_XLSB_temp_MSB >> 12; // 8 temp_MSB + 4 rest from press_XLSB
    uData.pressure = data_msb | data_xlsb;

    data_msb = ( (uint32_t) ulp_bme280_data_press_XLSB_temp_MSB & 0xff) << 12; // 8 
    data_lsb =  (uint32_t) ulp_bme280_data_temp_LSB_temp_XLSB >> 4; // shrink 4 bit
    uData.temperature = data_msb | data_lsb ;

    uData.humidity = ulp_bme280_data_hum_MSB_LSB;

    printf_w("bme280 parseSensorData: press_MSB_LSB 0x%x, press_XLSB_temp_MSB 0x%x, temp_LSB_temp_XLSB 0x%x, hum_MSB_LSB 0x%x\n", 
              ulp_bme280_data_press_MSB_LSB, ulp_bme280_data_press_XLSB_temp_MSB, ulp_bme280_data_temp_LSB_temp_XLSB, ulp_bme280_data_hum_MSB_LSB);
    printf_w("bme280 parseSensorData: temperature %d (%x), pressure %d (%x), humidity %d (%x)\n", 
              uData.temperature,uData.temperature,  uData.pressure,  uData.pressure, uData.humidity, uData.humidity );

}

#ifdef MAIN_CORE_INIT_SENSORS

void BME280Meter::parseTempPressCalibData(const uint8_t *reg_data)
{
    calibData.dig_t1 = BME280_CONCAT_BYTES(reg_data[1], reg_data[0]);
    calibData.dig_t2 = (int16_t)BME280_CONCAT_BYTES(reg_data[3], reg_data[2]);
    calibData.dig_t3 = (int16_t)BME280_CONCAT_BYTES(reg_data[5], reg_data[4]);
    calibData.dig_p1 = BME280_CONCAT_BYTES(reg_data[7], reg_data[6]);
    calibData.dig_p2 = (int16_t)BME280_CONCAT_BYTES(reg_data[9], reg_data[8]);
    calibData.dig_p3 = (int16_t)BME280_CONCAT_BYTES(reg_data[11], reg_data[10]);
    calibData.dig_p4 = (int16_t)BME280_CONCAT_BYTES(reg_data[13], reg_data[12]);
    calibData.dig_p5 = (int16_t)BME280_CONCAT_BYTES(reg_data[15], reg_data[14]);
    calibData.dig_p6 = (int16_t)BME280_CONCAT_BYTES(reg_data[17], reg_data[16]);
    calibData.dig_p7 = (int16_t)BME280_CONCAT_BYTES(reg_data[19], reg_data[18]);
    calibData.dig_p8 = (int16_t)BME280_CONCAT_BYTES(reg_data[21], reg_data[20]);
    calibData.dig_p9 = (int16_t)BME280_CONCAT_BYTES(reg_data[23], reg_data[22]);
    calibData.dig_h1 = reg_data[25];
}

void BME280Meter::parseHumidityCalibData(const uint8_t *reg_data)
{
    int16_t dig_h4_lsb;
    int16_t dig_h4_msb;
    int16_t dig_h5_lsb;
    int16_t dig_h5_msb;

    calibData.dig_h2 = (int16_t)BME280_CONCAT_BYTES(reg_data[1], reg_data[0]);
    calibData.dig_h3 = reg_data[2];
    dig_h4_msb = (int16_t)(int8_t)reg_data[3] * 16;
    dig_h4_lsb = (int16_t)(reg_data[4] & 0x0F);
    calibData.dig_h4 = dig_h4_msb | dig_h4_lsb;
    dig_h5_msb = (int16_t)(int8_t)reg_data[5] * 16;
    dig_h5_lsb = (int16_t)(reg_data[4] >> 4);
    calibData.dig_h5 = dig_h5_msb | dig_h5_lsb;
    calibData.dig_h6 = (int8_t)reg_data[6];
}

void BME280Meter::outCalibData() {
    printf_w("calibData %p\n", &calibData );

    printf_w("BME280 calib: dig_t1 0x%x, dig_t2 0x%x, dig_t3 0x%x, \n"
              "BME280 calib: dig_t1 %d, dig_t2 %d, dig_t3 %d, \n"
              "BME280 calib: dig_p1 0x%x, dig_p2 0x%x, dig_p3 0x%x, dig_p4 0x%x, dig_p5 0x%x, dig_p6 0x%x, dig_p7 0x%x, dig_p8 0x%x, dig_p9 0x%x\n"
              "BME280 calib: dig_h1 0x%x, dig_h2 0x%x, dig_h3 0x%x, dig_h4 0x%x, dig_h5 0x%x, dig_h6 0x%x\n",
              calibData.dig_t1, calibData.dig_t2, calibData.dig_t3, 
              calibData.dig_t1, calibData.dig_t2, calibData.dig_t3,  
 
              calibData.dig_p1, calibData.dig_p2, calibData.dig_p3, calibData.dig_p4, calibData.dig_p5,
              calibData.dig_p6, calibData.dig_p7, calibData.dig_p8, calibData.dig_p9,
              calibData.dig_h1, calibData.dig_h2, calibData.dig_h3, calibData.dig_h4,  calibData.dig_h5, calibData.dig_h6);
}

#else

void BME280Meter::parseTempPressCalibDataUlp()
{
    calibData.dig_t1 = BME280_SWAP_BYTES(ulp_bme280_calib_dig_t1);
    calibData.dig_t2 = (int16_t)BME280_SWAP_BYTES(ulp_bme280_calib_dig_t2);
    calibData.dig_t3 = (int16_t)BME280_SWAP_BYTES(ulp_bme280_calib_dig_t3);
    calibData.dig_p1 = BME280_SWAP_BYTES(ulp_bme280_calib_dig_p1);
    calibData.dig_p2 = (int16_t)BME280_SWAP_BYTES(ulp_bme280_calib_dig_p2);
    calibData.dig_p3 = (int16_t)BME280_SWAP_BYTES(ulp_bme280_calib_dig_p3);
    calibData.dig_p4 = (int16_t)BME280_SWAP_BYTES(ulp_bme280_calib_dig_p4);
    calibData.dig_p5 = (int16_t)BME280_SWAP_BYTES(ulp_bme280_calib_dig_p5);
    calibData.dig_p6 = (int16_t)BME280_SWAP_BYTES(ulp_bme280_calib_dig_p6);
    calibData.dig_p7 = (int16_t)BME280_SWAP_BYTES(ulp_bme280_calib_dig_p7);
    calibData.dig_p8 = (int16_t)BME280_SWAP_BYTES(ulp_bme280_calib_dig_p8);
    calibData.dig_p9 = (int16_t)BME280_SWAP_BYTES(ulp_bme280_calib_dig_p9);

    printf_w("bme280 cal data:  dig_t1 %d,  dig_t2 %d,  dig_t3 %d, "
            " dig_p1 %d, dig_p2 %d, dig_p3 %d, dig_p4 %d,"
            " dig_p5 %d, dig_p6 %d, dig_p7 %d, dig_p8 %d,"
            " dig_p9 %d\n",  calibData.dig_t1, calibData.dig_t2, calibData.dig_t3, 
            calibData.dig_p1, calibData.dig_p2, calibData.dig_p3, calibData.dig_p4,
            calibData.dig_p5, calibData.dig_p6, calibData.dig_p7, calibData.dig_p8,
            calibData.dig_p9);
/**/
}

void BME280Meter::parseHumidityCalibDataUlp( )
{
    int16_t dig_h4_lsb;
    int16_t dig_h4_msb;
    int16_t dig_h5_lsb;
    int16_t dig_h5_msb;
    
    calibData.dig_h1 = (ulp_bme280_calib_dig_h1 & 0xff);
    calibData.dig_h2 = (int16_t)BME280_SWAP_BYTES(ulp_bme280_calib_dig_h2);
    calibData.dig_h3 = ( ulp_bme280_calib_dig_h3 >> 8) & 0xff  ;// reg_data[2];   
    
    dig_h4_msb =  ((int16_t)ulp_bme280_calib_dig_h3 & 0xff) << 4  ;//(int16_t)(int8_t)reg_data[3] * 16;
    dig_h4_lsb =  ((int16_t)ulp_bme280_calib_dig_h4 >> 8) & 0x0f  ;//(int16_t)(reg_data[4] & 0x0F);
    calibData.dig_h4 = dig_h4_msb | dig_h4_lsb;
    dig_h5_msb = ((int16_t) ulp_bme280_calib_dig_h4 & 0xff ) << 4;//(int16_t)(int8_t)reg_data[5] * 16;
    dig_h5_lsb =   ((int16_t)ulp_bme280_calib_dig_h4 >> (8 + 4)) & 0x0f ;//(int16_t)(reg_data[4] >> 4);
    calibData.dig_h5 = dig_h5_msb | dig_h5_lsb;
    calibData.dig_h6 = (int8_t)( ulp_bme280_calib_dig_h5 >> 8) & 0xff;//(int8_t)reg_data[6];
    
    printf_w("bme280 cal data: dig_h1 0x%x, dig_h2 0x%x, dig_h3 0x%x, dig_h4 0x%x, dig_h5  0x%x, dig_h6 0x%x\n", 
         calibData.dig_h1,  calibData.dig_h2, calibData.dig_h3, calibData.dig_h4,  calibData.dig_h5, calibData.dig_h6);
}
#endif

float BME280Meter::readTemperature() {
    int32_t var1, var2;

    int32_t adc_T = uData.temperature;

    var1 = ((((adc_T >> 3) - ((int32_t) calibData.dig_t1 << 1))) *
          ((int32_t)calibData.dig_t2)) >>
         11;

    var2 = (((((adc_T >> 4) - ((int32_t) calibData.dig_t1)) *
            ((adc_T >> 4) - ((int32_t) calibData.dig_t1))) >>
           12) *
          ((int32_t) calibData.dig_t3)) >>
         14;

    t_fine = var1 + var2;

    float T = (t_fine * 5 + 128) >> 8;
    return T / 100;
}


float BME280Meter::readPressure() {
    int64_t var1, var2, p;

    // Must be done first to get the t_fine variable set up
    //readTemperature();

    int32_t adc_P = uData.pressure;

    var1 = ((int64_t) t_fine) - 128000;
    var2 = var1 * var1 * (int64_t) calibData.dig_p6;
    var2 = var2 + ((var1 * (int64_t) calibData.dig_p5) << 17);
    var2 = var2 + (((int64_t) calibData.dig_p4 ) << 35);
    var1 = ((var1 * var1 * (int64_t) calibData.dig_p3) >> 8) + ((var1 * (int64_t) calibData.dig_p2) << 12);
    var1 = (((((int64_t)1) << 47) + var1)) * ((int64_t)calibData.dig_p1) >> 33; 

    if (var1 == 0) {
        return 0; // avoid exception caused by division by zero
    }
    p = 1048576 - uData.pressure;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = (((int64_t) calibData.dig_p9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (((int64_t) calibData.dig_p8) * p) >> 19;

    p = ((p + var1 + var2) >> 8) + (((int64_t) calibData.dig_p7) << 4);
    return (float)p / 256;
}

float BME280Meter::readHumidity( )
{
    int32_t adc_H = uData.humidity;
    float humidity;
    double humidity_min = 0.0;
    double humidity_max = 100.0;
    double var1;
    double var2;
    double var3;
    double var4;
    double var5;
    double var6;

    var1 = ((double)t_fine) - 76800.0;
    var2 = (((double)calibData.dig_h4) * 64.0 + (((double)calibData.dig_h5) / 16384.0) * var1);
    var3 = adc_H - var2;
    var4 = ((double)calibData.dig_h2) / 65536.0;
    var5 = (1.0 + (((double)calibData.dig_h3) / 67108864.0) * var1);
    var6 = 1.0 + (((double)calibData.dig_h6) / 67108864.0) * var1 * var5;
   // printf_w("bme280 data: var1 %f, var2 %f, var3 %f, var4 %f, var5 %f, var6 %f\n" , var1, var2, var3, var4, var5, var6 ) ; 
    var6 = var3 * var4 * (var5 * var6);
    humidity = var6 * (1.0 - ((double)calibData.dig_h1) * var6 / 524288.0);
    printf_w("bme280 data: var1 %f, humidity %f \n", var6,  humidity);
    if (humidity > humidity_max)
    {
        humidity = humidity_max;
    }
    else if (humidity < humidity_min)
    {
        humidity = humidity_min;
    }

    return humidity;
}

 
void BME280Meter::read() {
    if (*ulp_error)
        return;
    printUlpData();
    
#ifndef MAIN_CORE_INIT_SENSORS
    parseTempPressCalibDataUlp();
    parseHumidityCalibDataUlp();
#endif

    parseSensorData();
    temperature = readTemperature();
    pressure    = readPressure();
    humidity    = readHumidity();

    printf_w("bme280 data: temperature %f , pressure %f, humidity %f%\n", temperature, pressure, humidity);
}

float BME280Meter::getPressure() {
    return pressure;
}

float BME280Meter::getHumidity() {
    return humidity;
}


void BME280Meter::wake(){
    ulp_sensors_switch_extern |= SENSOR_BME280;
            printf_w("PulseMeter::wake\n");

}

void BME280Meter::sleep(){
    ulp_sensors_switch_extern &= ~SENSOR_BME280;

}

//============================================================================================
