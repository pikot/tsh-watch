//  SPDX-FileCopyrightText: 2020-2021 Ivan Ivanov
//  SPDX-License-Identifier: GPL-3.0-or-later

#include "Arduino.h"
#include "version.h"

// should be synced with  ULP_SCL_PIN ULP_SDA_PIN  in version.h
#if defined IS_V1_3 || IS_V1_4
const gpio_num_t GPIO_SCL = GPIO_NUM_32;
const gpio_num_t GPIO_SDA = GPIO_NUM_33;
#else
const gpio_num_t GPIO_SCL = GPIO_NUM_32;
const gpio_num_t GPIO_SDA = GPIO_NUM_25;
#endif

enum ulp_sensor_type_t{
   SENSOR_INA219   = 0x01,
   SENSOR_LSM6DS3  = 0x02,
   SENSOR_MLX90615 = 0x04,
   SENSOR_MAX30100 = 0x08,
   SENSOR_BME280   = 0x10,
};

extern uint16_t ulp_stackEnd;

/*
extern uint16_t ulp_ds3231_inited;
extern uint16_t ulp_ina219_inited;
extern uint16_t ulp_lsm6ds3_inited;
extern uint16_t ulp_mlx90615_inited;
extern uint16_t ulp_max30100_inited;
extern uint16_t ulp_bme280_inited;
*/

extern uint16_t ulp_ina219_read_cnt;
extern uint16_t ulp_lsm6ds3_read_cnt;
extern uint16_t ulp_mlx90615_read_cnt;
extern uint16_t ulp_max30100_read_cnt;
extern uint16_t ulp_bme280_read_cnt;

      
extern uint16_t ulp_ina219_skip_cnt;
extern uint16_t ulp_lsm6ds3_skip_cnt;
extern uint16_t ulp_mlx90615_skip_cnt;
extern uint16_t ulp_max30100_skip_cnt;
extern uint16_t ulp_bme280_skip_cnt;


extern uint32_t ulp_entry;
extern uint16_t ulp_result;

extern uint16_t ulp_sample_counter;
extern uint16_t ulp_sensors_switch_extern;  // on / off switch for sensors
extern uint16_t ulp_sensors_working;


//ina219
extern int16_t ulp_ina219_error;
extern uint16_t ulp_ina219_config;
extern int16_t ulp_ina219_current;
extern int16_t ulp_ina219_voltage;
extern int16_t ulp_ina219_aggr_current;
extern int16_t ulp_ina219_currentDivider_mA;
extern int16_t ulp_ina219_calValue;
extern int32_t ulp_ina219_current_table[60];


//ds3231
extern uint16_t ulp_ds3231_update_flag;

extern int16_t ulp_ds3231_error;

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
extern int16_t ulp_lsm6ds3_error;
extern uint16_t ulp_lsm6ds3_inited;
extern uint16_t ulp_lsm6ds3_step_count;
extern uint16_t ulp_lsm6ds3_reseted_steps;
extern uint16_t ulp_lsm6ds3_driver_sign;
extern uint16_t ulp_lsm6ds3_error_cnt;

//mlx90615
extern int16_t  ulp_mlx90615_error;
extern uint16_t ulp_mlx90615_obj_temperature;
extern uint16_t ulp_mlx90615_amb_temperature;

extern uint16_t  ulp_mlx90615_b1;
extern uint16_t  ulp_mlx90615_b2;
extern uint16_t  ulp_mlx90615_pec;




// max30100
extern uint16_t  ulp_max30100_flags;
extern int16_t  ulp_max30100_error;
extern uint16_t  ulp_max30100_toRead;
extern uint16_t  ulp_max30100_fifo_num_available_samples;
extern uint16_t  ulp_max30100_fifo_overflow_counter;
extern uint16_t  ulp_max30100_fifo_filled;
extern uint32_t  ulp_max30100_raw_data[200] ;
//extern uint32_t  *ulp_max30100_raw_data; //
extern uint32_t  ulp_max30100_data_pointer;
extern uint16_t  ulp_max30100_loop_counter;
extern uint16_t  ulp_max30100_get_all_fifo_counter;
extern uint16_t  ulp_max30100_data_size;
extern uint16_t  ulp_max30100_data_size_after;
extern uint16_t  ulp_max30100_free_data_size;



//bme280
extern int16_t ulp_bme280_error;
extern uint16_t ulp_bme280_DEBUG;

extern uint16_t ulp_bme280_data_press_MSB_LSB;
extern uint16_t ulp_bme280_data_press_XLSB_temp_MSB;
extern uint16_t ulp_bme280_data_temp_LSB_temp_XLSB;
extern uint16_t ulp_bme280_data_hum_MSB_LSB;


#ifndef MAIN_CORE_INIT_SENSORS
extern uint16_t ulp_bme280_calib_dig_t1;
extern uint16_t ulp_bme280_calib_dig_t2;
extern uint16_t ulp_bme280_calib_dig_t3;
extern uint16_t ulp_bme280_calib_dig_p1;
extern uint16_t ulp_bme280_calib_dig_p2;
extern uint16_t ulp_bme280_calib_dig_p3;
extern uint16_t ulp_bme280_calib_dig_p4;
extern uint16_t ulp_bme280_calib_dig_p5;
extern uint16_t ulp_bme280_calib_dig_p6;
extern uint16_t ulp_bme280_calib_dig_p7;
extern uint16_t ulp_bme280_calib_dig_p8;
extern uint16_t ulp_bme280_calib_dig_p9;
extern uint16_t ulp_bme280_calib_dig_h1;
extern uint16_t ulp_bme280_calib_dig_h2;
extern uint16_t ulp_bme280_calib_dig_h3;
extern uint16_t ulp_bme280_calib_dig_h4;
extern uint16_t ulp_bme280_calib_dig_h5;
extern uint16_t ulp_bme280_calib_dig_h6;
#endif

void readUlpStack();
void ulp_read_max30100_raw_data();
void ulp_read_max30100_hex_raw_data ();

void uploadUlpProgram();
void initUlpProgram();
void runUlp() ;
void setupUlp( esp_sleep_wakeup_cause_t cause );
void printUlpStatus();
