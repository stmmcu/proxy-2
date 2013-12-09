/*
 * proxy.c
 *   An http proxy server.
 *   usage: proxy port
 *
 *   Nathan Bossart & Joe Mayer
 *   Dec 5, 2013
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

#define BUFSIZE 4096

// TODO: SIGPIPE handler

void fail(const char* str) {
  perror(str);
  exit(1);
}

// return domain for opening socket
char* parse_host(char* buffer) {
  char* ret = (char*) malloc(BUFSIZE*sizeof(char));
  strtok(buffer, "/");
  strcpy(ret, strtok(NULL, "/"));
  return ret;
}

// make sure connection is closed
char* gen_req(char* buffer) {
  // TODO: fix up request
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
    fputs(buffer, stdout);
    if (strlen(buf)<3) {
      break;
    }
  }
  
  char* req = (char*) malloc(BUFSIZE*sizeof(char));
  strcpy(req, buffer);
  char* host2 = parse_host(buffer);
  //char* req   = gen_req(buffer);
  memset(buffer, 0, sizeof(buffer));

  int err;
  struct addrinfo* host;
  err = getaddrinfo(host2, "80", NULL, &host);
  if (err) {
    fprintf(stderr, "%s : %s\n", host2, gai_strerror(err));
    exit(err); // TODO: don't exit here, fail with a 404
  } 

  // TODO: error checking!
  int sock2 = socket(host->ai_family, SOCK_STREAM, 0);

  if (connect(sock2, host->ai_addr, host->ai_addrlen)) {
    int n = write(sock, four04, strlen(four04));
    if (n < 0) {
      perror("write");
      exit(1);
    }
    
  } else {

    FILE* serv_stream    = fdopen(sock2, "w");
    FILE* serv_stream_rd = fdopen(sock2, "r");
    if (serv_stream == NULL || serv_stream_rd==NULL) {
      fail("fdopen");
    }
    setlinebuf(serv_stream);
    setlinebuf(serv_stream_rd);
    
    // write header
    printf("req is %s\n", req);
    int n = write(sfd, req, strlen(req));
    if (n < 0) {
      perror("write");
      exit(1);
    }

    char* buf2 = (char*) malloc(BUFSIZE*sizeof(char));
    memset(buf2, 0, sizeof(buf2));
    printf("4\n");

    // TODO: error checking!
    int i = 0;
    read(sfd, buf2, strlen(buf2));
    write(sfd, buf2, strlen(buf2));
    printf(buf2);
    free(buf2);
    
    fclose(serv_stream);
    fclose(serv_stream_rd);
    
  }

  freeaddrinfo(host);
  free(host2);
  free(req);

  printf("Connection closed on fdesc %d\n", sock);

  fclose(myclient);
  fclose(myclient_wr);
  pthread_exit(0);
}

int main(int argc, char* argv[]) {

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

  printf("Waiting for connections on port %s\n", argv[1]);

  int* new_sock;
  pthread_t new_thread;

  while(1) {
    new_sock = (int*) malloc(sizeof(int));
    *new_sock = accept(serv_sock, NULL, NULL);
    if (*new_sock < 0) {
      fail("accept");
    }

    printf("Accepted new connection on fdesc %d\n", *new_sock);

    pthread_create(&new_thread, NULL, serve_client, new_sock);
    pthread_detach(new_thread);
  }
}
