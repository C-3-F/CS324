#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <errno.h>

#include "sockhelper.h"

/* Recommended max object size */
#define MAX_OBJECT_SIZE 102400
#define MAX_EVENTS 64
#define READ_REQUEST 1
#define SEND_REQUEST 2
#define READ_RESPONSE 3
#define SEND_RESPONSE 4

static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10.15; rv:97.0) Gecko/20100101 Firefox/97.0";

struct request_info
{
	int client_proxy_sock;
	int proxy_server_sock;
	int state;
	char read_buf[MAX_OBJECT_SIZE];
	char write_buf[MAX_OBJECT_SIZE];
	int client_bytes_read;
	int bytes_to_write_to_server;
	int server_bytes_wrote;
	int server_bytes_read;
	int bytes_wrote_to_client;
};

int complete_request_received(char *);
void parse_request(char *, char *, char *, char *, char *);
void test_parser();
void print_bytes(unsigned char *, int);
int open_sfd(int port);
void handle_new_clients(int epoll_sfd, int listen_sfd);
void initialize_req_info_struct(struct request_info *req_info)


int main(int argc, char *argv[])
{
	// test_parser();
	// printf("%s\n", user_agent_hdr);

	int port;
	if (argc < 2)
	{
		port = 8948;
		// fprintf(stderr, "Usage: %s <port>\n", argv[0]);
		// exit(EXIT_FAILURE);
	}
	else
	{
		port = atoi(argv[1]);
	}

	// Create epoll instance
	int efd;
	if ((efd = epoll_create1(0)) < 0)
	{
		perror("Error with epoll_create1");
		exit(EXIT_FAILURE);
	}

	// Open socket and bind to port
	int listen_sfd = open_sfd(port);

	// create request_info struct for listen_sfd
	struct request_info *listen_struct = malloc(sizeof(struct request_info));
	initialize_req_info_struct(listen_struct);
	listen_struct->client_proxy_sock = listen_sfd;

	// Register the listen socket with epoll for reading and edge-triggered monitoring (EPOLLIN | EPOLLET)
	struct epoll_event event, events[MAX_EVENTS];
	event.data.ptr = listen_struct;
	event.events = EPOLLIN | EPOLLET;
	if (epoll_ctl(efd, EPOLL_CTL_ADD, listen_sfd, &event) < 0)
	{
		perror("Error with epoll_ctl");
		exit(EXIT_FAILURE);
	}

	// Loop that calls epoll_wait and handles events
	while (1)
	{
		int n = epoll_wait(efd, events, MAX_EVENTS, -1);

		if (n < 0)
		{
			perror("Error with epoll_wait");
			exit(EXIT_FAILURE);
		}

		// Loop through events and handle
		for (int i = 0; i < n; i++)
		{
			struct request_info *req_info = (struct request_info *)events[i].data.ptr;
			if (req_info->client_proxy_sock == listen_sfd)
			{
				handle_new_clients(efd, listen_sfd);
			}
			else
			{
				// TODO: Handle client requests
			}
		}
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
		"User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:68.0) Gecko/20100101 Firefox/68.0\r\n"
		"Accept-Language: en-US,en;q=0.5\r\n\r\n",

		"GET http://www.example.com:8080/index.html?foo=1&bar=2 HTTP/1.0\r\n"
		"Host: www.example.com:8080\r\n"
		"User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:68.0) Gecko/20100101 Firefox/68.0\r\n"
		"Accept-Language: en-US,en;q=0.5\r\n\r\n",

		"GET http://localhost:1234/home.html HTTP/1.0\r\n"
		"Host: localhost:1234\r\n"
		"User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:68.0) Gecko/20100101 Firefox/68.0\r\n"
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
		}
		else
		{
			printf("REQUEST INCOMPLETE\n");
		}
	}
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

	if (fcntl(sfd, F_SETFL, fcntl(sfd, F_GETFL, 0) | O_NONBLOCK) < 0)
	{
		fprintf(stderr, "error setting socket option\n");
		exit(1);
	}

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

void handle_new_clients(int epoll_sfd, int listen_sfd)
{
	while (1)
	{
		int client_proxy_sock = accept(listen_sfd, NULL, NULL);

		if (client_proxy_sock < 0)
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK)
			{
				break;
			}
			else
			{
				perror("Error accepting client");
				exit(EXIT_FAILURE);
			}
		}

		// set client file descriptor nonblocking
		if (fcntl(client_proxy_sock, F_SETFL, fcntl(client_proxy_sock, F_GETFL, 0) | O_NONBLOCK) < 0)
		{
			fprintf(stderr, "error setting socket option\n");
			exit(1);
		}

		// allocate memory for the request_info struct
		struct request_info *req_info = malloc(sizeof(struct request_info));
		initialize_req_info_struct(req_info);
		req_info->client_proxy_sock = client_proxy_sock;
		req_info->state = READ_REQUEST;

		// add client to epoll
		struct epoll_event event;
		event.data.ptr = req_info;
		event.events = EPOLLIN | EPOLLET;
		if (epoll_ctl(epoll_sfd, EPOLL_CTL_ADD, client_proxy_sock, &event) < 0)
		{
			fprintf(stderr, "error adding event\n");
			exit(1);
		}
		printf("New client connected\n");
		printf("File Descriptor: %d\n", client_proxy_sock);
	}
}

void initialize_req_info_struct(struct request_info *req_info)
{
	req_info->client_proxy_sock = -1;
	req_info->proxy_server_sock = -1;
	req_info->state = READ_REQUEST;
	memset(req_info->read_buf, 0, MAX_OBJECT_SIZE);
	memset(req_info->write_buf, 0, MAX_OBJECT_SIZE);
	req_info->client_bytes_read = 0;
	req_info->bytes_to_write_to_server = 0;
	req_info->server_bytes_wrote = 0;
	req_info->server_bytes_read = 0;
	req_info->bytes_wrote_to_client = 0;
}