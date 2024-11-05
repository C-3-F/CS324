// Replace PUT_USERID_HERE with your actual BYU CS user id, which you can find
// by running `id -u` on a CS lab machine.
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#define USERID 1823700680

#include <stdio.h>

#include "sockhelper.h"

int verbose = 0;

void print_bytes(unsigned char *bytes, int byteslen);

int main(int argc, char *argv[]) {
	char *server = argv[1];
	char *port = argv[2];
	char *level = argv[3];
	char *seed = argv[4];

	printf("server: %s | port: %s | level: %s | seed: %s\n", server, port, level,  seed);

	int port_num = atoi(port);
	int level_num = atoi(level);
	unsigned short seed_num = atoi(seed);
	seed_num = ntohs(seed_num);
	level_num = ntohs(level_num);
	unsigned int user_id = ntohl(USERID);
	unsigned char init_request_body[8];
	memset(init_request_body, 0, 8);
	
	memcpy(&init_request_body[1], &level_num, sizeof(int));
	memcpy(&init_request_body[2], &user_id, sizeof(unsigned int));
	memcpy(&init_request_body[6], &seed_num, sizeof(unsigned short));

	print_bytes(init_request_body, 8);


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
