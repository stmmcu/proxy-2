/*
 * client.c
 *   The client portion of an http proxy server.
 *   usage: client port
 *
 *   Nathan Bossart & Joe Mayer
 *   Dec 3, 2013
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

}

char* parse_host(char* url) {

}

char* parse_path(char* url) {

}

void* serve_client(void* v) {
  int sock = *(int*) v;
  free(v);

  FILE* myclient = fdopen(sock, "r");
  if (myclient == NULL) {
    perror("fdopen");
    pthread_exit(0);
  }
  setlinebuf(myclient);

  char buf[BUFSIZE];
  char* four04 = "HTTP/1.1 404 Not Found\nContent-Length: 219\nContent-Type: text/html\n\n<html><head><title>404 Not Found</title></head><body><h1>404 Not Found</h1><img src='http://www.climateaccess.org/sites/default/files/Obi%20wan.jpeg'></img><p>These aren't the bytes you're looking for.</p></body></html>";

  while(fgets(buf, BUFSIZE, myclient) != NULL) {
    fputs(buf, stdout);

    char* url  = parse_url(buf);
    char* host = parse_host(url);
    char* path = parse_path(url);

    inf err;
    struct addrinfo* host;
    err = getaddrinfo(host, "80", NULL, &host);
    if (err) {
      fprintf(stderr, "%s : %s\n", host, gai_strerror(err));
      exit(err);
    }

    int sock2 = socket(host->ai_family, SOCK_STREAM, 0);
    if (sock2 < 0  ||  connect(sock2, host->ai_addr, host->ai_addrlen)) {

      int n = write(sock, four04, strlen(four04));
      if (n < 0) {
	perror("write");
	exit(1);
      }

    } else {
      
      freeaddrinfo(host);
      
      FILE* serv_stream = fdopen(sock2, "w");
      if (serv_stream == NULL) {
	fail("fdopen");
      }
      setlinebuf(serv_stream);
      
      // TODO: change to write and read to socket instead of fgets and fputs
      char buf2[BUFSIZE];
      while (fgets(buf, BUFSIZE, stdin) != NULL) {
	if (fputs(buf, serv_stream) == EOF) {
	  break;
	}
      }
      
      fclose(serv_stream);
      
    }
  }

  printf("Connection closed on fdesc %d\n", sock);

  fclose(myclient);
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
