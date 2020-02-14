#ifndef EV3_API_HT_IR_V2_H
#define EV3_API_HT_IR_V2_H

#include "ev3_sensors.h"

#define HT_IR_V2_SENSOR_DEFAULT_MODE 0

extern SensorHandler * HTIrV2;

typedef enum HTIrV2ReadingMode {
    Modulated,
    Unmodulated ///< Untested,

} HTIrV2ReadingMode;

/**
 * Returns the direction from which the IR signal is coming from.
 * The direction ranges from 1 to 9. If no signal is detected, 0 is returned.
 * @param port port to which the sensor is connected
 * @param mode whether to read AC or DC signals
 * @return direction of the IR signal, -1 in case of error.
 */
int ReadHTIrV2Sensor(int port, HTIrV2ReadingMode mode);

#endif //EV3_API_HT_IR_V2_H
