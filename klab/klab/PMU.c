/*
 * Reference : Intel 64 and IA-32 Architectures Software Developer's Manual.
 */

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
	printf ("Done! ");

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
	printf ("Done! ");

	return ;
}

int select_event () {
	int event;

	printf ("Please select PMU event to monitor!\n");

	while (1){
		printf ("Type your event: ");
		scanf ("%d",&event);
		if ((0<=event)&&(event<=2)) break;
		else printf ("Incorrect input!\n");
	}

	return event;
}

int start_counter (int file_desc, int event) {
        struct MsrInOut msr_start1[] = { 
	        { MSR_WRITE, 0x186, 0x00418108, 0x00 }, // ia32_perfevtsel0, DTLB_LOAD_MISSES.MISS_CAUSES_A_WALK (19-22)
	        { MSR_WRITE, 0x187, 0x00418208, 0x00 }, // ia32_perfevtsel1, DTLB_LOAD_MISSES.MISS_COMPLETED (19-22)
	        { MSR_WRITE, 0x188, 0x00410149, 0x00 }, // ia32_perfevtsel2, DTLB_STORE_MISSES.MISS_CAUSES_A_WALK (19-24)                
                { MSR_WRITE, 0x189, 0x00410249, 0x00 }, // ia32_perfevtsel3, DTLB_STORE_MISSES.MISS_COMPLETED (19-24)
                { MSR_WRITE, 0x38d, 0x222, 0x00 },      // ia32_perf_fixed_ctr_ctrl: ensure 3 FFCs enabled
                { MSR_WRITE, 0x38f, 0x0f, 0x07 },       // ia32_perf_global_ctrl: enable 4 PMCs & 3 FFCs
                { MSR_STOP, 0x00, 0x00 }
        };       
    
    	struct MsrInOut msr_start2[] = {
		{ MSR_WRITE, 0x186, 0x004101d1, 0x00 }, // ia32_perfevtsel0, MEM_LOAD_UOPS_RETIRED.L1_HIT (19-29)
	        { MSR_WRITE, 0x187, 0x004108d1, 0x00 }, // ia32_perfevtsel1, MEM_LOAD_UOPS_RETIRED.L1_MISS(19-29)
		{ MSR_WRITE, 0x188, 0x004102d1, 0x00 }, // ia32_perfevtsel2, MEM_LOAD_UOPS_RETIRED.L2_HIT (19-29)
		{ MSR_WRITE, 0x189, 0x004110d1, 0x00 }, // ia32_perfevtsel3, MEM_LOAD_UOPS_RETIRED.L2_MISS(19-29)
	        { MSR_WRITE, 0x38d, 0x222, 0x00 },      // ia32_perf_fixed_ctr_ctrl: ensure 3 FFCs enabled
	        { MSR_WRITE, 0x38f, 0x0f, 0x07 },       // ia32_perf_global_ctrl: enable 4 PMCs & 3 FFCs
	        { MSR_STOP, 0x00, 0x00 }
	};


	printf ("Starting PMU counter...");
	switch (event) {
	case 1: // TLB load & store misses
		ioctl (file_desc, IOCTL_MSR_CMDS, (long long)msr_start1);
		break;
	case 2: // Memory retired uops with cache hit or miss
		ioctl (file_desc, IOCTL_MSR_CMDS, (long long)msr_start2);
		break;
	default :
		printf ("Incorrect event at starting counter\n");
		return -1;
	}

	printf ("Done! \n");

	return 0;
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

	printf ("Printing general purpose function couter\n");
	switch (event) {
	case 1: // TLB load & store misses
		printf ("Miss TLB load walk:   %7lld\n", msr_read[0].value);
                printf ("Miss TLB load compl:  %7lld\n", msr_read[1].value);
                printf ("Miss TLB store walk:  %7lld\n", msr_read[2].value);
	        printf ("Miss TLB store compl: %7lld\n", msr_read[3].value);
		break;
	case 2: // MEM_LOAD_UOPS_RETIRED with cache hit | miss
	        printf ("Retired uops L1 hit:  %7lld\n", msr_read[0].value);
	        printf ("Retired uops L1 miss: %7lld\n", msr_read[1].value);
	        printf ("Retired uops L2 hit:  %7lld\n", msr_read[2].value);
	        printf ("Retired uops L2 miss: %7lld\n", msr_read[3].value);
		break;
	default:
		printf ("Incorrect event selected. \n");
		return -1;
	}
	
	printf ("Printing fixed purpose function counter\n");
        printf ("instr retired:        %7lld\n", msr_read[4].value);
        printf ("core cycles:          %7lld\n", msr_read[5].value);
        printf ("ref cycles:           %7lld\n", msr_read[6].value);
	
	return 0;
}

void read_tsc (int file_desc){
	struct MsrInOut msr_readtsc[] = {
		{ MSR_RDTSC, 0x00, 0x00 },
		{ MSR_STOP, 0x00, 0x00 },
	};

	ioctl (file_desc, IOCTL_MSR_CMDS, (long long)msr_readtsc);

	printf ("TSC: %7lld\n", msr_readtsc[0].value);

	return ;
}

static void swap(int *a, int *b) {
    int tmp = *a;
    *a = *b;
    *b = tmp;
    return;
}

static void printarray(int *a, int size) {
    printf("array[%d]: [", size);
    int i;
    for(i=0; i<size; i++) {
        if(i==size-1) {
            printf("%d", a[i]);
            break;
        }
        printf("%d, ", a[i]);
    }
    printf("]\n");
    return;
}

static void testing(void) {
    int test[8];
    int i, j=1;
    for(i=0; i<8; i++) {
        j *= 7;
        test[i] = j%10;
    }
    printarray(test, 8);
    for(i=7; i>=0; i--) {
        for(j=0; j<i; j++) {
            if(test[j]>test[j+1]) {
                swap(&test[j], &test[j+1]);
                printarray(test, 8);
            }
        }
    }
}

int main (void) {
	int file_desc = open("/dev/msrdrv", O_RDWR);
	if (file_desc < 0) {
		printf("Can't open device file: %s\n", DEVICE_FILE_NAME);
		exit(-1);
	}

	int event, err_check;
	printf ("Hello! this is PMU counter. ");
	printf ("We would do some bubble sort.\n");
	printf ("You have some options to select the general purpose function counter event!\n");
	printf ("Fixed function counter and TSC's values will be given together always.\n");
        printf ("0: exit. \n");
        printf ("1: TLB load & store misses\n");
	printf ("2: Retired load uops with cache hit or miss\n");

	while (1){
		if (0 == (event = select_event())) break;
	    	reset_counter (file_desc);
		if (0 > (err_check = read_counter (file_desc, event))) break;
		read_tsc (file_desc);
		if (0 > (err_check = start_counter (file_desc, event))) break;
		testing();
		stop_counter(file_desc);
		if (0 > (err_check = read_counter (file_desc, event))) break;
		read_tsc (file_desc);
	}
	
	file_desc = close(file_desc);
	if (file_desc < 0) {
		printf("Can't close device file: %s\n", DEVICE_FILE_NAME);
		exit(-1);
	}

	printf ("Bye!\n");
	return 0;
}
