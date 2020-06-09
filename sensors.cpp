#include "sensors.hpp"
#include "esp_adc_cal.h"

//============================================================================================
#ifdef _USE_BMI160_
GyroscopeBMI160::GyroscopeBMI160() {
    steps = 0;
}

void GyroscopeBMI160::init(bool is_after_deepsleep) {
    inited = false;
KK
    gyro.begin(BMI160GenClass::I2C_MODE, Wire, i2c_addr, 2, is_after_deepsleep);
    bool sce = gyro.getStepCountEnabled();
    uint8_t sdm = gyro.getStepDetectionMode();

    print_w("StepCountEnabled ");  println_w( sce ? "true" : "false" );
    print_w("StepDetectionMode "); println_w( sdm );
    
    if ( !sce ) {
        println_w( "init gyro  step counter" ) ;
        gyro.setStepCountEnabled(true);
        sce = gyro.getStepCountEnabled();
        
        print_w("StepCountEnable #2  ");
        println_w(sce ? "true" : "false"  );
    }
    else {
        println_w( "gyro step inited" );
    }
    uint16_t sc = gyro.getStepCount();
    print_w("StepCount ");  println_w(sc );
    inited = true;
    return;//  gyro.getStepCount();
}


void GyroscopeBMI160::read_data() {
    if (false == inited) 
        return;
    steps      = gyro.getStepCount();
}

uint16_t GyroscopeBMI160::StepCount(){
    return  steps;
}

void GyroscopeBMI160::wake(){
}

void GyroscopeBMI160::sleep(){
}
#endif
//============================================================================================
#ifdef _USE_LSM6DS3_
GyroscopeLSM6DS3::GyroscopeLSM6DS3() {
    steps = 0;
}
GyroscopeLSM6DS3::~GyroscopeLSM6DS3(){
    free(gyro);
}
void GyroscopeLSM6DS3::init(bool is_after_deepsleep) {
    inited = false;
    gyro = new LSM6DS3Core( I2C_MODE, i2c_addr );

    if( gyro->beginCore() != 0 )
    {
        Serial.print("Error at beginCore().\n");
        return;
    }
    
  //Error accumulation variable
    uint8_t errorAccumulator = 0;

    uint8_t dataToWrite = 0;  //Temporary variable

    //Setup the accelerometer******************************
    dataToWrite = 0; //Start Fresh!
    //  dataToWrite |= LSM6DS3_ACC_GYRO_BW_XL_200Hz;
    dataToWrite |= LSM6DS3_ACC_GYRO_FS_XL_2g;
    dataToWrite |= LSM6DS3_ACC_GYRO_ODR_XL_26Hz;

    // //Now, write the patched together data
    errorAccumulator += gyro->writeRegister(LSM6DS3_ACC_GYRO_CTRL1_XL, dataToWrite);

  //  //Set the ODR bit
  //  errorAccumulator += gyro->readRegister(&dataToWrite, LSM6DS3_ACC_GYRO_CTRL4_C);
  //  dataToWrite &= ~((uint8_t)LSM6DS3_ACC_GYRO_BW_SCAL_ODR_ENABLED);

  
    // Enable embedded functions -- ALSO clears the pdeo step count
    int flag10 = 0x3C; //  all except reset pedometer
   // if (!is_after_deepsleep) 
     //     flag10 |= LSM6DS3_ACC_GYRO_PEDO_RST_STEP_ENABLED; // up reset flag
          
    errorAccumulator += gyro->writeRegister(LSM6DS3_ACC_GYRO_CTRL10_C, flag10);
    // Enable pedometer algorithm
    errorAccumulator += gyro->writeRegister(LSM6DS3_ACC_GYRO_TAP_CFG1, 0x40);
    // Step Detector interrupt driven to INT1 pin
    errorAccumulator += gyro->writeRegister( LSM6DS3_ACC_GYRO_INT1_CTRL, 0x10 );
  
  /*
    uint16_t sc = gyro.getStepCount();
    print_w("StepCount ");  println_w(sc );
*/
    inited = true;
    return;
}


void GyroscopeLSM6DS3::read_data() {
    if (false == inited) 
        return;

    uint8_t readDataByte = 0;
    uint16_t stepsTaken = 0;
  //Read the 16bit value by two 8bit operations
    gyro->readRegister(&readDataByte, LSM6DS3_ACC_GYRO_STEP_COUNTER_H);
    stepsTaken = ((uint16_t)readDataByte) << 8;
    gyro->readRegister(&readDataByte, LSM6DS3_ACC_GYRO_STEP_COUNTER_L);
    stepsTaken |= readDataByte;
    steps = stepsTaken;
}

uint16_t GyroscopeLSM6DS3::StepCount(){
    return  steps;
}

void GyroscopeLSM6DS3::wake(){
}

void GyroscopeLSM6DS3::sleep(){
}
#endif
//============================================================================================
ThermoMeter::ThermoMeter() {
    AmbientTempC = 0;
    ObjectTempC  = 0;
}

void ThermoMeter::init(){
#if defined(_USE_MLX90614_)
    therm.begin(); // Initialize the MLX90614
    therm.setUnit(TEMP_C);   
    therm.wake();
#elif defined(_USE_MLX90615_)
    therm.wakeUp();
#endif
    inited = true;
}

void ThermoMeter::update(){
#if defined(_USE_MLX90614_)
    therm.read();
#endif
}

void ThermoMeter::wake(){
    therm.wakeUp();
}

void ThermoMeter::sleep(){
#if defined(_USE_MLX90614_) 
    therm.sleep(); 
#elif defined(_USE_MLX90615_)
    therm.sleep(); // MLX9061* demands magic action with wire before go to sleep so it should be last wire sleep call
#endif
}

void ThermoMeter::read_data(){
#if defined(_USE_MLX90614_)
    AmbientTempC = therm.ambient(); 
    ObjectTempC  = therm.object();
#elif defined(_USE_MLX90615_)
    AmbientTempC = therm.readTemp(MLX90615::MLX90615_SRCA, MLX90615::MLX90615_TC);
    ObjectTempC  = therm.readTemp();
#endif
}

float ThermoMeter::AmbientC(){
    return AmbientTempC;
}

float ThermoMeter::ObjectC() {
    return ObjectTempC;
}
//============================================================================================
PulseMeter::PulseMeter(){
    heartRate = 0;
    spO2 = 0;
}

void PulseMeter::init(){
    pulse.begin();// PULSEOXIMETER_DEBUGGINGMODE_PULSEDETECT
    pulse.setIRLedCurrent(MAX30100_LED_CURR_7_6MA);
    inited = true;
   // pulse.setIRLedCurrent(MAX30100_LED_CURR_7_6MA);
}

void PulseMeter::update(){
    pulse.update();
}

void PulseMeter::wake(){
    pulse.resume();
}

void PulseMeter::sleep(){
    pulse.shutdown();
}   

void PulseMeter::read_data() {
    heartRate = pulse.getHeartRate();
    spO2      = pulse.getSpO2();
}

float PulseMeter::HeartRate(){
    return heartRate;
}

float PulseMeter::SpO2(){
    return spO2;
}

//============================================================================================
CurrentMeter::CurrentMeter() {
    vcc = 0;
}

void CurrentMeter::init(){
    ina219.begin();
    inited = true;
}

void CurrentMeter::read_data(){
    vcc =  ina219.getBusVoltage_V();
}

void CurrentMeter::wake(){
    ina219.powerSave(false);
}

void CurrentMeter::sleep(){
    ina219.powerSave(true);
}

float CurrentMeter::Vcc(){
    return vcc;
}

int cmp_volt(const uint32_t *a, const uint32_t *b)
{
    if (*a < *b) {
        return -1;
    } else if (*a == *b) {
        return 0;
    } else {
        return 1;
    }
}

uint32_t CurrentMeter::get_battery_voltage(void)
{
    uint32_t ad_volt_list[BATTERY_ADC_SAMPLE];
    esp_adc_cal_characteristics_t characteristics;

    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(BATTERY_ADC_CH, ADC_ATTEN_11db);
    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, ADC_VREF,
                             &characteristics);
/*
    for (uint32_t i = 0; i < BATTERY_ADC_SAMPLE; i++) {
        ESP_ERROR_CHECK(esp_adc_cal_get_voltage(BATTERY_ADC_CH,
                                                &characteristics, ad_volt_list + i));
    }
*/
    qsort(ad_volt_list, BATTERY_ADC_SAMPLE, sizeof(uint32_t),
          (int (*)(const void *, const void *))cmp_volt);

    // mean value
    return ad_volt_list[BATTERY_ADC_SAMPLE >> 1] * BATTERY_ADC_DIV;
}
