#include "soc/rtc_cntl_reg.h"
#include "soc/rtc_io_reg.h"
#include "soc/soc_ulp.h"
#include "stack.s"


.set DS3231_ADDR, 0x68
.set DS3231_REG_TIME, 0x00

.bss

   .global ds3231_error 
ds3231_error:
   .long 0
   .global ds3231_second 
ds3231_second:
   .long 0
   .global ds3231_minute
ds3231_minute:
   .long 0
   .global ds3231_hour 
ds3231_hour:
   .long 0
   .global ds3231_dayOfWeek 
ds3231_dayOfWeek:
   .long 0
   .global ds3231_day 
ds3231_day:
   .long 0
   .global ds3231_month 
ds3231_month:
   .long 0
   .global ds3231_year
ds3231_year:
   .long 0


   .global ds3231_set_second 
ds3231_set_second:
   .long 0
   .global ds3231_set_minute
ds3231_set_minute:
   .long 0
   .global ds3231_set_hour 
ds3231_set_hour:
   .long 0
   .global ds3231_set_dayOfWeek 
ds3231_set_dayOfWeek:
   .long 0
   .global ds3231_set_day 
ds3231_set_day:
   .long 0
   .global ds3231_set_month 
ds3231_set_month:
   .long 0
   .global ds3231_set_year
ds3231_set_year:
   .long 0
   

   
.text

/*
 * 
-----
21:58:01.084 -> 0x5 0x68 0x0 0x336 0x42 0x14 0x21 0x2 
21:58:01.084 -> 0x2 0x1 0x0 0x3f6 0x4b0 0x0 0x0 0x0 
21:58:01.084 -> 0x0 0x0 0x0 0x0 0x0 0x0 0x0 0x0 
21:58:01.084 -> 0x0 0x0 0x0 0x0 0x0 0x0 0x0 0x0 
21:58:01.084 -> 
21:58:01.084 -> wakeupReason ESP_SLEEP_WAKEUP_UNDEFINED, statRecord_cnt 0
21:58:01.084 -> ds3231 data: year 2000, month 1, day 2, dayOfWeek 2, hour 21, minute 14, second 42

 
21:58:04.036 -> 0x5 0x68 0x0 0x336 0x3dc 0x37e 0x4a5 0x2 
21:58:04.036 -> 0x2 0x1 0x0 0x3f6 0x4b0 0x0 0x0 0x0 
21:58:04.036 -> 0x0 0x0 0x0 0x0 0x0 0x0 0x0 0x0 
21:58:04.036 -> 0x0 0x0 0x0 0x0 0x0 0x0 0x0 0x0 

21:58:20.111 -> 0x5 0x68 0x0 0x336 0x0 0x0 0x0 0x0 
21:58:20.111 -> 0x0 0x0 0x0 0x3f6 0x4b0 0x0 0x0 0x0 
21:58:20.111 -> 0x0 0x0 0x0 0x0 0x0 0x0 0x0 0x0 
21:58:20.111 -> 0x0 0x0 0x0 0x0 0x0 0x0 0x0 0x0 

*/

   .global ds3231_getDateTime
ds3231_getDateTime:
   store ds3231_error 255

   move r1, DS3231_ADDR
   push r1
   move r1, DS3231_REG_TIME
   push r1 
   move r0, 7  
   move r2, ds3231_year  
   psr
   jump readMULTY
   add r3,r3,2 // remove call parameters from stack

   move r0,r2 // test for error
   jumpr ds3231_fail_read,1,ge
   store ds3231_error 0
   ret


   .global ds3231_setDateTime
ds3231_setDateTime:
   store ds3231_error 255
   
   move r1, DS3231_ADDR
   push r1
   move r1, DS3231_REG_TIME
   push r1 
   move r1, 7
   push r1 

   move r2, ds3231_set_second  
   psr
   jump writeMULTY
   add r3,r3,3 // remove call parameters from stack

   move r0,r2 // test for error
   jumpr ds3231_fail_read,1,ge
   store ds3231_error 0
   ret

   
   .global ds3231_fail_read
ds3231_fail_read:
   store ds3231_error SENSOR_STATUS_READ_ERROR
   ret


   
   
