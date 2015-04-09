/*
 * ioctl.c - the process to use ioctl's to control the kernel module
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
#include <fcntl.h>    /* open */
#include <unistd.h>   /* exit */
#include <sys/ioctl.h>    /* ioctl */

/* 
 * Functions for the ioctl calls 
 */

ioctl_set_msg(int file_desc, char *message)
{
  int ret_val;

  printf("IOCTL_SET_MSG:%lu\n", IOCTL_SET_MSG);
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

  printf("IOCTL_GET_MSG:%lu\n", IOCTL_GET_MSG);
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

  printf("IOCTL_GET_NTH_BYTE:%lu\n", IOCTL_GET_NTH_BYTE);
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

ioctl_tree(int file_desc) {
  int ret_val;
  struct infonode plist[128];
  int i=0, j, len;

  printf("IOCTL_TREE:%lu\n", IOCTL_TREE);
  ret_val = ioctl(file_desc, IOCTL_TREE, plist);

  if (ret_val < 0) {
    printf("ioctl_tree failed:%d\n", ret_val);
    exit(-1);
  }

//Printing out the stuff
  while(plist[i].pid != NULL) {
    i++;
  }
  len = i;

  for(; i>=0; i--) {
    for(j=0; j<len-i; j++) {
      printf("\t");
    }
    printf("\\process name(pid): %s(%d)\n", plist[i].pname, plist[i].pid);
  }

}


/* 
 * Main - Call the ioctl functions 
 */
main()
{
  int file_desc, ret_val;
  char *msg = "Message passed by ioctl\n";

  file_desc = open(DEVICE_FILE_NAME, 0);
  /*file_desc = open(DEVICE_FILE_NAME, O_RDWR);*/
  printf("Opening device file: %d\n", file_desc);
  if (file_desc < 0) {
    printf("Can't open device file: %s\n", DEVICE_FILE_NAME);
    exit(-1);
  }

  /*ioctl_get_nth_byte(file_desc);*/
  /*ioctl_get_msg(file_desc);*/
  /*ioctl_set_msg(file_desc, msg);*/
  ioctl_tree(file_desc);

  close(file_desc);
}
