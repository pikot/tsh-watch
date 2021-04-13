# tsh-watch

This is software for watches on esp32. Ulp version. 

![front view](http://vesovoy-control.ru/tsh_watch_foto/tsh_watch_front.png)

Main feature 

It 
- Shows date and time
- Counts steps, measure body temperature
- Collects data for 10 days on mcu flash and shows nice graphs
- Works ~3 days without charging (battery 130mAh)
- send data via wifi to server
- can sync date via ntp server

long perspective.
- Web interface 
- Android application, to data sync, and notifications.

Current state.

- MCU: ESP32 (collects data by ulp)
- Display: 1.3" SH1106 
- Pulsemeter: MAX30100 (switch off now, because data doesen't fit slow RTC memory)
- Pedometer:  LSM6DS3C
- Temperature sensor: MLX90615
- Current consumption sensor: INA219

In long perspective i'm going to change 
- display to 1.54" e-paper, 
- perometer to BMA423 (it detects different types of activity and hand tilt movement),  
and add
- humidity sensor and barometer bme289
- GPS L80-R or GP-1/2 (if can find proper place on pcb for antenna)

Current consumption of watches now ~2mAh

this is vcc graph for 130mAh battary

![vcc graph](http://vesovoy-control.ru/tsh_watch_foto/VCC.png)

Schema

Not ready, but can be build based on sources (main idea 2 different i2c buses for ulp and for screen, buttons on write only pins)


Links:

Youtube presentation -- https://youtu.be/CB8Ftyo_vDs (russian, with english subs)

All fresh info and photo about development in instagram -- https://www.instagram.com/tshideas/

Development process and plains -- https://github.com/pikot/tsh-watch/projects/1
