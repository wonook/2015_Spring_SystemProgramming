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

/* Bundle of argument for process_request(thread) */
typedef struct _args_thread {
  int connfd;
  int client_port;
  char haddr[20];
} args_thread_t;

/* Semaphores */
sem_t mutex_gethost;
sem_t mutex_log;
sem_t mutex_atoi;
sem_t mutex_bytecnt;

/* global variables */
int myport = -1;
int bytecnt = 0;

/*
 * Functions to define
 */
void *process_request(void* vargp);
int open_clientfd_ts(char *hostname, int port, sem_t *mutexp);
ssize_t Rio_readn_w(int fd, void *ptr, size_t nbytes);
ssize_t Rio_readlineb_w(rio_t *rp, void *usrbuf, size_t maxlen);
void Rio_writen_w(int fd, void *usrbuf, size_t n);

/*
 * Additional functions
 */
void write_log_ts(const char *client_addr,
               int port, int bytes, const char *buf);
void usage_error(int connfd);
void open_clientfd_error(int connfd);
int parse_message(char *message, char **hostname, char **port, char **string);

/*
 * main - Main routine for the proxy program
 */
int main(int argc, char **argv)
{
  /* Check arguments */
  if (argc != 2) {
      fprintf(stderr, "Usage: %s <port>\n", argv[0]);
      exit(0);
  }

  /* ignore SIGPIPE */
  Signal(SIGPIPE, SIG_IGN);

  /* Open listenfd, accept connection request, and deliver to main server. */
  myport = atoi(argv[1]);
  int listenfd;
  struct sockaddr_in clientaddr;
  int clientlen = sizeof(clientaddr);
  pthread_t tid;

  /* initialize semaphores */
  sem_init(&mutex_gethost, 0, 1);
  sem_init(&mutex_log, 0, 1);
  sem_init(&mutex_atoi, 0, 1);
  sem_init(&mutex_bytecnt, 0, 1);
  listenfd = Open_listenfd(myport);
  while (1) {
    /* malloc for protecting shared with other threads */
    args_thread_t *args_thread_p = (args_thread_t *)malloc(sizeof(args_thread_t));

    /* accept client connection requests */
    args_thread_p->connfd = Accept(listenfd, (SA *) &clientaddr, &clientlen);
    /* determine the domain name and IP address of the client and copy at args_thread */
    strcpy(args_thread_p->haddr, inet_ntoa(clientaddr.sin_addr));
    args_thread_p->client_port = ntohs(clientaddr.sin_port);
    /* print message */
    struct hostent *hp = Gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr, 
                                      sizeof(clientaddr.sin_addr.s_addr), AF_INET);
    printf("Proxy server connected to %s (%s), port %d\n",
           hp->h_name, args_thread_p->haddr, args_thread_p->client_port);

    /* create a thread for managing requests from this connection */
    Pthread_create(&tid, NULL, process_request, args_thread_p);
    /* print message */
    printf("Served by thread %u\n", tid);
  }
}

/*
 * process_request - Manage requests from a specific connection.
 * Required arguments is passed via vargp.
 */
void *process_request(void *vargp)
{
  pthread_t tid = pthread_self();
  char haddr[20];
  char buf[MAXLINE];
  int n;
  int connfd;
  int client_port;
  rio_t rio_client;
  rio_t rio_server;

  /* get argument from bundle */
  connfd      = ((args_thread_t *)vargp)->connfd;
  client_port = ((args_thread_t *)vargp)->client_port;
  strcpy(haddr, ((args_thread_t *)vargp)->haddr);
  Pthread_detach(tid);
  Free(vargp);

  /* recieve message from client */
  Rio_readinitb(&rio_client, connfd);
  while((n = Rio_readlineb_w(&rio_client, buf, MAXLINE)) > 0) {
    char *hostname;
    char *port;
    char *string;
    int clientfd;

    /* count total byte */
    P(&mutex_bytecnt);
    bytecnt += n;
    V(&mutex_bytecnt);
    /* print message */
    printf("Thread %u received %d bytes (%d total)\n", tid, n, bytecnt);

    /* Parse client message to get hostname & port & content.
       NOT used strtok() because of keeping thread-safety. */
    if (!parse_message(buf, &hostname, &port, &string)) {
      usage_error(connfd);
      continue;
    }

    /* make atoi() thread-safe */
    P(&mutex_atoi);
    int port_i = atoi(port);
    V(&mutex_atoi);

    /* handling illegal port number */
    if (port_i <= 0 || port_i == myport) {
      usage_error(connfd);
      continue;
    }

    /* open clientfd - use thread-safe version */
    if ((clientfd = open_clientfd_ts(hostname, port_i, &mutex_gethost)) < 0) {
      open_clientfd_error(connfd);
      continue;
    };
    /* print message */
    printf("Hostname: %s, port: %d, string: %s", hostname, port_i, string);

    /* deliver string to server and receive response of it */
    /* write log, deliver response to client and close connection between server */
    Rio_readinitb(&rio_server, clientfd);
    Rio_writen_w(clientfd, string, strlen(string));
    if ((n = Rio_readlineb_w(&rio_server, buf, MAXLINE)) > 0) {
      write_log_ts(haddr, client_port, n, buf);
      Rio_writen_w(connfd, buf, n);
      Close(clientfd);
    } else {
      /* server doesn't response */
      usage_error(connfd);
    }
  }

  Close(connfd);
  return NULL;
}

/*
 * parse_message - Parse message from the client thread-safely.
 * If it is appropriate, return 1, else 0.
 */
int parse_message(char *message, char **hostname, char **port, char **string)
{
  char *chp = message;

  /* get hostname */
  while (*chp == ' ') chp++;
  *hostname = chp;
  while (1) {
    if (*chp == '\0')
      return 0;
    if (*chp == ' ')
      break;
    chp++;
  } /* chp is ' ' */
  *(chp++) = '\0';
  
  /* get port */
  while (*chp == ' ') chp++;
  *port = chp;
  while (1) {
    if (*chp == '\0')
      return 0;
    if (*chp == ' ')
      break;
    chp++;
  } /* chp is ' ' */
  *(chp++) = '\0';

  /* get message */
  while (*chp == ' ') chp++;
  if (*chp == '\n' || *chp == '\0')
    return 0;
  *string = chp;
  return 1;    
}

/*
 * open_clientfd_error - Called when open_clientfd_ts() error occurs.
 */
void open_clientfd_error(int connfd)
{
  char *errmsg = "open_clientfd_ts error\n";
  Rio_writen_w(connfd, errmsg, strlen(errmsg));
  printf("%s", errmsg);
}

/*
 * usage_error - Called when the client put illegal message format.
 * Deliver error message to client.
 */
void usage_error(int connfd) {
  char *errmsg = "Proxy Usage: <hostname> <port> <string>\n";
  Rio_writen_w(connfd, errmsg, strlen(errmsg));
  printf("%s", errmsg);
}

/*
 * write_log - Thread-safe writer function to log at PROXY_LOG.
 * To make thread-safe, use mutex_log semaphore.
 * TODO - Is it thread-safe?
 */
void write_log_ts(const char *client_addr, int port, int bytes, const char *buf)
{
  FILE *fp;
  time_t timer;
  struct tm localtm;
  int buf_int[10];
  char buf_time[50];

  P(&mutex_log);    /* lock */

  if ((fp = fopen(PROXY_LOG, "a+")) == NULL) {
    printf("%s open error\n", PROXY_LOG);
    exit(0);
  }

  /* get current time via struct tm */
  /* use reenterance version of localtime() */
  timer = time(NULL);
  localtime_r(&timer, &localtm);
  asctime_r(&localtm, buf_time);
  buf_time[strlen(buf_time)-1] = ' ';
  strcat(buf_time, tzname[1]);

  /* write to file */
  fputs(buf_time, fp);
  fputs(": ", fp);
  fputs(client_addr, fp);
  fputs(" ", fp);
  sprintf(buf_int, "%d", port);
  fputs(buf_int, fp);
  fputs(" ", fp);
  sprintf(buf_int, "%d", bytes);
  fputs(buf_int, fp);
  fputs(" ", fp);
  fputs(buf, fp);

  fclose(fp);

  V(&mutex_log);    /* unlock */
}

/*
 * open_clientfd_ts - Thread-safe version of its original function.
 * The original is thread-unsafe because of gethostbyname().
 * Use lock-and-copy technique for the function.
 */
int open_clientfd_ts(char *hostname, int port, sem_t *mutexp)
{
  int clientfd;
  struct hostent *hp;
  struct hostent *priv_hp = (struct hostent *)malloc(sizeof(struct hostent));
  struct sockaddr_in serveraddr;

  if ((clientfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    return -1; /* check errno for cause of error */

  /* fill in the server's IP address and port */

  /* gethostbyname() is thread-unsafe, so that use lock-and-copy technique */
  P(mutexp);     /* lock */
  if ((hp = gethostbyname(hostname)) == NULL) {
    V(mutexp);   /* unlock */
    return -2;   /* check h_errno for cause of error */
  }
  /* copy to private and unlock */
  memcpy(priv_hp, hp, sizeof(struct hostent));
  V(mutexp);
  /* end lock & copy */

  bzero((char *) &serveraddr, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  bcopy((char *)priv_hp->h_addr_list[0],
        (char *)&serveraddr.sin_addr.s_addr, priv_hp->h_length);
  serveraddr.sin_port = htons(port);
  free(priv_hp);

  /* Establish a connection with the server */
  if (connect(clientfd, (SA *) &serveraddr, sizeof(serveraddr)) < 0) {
    return -1;
  }
  return clientfd;
}

/*
 * Rio_readn_w - Wrapper function of rio_readn().
 * It doesn't terminate program even if there is a error
 * and return 0, as though it encountered EOF on the socket.
 */
ssize_t Rio_readn_w(int fd, void *ptr, size_t nbytes)
{
  ssize_t n;

  if ((n = rio_readn(fd, ptr, nbytes)) < 0) {
    printf("Rio_readn error\n");
    return 0;
  }
  return n;
}

/*
 * Rio_readlineb_w - Wrapper function of rio_readlineb().
 * It doesn't terminate program even if there is a error.
 * and return 0, as though it encountered EOF on the socket.
 */
ssize_t Rio_readlineb_w(rio_t *rp, void *usrbuf, size_t maxlen)
{
  ssize_t rc;

  if ((rc = rio_readlineb(rp, usrbuf, maxlen)) < 0) {
    printf("Rio_readlineb error\n");
    return 0;
  }
  return rc;
}

/*
 * Rio_writen_w - Wrapper function of Rio_writen().
 */
void Rio_writen_w(int fd, void *usrbuf, size_t n)
{
  size_t nleft = n;
  ssize_t nwritten;
  char *bufp = usrbuf;

  while (nleft > 0) {
    if ((nwritten = write(fd, bufp, nleft)) <= 0) {
      if (errno == EINTR)  /* interrupted by sig handler return */
        nwritten = 0;    /* and call write() again */
      else
        return;       /* errorno set by write() */
    }
    nleft -= nwritten;
    bufp += nwritten;
  }
  return;
}

