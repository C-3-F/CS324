#include <stdio.h>
#include <string.h>

#include "sockhelper.h"

/* Recommended max object size */
#define MAX_OBJECT_SIZE 102400

static const char *user_agent_hdr =
    "User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10.15; rv:97.0) "
    "Gecko/20100101 Firefox/97.0";

int complete_request_received(char *);
void parse_request(char *, char *, char *, char *, char *);
void test_parser();
void print_bytes(unsigned char *, int);

int main(int argc, char *argv[]) {
  test_parser();
  printf("%s\n", user_agent_hdr);
  return 0;
}

int complete_request_received(char *request) {

  char *end_header = strstr(request, "\r\n\r\n");
  return end_header == NULL ? 0 : 1;
}

void parse_request(char *request, char *method, char *hostname, char *port,
                   char *path) {

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
  // Move beyond the first space, so beginning_of_thing now points to the start
  // of the URL.
  beginning_of_thing = end_of_thing + 1;
  // Continue this pattern to get the URL, and then extract the components of
  // the URL the same way.

  char url[256];

  // Second space to get URL
  end_of_thing = strstr(beginning_of_thing, " ");
  strncpy(url, beginning_of_thing, end_of_thing - beginning_of_thing);

  // Parse parts of the URL

  beginning_of_thing = strstr(url, "://") + 3;
  end_of_thing = strstr(beginning_of_thing, ":");
  if (end_of_thing != NULL) {
    // Copy the hostname
    strncpy(hostname, beginning_of_thing, end_of_thing - beginning_of_thing);

    // Get the port
    beginning_of_thing = end_of_thing + 1;
    end_of_thing = strstr(beginning_of_thing, "/");
    strncpy(port, beginning_of_thing, end_of_thing - beginning_of_thing);
  } else {
    end_of_thing = strstr(beginning_of_thing, "/");
    strncpy(hostname, beginning_of_thing, end_of_thing - beginning_of_thing);
    strcpy(port, "80");
  }

  // Copy the path -- Which is just the rest of the URL
  strcpy(path, end_of_thing);

	char *headers = strstr(request, "\r\n");

}

void test_parser() {
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

  for (i = 0; reqs[i] != NULL; i++) {
    printf("Testing %s\n", reqs[i]);
    if (complete_request_received(reqs[i])) {
      printf("REQUEST COMPLETE\n");
      parse_request(reqs[i], method, hostname, port, path);
      printf("METHOD: %s\n", method);
      printf("HOSTNAME: %s\n", hostname);
      printf("PORT: %s\n", port);
      printf("PATH: %s\n", path);
      printf("\n\n----------------------------------\n\n");
    } else {
      printf("REQUEST INCOMPLETE\n");
    }
  }
}

void print_bytes(unsigned char *bytes, int byteslen) {
  int i, j, byteslen_adjusted;

  if (byteslen % 8) {
    byteslen_adjusted = ((byteslen / 8) + 1) * 8;
  } else {
    byteslen_adjusted = byteslen;
  }
  for (i = 0; i < byteslen_adjusted + 1; i++) {
    if (!(i % 8)) {
      if (i > 0) {
        for (j = i - 8; j < i; j++) {
          if (j >= byteslen_adjusted) {
            printf("  ");
          } else if (j >= byteslen) {
            printf("  ");
          } else if (bytes[j] >= '!' && bytes[j] <= '~') {
            printf(" %c", bytes[j]);
          } else {
            printf(" .");
          }
        }
      }
      if (i < byteslen_adjusted) {
        printf("\n%02X: ", i);
      }
    } else if (!(i % 4)) {
      printf(" ");
    }
    if (i >= byteslen_adjusted) {
      continue;
    } else if (i >= byteslen) {
      printf("   ");
    } else {
      printf("%02X ", bytes[i]);
    }
  }
  printf("\n");
  fflush(stdout);
}
