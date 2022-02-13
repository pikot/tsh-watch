# tsh-watch

This is software for watches on esp32. Ulp version. 

![front view](https://hackster.imgix.net/uploads/attachments/1410249/img_20220117_071726_5IQAxgtHM6.jpg?auto=compress%2Cformat&w=740&h=555&fit=max)

Main feature 

It 
- Shows date and time
- Counts steps, measures body temperature, detects atmospheric pressure and air humidity
- Collects data for 10 days on mcu flash and shows nice graphs
- Works ~4 days without charging (battery 300mAh) // previous result ~3 days with oled display and 180mAh battery
- send data via wifi to server
- can sync date via ntp server

long perspective.
- Web interface 
- Android application, to data sync, and notifications.

Current state.

- MCU: ESP32 (collects data by ulp)
- Display: 1.54" 200x200 GDEH0154D67 
- Pulsemeter: MAX30100 (switch off now, because data doesen't fit slow RTC memory)
- Pedometer:  LSM6DS3C
- Temperature sensor: MLX90615
- Current consumption sensor: INA219
- Barometer and air humidity sensor: BME280
 
In long perspective i'm going to change 
- pedometer to BMA423 (it detects different types of activity and hand tilt movement),  
- barometer/air humidity/VOC sensor BME680 

Current consumption of watches now ~2mAh

this is vcc graph for 130mAh battary

![vcc graph](http://vesovoy-control.ru/tsh_watch_foto/VCC.png)

Schema

Not ready, but can be build based on sources (main idea 2 different i2c buses for ulp and for screen, buttons on write only pins)


Links:

Youtube presentation -- https://youtu.be/CB8Ftyo_vDs (russian, with english subs)

All fresh info and photo about development in instagram -- https://www.instagram.com/tshideas/

Development process and plains -- https://github.com/pikot/tsh-watch/projects/1
