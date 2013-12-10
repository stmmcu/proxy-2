/*
 * proxy.c
 *   An http proxy server.
 *   usage: proxy port
 *
 *   Nathan Bossart & Joe Mayer
 *   Dec 10, 2013
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

#define BUFSIZE 10240
#define FOURZEROFOUR "HTTP/1.1 404 Not Found\nContent-Length: 219\nContent-Type: text/html\n\n<html><head><title>404 Not Found</title></head><body><h1>404 Not Found</h1><img src='http://www.climateaccess.org/sites/default/files/Obi%20wan.jpeg'></img><p>These aren't the bytes you're looking for.</p></body></html>\0"
#define FOURZEROZERO "HTTP/1.1 400 Bad Request\nContent-Length: 230\nContent-Type: text/html\n\n<html><head><title>400 Bad Request</title></head><body><h1>400 Bad Request</h1><img src='http://www.geeksofdoom.com/GoD/img/2007/05/2007-05-25-choke_lg.jpg'></img><p>If this is an HTTP request, where is the host?</p></body></html>\0"

// handle broken pipes
void broken_pipe_handler(int signum) {}  // this function intentionally left blank

// nice failure functions
void fail(const char* str)        {perror(str); exit(1);}
void fail_thread(const char* str) {perror(str); pthread_exit(0);}

// parse request for host
char* parse_host(char* buf, char** saveptr) {
  char* tok;
  tok = strtok_r(buf, " \r\n", saveptr);
  while (tok != NULL) {
    if (strcmp(tok, "Host:")==0) {
      tok = strtok_r(NULL, " \r\n", saveptr);
      return tok;
    }
    tok = strtok_r(NULL, " \r\n", saveptr);
  }
  return NULL;
}

// clean up before terminating thread
void close_connections(struct addrinfo* host, int sock, FILE* f) {
  if (host!=NULL) freeaddrinfo(host);
  fclose(f);
  printf("\e[1;34mConnection closed on fdesc %d\n\e[0m", sock);
  close(sock);
  pthread_exit(0);
}

// serve each connection
void* serve_client(void* v) {
  int sock_client = *(int*) v;
  free(v);

  // fdesc for client socket
  FILE* client_r = fdopen(sock_client, "r");
  if (client_r==NULL) fail_thread("fdopen");
  setlinebuf(client_r);
  
  // obtain header
  char buf[BUFSIZE];
  char req[BUFSIZE];
  while (fgets(buf, BUFSIZE, client_r) != NULL) {
    strcat(req, buf);
    if (strlen(buf) < 3) break;
  }

  // find host and port
  char* svptr;
  char  req_cpy[BUFSIZE];
  strcpy(req_cpy, req);
  char* host_name = parse_host(req_cpy, &svptr);
  if (host_name==NULL) {
    int n = write(sock_client, FOURZEROZERO, strlen(FOURZEROZERO));
    if (n<0) fail_thread("write");
    close_connections(NULL, sock_client, client_r);
  }

  // dns stuff
  int err;
  struct addrinfo* host;
  err = getaddrinfo(host_name, "80", NULL, &host);
  if (err) {
    fprintf(stderr, "%s : %s\n", host_name, gai_strerror(err));
    int n = write(sock_client, FOURZEROFOUR, strlen(FOURZEROFOUR));
    if (n<0) fail_thread("write");
    close_connections(host, sock_client, client_r);
  }
  
  // make server socket
  int sock_server = socket(host->ai_family, SOCK_STREAM, 0);
  if (sock_server < 0) fail_thread("socket");
  
  // connect to server socket
  if (connect(sock_server, host->ai_addr, host->ai_addrlen)) {
      
    // we've failed, give a 404 and exit
    int n = write(sock_client, FOURZEROFOUR, strlen(FOURZEROFOUR));
    if (n<0) fail_thread("write");
    close_connections(host, sock_client, client_r);
    
  } else {
    
    // write header
    FILE* server_w = fdopen(sock_server, "w");
    if (server_w == NULL) fail_thread("fdopen");
    setlinebuf(server_w);
    printf("----------\n%s----------\n", req);
    fputs(req, server_w);
    
    // get response
    int n = read(sock_server, buf, BUFSIZE);
    while (n>0) {
      n = write(sock_client, buf, n);
      if (n<0) fail_thread("write");
      n = read(sock_server, buf, BUFSIZE);
    }
    if (n<0) fail_thread("read");
    fclose(server_w);
  }
  close(sock_server);
  close_connections(host, sock_client, client_r);
}

int main(int argc, char* argv[]) {

  // check command line format
  if (argc != 2) {
    fprintf(stderr, "usage: %s port\n", argv[0]);
    exit(1);
  }

  // sometimes pipes break, don't exit
  signal(SIGPIPE, broken_pipe_handler);
  siginterrupt(SIGINT, 1);

  // make passive socket at port
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
  if (serv_sock < 0) fail("socket");
  if (bind(serv_sock, host->ai_addr, host->ai_addrlen)) fail("bind");
  if (listen(serv_sock, 5)) fail("listen");
  freeaddrinfo(host);

  // now we wait
  printf("\e[1;34mWaiting for connections on port %s\n\e[0m", argv[1]);
  int*      new_sock;
  pthread_t new_thread;
  while (1) {
    new_sock  = (int*) malloc(sizeof(int));
    *new_sock = accept(serv_sock, NULL, NULL);
    if (*new_sock < 0) fail("accept");
    printf("\e[1;34mAccepted new connection on fdesc %d\n\e[0m", *new_sock);
    pthread_create(&new_thread, NULL, serve_client, new_sock);
    pthread_detach(new_thread);
  }
}
