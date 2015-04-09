/*
 *  ioctl.c - the process to use ioctl's to control the kernel module
 *
 *  Until now we could have used cat for input and output.  But now
 *  we need to do ioctl's, which require writing our own process.
 */

/* 
 * device specifics, such as ioctl numbers and the
 * major device file. 
 */
#include "chardev.h"

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>		/* open */
#include <unistd.h>		/* exit */
#include <sys/ioctl.h>		/* ioctl */

/* 
 * Functions for the ioctl calls 
 */

/* my struct having process informantion */
struct pinfo {
	char pname[16];
	int processid;
};

ioctl_tree(int file_desc){
	struct pinfo plist[128];
	int ret_val, len, i, j;

	ret_val = ioctl(file_desc, 0, plist);

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

ioctl_set_msg(int file_desc, char *message)
{
	int ret_val;

	ret_val = ioctl(file_desc, IOCTL_SET_MSG, message);

	if (ret_val < 0) {
		printf("ioctl_set_msg failed:%d\n", ret_val);
		exit(-1);
	}
}

ioctl_get_msg(int file_desc)
{
	int ret_val;
	char message[100];

	/* 
	 * Warning - this is dangerous because we don't tell
	 * the kernel how far it's allowed to write, so it
	 * might overflow the buffer. In a real production
	 * program, we would have used two ioctls - one to tell
	 * the kernel the buffer length and another to give
	 * it the buffer to fill
	 */
	ret_val = ioctl(file_desc, IOCTL_GET_MSG, message);

	if (ret_val < 0) {
		printf("ioctl_get_msg failed:%d\n", ret_val);
		exit(-1);
	}

	printf("get_msg message:%s\n", message);
}

ioctl_get_nth_byte(int file_desc)
{
	int i;
	char c;

	printf("get_nth_byte message:");

	i = 0;
	do {
		c = ioctl(file_desc, IOCTL_GET_NTH_BYTE, i++);

		if (c < 0) {
			printf
			    ("ioctl_get_nth_byte failed at the %d'th byte:\n",
			     i);
			exit(-1);
		}

		putchar(c);
	} while (c != 0);
	putchar('\n');
}

/* 
 * Main - Call the ioctl functions 
 */
int main()
{
	int file_desc, ret_val;
	char *msg = "Message passed by ioctl\n";

	file_desc = open("/dev/chardev", O_RDWR);
        printf("the open funciont's result is: %d\n", file_desc);
	if (file_desc < 0) {
		printf("Can't open device file: %s\n", DEVICE_FILE_NAME);
		exit(-1);
	}

	ioctl_tree(file_desc);	
	
	close(file_desc);

	return 0;
}
