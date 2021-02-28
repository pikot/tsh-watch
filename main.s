#include "soc/rtc_cntl_reg.h"
#include "soc/rtc_io_reg.h"
#include "soc/soc_ulp.h"
#include "stack.s"

.set SENSOR_INA219,   0x01
.set SENSOR_LSM6DS3,  0x02
.set SENSOR_MLX90615, 0x04
.set SENSOR_MAX30100, 0x08

.bss
/*
  */ 
   .global sample_counter
sample_counter:
   .long 0
/**/
   .global result
result:
   .long 0

   .global sensors_switch_extern
sensors_switch_extern:
   .long 0
   
   .global sensors_switch
sensors_switch:
   .long 0
   
   .global sensors_working
sensors_working:
   .long 0
   
   .global refresh_sensors
refresh_sensors:
   .long 0
      
   .global ina219_read_cnt
ina219_read_cnt:
   .long 0
   .global lsm6ds3_read_cnt
lsm6ds3_read_cnt:
   .long 0
   .global mlx90615_read_cnt
mlx90615_read_cnt:
   .long 0
   .global max30100_read_cnt
max30100_read_cnt:
   .long 0

      
   .global ina219_skip_cnt
ina219_skip_cnt:
   .long 0
   .global lsm6ds3_skip_cnt
lsm6ds3_skip_cnt:
   .long 0
   .global mlx90615_skip_cnt
mlx90615_skip_cnt:
   .long 0
   .global max30100_skip_cnt
max30100_skip_cnt:
   .long 0
   
   .global ds3231_update_flag
ds3231_update_flag:
   .long 0
   
   .global stack
stack:
   .skip 80
   .global stackEnd
stackEnd:
   .long 0
   
   
.text
   .global entry
entry:
   move r3, stackEnd

   store_mem sensors_switch_extern sensors_switch 

   psr
   jump ds3231_getDateTime  // anyway at start, try to get current time  

   move r2, ds3231_update_flag       //  update time logic if needed
   ld  r0,r2,0x0
   jumpr skip_time_update, 0, eq
   
   move r0, ds3231_set_year       //  skip update if year == 2000
   jumpr skip_time_update, 0, eq
   psr
   jump ds3231_setDateTime
   
   store  ds3231_update_flag 0
   store  ds3231_set_day 0

skip_time_update:

   move r2, ds3231_second   
   ld  r0,r2,0
   jumpr skip_wake_mlx90615, 89, lt //  this is raw format from ds3231   > 59

   if_noflag sensors_switch SENSOR_MLX90615 skip_wake_mlx90615
 //  if_noflag sensors_working SENSOR_MLX90615 skip_wake_mlx90615
   // need some time to start after wake, so send wake first, do some work with other sensor and only than read
   psr
   jump mlx90615_wake
skip_wake_mlx90615:
   
   // this is a long long logic, i tried separate logic but it costs high-memory  
   if_flag sensors_switch SENSOR_INA219 start_read_ina219
   if_noflag sensors_working SENSOR_INA219 skip_read_ina219
   psr
   jump ina219_powerDown 
   del_flag sensors_working SENSOR_INA219
   inc  ina219_skip_cnt
   jump skip_read_ina219
   
start_read_ina219:
   psr
   jump ina219_readdata
   set_flag sensors_working SENSOR_INA219
   inc ina219_read_cnt
skip_read_ina219:

   /// test this logic in real life !!!!!!!!!!!!!!!!
  
   move r2, ds3231_second 
   ld  r0,r2,0x0
   and r0, r0, 0x1
   jumpr skip_read_lsm6ds3, 0x0, eq // 
 
   if_flag sensors_switch SENSOR_LSM6DS3 start_read_lsm6ds3
   if_noflag sensors_working SENSOR_LSM6DS3 skip_read_lsm6ds3
   psr
   jump lsm6ds3_poweroff
   del_flag sensors_working SENSOR_LSM6DS3
   inc lsm6ds3_skip_cnt
   jump skip_read_lsm6ds3
   
start_read_lsm6ds3:
   move r2, 70    //  mb problem
   psr
   jump waitMs
   
   psr
   jump lsm6ds3_config_pedometer
   set_flag sensors_working SENSOR_LSM6DS3
   inc lsm6ds3_read_cnt
skip_read_lsm6ds3:

   move r2, ds3231_second   
   ld  r0,r2,0
   jumpr inc_sample, 89, lt //  this is raw format from ds3231   > 59

   if_flag sensors_switch SENSOR_MLX90615 start_read_mlx90615
   if_noflag sensors_working SENSOR_MLX90615 skip_read_mlx90615
   psr
   jump mlx90615_sleep
   del_flag sensors_working SENSOR_MLX90615
   inc mlx90615_skip_cnt
   jump skip_read_mlx90615
   
start_read_mlx90615:
   psr
   jump mlx90615_readdata
   set_flag sensors_working SENSOR_MLX90615
   inc mlx90615_read_cnt
   
skip_read_mlx90615:

   if_flag sensors_switch SENSOR_MAX30100 start_read_max30100
   if_noflag sensors_working SENSOR_MAX30100 skip_read_max30100
    psr
    jump max30100_shutdown
    del_flag sensors_working SENSOR_MAX30100
    inc max30100_skip_cnt
    jump skip_read_max30100
    
start_read_max30100:
   psr 
   jump max30100_init
   set_flag sensors_working SENSOR_MAX30100
   inc max30100_read_cnt
// psr 
 // jump max30100_resume

   /*
  move r2, 1    // Read sample counter 
   psr
   jump waitMs
   
   psr 
   jump max30100_readFifoData
    
 
 //  psr
 //  jump max30100_shutdown
   move r2, 5000    // Read sample counter 
   psr
   */
skip_read_max30100:


inc_sample:
    inc sample_counter    // Read sample counter 

    .global exit
exit:
    halt

    .global wake_up
wake_up:
    /* Check if the system can be woken up */
    READ_RTC_REG(RTC_CNTL_DIAG0_REG, 19, 1)
    and r0, r0, 1
    jump exit, eq
    /* Wake up the SoC, end program */
    wake
    WRITE_RTC_FIELD(RTC_CNTL_STATE0_REG, RTC_CNTL_ULP_CP_SLP_TIMER_EN, 0)
    halt

// Wait for r2 milliseconds
    .global waitMs
waitMs:
    wait 8000
    sub r2,r2,1
    jump doneWaitMs,eq
    jump waitMs
doneWaitMs:
    ret



   
