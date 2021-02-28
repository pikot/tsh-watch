#include "soc/rtc_cntl_reg.h"
#include "soc/rtc_io_reg.h"
#include "soc/soc_ulp.h"
#include "stack.s"
//#include "i2c.s"



.set MLX90615_ADDR, 0x5B

.set MLX90615_AMBIENT_TEMPERATURE, 0x26
.set MLX90615_OBJECT_TEMPERATURE, 0x27
.set MLX90615_SLEEP, 0xC6


.bss
   .global mlx90615_error
mlx90615_error:
   .long 0
   .global mlx90615_obj_temperature
mlx90615_obj_temperature:
   .long 0
   .global mlx90615_amb_temperature
mlx90615_amb_temperature:
   .long 0

   .global mlx90615_b1
mlx90615_b1:
   .long 0
   .global mlx90615_b2
mlx90615_b2:
   .long 0
   .global mlx90615_pec
mlx90615_pec:
   .long 0
.text


   .global mlx90615_readdata
mlx90615_readdata:
 //  psr 
  // jump mlx90615_wake
  
   move r2, 300    // Valid data will be available typically 0.3 seconds after the device has woken up.
   psr
   jump waitMs

   psr
   jump mlx90615_getAmbientTemp

   psr
   jump mlx90615_getObjectTemp

   psr
   jump mlx90615_sleep
ret
   
   .global mlx90615_getObjectTemp
mlx90615_getObjectTemp:

   move r1,MLX90615_ADDR
   push r1
   move r1,MLX90615_OBJECT_TEMPERATURE
   push r1
   move r0, 3
   move r2, mlx90615_pec  
   psr
   jump readMULTY
   add r3,r3,2 // remove call parameters from stack

   move r1, mlx90615_b1  // save result
   ld r0, r1, 0
   move r1, mlx90615_b2  // save result
   ld r2, r1, 0
   
   lsh r2,r2,8
   or r2,r2,r0

   move r0, mlx90615_obj_temperature
   st r2, r0, 0
   ret
   
   .global mlx90615_getAmbientTemp
mlx90615_getAmbientTemp:
   move r1,MLX90615_ADDR
   push r1
   move r1,MLX90615_AMBIENT_TEMPERATURE
   push r1
   move r0, 3
   move r2, mlx90615_pec  
   psr
   jump readMULTY
   add r3,r3,2 // remove call parameters from stack

   move r1, mlx90615_b1  // save result
   ld r0, r1, 0
   move r1, mlx90615_b2  // save result
   ld r2, r1, 0

   lsh r2,r2,8
   or r2,r2,r0

   move r0, mlx90615_amb_temperature
   st r2, r0, 0
   ret
   
fail_mlx90615:
   store mlx90615_error 1
   ret
   
   .global mlx90615_sleep
mlx90615_sleep:
// SA=0x5B, Command=0xC6, PEC=0x6D (SA_W=0xB6 Command=0xC6 PEC=0x6D)// hardcode for sleep command, adress  fixed
   move r1, MLX90615_ADDR  // SA_W=0xB6
   push r1
   move r1, 0xC6  // Command=0xC6
   push r1
   move r1, 0x6D  // PEC=0x6D
   push r1
   psr
   jump write8
 
   add r3,r3,3 // remove call parameters from stack
   move r0,r2 // test for error in r2
   jumpr fail_mlx90615,1,ge
   ret

   .global mlx90615_wake
mlx90615_wake:
    WRITE_RTC_REG(RTC_GPIO_ENABLE_W1TS_REG, RTC_GPIO_ENABLE_W1TS_S + SCL_PIN, 1, 1) // SET 0 to scl

    move r2, 55    // wait 10ms to wake up,  Valid data will be available typically 0.3 seconds after the device has woken up.
    psr
    jump waitMs
    ret
    
    /*
 wake_stretch_read: // read 1 from scl -- this is good sign that i2c waked
    READ_RTC_REG(RTC_GPIO_IN_REG, RTC_GPIO_IN_NEXT_S + SCL_PIN, 1) 
    jumpr wake_stretch_read,1,lt
  ret
  */
//   https://github.com/joaobarroca93/MLX90615/blob/9f5d3a79b2e3032abb9d0bd01e18334830f1d5dc/MLX90615.cpp

/*
      float getTemperature(int Temperature_kind, bool fahrenheit = false) {
        float celsius;
        uint16_t tempData;

        readReg(Temperature_kind, &tempData);

        double tempFactor = 0.02; // 0.02 degrees per LSB (measurement resolution of the MLX90614)

        // This masks off the error bit of the high byte
        celsius = ((float)tempData * tempFactor) - 0.01;

        celsius = (float)(celsius - 273.15);

        return fahrenheit ? (celsius * 1.8) + 32.0 : celsius;
    }

    boolean MLX90615::sleep(void)
{
  CRC8 crc(MLX90615_CRC8POLY);
    // Build the CRC-8 of all bytes to be sent.
    crc.crc8(_addr << 1);
    _crc8 = crc.crc8(MLX90615_SLEEP_MODE);
    Serial.print("PEC (Sleep Mode) = ");
    Serial.println(_crc8);
    // Send the slave address then the command.
  Wire.beginTransmission(_addr);
    Wire.write(MLX90615_SLEEP_MODE);
    // Then write the crc and set the r/w error status bits.
    Wire.write(_crc8);
    _rwError |= (1 << Wire.endTransmission(true)) >> 1;
    // Now we need to keep SCL high
    pinMode(A5, OUTPUT);
    pinMode(A4, OUTPUT);
    digitalWrite(A5, HIGH);
    digitalWrite(A4, LOW);
    // Clear r/w errors if using broadcast address.
    if(_addr == MLX90615_BROADCASTADDR) _rwError &= MLX90615_NORWERROR;
  return _sleep = true;
}

boolean MLX90615::wakeUp(void)
{
  pinMode(A5, OUTPUT);
  digitalWrite(A5, LOW);
  delay(10);
  Wire.begin();
  delay(400);
  MLX90615::begin();
  return _ready;
}
    */
