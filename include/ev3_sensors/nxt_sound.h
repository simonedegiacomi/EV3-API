#ifndef EV3_API_NXT_SOUND_H
#define EV3_API_NXT_SOUND_H

#include "ev3_sensors.h"

#define NXT_SOUND_SENSOR_DEFAULT_MODE 0

extern SensorHandler * NXTSound;

// TODO: Mode not implemented yet
typedef enum NXTSoundReadingMode {
    DB,
    DBA
} NXTSoundReadingMode;

int ReadNXTSoundSensor(int port, NXTSoundReadingMode mode);

#endif //EV3_API_NXT_SOUND_H
