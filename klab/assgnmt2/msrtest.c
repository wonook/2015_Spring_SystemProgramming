#include "msrdrv.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>

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
    };

    printf ("Reseting & stopping PMU counter...\n");

    ioctl (file_desc, IOCTL_MSR_CMDS, (long long)msr_reset);

    printf ("Done!\n");

    return ;
}

int select_event (int file_desc) {
    int event;

    printf ("Please select the PMU event to monitor!\n");
    printf ("1: uops retired.\n");
    printf ("2: uops issued.\n");
    printf ("3. stalled cycles.\n");
    printf ("4. resource stalls.\n");
    printf ("5. instr retired.\n");
    printf ("6. core cycles.\n");
    printf ("7. ref cycles.\n");

    while (1){
        printf ("Type your event: ");
        scanf ("%d", &event);
        if ((0<event)&&(event<8)) break;
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

    printf ("Starting PMU counter...\n");

    ioctl (file_desc, IOCTL_MSR_CMDS, (long long)msr_start);

    printf ("Done!\n");

    return ;
}

void read_counter (int file_desc, int event) {
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

    printf ("Reading PMU counter...\n");

    ioctl (file_desc, IOCTL_MSR_CMDS, (long long)msr_read);

    printf ("Done!\n");

    switch (event) {
        case 1: // uops retired
            printf ("uops retired: ");
            break;
        case 2: // uops issued
            printf ("uops issued: ");
            break;
        case 3: // stalled cycles
            printf ("stalled cycles: ");
            break;
        case 4: // resource stalls
            printf ("resource stalls: ");
            break;
        case 5: // instr retired
            printf ("instr retired: ");
            break;
        case 6: // core cycles
            printf("core cycles: ");
            break;
        case 7: // ref cycles
            printf("ref cycles: ");
            break;
    }

    printf ("%7lld\n", msr_read[event-1].value);

}



static int loadDriver()
{
    int fd;
    /*fd = open("/dev/" DEV_NAME, O_RDWR);*/
    fd = open("/dev/" DEV_NAME, 2);
    printf("Opening device file: %s...\n", "/dev/" DEV_NAME);
    if (fd == -1) {
        printf("Can't open device file: %s\n", "/dev/" DEV_NAME);
        exit(-1);
        perror("Failed to open /dev/" DEV_NAME);
    }
    return fd;
}

static void closeDriver(int fd)
{
    int e;
    e = close(fd);
    if (e == -1) {
        printf("Failed to close fd");
        perror("Failed to close fd");
    }
}

/*
 * Reference:
 * Intel Software Developer's Manual Vol 3B "253669.pdf" August 2012
 * Intel Software Developer's Manual Vol 3C "326019.pdf" August 2012
 */
int main(void)
{
    int fd;
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
        { MSR_STOP, 0x00, 0x00 }
    };


    fd = loadDriver();
    ioctl(fd, IOCTL_MSR_CMDS, (long long)msr_start);
    printf("This is a hex number 0x%x\n", -1);
    ioctl(fd, IOCTL_MSR_CMDS, (long long)msr_stop);
    printf("uops retired:    %7lld\n", msr_stop[2].value);
    printf("uops issued:     %7lld\n", msr_stop[3].value);
    printf("stalled cycles:  %7lld\n", msr_stop[4].value);
    printf("resource stalls: %7lld\n", msr_stop[5].value);
    printf("instr retired:   %7lld\n", msr_stop[6].value);
    printf("core cycles:     %7lld\n", msr_stop[7].value);
    printf("ref cycles:      %7lld\n", msr_stop[8].value);
    closeDriver(fd);
    return 0;
}