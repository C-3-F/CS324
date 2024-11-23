#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <pthread.h>

#include "sockhelper.h"
#include "sbuf.h"

/* Recommended max object size */
#define MAX_OBJECT_SIZE 102400
#define NUM_THREADS 8
#define SBUFSIZE 5

static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10.15; rv:97.0) "
    "Gecko/20100101 Firefox/97.0";

int complete_request_received(char *);
void parse_request(char *, char *, char *, char *, char *);
void test_parser();
void print_bytes(unsigned char *, int);
int open_sfd(int port);
void *handle_client(void *vargp);

sbuf_t sbuf;

int main(int argc, char *argv[])
{
  // test_parser();
  printf("%s\n", user_agent_hdr);

  int sfd = open_sfd(atoi(argv[1]));

  sbuf_init(&sbuf, SBUFSIZE);
  pthread_t tid;

  for (int i = 0; i < NUM_THREADS; i++)
  {
    pthread_create(&tid, NULL, handle_client, NULL);
  }

  while (1)
  {
    struct sockaddr_storage remote_addr_ss;
    struct sockaddr *remote_addr = (struct sockaddr *)&remote_addr_ss;
    socklen_t addr_len = sizeof(struct sockaddr_storage);
    printf("accepting connections\n");
    // int *connfdptr = malloc(sizeof(int));
    // *connfdptr = accept(sfd, remote_addr, &addr_len);
    int connfd = accept(sfd, remote_addr, &addr_len);

    sbuf_insert(&sbuf, connfd);
  }
  return 0;
}

int complete_request_received(char *request)
{

  char *end_header = strstr(request, "\r\n\r\n");
  return end_header == NULL ? 0 : 1;
}

void parse_request(char *request, char *method, char *hostname, char *port,
                   char *path)
{

  // The first thing to extract is the method, which is at the beginning of the
  // request, so we point beginning_of_thing to the start of req.
  char *beginning_of_thing = request;
  // Remember that strstr() relies on its first argument being a string--that
  // is, that it is null-terminated.
  char *end_of_thing = strstr(beginning_of_thing, " ");
  // At this point, end_of_thing is either NULL if no space is found or it
  // points to the space.  Because your code will only have to deal with
  // well-formed HTTP requests for this lab, you won't need to worry about
  // end_of_thing being NULL.  But later uses of strstr() might require a
  // conditional, such as when searching for a colon to determine whether or not
  // a port was specified.
  //
  // Copy the first n (end_of_thing - beginning_of_thing) bytes of
  // req/beginning_of_things to method.
  strncpy(method, beginning_of_thing, end_of_thing - beginning_of_thing);
  method[end_of_thing - beginning_of_thing] = '\0';
  // Move beyond the first space, so beginning_of_thing now points to the start
  // of the URL.
  beginning_of_thing = end_of_thing + 1;
  // Continue this pattern to get the URL, and then extract the components of
  // the URL the same way.

  char url[256];

  // Second space to get URL
  end_of_thing = strstr(beginning_of_thing, " ");
  strncpy(url, beginning_of_thing, end_of_thing - beginning_of_thing);
  url[end_of_thing - beginning_of_thing] = '\0';

  // Parse parts of the URL

  beginning_of_thing = strstr(url, "://") + 3;
  end_of_thing = strstr(beginning_of_thing, ":");
  if (end_of_thing != NULL)
  {
    // Copy the hostname
    strncpy(hostname, beginning_of_thing, end_of_thing - beginning_of_thing);
    hostname[end_of_thing - beginning_of_thing] = '\0';

    // Get the port
    beginning_of_thing = end_of_thing + 1;
    end_of_thing = strstr(beginning_of_thing, "/");
    strncpy(port, beginning_of_thing, end_of_thing - beginning_of_thing);
    port[end_of_thing - beginning_of_thing] = '\0';
  }
  else
  {
    end_of_thing = strstr(beginning_of_thing, "/");
    strncpy(hostname, beginning_of_thing, end_of_thing - beginning_of_thing);
    hostname[end_of_thing - beginning_of_thing] = '\0';
    strcpy(port, "80");
  }

  // Copy the path -- Which is just the rest of the URL
  strcpy(path, end_of_thing);
  path[strlen(path)] = '\0';

  // char *headers = strstr(request, "\r\n");
}

void test_parser()
{
  int i;
  char method[16], hostname[64], port[8], path[64];

  char *reqs[] = {
      "GET http://www.example.com/index.html HTTP/1.0\r\n"
      "Host: www.example.com\r\n"
      "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:68.0) Gecko/20100101 "
      "Firefox/68.0\r\n"
      "Accept-Language: en-US,en;q=0.5\r\n\r\n",

      "GET http://www.example.com:8080/index.html?foo=1&bar=2 HTTP/1.0\r\n"
      "Host: www.example.com:8080\r\n"
      "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:68.0) Gecko/20100101 "
      "Firefox/68.0\r\n"
      "Accept-Language: en-US,en;q=0.5\r\n\r\n",

      "GET http://localhost:1234/home.html HTTP/1.0\r\n"
      "Host: localhost:1234\r\n"
      "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:68.0) Gecko/20100101 "
      "Firefox/68.0\r\n"
      "Accept-Language: en-US,en;q=0.5\r\n\r\n",

      "GET http://www.example.com:8080/index.html HTTP/1.0\r\n",

      NULL};

  for (i = 0; reqs[i] != NULL; i++)
  {
    printf("Testing %s\n", reqs[i]);
    if (complete_request_received(reqs[i]))
    {
      printf("REQUEST COMPLETE\n");
      parse_request(reqs[i], method, hostname, port, path);
      printf("METHOD: %s\n", method);
      printf("HOSTNAME: %s\n", hostname);
      printf("PORT: %s\n", port);
      printf("PATH: %s\n", path);
      printf("\n\n----------------------------------\n\n");
    }
    else
    {
      printf("REQUEST INCOMPLETE\n");
    }
  }
}

int open_sfd(int port)
{
  const int ADDR_FAM = AF_INET;
  const int SOCK_TYPE = SOCK_STREAM;

  int sfd;
  if ((sfd = socket(ADDR_FAM, SOCK_TYPE, 0)) < 0)
  {
    perror("Error creating socket");
    exit(EXIT_FAILURE);
  }

  int optval = 1;
  setsockopt(sfd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));

  struct sockaddr_storage local_addr_ss;
  struct sockaddr *local_addr = (struct sockaddr *)&local_addr_ss;

  populate_sockaddr(local_addr, ADDR_FAM, NULL, port);
  if (bind(sfd, local_addr, sizeof(struct sockaddr_storage)) < 0)
  {
    perror("Could not bind");
    exit(EXIT_FAILURE);
  }

  listen(sfd, 100);

  return sfd;
}

int open_client_sfd(char *hostname, char *port)
{
  const int ADDR_FAM = AF_INET;
  const int SOCK_TYPE = SOCK_STREAM;
  int sfd;
  struct sockaddr_storage remote_addr_ss;
  struct sockaddr *remote_addr = (struct sockaddr *)&remote_addr_ss;

  struct addrinfo info;
  memset(&info, 0, sizeof(struct addrinfo));
  info.ai_family = ADDR_FAM;
  info.ai_socktype = SOCK_TYPE;

  struct addrinfo *remote_addr_info;
  getaddrinfo(hostname, port, &info, &remote_addr_info);

  struct addrinfo *rp;
  for (rp = remote_addr_info; rp != NULL; rp = rp->ai_next)
  {
    sfd = socket(rp->ai_family, rp->ai_socktype, 0);
    if (sfd < 0)
    {
      continue;
    }

    memcpy(remote_addr, rp->ai_addr, sizeof(struct sockaddr_storage));

    if (connect(sfd, remote_addr, sizeof(struct sockaddr_storage)) >= 0)
    {
      break;
    }
    close(sfd);
  }
  if (rp == NULL)
  {
    perror("Could not connect");
    exit(EXIT_FAILURE);
  }

  return sfd;
}

void *handle_client(void *vargp)
{

  // int conn = *((int *)vargp);
  // pthread_detach(pthread_self());
  // free(vargp);

  pthread_detach(pthread_self());
  while (1)
  {
    int conn = sbuf_remove(&sbuf);

    char buf[1024];
    ssize_t total_bytes = 0;

    while (1)
    {
      total_bytes += recv(conn, buf + total_bytes, sizeof(buf - total_bytes), 0);
      buf[total_bytes] = '\0';
      if (complete_request_received(buf))
      {
        break;
      }
    }

    print_bytes((unsigned char *)buf, total_bytes);

    char method[16], hostname[64], port[8], path[64];
    parse_request(buf, method, hostname, port, path);
    printf("Method: %s\n", method);
    printf("Hostname: %s\n", hostname);
    printf("Port: %s\n", port);
    printf("Path: %s\n", path);

    // Create new request

    char new_request[1024];
    int offset = 0;

    offset += sprintf(new_request, "%s %s HTTP/1.0\r\n", method, path);
    if (strcmp(port, "80") == 0)
    {
      offset += sprintf(new_request + offset, "Host: %s\r\n", hostname);
    }
    else
    {
      offset += sprintf(new_request + offset, "Host: %s:%s\r\n", hostname, port);
    }
    offset += sprintf(new_request + offset, "%s\r\n", user_agent_hdr);
    offset += sprintf(new_request + offset, "Connection: close\r\n");
    offset += sprintf(new_request + offset, "Proxy-Connection: close\r\n");
    offset += sprintf(new_request + offset, "\r\n");

    printf("new request print: \n");
    print_bytes((unsigned char *)new_request, strlen(new_request));

    // printf("Connecting to server\n");
    int server_conn = open_client_sfd(hostname, port);

    // printf("Sending request to server\n");
    send(server_conn, new_request, strlen(new_request), 0);

    char response[16384];
    ssize_t total_bytes_response = 0;
    while (1)
    {
      int bytes_received = recv(server_conn, response + total_bytes_response, sizeof(response) - total_bytes_response, 0);
      total_bytes_response += bytes_received;
      if (bytes_received == 0)
      {
        response[total_bytes_response] = '\0';
        break;
      }
    }

    // printf("Response: \n");
    print_bytes((unsigned char *)response, total_bytes_response);

    send(conn, response, total_bytes_response, 0);

    close(conn);
  }
  // return NULL;
}

void print_bytes(unsigned char *bytes, int byteslen)
{
  int i, j, byteslen_adjusted;

  if (byteslen % 8)
  {
    byteslen_adjusted = ((byteslen / 8) + 1) * 8;
  }
  else
  {
    byteslen_adjusted = byteslen;
  }
  for (i = 0; i < byteslen_adjusted + 1; i++)
  {
    if (!(i % 8))
    {
      if (i > 0)
      {
        for (j = i - 8; j < i; j++)
        {
          if (j >= byteslen_adjusted)
          {
            printf("  ");
          }
          else if (j >= byteslen)
          {
            printf("  ");
          }
          else if (bytes[j] >= '!' && bytes[j] <= '~')
          {
            printf(" %c", bytes[j]);
          }
          else
          {
            printf(" .");
          }
        }
      }
      if (i < byteslen_adjusted)
      {
        printf("\n%02X: ", i);
      }
    }
    else if (!(i % 4))
    {
      printf(" ");
    }
    if (i >= byteslen_adjusted)
    {
      continue;
    }
    else if (i >= byteslen)
    {
      printf("   ");
    }
    else
    {
      printf("%02X ", bytes[i]);
    }
  }
  printf("\n");
  fflush(stdout);
}
