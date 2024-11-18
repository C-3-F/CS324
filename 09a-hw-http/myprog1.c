
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
int main(int argc, char* argv[]) {
	int content_length = atoi(getenv("CONTENT_LENGTH"));
	char *query_string = getenv("QUERY_STRING");


	// printf("TEST--Content Length ENV: %d\n", content_length);
	// printf("TEST--Query string ENV: %s\n", query_string);

	char buf[content_length + 1];

	ssize_t bytes_read = read(STDIN_FILENO, buf, content_length);
	// printf("TEST--Bytes Read: %zd\n", bytes_read);
	buf[bytes_read] = '\0';

	char body[1024];
	int body_size = 0;
	body_size += sprintf(body + body_size, "Hello CS324\n");
	body_size += sprintf(body + body_size, "Query string: %s\n", query_string);
	body_size += sprintf(body + body_size, "Request body: %s\n", buf);
	
	fprintf(stdout, "Content-Type: text/plain\r\n");
	fprintf(stdout, "Content-Length: %lu\r\n", strlen(body));
	fprintf(stdout, "\r\n");


	// fprintf(stdout, "{ [%lu bytes data]\n", strlen(body));
	// printf("TEST--body size: %d\n", output_size);
	fprintf(stdout, body, body_size);
	return 0;
}


