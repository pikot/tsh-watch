# tsh-watch

This is software for watches on esp32. 

Main feature (concept)

It 
- Shows date and time
- Counts steps, pulse, body temperature
- Collects data and shows nice graphs
- Works more than 1 day without charging

long perspective.
- Web interface 
- Android application, to data sync, and notifications.

Current state.

- MCU: ESP32
- Display: 1.3" SH1106 
- Pulsemeter: MAX30100 (can be changed to MAX30102)
- Pedometer: BMI160 (can be changed to LSM6DS3TR)
- Temperature sensor: MLX90615, (can be changed to MLX90614)
- Current consumption sensor: INA219

In long perspective i'm going to change display to 1.54" e-paper, perometer to BMA423 (it detects different types of activity and hand tilt movement)
