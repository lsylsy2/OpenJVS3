#ifndef DEVICE_H_
#define DEVICE_H_

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <time.h>
#include <stdarg.h>
#include <sys/ioctl.h>
#include <linux/serial.h>

int initDevice(char *devicePath);
int closeDevice();
int readByte(char *byte);
int writeByte(char byte);
int setSerialAttributes(int fd, int myBaud);
int setSerialLowLatency(int fd);
int setSyncPin(int a);


#endif // DEVICE_H_