#ifndef DEVICE_H
#define DEVICE_H
typedef struct DEVICE{
	void (struct DEVICE *) DRIVER_FUNCTION;
	//maybe port validation or smth
	stdIO * streamID;
	char * DEV_NAME;
}DEVICE;
#endif
