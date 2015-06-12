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

// pointer to the log file
FILE* log_file;
// mutexes
sem_t mutex, mutex_log;
int totalcnt;

// struct for the arguments passed onto the thread
struct thread_args {
    int connectfd;
    int connectport;
    char hostaddr[MAXLINE];
};

/*
 * main - Main routine for the proxy program
 */
int main(int argc, char **argv)
{
    int listenfd, listenport; // listening port num and listening descriptor.
    struct sockaddr_in clientaddr;
    int clientleng = sizeof(clientaddr);
    pthread_t tid;

    /* Check arguments */
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port number>\n", argv[0]);
        exit(0);
    }

    // set up logfile
    log_file = Fopen(PROXY_LOG, "w");
    // initialize totalcnt
    totalcnt = 0;
    // initialize mutex to 1.
    Sem_init(&mutex, 0, 1);
    Sem_init(&mutex_log, 0, 1);

    // set up listening descriptor
    listenport = atoi(argv[1]);
    listenfd = Open_listenfd(listenport);

    while(1) {
        struct thread_args *argp = (struct thread_args*) malloc(sizeof(struct thread_args));

        //set up connection descriptor.
        argp->connectfd = Accept(listenfd, (SA *) &clientaddr, &clientleng);
        argp->connectport = ntohs(clientaddr.sin_port);
        strcpy(argp->hostaddr, inet_ntoa(clientaddr.sin_addr));

        struct hostent *hp = Gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr, sizeof(clientaddr.sin_addr.s_addr), AF_INET);

        printf("Proxy server connected to %s (%s), port %d\n", hp->h_name, argp->hostaddr, argp->connectport);

        // create a thread that processes given request.
        Pthread_create(&tid, NULL, process_request, argp);
    }

    exit(0);
}

/*
 * Each thread processes each request through the variables passed
 */
void *process_request(void* vargp) {
    pthread_t tid = pthread_self();
    printf("Served by thread %u\n", tid);
    Pthread_detach(tid);

    int connectfd = ((struct thread_args*)vargp)->connectfd;
    int connectport = ((struct thread_args*)vargp)->connectport;
    char hostaddr[MAXLINE];
    strcpy(hostaddr, ((struct thread_args*)vargp)->hostaddr);

#ifdef DEBUG
  printf("| process request -> connectfd:%d connectport:%d, hostaddr:%s\n", connectfd, ((struct thread_args*)vargp)->connectport, hostaddr);
#endif
    // this frees argp in main()
    Free(vargp);

    // read stuff from the connectfd.
    char buffer[MAXLINE];
    rio_t rio_client;
    rio_t rio_server;
    int cnt, i;

    Rio_readinitb(&rio_client, connectfd);
    while(1) {
        if((cnt = Rio_readlineb_w(&rio_client, buffer, MAXLINE)) <= 0) {
            printf("bad request error\n");
            close(connectfd);
            return;
        }

        char *host, *portstr, *msg;
        int clientfd, port;

        P(&mutex);
        totalcnt += cnt;
        V(&mutex);
        printf("Thread %u received %d bytes (%d total)\n", tid, cnt, totalcnt);

        char *proxyerrormsg = "proxy usage: <host> <port> <message>\n";
        // process out the buffer
#ifdef DEBUG
printf("| | string to process:%s", buffer);
#endif
        host = strtok(buffer, " ");
        i = strlen(host);
        if(cnt <= i || cnt == i+2) { // only one argument
            Rio_writen_w(connectfd, proxyerrormsg, strlen(proxyerrormsg));
            printf("%s", proxyerrormsg);
            continue;
        }
        portstr = strtok(NULL, " ");
        i += strlen(portstr) + 1;
        if(cnt <= i || cnt == i+2 || portstr[0] < '0' || portstr[0] > '9') { // only two arguments OR portstr[0]!=number
            Rio_writen_w(connectfd, proxyerrormsg, strlen(proxyerrormsg));
            printf("%s", proxyerrormsg);
            continue;
        }
        msg = strtok(NULL, " ");

        P(&mutex);
        port = atoi(portstr);
        V(&mutex);
        if(port <= 0 || port == connectport) { // error with the port number
            Rio_writen_w(connectfd, proxyerrormsg, strlen(proxyerrormsg));
            printf("%s", proxyerrormsg);
            continue;
        }
#ifdef DEBUG
        printf("| | after processing: host:%s, port:%s, msg:%s", host, portstr, msg);
#endif

        // open clientfd
    }
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
