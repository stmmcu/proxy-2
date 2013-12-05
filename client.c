/*
 * client.c
 *   The client portion of an http proxy server.
 *   usage: client port
 *
 *   Nathan Bossart & Joe Mayer
 *   Dec 4, 2013
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

#define BUFSIZE 512

void fail(const char* str) {
  perror(str);
  exit(1);
}

char* parse_url(char buf[]) {
  char* ret = (char*) malloc(30*sizeof(char)); // for testing
  strcpy(ret, "http://www.nathanbossart.com");
  return ret;
}

char* parse_host(char* url) {
  char* ret = (char*) malloc(20*sizeof(char)); // for testing
  strcpy(ret, "www.nathanbossart.com");
  return ret;
}

char* parse_path(char* url) {
  char* ret = (char*) malloc(20*sizeof(char)); // for testing
  strcpy(ret, "/kjdf");
  return ret;
}

char* gen_req(char* buf, char* host, char* path) {
  char* ret = (char*) malloc(255*sizeof(char)); // for testing
  strcpy(ret, "GET /kjdf/ HTTP/1.1\r\nHost: nathanbossart.com\r\n\r\n");
  return ret;
}

void* serve_client(void* v) {
  int sock = *(int*) v;
  free(v);

  FILE* myclient = fdopen(sock, "r");
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

  char* url   = parse_url(buffer);
  char* host2 = parse_host(url);
  char* path  = parse_path(url);
  char* req   = gen_req(buffer, host2, path);
  memset(buffer, 0, sizeof(buffer));

  int err;
  struct addrinfo* host;
  err = getaddrinfo(host2, "80", NULL, &host);
  if (err) {
    fprintf(stderr, "%s : %s\n", host2, gai_strerror(err));
    exit(err);
  }
  
  int sock2 = socket(host->ai_family, SOCK_STREAM, 0);
  if (sock2 < 0  ||  connect(sock2, host->ai_addr, host->ai_addrlen)) {
  
    printf("1\n");
  
    int n = write(sock, four04, strlen(four04));
    if (n < 0) {
      perror("write");
      exit(1);
    }
    
  } else {

    printf("2\n");
    
    FILE* serv_stream = fdopen(sock2, "w");
    FILE* serv_stream_rd = fdopen(sock2, "r");
    if (serv_stream == NULL || serv_stream_rd==NULL) {
      fail("fdopen");
    }
    setlinebuf(serv_stream);
    setlinebuf(serv_stream_rd);
    
    // write header
    //write(sock2, req, sizeof(req));
    fputs(req, serv_stream);

    printf("3\n");

    /*
    char buf2[BUFSIZE];
    while (read(sock2, buf2, BUFSIZE) != NULL) {
      write(sock, buf2, sizeof(buf2));
    }
    */
    char buf2[BUFSIZE];
    while (fgets(buf2, BUFSIZE, serv_stream_rd) != NULL) {

      printf("4\n");

      if (fputs(buf2, myclient_wr) == EOF) {
	break;
      }
    }
    
    fclose(serv_stream);
    fclose(serv_stream_rd);
    
  }

  freeaddrinfo(host);
  free(url);
  free(host2);
  free(path);
  free(req);

  printf("Connection closed on fdesc %d\n", sock);

  fclose(myclient);
  fclose(myclient_wr);
  pthread_exit(0);
}

int main(int argc, char* argv[]) {

  int err;
  struct addrinfo* host;
  struct addrinfo hints;
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
