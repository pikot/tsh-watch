//  SPDX-FileCopyrightText: 2020-2021 Ivan Ivanov 
//  SPDX-License-Identifier: GPL-3.0-or-later

/*
   Must allocate more memory for the ulp in
   esp32/tools/sdk/include/sdkconfig.h
   -> #define CONFIG_ULP_COPROC_RESERVE_MEM
   for this sketch to compile. 2048b seems
   good.
*/

#include <stdio.h>
#include <math.h>

#include "esp_sleep.h"
#include "soc/rtc_cntl_reg.h"
#include "soc/rtc_io_reg.h"
#include "soc/sens_reg.h"
#include "soc/soc.h"
#include "driver/rtc_io.h"
#include "esp32/ulp.h"
#include "sdkconfig.h"
#include "ulp_main.h"
#include "ulptool.h"
#include "utils.h"
#include "version.h"


extern const uint8_t ulp_main_bin_start[] asm("_binary_ulp_main_bin_start");
extern const uint8_t ulp_main_bin_end[]   asm("_binary_ulp_main_bin_end");



static void startUlpProgram();

int32_t tm = 1;
int32_t tb = 3600;

void printUlpStatus() {
    printf_w("ulp status: sample_counter %d, errors: ina219 %d, ds3231 %d, lsm6ds3 %d, mlx90615 %d, max30100 %d, bme280 %d\n",
           ulp_sample_counter, ulp_ina219_error, ulp_ds3231_error, ulp_lsm6ds3_error, ulp_mlx90615_error, ulp_max30100_error, ulp_bme280_error);

    printf_w("ulp status addr: sample_counter %p, errors: ina219 %p, ds3231 %p, lsm6ds3 %p, mlx90615 %p, max30100 %p, bme280 %p\n",
           &ulp_sample_counter, &ulp_ina219_error, &ulp_ds3231_error, &ulp_lsm6ds3_error, &ulp_mlx90615_error, &ulp_max30100_error, &ulp_bme280_error);
/*
    printf_w("ulp status:  sensors 0x%X, sensors_working 0x%X, lsm6ds3_inited 0x%X\n", 
            ulp_sensors_switch_extern, ulp_sensors_working, ulp_lsm6ds3_inited);
*/
    printf_w("ulp status: ulp_sensors_switch_extern 0x%X\n", ulp_sensors_switch_extern);
    printf_w("read_cnt: ina219 %d,\t lsm6ds3 %d,\t mlx90615 %d,\t max30100 %d, bme280 %d\n",
             ulp_ina219_read_cnt, ulp_lsm6ds3_read_cnt, ulp_mlx90615_read_cnt, ulp_max30100_read_cnt, ulp_bme280_read_cnt);
    printf_w("skip_cnt: ina219 %d,\t lsm6ds3 %d,\t mlx90615 %d,\t max30100 %d, bme280 %d\n",
             ulp_ina219_skip_cnt, ulp_lsm6ds3_skip_cnt, ulp_mlx90615_skip_cnt, ulp_max30100_skip_cnt, ulp_bme280_skip_cnt);

/*
  //  readUlpStack();
/* 
    printf_w("ulp status:  cnt_read %d, icounter %d, ulp_resul_pointer %d\n",
           ulp_cnt_read, ulp_icounter, ulp_resul_pointer);*/
}

void setupUlp( esp_sleep_wakeup_cause_t cause ) {
    if (cause != ESP_SLEEP_WAKEUP_ULP) {
        printf_w("ULP wakeup, init_ulp_program\n");
        initUlpProgram();
    }
    else 
        printf_w("ULP wakeup, it shouldn't run!!!!! \n");
}


void runUlp() {
    printf_w("ulp_run start\n");
    startUlpProgram();
}

void uploadUlpProgram()
{
    printf_w("initUlpProgram, memory size %d, %d\n", (ulp_main_bin_end - ulp_main_bin_start),  (ulp_main_bin_end - ulp_main_bin_start) / sizeof(uint32_t) );
    esp_err_t err = ulptool_load_binary(0, ulp_main_bin_start,
                                      (ulp_main_bin_end - ulp_main_bin_start) / sizeof(uint32_t));
    ESP_ERROR_CHECK(err);
}

void initUlpProgram()
{
   // printf_w("initUlpProgram, memory size %d, %d\n", (ulp_main_bin_end - ulp_main_bin_start),  (ulp_main_bin_end - ulp_main_bin_start) / sizeof(uint32_t) );
    // esp_err_t err = ulptool_load_binary(0, ulp_main_bin_start,
    //                                  (ulp_main_bin_end - ulp_main_bin_start) / sizeof(uint32_t));

    rtc_gpio_init(GPIO_SCL);
    rtc_gpio_set_direction(GPIO_SCL, RTC_GPIO_MODE_INPUT_ONLY);
    rtc_gpio_pulldown_dis(GPIO_SCL);
    rtc_gpio_pullup_en(GPIO_SCL);
  
    rtc_gpio_init(GPIO_SDA);
    rtc_gpio_set_direction(GPIO_SDA, RTC_GPIO_MODE_INPUT_ONLY);
    rtc_gpio_pulldown_dis(GPIO_SDA);
    rtc_gpio_pullup_en(GPIO_SDA);

    // Set ULP wake up period to 
    ulp_set_wakeup_period(0, 1 * 1000 * 1000); // 1000ms
}

void readUlpStack() {
    for (int i =0; i < 32; i++) {
        printf_w("0x%000x " ,  (uint16_t) *( &ulp_stackEnd - i) );
        if (0 == (i+1) % 8)
            printf_w("\n");
    }
    printf_w("\n");
}

static void startUlpProgram()
{
    esp_err_t err = ulp_run((&ulp_entry - RTC_SLOW_MEM) / sizeof(uint32_t));
    ESP_ERROR_CHECK(err);
}
