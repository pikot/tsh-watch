#include "Arduino.h"


enum ulp_sensor_type_t{
   SENSOR_INA219   = 0x01,
   SENSOR_LSM6DS3  = 0x02,
   SENSOR_MLX90615 = 0x04,
   SENSOR_MAX30100 = 0x08,
};
extern uint16_t ulp_stackEnd;

extern uint16_t ulp_ina219_read_cnt;
extern uint16_t ulp_lsm6ds3_read_cnt;
extern uint16_t ulp_mlx90615_read_cnt;
extern uint16_t ulp_max30100_read_cnt;

      
extern uint16_t ulp_ina219_skip_cnt;
extern uint16_t ulp_lsm6ds3_skip_cnt;
extern uint16_t ulp_mlx90615_skip_cnt;
extern uint16_t ulp_max30100_skip_cnt;
   

extern uint32_t ulp_entry;
extern uint16_t ulp_result;

extern uint16_t ulp_sample_counter;
extern uint16_t ulp_sensors_switch_extern;  // on / off switch for sensors
extern uint16_t ulp_sensors_working;


//ina219
extern uint16_t ulp_ina219_error;
extern uint16_t ulp_ina219_config;
extern int16_t ulp_ina219_current;
extern int16_t ulp_ina219_voltage;
extern int16_t ulp_ina219_aggr_current;
extern int16_t ulp_ina219_currentDivider_mA;
extern int16_t ulp_ina219_calValue;
extern int32_t ulp_ina219_current_table[60];


//ds3231
extern uint16_t ulp_ds3231_update_flag;

extern uint8_t ulp_ds3231_error;
extern uint8_t ulp_ds3231_year;
extern uint8_t ulp_ds3231_month;
extern uint8_t ulp_ds3231_day;
extern uint8_t ulp_ds3231_dayOfWeek;
extern uint8_t ulp_ds3231_hour;
extern uint8_t ulp_ds3231_minute;
extern uint8_t ulp_ds3231_second;

extern int32_t ulp_ds3231_set_second;
extern uint8_t ulp_ds3231_set_minute;
extern uint8_t ulp_ds3231_set_hour;
extern uint8_t ulp_ds3231_set_dayOfWeek;
extern uint8_t ulp_ds3231_set_day;
extern uint8_t ulp_ds3231_set_month;
extern uint8_t ulp_ds3231_set_year;


//lsm6ds3
extern uint16_t ulp_lsm6ds3_error;
extern uint16_t ulp_lsm6ds3_inited;
extern uint16_t ulp_lsm6ds3_step_count;
extern uint16_t ulp_lsm6ds3_reseted_steps;
extern uint16_t ulp_lsm6ds3_driver_sign;
extern uint16_t ulp_lsm6ds3_error_cnt;

//mlx90615
extern uint16_t  ulp_mlx90615_error;
extern uint16_t ulp_mlx90615_obj_temperature;
extern uint16_t ulp_mlx90615_amb_temperature;

extern uint16_t  ulp_mlx90615_b1;
extern uint16_t  ulp_mlx90615_b2;
extern uint16_t  ulp_mlx90615_pec;


// max30100
extern uint16_t  ulp_max30100_flags;
extern uint16_t  ulp_max30100_error;
extern uint16_t  ulp_max30100_toRead;
extern uint16_t  ulp_max30100_r2;
extern uint16_t  ulp_max30100_r3;
extern uint32_t  ulp_max30100_raw_data[200] ;
extern uint32_t  ulp_max30100_data_pointer;


void readUlpStack();

void initUlpProgram();
void runUlp() ;
void setupUlp( esp_sleep_wakeup_cause_t cause );
void printUlpStatus();
