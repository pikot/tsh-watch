# tsh-watch

This is software for watches on esp32. Ulp version. 

![front view](https://hackster.imgix.net/uploads/attachments/1410249/img_20220117_071726_5IQAxgtHM6.jpg?auto=compress%2Cformat&w=740&h=555&fit=max)
<img src="https://cdn.hackaday.io/images/9576841710053356652.jpg"  height="555" />


Main feature 

It 
- Shows date and time
- Counts steps, measures body temperature, detects atmospheric pressure and air humidity
- Collects data for 10 days on mcu flash and shows nice graphs
- Works ~9.5 days on one chare (battery 400mAh, without temperature sensor) // ~8 days on one charge (battery 400mAh with temperature sensor)
- sends data via wifi to server
- can sync date via ntp server

long perspective.
- Web interface 
- Android application, to data sync, and notifications.

Current state.

- MCU: ESP32 (collects data by ulp)
- Display: 1.54" 200x200 GDEW0154M09 (it supports partial refresh, currently in progress flexible 2.9 eink display support)
- Pulsemeter: MAX30100 (switch off now, because data doesen't fit slow RTC memory)
- Pedometer:  LSM6DS3C
- Temperature sensor: MLX90615
- Current consumption sensor: INA219
- Barometer and air humidity sensor: BME280
 
In long perspective i'm going to change 
- pedometer to BMA400 (it detects different types of activity and hand tilt movement),  
- barometer/air humidity/VOC sensor BME680
- GSR sensor 

Current consumption of watches now ~2mAh

this is vcc graph for 400mAh battary

![](https://cdn.hackaday.io/images/9673941710052460953.jpg)

Schema:

Not ready 

Links:

All fresh info and photo about development in instagram -- https://www.instagram.com/tshideas/

Hackaday project loglink -- https://hackaday.io/project/184604-tshwatch-watch-based-on-esp32/details

Youtube presentation -- https://youtu.be/CB8Ftyo_vDs (russian, with english subs)

Development process and plains -- https://github.com/pikot/tsh-watch/projects/1
