// Replace PUT_USERID_HERE with your actual BYU CS user id, which you can find
// by running `id -u` on a CS lab machine.
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>

#define MAX_RESPONSE_SIZE 256

#define USERID 1823700680

#include <stdio.h>

#include "../include/sockhelper.h"

int verbose = 0;

struct response_values {
  unsigned char chunk_length;
  unsigned char null_terminated_chunk_length;
  char chunk[MAX_RESPONSE_SIZE - 8];
  unsigned char op_code;
  unsigned short op_param;
  unsigned int nonce;
};


void print_bytes(unsigned char *bytes, int byteslen);
int extract_response_values(const unsigned char *response, struct response_values *values);
void print_response_values (const struct response_values * values);

int main(int argc, char *argv[]) {
  char *server = argv[1];
  char *port = argv[2];
  char *level = argv[3];
  char *seed = argv[4];

  printf("server: %s | port: %s | level: %s | seed: %s\n", server, port, level,
         seed);

  int port_num = atoi(port);
  int level_num = atoi(level);
  unsigned short seed_num = atoi(seed);
  seed_num = ntohs(seed_num);
  unsigned int user_id = ntohl(USERID);
  unsigned char init_request_body[8];
  memset(init_request_body, 0, 8);

  memcpy(&init_request_body[1], &level_num, sizeof(int));
  memcpy(&init_request_body[2], &user_id, sizeof(unsigned int));
  memcpy(&init_request_body[6], &seed_num, sizeof(unsigned short));

  // print_bytes(init_request_body, 8);

  // Socket setup
  struct addrinfo hints;
  memset(&hints, 0, sizeof(struct addrinfo));

  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_DGRAM;

  struct addrinfo *info;

  int s = getaddrinfo(server, port, &hints, &info);
  int sock = socket(info->ai_family, info->ai_socktype, 0);
  struct sockaddr_storage remote_addr_ss;
  struct sockaddr *remote_addr = (struct sockaddr *)&remote_addr_ss;

  // Set up remote addr reference
  printf("Setting up remote\n");
  memcpy(remote_addr, info->ai_addr, sizeof(struct sockaddr_storage));
  char remote_ip[INET6_ADDRSTRLEN];
  unsigned short remote_port = 0;

  printf("populating addr\n");
  // populate_sockaddr(remote_addr, hints.ai_family, NULL, remote_port );
  parse_sockaddr(remote_addr, remote_ip, &remote_port);

  // Set up local addr reference
  printf("Setting up local\n");

  struct sockaddr_storage local_addr_ss;
  struct sockaddr *local_addr = (struct sockaddr *)&local_addr_ss;

  char local_ip[INET6_ADDRSTRLEN];
  unsigned short local_port;

  socklen_t addr_len = sizeof(struct sockaddr_storage);
  s = getsockname(sock, local_addr, &addr_len);
  parse_sockaddr(local_addr, local_ip, &local_port);

  printf("addr info: ip: %s | port: %d\n", remote_ip, remote_port);
  // Server Test

  printf("Setting up server test\n");
  ssize_t n_written =
      sendto(sock, init_request_body, 8, 0, remote_addr, addr_len);

  unsigned char response_buf[MAX_RESPONSE_SIZE];
  ssize_t n_received = recvfrom(sock, response_buf, MAX_RESPONSE_SIZE, 0,
                                remote_addr, &addr_len);

  printf("Response size: %zu\n", n_received);
  print_bytes(response_buf, n_received);

  struct response_values *values = malloc(sizeof(struct response_values));
  
  printf("extracting\n");
  int err = extract_response_values(response_buf, values);
  printf("printing\n");

  print_response_values(values);
  // ******* Levels ********
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

int extract_response_values(const unsigned char * response, struct response_values *values) {

  values->chunk_length = response[0];
  if (values->chunk_length > 127) {
    return -1;
  }

  memcpy(&values->chunk, response + 1, (size_t)values->chunk_length);
  values->chunk[values->chunk_length] = '\0';
  
  memcpy(&values->op_code, response + values->chunk_length + 1, 1);

  unsigned short network_op_param;
  memcpy(&network_op_param, response + values->chunk_length + 2, 2);
  values->op_param = ntohs(network_op_param);

  unsigned int network_nonce;
  memcpy(&network_nonce, response + values->chunk_length + 4, 4);
  values->nonce = ntohl(network_nonce);

  values->null_terminated_chunk_length = values->chunk_length + 1;
  return 0;
}


void print_response_values (const struct response_values * values) {
  printf("chunk length: %d\n", values->chunk_length);
  printf("null-terminated chunk length: %d\n", values->null_terminated_chunk_length);
  printf("chunk content: %s\n", values->chunk);
  printf("op code: %x\n", values->op_code);
  printf("op param: %x\n", values->op_param);
  printf("nonce: %u\n", values->nonce);
}
