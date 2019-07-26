#include "../ev3_inputs/ev3_input_uart.h"
#include "../ev3_time.h"
#include "ev3_color.h"

// TODO: Move in copied folder
#define EV3_COLOR_SENSOR_TYPE           29
#define EV3_COLOR_SENSOR_REFLECT_MODE   0
#define EV3_COLOR_SENSOR_AMBIENT_MODE   1
#define EV3_COLOR_SENSOR_COLOR_MODE     2
#define EV3_COLOR_SENSOR_RGB_MODE       4

#define EV3_COLOR_SENSOR_DEFAULT_MODE   EV3_COLOR_SENSOR_REFLECT_MODE


SensorHandler * EV3Color = &(SensorHandler){
        .Init = initEV3ColorSensor,
        .Exit = exitEV3ColorSensor
};

bool initEV3ColorSensor(int port) {
    setUARTSensorModeIfNeeded(port, EV3_COLOR_SENSOR_TYPE, EV3_COLOR_SENSOR_DEFAULT_MODE);
}

int ReadEV3ColorSensorLight(int port, LightMode mode) {
    setUARTSensorModeIfNeeded(port, EV3_COLOR_SENSOR_TYPE, getEV3ColorLightSensorModeConstant(mode));
    Wait(200);
    
    // TODO: Modify mode
    DATA8 data;
    int readResult = readFromUART(port, &data, 1);
    if (readResult < 0) { // TODO: Handle error
        return -1;
    }
    return data;
}

int getEV3ColorLightSensorModeConstant (LightMode mode) {
    if (mode == ReflectedLight) {
        return EV3_COLOR_SENSOR_REFLECT_MODE;
    } else {
        return EV3_COLOR_SENSOR_AMBIENT_MODE;
    }
}

Color ReadEV3ColorSensor(int port) {
    setUARTSensorModeIfNeeded(port, EV3_COLOR_SENSOR_TYPE, EV3_COLOR_SENSOR_COLOR_MODE);
    Wait(200);

    // TODO: Modify mode
    DATA8 data;
    int readResult = readFromUART(port, &data, 1);
    if (readResult < 0) { // TODO: Handle error
        return -1;
    }
    return data;
}


RGB ReadEV3ColorSensorRGB(int port) {
    setUARTSensorModeIfNeeded(port, EV3_COLOR_SENSOR_TYPE, EV3_COLOR_SENSOR_RGB_MODE);


    /**
	* The first 6 bytes in data are the colors: 2 byte for each color.
	* The range of each color value is from 0 to 1023.
	*/
    DATA8 data[6];
    readFromUART(port, data, 6);
    // TODO: Handle error

    return (RGB) {
            .red    = ((uint8_t)data[0]) + (((uint8_t) data[1]) << 8u),
            .green  = ((uint8_t)data[2]) + (((uint8_t) data[3]) << 8u),
            .blue   = ((uint8_t)data[4]) + (((uint8_t) data[5]) << 8u)
    };
}

void exitEV3ColorSensor(int port) {

}