#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char *argv[]) {
	int pid;

	printf("Starting program; process has pid %d\n", getpid());

	FILE *file = fopen("fork-output.txt","w");

	fprintf(file, "BEFORE FORK (%d)", fileno(file));

	fflush(file);

	int fd[2];
	pipe(fd);
	if ((pid = fork()) < 0) {
		fprintf(stderr, "Could not fork()");
		exit(1);
	}

	/* BEGIN SECTION A */

	printf("Section A;  pid %d\n", getpid());
//	sleep(5);

	/* END SECTION A */
	if (pid == 0) {
		/* BEGIN SECTION B */

//		printf("Section B\n");
//		sleep(10);	
//		write(fd[1], "hello from Section B\n", 21);
//		close(fd[1]);

		char *newenviron[] = { NULL };

		printf("Program \"%s\" has pid %d. Sleeping.\n", argv[0], getpid());
//		sleep(30);

		if (argc <= 1) {
			printf("No program to exec.  Exiting...\n");
			exit(0);
		}

		printf("Running exec of \"%s\"\n", argv[1]);
		execve(argv[1], &argv[1], newenviron);
		printf("End of program \"%s\".\n", argv[0]);


		exit(0);

		/* END SECTION B */
	} else {
		/* BEGIN SECTION C */

		printf("Section C\n");

		sleep(10);
		close(fd[1]);
		
		char input[30];
		char input2[30];
		int bytes = read(fd[0], input, 21);
		printf("Read (%d) bytes\n", bytes);
		input[22] = '\0';
		printf(input);


		bytes = read(fd[0], input2, 21);
		printf("Read (%d) bytes\n", bytes);
		input2[22] = '\0';
		printf(input2);
		exit(0);

		/* END SECTION C */
	}
	/* BEGIN SECTION D */

	printf("Section D\n");
//	sleep(30);

	/* END SECTION D */
}

