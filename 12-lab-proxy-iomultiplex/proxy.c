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
#define MAX_REQUEST_SIZE 1024
#define MAX_RESPONSE_SIZE 16384
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
int open_client_sfd(char *hostname, char *port);
void handle_new_clients(int epoll_sfd, int listen_sfd);
void handle_client(int epoll_sfd, struct request_info *req_info);
void initialize_req_info_struct(struct request_info *req_info);

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
		int n = epoll_wait(efd, events, MAX_EVENTS, 1000);

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
				handle_client(efd, req_info);
			}
		}
	}

	free(listen_struct);
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
			// Set socket to non-blocking
			if (fcntl(sfd, F_SETFL, fcntl(sfd, F_GETFL, 0) | O_NONBLOCK) < 0)
			{
				fprintf(stderr, "error setting socket option\n");
				exit(1);
			}
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

void handle_client(int epoll_sfd, struct request_info *req_info)
{
	int client_proxy_sock = req_info->client_proxy_sock;
	int state = req_info->state;

	printf("Handling client descriptor %d\n", client_proxy_sock);
	printf("State: %d\n", state);

	if (state == READ_REQUEST)
	{
		while (1)
		{
			int bytes_read = recv(client_proxy_sock, req_info->read_buf + req_info->client_bytes_read, MAX_REQUEST_SIZE, 0);
			if (bytes_read < 0)
			{
				if (errno == EAGAIN || errno == EWOULDBLOCK)
				{
					break;
				}
				else
				{
					perror("Error reading from client");
					close(req_info->client_proxy_sock);
					free(req_info);
					exit(EXIT_FAILURE);
				}
			}
			else
			{
				req_info->client_bytes_read += bytes_read;
				if (complete_request_received(req_info->read_buf))
				{
					print_bytes((unsigned char *)req_info->read_buf, req_info->client_bytes_read);

					// Add null terminator to request
					req_info->read_buf[req_info->client_bytes_read] = '\0';

					// Parse client request to forward to server
					char method[16], hostname[64], port[8], path[64];
					parse_request(req_info->read_buf, method, hostname, port, path);

					printf("parsed request\n");
					printf("METHOD: %s\n", method);
					printf("HOSTNAME: %s\n", hostname);
					printf("PORT: %s\n", port);
					printf("PATH: %s\n", path);
					// Create new request

					char *new_request = req_info->write_buf;
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

					print_bytes((unsigned char *)new_request, offset);

					req_info->bytes_to_write_to_server = strlen(new_request);

					// Create socket to server
					int proxy_server_sock = open_client_sfd(hostname, port);
					req_info->proxy_server_sock = proxy_server_sock;

					// Deregister client from epoll
					if (epoll_ctl(epoll_sfd, EPOLL_CTL_DEL, client_proxy_sock, NULL) < 0)
					{
						perror("Error deregistering client from epoll");
						exit(EXIT_FAILURE);
					}

					// Register server with epoll
					struct epoll_event event;
					event.data.ptr = req_info;
					event.events = EPOLLOUT | EPOLLET;
					if (epoll_ctl(epoll_sfd, EPOLL_CTL_ADD, proxy_server_sock, &event) < 0)
					{
						perror("Error registering server with epoll");
						exit(EXIT_FAILURE);
					}

					// Set state to SEND_REQUEST
					req_info->state = SEND_REQUEST;
					break;
				}
			}
		}

		return;
	}
	else if (state == SEND_REQUEST)
	{
		while (1)
		{
			int len = req_info->bytes_to_write_to_server;
			int bytes_sent = send(req_info->proxy_server_sock, req_info->write_buf + req_info->server_bytes_wrote, len, 0);
			if (bytes_sent < 0)
			{
				if (errno == EAGAIN || errno == EWOULDBLOCK)
				{
					break;
				}
				else
				{
					perror("Error sending request to server");
					close(req_info->proxy_server_sock);
					close(req_info->client_proxy_sock);
					free(req_info);
					exit(EXIT_FAILURE);
				}
			}
			else
			{
				req_info->server_bytes_wrote += bytes_sent;
				if (req_info->server_bytes_wrote == req_info->bytes_to_write_to_server)
				{

					// Deregister server from epoll
					if (epoll_ctl(epoll_sfd, EPOLL_CTL_DEL, req_info->proxy_server_sock, NULL) < 0)
					{
						perror("Error deregistering server from epoll");
						exit(EXIT_FAILURE);
					}

					// Register client with epoll
					struct epoll_event event;
					event.data.ptr = req_info;
					event.events = EPOLLIN | EPOLLET;
					if (epoll_ctl(epoll_sfd, EPOLL_CTL_ADD, req_info->proxy_server_sock, &event) < 0)
					{
						perror("Error registering client with epoll");
						exit(EXIT_FAILURE);
					}

					// Set state to READ_RESPONSE
					req_info->state = READ_RESPONSE;
					break;
				}
			}
		}
	}
	else if (state == READ_RESPONSE)
	{
		while (1)
		{
			int bytes_read = recv(req_info->proxy_server_sock, req_info->write_buf + req_info->server_bytes_read, MAX_RESPONSE_SIZE, 0);
			if (bytes_read < 0)
			{
				if (errno == EAGAIN || errno == EWOULDBLOCK)
				{
					break;
				}
				else
				{
					perror("Error reading from server");
					close(req_info->proxy_server_sock);
					close(req_info->client_proxy_sock);
					free(req_info);
					exit(EXIT_FAILURE);
				}
			}
			else if (bytes_read == 0)
			{ // Connection closed
				printf("Connection closed. Printing response:\n");
				close(req_info->proxy_server_sock);

				// Null terminate the response
				req_info->write_buf[req_info->server_bytes_read] = '\0';

				print_bytes((unsigned char *)req_info->write_buf, req_info->server_bytes_read);

				// Register client proxy to write
				struct epoll_event event;
				event.data.ptr = req_info;
				event.events = EPOLLOUT | EPOLLET;

				if (epoll_ctl(epoll_sfd, EPOLL_CTL_ADD, req_info->client_proxy_sock, &event) < 0)
				{
					perror("Error registering client with epoll");
					exit(EXIT_FAILURE);
				}

				// Set state to SEND_RESPONSE
				req_info->state = SEND_RESPONSE;
				break;
			}
			else
			{
				req_info->server_bytes_read += bytes_read;
			}
		}
	}
	else if (state == SEND_RESPONSE)
	{
		while (1)
		{
			int bytes_sent = send(req_info->client_proxy_sock, req_info->write_buf + req_info->bytes_wrote_to_client, req_info->server_bytes_read - req_info->bytes_wrote_to_client, 0);
			if (bytes_sent < 0)
			{
				if (errno == EAGAIN || errno == EWOULDBLOCK)
				{
					break;
				}
				else
				{
					perror("Error sending response to client");
					close(req_info->client_proxy_sock);
					free(req_info);
					exit(EXIT_FAILURE);
				}
			}
			else
			{
				req_info->bytes_wrote_to_client += bytes_sent;
				if (req_info->bytes_wrote_to_client == req_info->server_bytes_read)
				{
					close(req_info->client_proxy_sock);
					free(req_info);
					break;
				}
			}
		}
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