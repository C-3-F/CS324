#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "sockhelper.h"

#define BUF_SIZE 500

int main(int argc, char *argv[]) {

  /* Check usage */
  if (argc < 3 || ((strcmp(argv[1], "-4") == 0 || strcmp(argv[1], "-6") == 0) &&
                   argc < 4)) {
    fprintf(stderr, "Usage: %s [ -4 | -6 ] host port msg...\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  /* Use only IPv4 (AF_INET) if -4 is specified;
   * Use only IPv6 (AF_INET6) if -6 is specified;
   * Try both if neither is specified. */
  int af = AF_UNSPEC;
  int hostindex;
  if (strcmp(argv[1], "-4") == 0 || strcmp(argv[1], "-6") == 0) {
    if (strcmp(argv[1], "-6") == 0) {
      af = AF_INET6;
    } else { // (strcmp(argv[1], "-4") == 0) {
      af = AF_INET;
    }
    hostindex = 2;
  } else {
    hostindex = 1;
  }

  struct addrinfo hints;
  // Initialize everything to 0
  memset(&hints, 0, sizeof(struct addrinfo));
  // Set the address family to either AF_INET (IPv4-only), AF_INET6
  // (IPv6-only), or AF_UNSPEC (either IPv4 or IPv6), depending on what
  // was passed in on the command line.
  hints.ai_family = af;
  // Use type SOCK_DGRAM (UDP)
  hints.ai_socktype = SOCK_STREAM;

  /* SECTION A - pre-socket setup; getaddrinfo() */

  struct addrinfo *result;
  int s;
  s = getaddrinfo(argv[hostindex], argv[hostindex + 1], &hints, &result);
  if (s != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
    exit(EXIT_FAILURE);
  }

  /* SECTION B - pre-socket setup; getaddrinfo() */

  int sfd;
  int addr_fam;
  socklen_t addr_len;

  // Address information is stored in local_addr_ss, which is of type
  // struct addr_storage.  However, all functions require a parameter of
  // type struct sockaddr *.  Instead of type-casting everywhere, we
  // declare local_addr, which is of type struct sockaddr *, point it to
  // the address of local_addr_ss, and use local_addr everywhere.
  struct sockaddr_storage local_addr_ss;
  struct sockaddr *local_addr = (struct sockaddr *)&local_addr_ss;
  char local_ip[INET6_ADDRSTRLEN];
  unsigned short local_port;

  // See notes above for local_addr_ss and local_addr_ss.
  struct sockaddr_storage remote_addr_ss;
  struct sockaddr *remote_addr = (struct sockaddr *)&remote_addr_ss;
  char remote_ip[INET6_ADDRSTRLEN];
  unsigned short remote_port;

  struct addrinfo *rp;
  for (rp = result; rp != NULL; rp = rp->ai_next) {
    // Loop through every entry in the linked list populated by
    // getaddrinfo().   If socket(2) (or connect(2)) fails, we
    // close the socket and try the next address.
    //
    // For each iteration of the loop, rp points to an instance of
    // struct * addrinfo that contains the following members:
    //
    // ai_family: The address family for the given address. This is
    //         either AF_INET (IPv4) or AF_INET6 (IPv6).
    // ai_socktype: The type of socket.  This is either SOCK_DGRAM
    //         or SOCK_STREAM.
    // ai_addrlen: The length of the structure used to hold the
    //         address (different for AF_INET and AF_INET6).
    // ai_addr: The struct sockaddr_storage that holds the IPv4 or
    //         IPv6 address and port.

    sfd = socket(rp->ai_family, rp->ai_socktype, 0);
    if (sfd < 0) {
      // error creating the socket
      continue;
    }

    addr_fam = rp->ai_family;
    addr_len = rp->ai_addrlen;
    // Copy the value of rp->ai_addr to the struct sockaddr_storage
    // pointed to by remote_addr.
    memcpy(remote_addr, rp->ai_addr, sizeof(struct sockaddr_storage));

    // Extract the remote IP address and port from remote_addr
    // using parse_sockaddr().  parse_sockaddr() is defined in
    // ../code/sockhelper.c.
    parse_sockaddr(remote_addr, remote_ip, &remote_port);
    fprintf(stderr, "Connecting to %s:%d (addr family: %d)\n", remote_ip,
            remote_port, addr_fam);

    // If connect() succeeds, then break out of the loop; we will
    // use the current address as our remote address.
    if (connect(sfd, remote_addr, addr_len) >= 0)
      break;

    close(sfd);
  }

  // connect() did not succeed with any address returned by
  // getaddrinfo().
  if (rp == NULL) {
    fprintf(stderr, "Could not connect\n");
    exit(EXIT_FAILURE);
  }

  // Free the member associated with the linked list populated by
  // getaddrinfo()
  freeaddrinfo(result);

  // NOTE: addrlen needs to be initialized before every call to
  // getsockname().  See the man page for getsockname().
  addr_len = sizeof(struct sockaddr_storage);
  s = getsockname(sfd, local_addr, &addr_len);
  // Extract the IP address and port from remote_addr using
  // parse_sockaddr().  parse_sockaddr() is defined in
  // ../code/sockhelper.c.
  parse_sockaddr(local_addr, local_ip, &local_port);
  fprintf(stderr, "Local socket info: %s:%d (addr family: %d)\n", local_ip,
          local_port, addr_fam);

  /* SECTION C - interact with server; send, receive, print messages */

  int MAX_BYTES = 4096;
  int CHUNK_SIZE = 512;
  unsigned char buf[4096];
  size_t bytes_read;

  size_t total_bytes_read = 0;

  while (total_bytes_read < MAX_BYTES) {

    size_t bytes_to_read = (MAX_BYTES - total_bytes_read < CHUNK_SIZE)
                                ? MAX_BYTES - total_bytes_read
                                : CHUNK_SIZE;

    bytes_read = read(STDIN_FILENO, buf + total_bytes_read, bytes_to_read);

    if (bytes_read < 0) {
      perror("read error\n");
    }

    if (bytes_read == 0) { // EOF
      break;
    }
    total_bytes_read += bytes_read;
  }
  fprintf(stderr, "read %zu bytes\n", total_bytes_read);

  size_t total_bytes_wrote = 0;
  size_t bytes_wrote;

  while (total_bytes_wrote <= total_bytes_read) {

    size_t bytes_to_write = (total_bytes_read - total_bytes_wrote < CHUNK_SIZE)
                                ? total_bytes_read - total_bytes_wrote
                                : CHUNK_SIZE;

    bytes_wrote = send(sfd, buf + total_bytes_wrote, bytes_to_write, 0);
    
    if (bytes_wrote < 0) {
      perror("read error\n");
    }

    if (bytes_wrote == 0) { // EOF
      break;
    }
    total_bytes_wrote += bytes_wrote;
  }
  fprintf(stderr, "wrote %zu bytes\n", total_bytes_wrote);


  int MAX_RETURN_BYTES = 16384;
  unsigned char rec_return[MAX_RETURN_BYTES];
  total_bytes_read = 0;
  bytes_read = 0;

  while (total_bytes_read < MAX_RETURN_BYTES) {

    size_t bytes_to_read = (MAX_RETURN_BYTES - total_bytes_read < CHUNK_SIZE)
                                ? MAX_RETURN_BYTES - total_bytes_read
                                : CHUNK_SIZE;

    bytes_read = recv(sfd, rec_return + total_bytes_read, bytes_to_read, 0);

    if (bytes_read < 0) {
      perror("read error\n");
    }

    if (bytes_read == 0) { // EOF
      break;
    }
    total_bytes_read += bytes_read;
  }


  int status = write(STDOUT_FILENO, rec_return, total_bytes_read);
  if (status == -1)
  {
    perror("write error");
  }

  exit(EXIT_SUCCESS);
}
