#ifndef EV3_API_BACK_COMPATIBILITY_PRIVATE_H
#define EV3_API_BACK_COMPATIBILITY_PRIVATE_H

#include "include/ev3_sensors/c4ev3_compatibility.h"

bool initializeAllSensors(void);

void initializeBackCompatibilityIfNeeded(void);

int readEV3IrSensorSeekForBackCompatibility(int port);

#endif
