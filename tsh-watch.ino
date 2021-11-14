//  SPDX-FileCopyrightText: 2020-2021 Ivan Ivanov
//  SPDX-License-Identifier: GPL-3.0-or-later
 
#include "hardware.hpp"

Hardware watch;

void setup() 
{
    watch.init();
    printf_w("CONFIG_ULP_COPROC_RESERVE_MEM = %d\n" , CONFIG_ULP_COPROC_RESERVE_MEM);
}

void loop() 
{
    watch.update();
    watch.runPowerSafe();
}

// --------------------------------------------------
// callback from menu and external actions

void CALLBACK_FUNCTION onBack(int id) 
{
    watch.showActivity( WATCH_ACTICITY );
    returnToMenu();
}

void CALLBACK_FUNCTION onAutoUpdateCh(int id) 
{
    watch.syncTimeViaWifi();
}

void CALLBACK_FUNCTION onDateCh(int id) 
{
    DateStorage dt = menuDate.getDate();
    watch.setDate(dt);
}

void CALLBACK_FUNCTION onTimeCh(int id) 
{
    TimeStorage tm = menuTime.getTime();
    watch.setTime(tm);
}

void CALLBACK_FUNCTION onTimezoneCh(int id) 
{
    int tz  = menuTimeZone.getCurrentValue(); 
    watch.setTimezone(tz, true);
}

void CALLBACK_FUNCTION onTemperatureCh(int id) 
{
    bool val = menuTemperature.getBoolean();
    watch.setTempSwitch(val, true);
}

void CALLBACK_FUNCTION onPulseMeterCh(int id) 
{
    bool val = menuPulseMeter.getBoolean();
    watch.setPulseSwitch(val, true);
}

void CALLBACK_FUNCTION onBaroMeterCh(int id) 
{
    bool val = menuBaroMeter.getBoolean();
    watch.setBaroSwitch(val, true);
}

void CALLBACK_FUNCTION onPedoMeterCh(int id) 
{
    bool val = menuPedoMeter.getBoolean();
    watch.setPedoSwitch(val, true);
}
void CALLBACK_FUNCTION onSetupWiFi(int id) 
{
    printf_w("Start Wifi portal\n");
    watch.showWifiPortal();
}

void windowCallbackFn(unsigned int currentValue, RenderPressMode userClicked, displayActivity_t _activity)
{
    if (0 == currentValue)    // if the encoder / select button is held, we go back to the menu.
        renderer.giveBackDisplay();
    else if( watch.getGraphStarting()) {
        // you need to handle the clearing and preparation of the display when you're first called.
        // the easiest way is to set a flag such as this and then prepare the display.
        watch.setGraphStarting(false);
        switches.getEncoder()->changePrecision( MAX_FILES_CNT+1, 1);
    }
    else {
        watch.start_graph_logic(_activity, currentValue-1);
        watch.showGraph();
    }
}

void tempCallbackFn(unsigned int currentValue, RenderPressMode userClicked)
{
    windowCallbackFn(currentValue, userClicked, GRAPH_BODY_TEMP_ACTIVITY);
}

void CALLBACK_FUNCTION onGraphTempPressed(int id) 
{
    watch.setGraphStarting(true);
    showWindow(tempCallbackFn);
}

//!!! Если открыть график после открытия другого, вначале показывается прошлый, и только после пересчета новый (need fix)

void stepsCallbackFn(unsigned int currentValue, RenderPressMode userClicked)  
{
    windowCallbackFn(currentValue, userClicked, GRAPH_STEPS_ACTIVITY);
}

void CALLBACK_FUNCTION onGraphStepsPressed(int id) 
{
    watch.setGraphStarting(true);
    showWindow(stepsCallbackFn);
}

void CALLBACK_FUNCTION onSyncStatViaWiFi(int id) 
{
    watch.syncStatViaWifi();
    watch.syncStatHrViaWifi();
}

void saveWifiConfigCallback () 
{
      watch.updateWebApiConfig();
}
