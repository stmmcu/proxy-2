/*
 * proxy.c
 *   An http proxy server.
 *   usage: proxy port
 *
 *   Nathan Bossart & Joe Mayer
 *   Dec 9, 2013
 */
#include <iostream>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <pthread.h>
#include <string.h>
#include <signal.h>

#define BUFSIZE 4096

// handle broken pipes
void broken_pipe_handler(int signum) {
  // this space intentionally left blank
}

void fail(const char* str) {
  perror(str);
  exit(1);
}

// return domain for opening socket
char* parse_host(char* buffer, char** saveptr) {
  char* ret;
  ret = strtok_r(buffer, " \r\n", saveptr);
  while (ret != NULL) {
    if (strcmp(ret, "Host:")==0) {
      ret = strtok_r(NULL, " \r\n", saveptr);
      return ret;
    }
    ret = strtok_r(NULL, " \r\n", saveptr);
  }
  return NULL;
}

// make sure connection is closed
char* gen_req(char* buffer) {
  // TODO: fix
  char* ret  = (char*) malloc(BUFSIZE*sizeof(char));
  strcpy(ret, buffer);
  return ret;
}

void* serve_client(void* v) {
  int sock = *(int*) v;
  free(v);

  FILE* myclient    = fdopen(sock, "r");
  FILE* myclient_wr = fdopen(sock, "w");
  if (myclient == NULL || myclient_wr==NULL) {
    perror("fdopen");
    pthread_exit(0);
  }
  setlinebuf(myclient);
  setlinebuf(myclient_wr);

  char buf[BUFSIZE];
  char buffer[BUFSIZE];
  char* four04 = "HTTP/1.1 404 Not Found\nContent-Length: 219\nContent-Type: text/html\n\n<html><head><title>404 Not Found</title></head><body><h1>404 Not Found</h1><img src='http://www.climateaccess.org/sites/default/files/Obi%20wan.jpeg'></img><p>These aren't the bytes you're looking for.</p></body></html>";
  
  while (fgets(buf, BUFSIZE, myclient) != NULL) {
    strcat(buffer, buf);
    if (strlen(buf)<3) {
      break;
    }
  }
  
  char* req = (char*) malloc(BUFSIZE*sizeof(char));
  strcpy(req, buffer);
  char* saveptr;
  char* host2 = parse_host(buffer, &saveptr);
  if (host2 == NULL) {
    pthread_exit(0);
  }
  //char* req   = gen_req(buffer);

  int err;
  struct addrinfo* host;
  err = getaddrinfo(host2, "80", NULL, &host);
  if (err) {
    fprintf(stderr, "%s : %s\n", host2, gai_strerror(err));
    int l = write(sock, four04, strlen(four04));
    if (l < 0) {
      perror("write");
      pthread_exit(0);
    }
    freeaddrinfo(host);
    free(req);
    printf("\e[1;34mConnection closed on fdesc %d\n\e[0m", sock);
    close(sock);
    fclose(myclient);
    fclose(myclient_wr);
    pthread_exit(0);
  } 

  int sock2 = socket(host->ai_family, SOCK_STREAM, 0);
  if (sock2 < 0) {
    perror("socket");
    pthread_exit(0);
  }

  if (connect(sock2, host->ai_addr, host->ai_addrlen)) {
    int n = write(sock, four04, strlen(four04));
    if (n < 0) {
      perror("write");
      pthread_exit(0);
    }
    freeaddrinfo(host);
    free(req);
    printf("\e[1;34mConnection closed on fdesc %d\n\e[0m", sock);
    close(sock);
    fclose(myclient);
    fclose(myclient_wr);
    pthread_exit(0);
  } else {

    // write header
    FILE* sfd = fdopen(sock2, "w");
    if (sfd == NULL) {
      perror("fdopen");
      pthread_exit(0);
    }
    setlinebuf(sfd);
    printf("---\n%s---\n", req);
    fputs(req, sfd);

    char buf2[BUFSIZE];
    int i = 0;
    i = read(sock2, buf2, BUFSIZE);
    while (i > 0 ) {
      int k = write(sock, buf2, i);
      if (k < 0) {
	perror("write");
	pthread_exit(0);
      }
      i = read(sock2, buf2, BUFSIZE);
    }
    if (i < 0) {
      perror("read");
      pthread_exit(0);
    }
    fclose(sfd);
  }
  freeaddrinfo(host);
  free(req);
  printf("\e[1;34mConnection closed on fdesc %d\n\e[0m", sock);
  close(sock);
  close(sock2);
  fclose(myclient);
  fclose(myclient_wr);
  pthread_exit(0);
}

int main(int argc, char* argv[]) {

  signal(SIGPIPE, broken_pipe_handler);
  siginterrupt(SIGINT, 1);

  int err;
  struct addrinfo* host;
  struct addrinfo  hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_flags = AI_PASSIVE;
  err = getaddrinfo(NULL, argv[1], &hints, &host);
  if (err) {
    fprintf(stderr, "%s : %s\n", argv[1], gai_strerror(err));
    exit(err);
  }
  
  int serv_sock = socket(host->ai_family, SOCK_STREAM, 0);
  if (serv_sock < 0) {
    fail("socket");
  }

  if (bind(serv_sock, host->ai_addr, host->ai_addrlen)) {
    fail("bind");
  }

  if (listen(serv_sock, 5)) {
    fail("listen");
  }

  freeaddrinfo(host);

  printf("\e[1;34mWaiting for connections on port %s\n\e[0m", argv[1]);

  int* new_sock;
  pthread_t new_thread;

  while(1) {
    new_sock = (int*) malloc(sizeof(int));
    *new_sock = accept(serv_sock, NULL, NULL);
    if (*new_sock < 0) {
      fail("accept");
    }

    printf("\e[1;34mAccepted new connection on fdesc %d\n\e[0m", *new_sock);

    pthread_create(&new_thread, NULL, serve_client, new_sock);
    pthread_detach(new_thread);
  }
}
