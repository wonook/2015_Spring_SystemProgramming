#include "msrdrv.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>

void stop_counter (int file_desc) {
	struct MsrInOut msr_stop[] = {
	    	{ MSR_WRITE, 0x38f, 0x00, 0x00 },	// ia32_perf_global_ctrl: disable 4 PMCs & 3 FFCs
                { MSR_WRITE, 0x38d, 0x00, 0x00 },       // ia32_perf_fixed_ctrl: clean up FFC ctrls
		{ MSR_STOP, 0x00, 0x00 }
	};

	printf ("Stopping PMU counter...");
	ioctl (file_desc, IOCTL_MSR_CMDS, (long long)msr_stop);
	printf ("Done!\n");

	return ;
}

void reset_counter (int file_desc) {
	struct MsrInOut msr_reset[] = {
		{ MSR_WRITE, 0x38f, 0x00, 0x00 },	// ia32_perf_global_ctrl: disable 4 PMCs & 3 FFCs
                { MSR_WRITE, 0x38d, 0x00, 0x00 },	// ia32_perf_fixed_ctrl: clean up FFC ctrls
		{ MSR_WRITE, 0xc1, 0x00, 0x00 },        // ia32_pmc0: zero value (35-5)
                { MSR_WRITE, 0xc2, 0x00, 0x00 },        // ia32_pmc1: zero value (35-5)
                { MSR_WRITE, 0xc3, 0x00, 0x00 },        // ia32_pmc2: zero value (35-5)
                { MSR_WRITE, 0xc4, 0x00, 0x00 },        // ia32_pmc3: zero value (35-5)
	        { MSR_WRITE, 0x309, 0x00, 0x00 },       // ia32_fixed_ctr0: zero value (35-17)
		{ MSR_WRITE, 0x30a, 0x00, 0x00 },       // ia32_fixed_ctr1: zero value (35-17)
        	{ MSR_WRITE, 0x30b, 0x00, 0x00 },       // ia32_fixed_ctr2: zero value (35-17)
		{ MSR_STOP, 0x00, 0x00 }
	};

	stop_counter (file_desc);
	printf ("Reseting PMU counter...");
        ioctl (file_desc, IOCTL_MSR_CMDS, (long long)msr_reset);
	printf ("Done!\n");

	return ;
}

int select_event () {
	int event;

	printf ("Please select PMU event to monitor!\n");

	while (1){
		printf ("Type yout event: ");
		scanf ("%d",&event);
		if ((0<=event)&&(event<=8)) break;
		else printf ("Incorrect input!\n");
	}

	return event;
}

void start_counter (int file_desc) {
	struct MsrInOut msr_start[] = {
		{ MSR_WRITE, 0x186, 0x004101c2, 0x00 }, // ia32_perfevtsel1, UOPS_RETIRED.ALL (19-28)
        	{ MSR_WRITE, 0x187, 0x0041010e, 0x00 }, // ia32_perfevtsel0, UOPS_ISSUED.ANY (19.22)
        	{ MSR_WRITE, 0x188, 0x01c1010e, 0x00 }, // ia32_perfevtsel2, UOPS_ISSUED.ANY-stalls (19-22)
       		{ MSR_WRITE, 0x189, 0x004101a2, 0x00 }, // ia32_perfevtsel3, RESOURCE_STALLS.ANY (19-27)
        	{ MSR_WRITE, 0x38d, 0x222, 0x00 },      // ia32_perf_fixed_ctr_ctrl: ensure 3 FFCs enabled
       		{ MSR_WRITE, 0x38f, 0x0f, 0x07 },       // ia32_perf_global_ctrl: enable 4 PMCs & 3 FFCs
	        { MSR_STOP, 0x00, 0x00 }
	};

	printf ("Starting PMU counter...");
        ioctl (file_desc, IOCTL_MSR_CMDS, (long long)msr_start);
	printf ("Done!\n");

	return ;
}

int read_counter (int file_desc, int event) {
	struct MsrInOut msr_read[] = {
        	{ MSR_READ, 0xc1, 0x00 },               // ia32_pmc0: read value (35-5)
        	{ MSR_READ, 0xc2, 0x00 },               // ia32_pmc1: read value (35-5)
        	{ MSR_READ, 0xc3, 0x00 },               // ia32_pmc2: read value (35-5)
        	{ MSR_READ, 0xc4, 0x00 },               // ia32_pmc3: read value (35-5)
        	{ MSR_READ, 0x309, 0x00 },              // ia32_fixed_ctr0: read value (35-17)
        	{ MSR_READ, 0x30a, 0x00 },              // ia32_fixed_ctr1: read value (35-17)
        	{ MSR_READ, 0x30b, 0x00 },              // ia32_fixed_ctr2: read value (35-17)
        	{ MSR_STOP, 0x00, 0x00 }
    	};

	printf ("Reading PMU counter...");
        ioctl (file_desc, IOCTL_MSR_CMDS, (long long)msr_read);
        printf ("Done!\n");

	switch (event) {
	case 8: // all
	case 1: // uops retired
		printf ("uops retired:    %7lld\n", msr_read[0].value);
		if (event != 8) break;
	case 2: // uops issued
                printf ("uops issued:     %7lld\n", msr_read[1].value);
                if (event != 8) break;
	case 3: // stalled cycles
                printf ("stalled cycles:  %7lld\n", msr_read[2].value);
		if (event != 8) break;
	case 4: // resource stalls
	        printf ("resource stalls: %7lld\n", msr_read[3].value);
		if (event != 8) break;
	case 5: // instr retired
                printf ("instr retired:   %7lld\n", msr_read[4].value);
		if (event != 8) break;
	case 6: // core cycles
                printf ("core cycles:     %7lld\n", msr_read[5].value);
		if (event != 8) break;
	case 7: // ref cycles
                printf ("ref cycles:      %7lld\n", msr_read[6].value);
		break;
	default:
		printf ("Incorrect event selected.\n");
		return -1;
	}
	
	return 0;
}

void read_tsc (int file_desc){
	struct MsrInOut msr_readtsc[] = {
		{ MSR_RDTSC, 0x00, 0x00 },
		{ MSR_STOP, 0x00, 0x00 },
	};

	printf ("Reading TSC...");
	ioctl (file_desc, IOCTL_MSR_CMDS, (long long)msr_readtsc);
	printf ("Done!\n");

	printf ("TSC: %7lld\n", msr_readtsc[0].value);

	return ;
}

int main (void) {
	int file_desc = open("/dev/msrdrv", O_RDWR);
	if (file_desc < 0) {
		printf("Can't open device file: %s\n", DEVICE_FILE_NAME);
		exit(-1);
	}

	int event, err_check;
	printf ("Hello! this is PMU counter.\n");
        printf ("Please select PMU event to monitor!\n");
        printf ("0: exit. \n");
        printf ("1: uops retired.\n");
        printf ("2: uops issued.\n");
        printf ("3. stalled cycles.\n");
        printf ("4. resource stalls.\n");
        printf ("5. instr retired.\n");
        printf ("6. core cycles.\n");
        printf ("7. ref cycles.\n");
        printf ("8. all.\n");

	while (1){
		if (0 == (event = select_event())) break;
		if (0 > (err_check = read_counter (file_desc, event))) break;
	}
	
	file_desc = close(file_desc);
	if (file_desc < 0) {
		printf("Can't close device file: %s\n", DEVICE_FILE_NAME);
		exit(-1);
	}

	printf ("Bye!\n");
	return 0;
}
