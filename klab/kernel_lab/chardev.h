// chardev.h - the header file for chardev.c

#ifndef CHARDEV_H
#define CHARDEV_H

#include <linux/ioctl.h>

#define DEVICE_NAME "chardev"
#define DEVICE_FILE_NAME "chardev"
#define MAJOR_NUM 77	// Major device number. We need this to open device file.

#define IOCTL_CHR_CMDS _IO(MAJOR_NUM, 1)

#endif
