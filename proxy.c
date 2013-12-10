/*
 * proxy.c
 *   An http proxy server.
 *   usage: proxy port
 *
 *   Nathan Bossart & Joe Mayer
 *   Dec 10, 2013
 */

#define BUFSIZE 4096
#define FOURZEROFOUR "HTTP/1.1 404 Not Found\nContent-Length: 219\nContent-Type: text/html\n\n<html><head><title>404 Not Found</title></head><body><h1>404 Not Found</h1><img src='http://www.climateaccess.org/sites/default/files/Obi%20wan.jpeg'></img><p>These aren't the bytes you're looking for.</p></body></html>"

// handle broken pipes
void broken_pipe_handler(int signum) {}  // this function intentionally left blank

// nice failure function
void fail(const char* str) {perror(str); exit(1);}

// parse request for host
char* parse_host(char* buf, char** saveptr) {
  char* tok;
  tok = strtok_r(buf, " \r\n", saveptr);
  while (tok != NULL) {
    if (strcmp(ret, "Host:")==0) {
      tok = strtok_r(NULL, " \r\n", saveptr);
      return tok;
    }
    tok = strtok_r(NULL, " \r\n", saveptr);
  }
  return NULL;
}

// make sure connection closes
void close_req(char* buf) {
  // TODO: fill this in
}

// serve each connection
void* serve_client(void* v) {
  int sock = *(int*) v;
  free(v);

  FILE* client_r = fdopen(sock, "r");
  FILE* client_w = fdopen(sock, "w");
  if (client_r==NULL || client_w==NULL) {
    perror("fdopen");
    pthread_exit(0);
  }
  setlinebuf(client_r);
  setlinebuf(client_w);
  
  char buf[BUFSIZE];
  char req[BUFSIZE];
  while (fgets(buf, BUFSIZE, client_r) != NULL) {
    // TODO: check that req is big enough and resize otherwise!
    strcat(req, buf);
    if (strlen(buf) < 3) break;
  }

  char* svptr1, svptr2;
  char* host_name = parse_host(req, &svptr1);
  char* port_nmbr = parse_port(req, &svptr2);
  if (host_name==NULL || port_nmbr==NULL) pthread_exit(0);
  close_req(req);

  int err;
  struct addrinfo* host;
  err = getaddrinfo(host_name, port_nmbr, NULL, &host);
  if (err) {

    // TODO: this is where you left off

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
  if (bind(serv_socket, host->ai_addr, host->ai_addrlen)) fail("bind");
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
    printf("\e[1;34Accepted new connection on fdesc %d\n\e[0m", *new_sock);
    pthread_create(&new_thread, NULL, serve_client, new_sock);
    pthread_detach(new_thread);
  }
}
