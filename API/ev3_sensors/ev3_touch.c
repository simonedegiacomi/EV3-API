#include "contrib/lms2012/ev3_basictypes.h"
#include "ev3_inputs/ev3_input_analog.h"
#include "ev3_sensors/ev3_sensors.h"
#include "include/ev3_sensors/ev3_touch.h"

//
// PRIVATE DECLARATIONS
//
static bool initEV3Touch(int port);

static void setEV3UltrasonicSensorMode(int port, int mode);

//
// GLOBAL DATA
//

SensorHandler * EV3Touch = &(SensorHandler){
        .Init = initEV3Touch,
        .Exit = NULL,
        .currentSensorMode = {NONE_MODE, NONE_MODE, NONE_MODE, NONE_MODE}
};

//
// IMPLEMENTATION
//

bool initEV3Touch(int port) {
    EV3Touch->currentSensorMode[port] = EV3_TOUCH_SENSOR_TOUCH_MODE;
    return true;
}

bool ReadEV3TouchSensor(int sensorPort) {
    DATA16 data = readNewDumbSensor(sensorPort);
    int temp = data / 256;
    return temp != 0;
}

