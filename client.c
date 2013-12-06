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

#define BUFSIZE 4096  // ALL of the memory

// TODO: SIGPIPE handler

void fail(const char* str) {
  perror(str);
  exit(1);
}

// return domain for opening socket
char* parse_host(char* buffer) {
  char* ret = (char*) malloc(BUFSIZE*sizeof(char)); // for testing
  strtok(buffer, "/");
  strcpy(ret, strtok(NULL, "/"));
  return ret;
}

// make sure connection is closed
char* gen_req(char* buffer) {
  char* ret  = (char*) malloc(BUFSIZE*sizeof(char)); // for testing
  /*
  char* pars = (char*) malloc(BUFSIZE*sizeof(char));
  pars = strtok(buffer, " \n");
  while (pars != NULL) {
    strcat(ret, pars);
    if (strcmp(pars, "Connection:")==0 || strcmp(pars, "Proxy-Connection:")==0) {
      pars = strtok(NULL, " \n");
      strcat(ret, "close\r");
    }
    pars = strtok(NULL, " \n");
    strcat(ret, " ");
  }
  free(pars);
  */
  strcpy(ret, buffer);
  //strcpy(ret, "GET /kjdf/ HTTP/1.1\r\nHost: nathanbossart.com\r\n\r\n");
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
  
  fprintf(stderr, buffer);
  
  char* req = (char*) malloc(BUFSIZE*sizeof(char));
  strcpy(req, buffer);
  char* host2 = parse_host(buffer);
  //char* req   = gen_req(buffer);
  fprintf(stderr, "---REQ\n");
  fprintf(stderr, req);
  fprintf(stderr, "\n--ENDREQ\n");
  fprintf(stderr, "---HOST\n");
  fprintf(stderr, host2);
  fprintf(stderr, "\n---ENDREQ\n");
  memset(buffer, 0, sizeof(buffer));

  int err;
  struct addrinfo* host;
  err = getaddrinfo(host2, "80", NULL, &host);
  if (err) {
    fprintf(stderr, "%s : %s\n", host2, gai_strerror(err));
    exit(err);
  }

  printf("a1\n");
  
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
    
    FILE* serv_stream    = fdopen(sock2, "w");
    FILE* serv_stream_rd = fdopen(sock2, "r");
    if (serv_stream == NULL || serv_stream_rd==NULL) {
      fail("fdopen");
    }
    setlinebuf(serv_stream);
    setlinebuf(serv_stream_rd);
    
    // write header
    int n = write(sock2, req, strlen(req));
    if (n < 0) {
      perror("write");
      exit(1);
    }
    //fputs(req, serv_stream);

    printf("3\n");

    // TODO: this doesn't work!
    
    char* buf2 = (char*) malloc(BUFSIZE*sizeof(char));
    memset(buf2, 0, sizeof(buf2));
    printf("4\n");
    while (read(sock2, buf2, strlen(buf2)) >= 0) {
      if (write(sock, buf2, strlen(buf2)) < 0) {
	printf("6\n");
	break;
      }
      printf(buf2);
    }
    free(buf2);
    
    /*
    char buf2[BUFSIZE];
    memset(buf2, 0, sizeof(buf2));
    while (fgets(buf2, BUFSIZE, serv_stream_rd) != NULL) {

      printf("4\n");

      if (fputs(buf2, myclient_wr) == EOF) {
        break;
      }
    }
    */
    
    
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
