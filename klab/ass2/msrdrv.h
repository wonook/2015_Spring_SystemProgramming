// msrdrv.h - the header file for msrdrv.c

#ifndef _MG_MSRDRV_H
#define _MG_MSRDRV_H

#include <linux/ioctl.h>
#include <linux/types.h>

#define DEVICE_NAME "msrdrv"
#define DEVICE_FILE_NAME "msrdrv"
#define MAJOR_NUM 33	// Major device number. We need this to open device file.
#define MINOR_NUM 0

#define MSR_VEC_LIMIT 32

#define IOCTL_MSR_CMDS _IO(MAJOR_NUM, 1)

enum MsrOperation {
	MSR_NOP	  = 0,
	MSR_READ  = 1,
	MSR_WRITE = 2,
	MSR_STOP  = 3,
	MSR_RDTSC = 4
};

struct MsrInOut{
	unsigned int op;	// MsrOperation
	unsigned int ecx;	// msr identifier
	union {
		struct {
			unsigned int eax;	// low double word
			unsigned int edx;	// high double word
		};
		unsigned long long value;	// quad word
	};
};

#endif
