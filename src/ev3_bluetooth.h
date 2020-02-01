#ifdef __cplusplus
extern "C" {
#endif

#ifndef ev3_bluetooth_h
#define ev3_bluetooth_h

#define BLUETOOTH_ADDRESS_LENGTH            sizeof("00:00:00:00:00:00")
#define	MAX_BLUETOOTH_NAME_LENGTH	        128

void BluetoothInit();

typedef int BluetoothConnectionHandle;

/**
 * Connect to the remote device given his bluetooth name or address
 * @param nameOrAddress
 * @return Connection handle or -1 if wasn't possible to find the address given the device name
 */
BluetoothConnectionHandle ConnectTo(const char * nameOrAddress);

/**
 * Wait for an incoming blueooth connection from another device
 * @return Connection handle or -1 if there was an error while waiting for the connection
 */
BluetoothConnectionHandle WaitConnection();

void SendStringTo(BluetoothConnectionHandle to, const char * str);

int ReceiveStringFrom(BluetoothConnectionHandle from, char * buffer, int bufferLength);

void DisconnectFrom(BluetoothConnectionHandle from);

void BluetoothExit();

#endif // ev3_bluetooth_h

#ifdef __cplusplus
}
#endif
