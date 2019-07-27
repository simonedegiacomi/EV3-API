#include "../../copied/lms2012/ev3_analog.h"
#include "ev3_input.h"
#include "ev3_input_uart.h"
#include "ev3_input_analog.h"
#include "ev3_input_iic.h"


DEVCON devCon;

static bool ev3SensorsInitialized = false;

bool initInput() {
    if (ev3SensorsInitialized) {
        return false;
    }
    ANALOG * analogSensors = initEV3AnalogInput();
//    if (analogSensors == NULL) {
//        return false;
//    }
    bool uartInitialized = initEV3UARTInput(analogSensors);
    bool iicInitialized = initEV3IICnput();
    if (!uartInitialized || !iicInitialized) {
        exitInput();
        return false;
    }
    ev3SensorsInitialized = true;
    return true;
}

void exitInput(){
    exitEV3AnalogInput();
    exitEV3UARTInput();
    exitEV3IICInput();
}
