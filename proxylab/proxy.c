/*
 * proxy.c - CS:APP Web proxy
 *
 * Student ID: 2013-11406
 *         Name: Won Wook SONG
 * 
 * IMPORTANT: Give a high level description of your code here. You
 * must also provide a header comment at the beginning of each
 * function that describes what that function does.
 */ 

#include "csapp.h"

/* The name of the proxy's log file */
#define PROXY_LOG "proxy.log"

/* Undefine this if you don't want debugging output */
#define DEBUG

/*
 * Functions to define
 */
void *process_request(void* vargp);
int open_clientfd_ts(char *hostname, int port, sem_t *mutexp);
ssize_t Rio_readn_w(int fd, void *ptr, size_t nbytes);
ssize_t Rio_readlineb_w(rio_t *rp, void *usrbuf, size_t maxlen);
void Rio_writen_w(int fd, void *usrbuf, size_t n);
void print_log(struct sockaddr_in *sockaddr, int portnum, int size, char* msg);

FILE* log_file;
sem_t mutex, mutex_log;

/*
 * main - Main routine for the proxy program
 */
int main(int argc, char **argv)
{
    int listenfd, port; // port num and listening descriptor.
    pthread_t tid;

    /* Check arguments */
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port number>\n", argv[0]);
        exit(0);
    }

    // set up logfile
    log_file = Fopen(PROXY_LOG, "w");
    // initialize mutex to 1.
    Sem_init(&mutex, 0, 1);

    // set up listening descriptor
    port = atoi(argv[1]);
    listenfd = Open_listenfd(port);



    exit(0);
}

void *process_request(void* vargp) {
}

int open_clientfd_ts(char *hostname, int port, sem_t *mutexp) {
}

ssize_t Rio_readn_w(int fd, void *ptr, size_t nbytes) {
    ssize_t read_count;  
    if ((read_count = rio_readn(fd, ptr, nbytes)) < 0) {  
        printf("Warning: rio_readn failed\n");  
        return 0; 
    }
    return read_count;  
}

ssize_t Rio_readlineb_w(rio_t *rp, void *usrbuf, size_t maxlen) {
    ssize_t read_count;
    if ((read_count = rio_readlineb(rp, usrbuf, maxlen)) < 0) {  
        printf("Warning: rio_readlineb failed\n");
        return 0;
    }
    return read_count;
}

void Rio_writen_w(int fd, void *usrbuf, size_t n) {
    if (rio_writen(fd, usrbuf, n) != n) {
        printf("Warning: rio_writen failed.\n");
    }
}

void print_log(struct sockaddr_in *sockaddr, int portnum, int size, char* msg) {
    char log_string[512]; 

    time_t now;
    char time_str[128];

    now = time(NULL);
    strftime(time_str, MAXLINE, "%a %d %b %Y %H:%M:%S %Z", localtime(&now));

    unsigned long host;
    unsigned char a, b, c, d;
    host = ntohl(sockaddr->sin_addr.s_addr);
    a = host >> 24;
    b = (host >> 16) & 0xff;
    c = (host >> 8) & 0xff;
    d = host & 0xff;

    sprintf(log_string, "%s: %d.%d.%d.%d %d %d %s\n", time_str, a, b, c, d, portnum, size, msg);

    P(&mutex_log);
    fprintf(log_file, log_string);
    fflush(log_file);
    V(&mutex_log);

    return;
}
