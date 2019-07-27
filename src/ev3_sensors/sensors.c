#include <stddef.h>
#include "../ev3_constants.h"
#include "sensors.h"

static SensorHandler *currentSensorHandlers[NUM_INPUTS] = {NULL, NULL, NULL, NULL};

bool SetAllSensors(SensorHandler *port1, SensorHandler *port2, SensorHandler *port3, SensorHandler *port4) {
    SensorHandler * sensorHandlers[] = {port1, port2, port3, port4};
    int i;
    for (i = 0; i < NUM_INPUTS; i++) {
        if (sensorHandlers[i] != NULL) {
            bool res = SetSensor(i, sensorHandlers[i]);
            if (!res) {
                // TODO: Deallocate
                return false;
            }
        }
    }
}

bool SetSensor(int port, SensorHandler *sensor) {
    currentSensorHandlers[port] = sensor;
    bool res = currentSensorHandlers[port]->Init(port);
    if (!res) {
        // TODO: Deallocate
        return false;
    }
}

SensorHandler *GetSensor(int port) {
    return currentSensorHandlers[port];
}