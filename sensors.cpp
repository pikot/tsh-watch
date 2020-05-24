#include "sensors.hpp"
//============================================================================================
Gyroscope::Gyroscope() {
    steps = 0;
}

void Gyroscope::init(bool is_after_deepsleep) {
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


void Gyroscope::read_data() {
    steps      = gyro.getStepCount();
}

uint16_t Gyroscope::StepCount(){
    return  steps;
}

void Gyroscope::wake(){
}

void Gyroscope::sleep(){
}

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
