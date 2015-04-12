/*
 * tree.c - the user process connecting device file and take processes information.
 */
#include "chardev.h"

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>		/* open */
#include <unistd.h>		/* exit */
#include <sys/ioctl.h>		/* ioctl */

/* my struct having process informantion */
struct pinfo {
	char pname[16];
	int processid;
};

ioctl_tree(int file_desc){
	struct pinfo plist[128];
	int ret_val, len, i, j;

	ret_val = ioctl(file_desc, IOCTL_CHR_CMDS, plist);

        if (ret_val < 0) {
                printf("ioctl_tree failed:%d\n", ret_val);
                exit(-1);
        }

        printf("get_process_tree:\n");
	for(len=0;;len++){
		if (plist[len].processid == NULL) break;
	}
 
	i = 0;
	for(len;len>=0;len--){
		for(j=0;j<i;j++)
			printf("	");
		i++;
		printf ("\\process name: %s, process id: %d\n", plist[len].pname, plist[len].processid);
	}
	
}

int main()
{
	int file_desc;

	file_desc = open("/dev/chardev", O_RDWR);
        printf("the open funciont's result is: %d\n", file_desc);
	if (file_desc < 0) {
		printf ("Can't open device file: %s\n", DEVICE_FILE_NAME);
		exit (-1);
	}

	ioctl_tree(file_desc);	
	
	file_desc = close(file_desc);
	if (file_desc < 0) {
	    	printf ("Can't close device file: %s\n", DEVICE_FILE_NAME);
		exit (-1);
	}
	return 0;
}
