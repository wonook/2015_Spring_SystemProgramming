#include "msrdrv.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>

int main(void)
{
	struct MsrInOut msr_start[] = {
		{ MSR_WRITE, 0x38f, 0x00, 0x00 },       // ia32_perf_global_ctrl: disable 4 PMCs & 3 FFCs
        	{ MSR_WRITE, 0xc1, 0x00, 0x00 },        // ia32_pmc0: zero value (35-5)
       		{ MSR_WRITE, 0xc2, 0x00, 0x00 },        // ia32_pmc1: zero value (35-5)
        	{ MSR_WRITE, 0xc3, 0x00, 0x00 },        // ia32_pmc2: zero value (35-5)
     		{ MSR_WRITE, 0xc4, 0x00, 0x00 },        // ia32_pmc3: zero value (35-5)
        { MSR_WRITE, 0x309, 0x00, 0x00 },       // ia32_fixed_ctr0: zero value (35-17)
        { MSR_WRITE, 0x30a, 0x00, 0x00 },       // ia32_fixed_ctr1: zero value (35-17)
        { MSR_WRITE, 0x30b, 0x00, 0x00 },       // ia32_fixed_ctr2: zero value (35-17)
        { MSR_WRITE, 0x186, 0x004101c2, 0x00 }, // ia32_perfevtsel1, UOPS_RETIRED.ALL (19-28)
        { MSR_WRITE, 0x187, 0x0041010e, 0x00 }, // ia32_perfevtsel0, UOPS_ISSUED.ANY (19.22)
        { MSR_WRITE, 0x188, 0x01c1010e, 0x00 }, // ia32_perfevtsel2, UOPS_ISSUED.ANY-stalls (19-22)
        { MSR_WRITE, 0x189, 0x004101a2, 0x00 }, // ia32_perfevtsel3, RESOURCE_STALLS.ANY (19-27)
        { MSR_WRITE, 0x38d, 0x222, 0x00 },      // ia32_perf_fixed_ctr_ctrl: ensure 3 FFCs enabled
        { MSR_WRITE, 0x38f, 0x0f, 0x07 },       // ia32_perf_global_ctrl: enable 4 PMCs & 3 FFCs
        { MSR_STOP, 0x00, 0x00 }
    };

    struct MsrInOut msr_stop[] = {
        { MSR_WRITE, 0x38f, 0x00, 0x00 },       // ia32_perf_global_ctrl: disable 4 PMCs & 3 FFCs
        { MSR_WRITE, 0x38d, 0x00, 0x00 },       // ia32_perf_fixed_ctr_ctrl: clean up FFC ctrls
        { MSR_READ, 0xc1, 0x00 },               // ia32_pmc0: read value (35-5)
        { MSR_READ, 0xc2, 0x00 },               // ia32_pmc1: read value (35-5)
        { MSR_READ, 0xc3, 0x00 },               // ia32_pmc2: read value (35-5)
        { MSR_READ, 0xc4, 0x00 },               // ia32_pmc3: read value (35-5)
        { MSR_READ, 0x309, 0x00 },              // ia32_fixed_ctr0: read value (35-17)
        { MSR_READ, 0x30a, 0x00 },              // ia32_fixed_ctr1: read value (35-17)
        { MSR_READ, 0x30b, 0x00 },              // ia32_fixed_ctr2: read value (35-17)
	{ MSR_RDTSC, 0x00, 0x00 },
        { MSR_STOP, 0x00, 0x00 }
    };

	int file_desc = open("/dev/msrdrv", O_RDWR);
	if (file_desc < 0) {
		printf(" Can't open device file: %s\n", DEV_NAME);
		exit(-1);
	}

	ioctl(file_desc, IOCTL_MSR_CMDS, (long long)msr_start);
	printf("This is a hex number 0x%x\n", -1);
	ioctl(file_desc, IOCTL_MSR_CMDS, (long long)msr_stop);
	printf("uops retired:    %7lld\n", msr_stop[2].value);
	printf("uops issued:     %7lld\n", msr_stop[3].value);
	printf("stalled cycles:  %7lld\n", msr_stop[4].value);
	printf("resource stalls: %7lld\n", msr_stop[5].value);
	printf("instr retired:   %7lld\n", msr_stop[6].value);
	printf("core cycles:     %7lld\n", msr_stop[7].value);
	printf("ref cycles:      %7lld\n", msr_stop[8].value);
	printf("TSC:		 %7lld\n", msr_stop[9].value);
	close(file_desc);
	
	return 0;
}
