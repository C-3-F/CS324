/*
 * tsh - A tiny shell program with job control
 *
 * <Put your name and login ID here>
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

/* Misc manifest constants */
#define MAXLINE 1024 /* max line size */
#define MAXARGS 128	 /* max args on a command line */

/* Global variables */
extern char **environ;	 /* defined in libc */
char prompt[] = "tsh> "; /* command line prompt (DO NOT CHANGE) */
int verbose = 0;		 /* if true, print additional output */
char sbuf[MAXLINE];		 /* for composing sprintf messages */

/* Function prototypes */

/* Here are the functions that you will implement */
void eval(char *cmdline);
int builtin_cmd(char **argv);

/* Here are helper routines that we've provided for you */
int parseline(const char *cmdline, char **argv);
int parseargs(char **argv, int *cmds, int *stdin_redir, int *stdout_redir);
void performRedirection(int *stin_redirs, int *stdout_redirs, char **argv, int index);

void usage(void);
void unix_error(char *msg);
void app_error(char *msg);
typedef void handler_t(int);

/*
 * main - The shell's main routine
 */
int main(int argc, char **argv)
{
	int c;
	char cmdline[MAXLINE];
	int emit_prompt = 1; /* emit prompt (default) */

	/* Redirect stderr to stdout (so that driver will get all output
	 * on the pipe connected to stdout) */
	dup2(1, 2);

	/* Parse the command line */
	while ((c = getopt(argc, argv, "hvp")) >= 0)
	{
		switch (c)
		{
		case 'h': /* print help message */
			usage();
			break;
		case 'v': /* emit additional diagnostic info */
			verbose = 1;
			break;
		case 'p':			 /* don't print a prompt */
			emit_prompt = 0; /* handy for automatic testing */
			break;
		default:
			usage();
		}
	}

	/* Execute the shell's read/eval loop */
	while (1)
	{

		/* Read command line */
		if (emit_prompt)
		{
			printf("%s", prompt);
			fflush(stdout);
		}
		if ((fgets(cmdline, MAXLINE, stdin) == NULL) && ferror(stdin))
			app_error("fgets error");
		if (feof(stdin))
		{ /* End of file (ctrl-d) */
			fflush(stdout);
			exit(0);
		}

		/* Evaluate the command line */
		eval(cmdline);
		fflush(stdout);
		fflush(stdout);
	}

	exit(0); /* control never reaches here */
}

/*
 * eval - Evaluate the command line that the user has just typed in
 *
 * If the user has requested a built-in command (quit) then execute it
 * immediately. Otherwise, build a pipeline of commands and wait for all of
 * them to complete before returning.
 */
void eval(char *cmdline)
{
	char *argv[MAXLINE];
	parseline(cmdline, argv);
	int cmds[MAXARGS];
	int stin_redirs[MAXARGS];
	int stdout_redirs[MAXARGS];

	int numCommands = parseargs(argv, cmds, stin_redirs, stdout_redirs);
	int pids[numCommands];
	builtin_cmd(argv);
	int pgId = 0;

	int pipes[2] = {0, 0};	  // Pipe for writing
	int prevPipes[2] = {0,0}; // Previous pipe

	for (int i = 0; i < numCommands; i++) // Loop over every command
	{
		// fprintf(stderr, "starting command: %s\n", argv[cmds[i]]);


		if (i != numCommands - 1) // Not the last command
		{
			if (pipe(pipes) == -1)
			{
				fprintf(stderr, "failed to create pipe\n");
			}
		}

		int pid = fork();

		if (pid == 0) // Is child process
		{
			// fprintf(stderr, "in child process\n");

			performRedirection(stin_redirs, stdout_redirs, argv, i);
			// fprintf(stderr, "finish performRedirection\n");

			if (i != 0) // Not the first command so read from pipe
			{
				// fprintf(stderr, "dup to read side\n");

				dup2(prevPipes[0], STDIN_FILENO); // Map right side of the previous pipe (read stide) to stdin
				if (prevPipes[0] == 0 || prevPipes[1] == 0)
				{
					fprintf(stderr, "previous pipe reference lost\n");
					exit(-1);
				}
				// fprintf(stderr, "closing pervious pipes: %d,%d\n", prevPipes[0], prevPipes[1]);

				// close(prevPipes[0]); // Close read end in child
				// close(prevPipes[1]); // Close write end of previous pipe
			}
			if (i != numCommands - 1) // If not the last command, write to the next pipe
			{
				// fprintf(stderr, "dup to write side\n");

				dup2(pipes[1], STDOUT_FILENO); // Map left side of the pipe (write side) to stdout
											   // fprintf(stderr, "closing current pipes: %d,%d\n", pipes[0], pipes[1]);
			}
			if (pipes[0] != 0 && pipes[1] != 0)
			{
				close(pipes[1]);
				close(pipes[0]);
			}

			// fprintf(stderr, "executing cmd: %s\n", argv[cmds[i]]);
			int status = execv(argv[cmds[i]], &argv[cmds[i]]); // Execute
			// fprintf(stderr, "performed exec: %s\n", argv[cmds[i]]);
			if (status == -1)
			{
				fprintf(stderr, "execv failed");
				exit(EXIT_FAILURE);
			}
			// fprintf(stderr, "child exiting\n");
			exit(status);
		}
		else if (pid != 0)
		{
			// fprintf(stderr, "in parent job\n");
			pids[i] = pid;
			if (pgId == 0)
				pgId = pid;
			// fprintf(stderr, "pid: %d, pgid: %d\n", pid, pgId);
			setpgid(pid, pgId);
			if (i != 0) // If not first command, close the previous pipe
			{
				close(prevPipes[0]); // Close the previous read end
									 // close(prevPipes[1]); // Close the previous write end
			}
			if (i != numCommands - 1) // If not the last command, set prevPipe to current.
			{
				close(pipes[1]);
			}
			prevPipes[0] = pipes[0]; // Save the current read end for the next command
			prevPipes[1] = pipes[1]; // Save the current write end
		}
	}

	for (int i = 0; i < numCommands; i++)
	{
		waitpid(pids[i], NULL, 0);
	}

	return;
}

void performRedirection(int *stin_redirs, int *stdout_redirs, char **argv, int index)
{
	// fprintf(stderr, "performRedirection\n");
	if (stin_redirs[index] > 0)
	{ // check for std in redirects
		// fprintf(stderr, "performRedirection - stdin\n");

		char *inFilePath = argv[stin_redirs[index]];
		int inFileDescriptor = open(inFilePath, O_RDONLY);
		if (dup2(inFileDescriptor, STDIN_FILENO) == -1)
		{
			fprintf(stderr, "Failed to Duplicate FD to stdin\n");
		}
		close(inFileDescriptor);
	}
	if (stdout_redirs[index] > 0)
	{ // check for stdout redirects
		// fprintf(stderr, "performRedirection - stdout\n");

		char *outFilePath = argv[stdout_redirs[index]];
		int outFileDescriptor = open(outFilePath, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
		if (dup2(outFileDescriptor, STDOUT_FILENO) == -1)
		{
			fprintf(stderr, "Failed to Duplicate FD to stdout\n");
		}
		close(outFileDescriptor);
	}
}

/*
 * parseargs - Parse the arguments to identify pipelined commands
 *
 * Walk through each of the arguments to find each pipelined command.  If the
 * argument was | (pipe), then the next argument starts the new command on the
 * pipeline.  If the argument was < or >, then the next argument is the file
 * from/to which stdin or stdout should be redirected, respectively.  After it
 * runs, the arrays for cmds, stdin_redir, and stdout_redir all have the same
 * number of items---which is the number of commands in the pipeline.  The cmds
 * array is populated with the indexes of argv corresponding to the start of
 * each command sequence in the pipeline.  For each slot in cmds, there is a
 * corresponding slot in stdin_redir and stdout_redir.  If the slot has a -1,
 * then there is no redirection; if it is >= 0, then the value corresponds to
 * the index in argv that holds the filename associated with the redirection.
 *
 */
int parseargs(char **argv, int *cmds, int *stdin_redir, int *stdout_redir)
{
	int argindex = 0; /* the index of the current argument in the current cmd */
	int cmdindex = 0; /* the index of the current cmd */

	if (!argv[argindex])
	{
		return 0;
	}

	cmds[cmdindex] = argindex;
	stdin_redir[cmdindex] = -1;
	stdout_redir[cmdindex] = -1;
	argindex++;
	while (argv[argindex])
	{
		if (strcmp(argv[argindex], "<") == 0)
		{
			argv[argindex] = NULL;
			argindex++;
			if (!argv[argindex])
			{ /* if we have reached the end, then break */
				break;
			}
			stdin_redir[cmdindex] = argindex;
		}
		else if (strcmp(argv[argindex], ">") == 0)
		{
			argv[argindex] = NULL;
			argindex++;
			if (!argv[argindex])
			{ /* if we have reached the end, then break */
				break;
			}
			stdout_redir[cmdindex] = argindex;
		}
		else if (strcmp(argv[argindex], "|") == 0)
		{
			argv[argindex] = NULL;
			argindex++;
			if (!argv[argindex])
			{ /* if we have reached the end, then break */
				break;
			}
			cmdindex++;
			cmds[cmdindex] = argindex;
			stdin_redir[cmdindex] = -1;
			stdout_redir[cmdindex] = -1;
		}
		argindex++;
	}

	return cmdindex + 1;
}

/*
 * parseline - Parse the command line and build the argv array.
 *
 * Characters enclosed in single quotes are treated as a single
 * argument.  Return true if the user has requested a BG job, false if
 * the user has requested a FG job.
 */
int parseline(const char *cmdline, char **argv)
{
	static char array[MAXLINE]; /* holds local copy of command line */
	char *buf = array;			/* ptr that traverses command line */
	char *delim;				/* points to first space delimiter */
	int argc;					/* number of args */
	int bg;						/* background job? */

	strcpy(buf, cmdline);
	buf[strlen(buf) - 1] = ' ';	  /* replace trailing '\n' with space */
	while (*buf && (*buf == ' ')) /* ignore leading spaces */
		buf++;

	/* Build the argv list */
	argc = 0;
	if (*buf == '\'')
	{
		buf++;
		delim = strchr(buf, '\'');
	}
	else
	{
		delim = strchr(buf, ' ');
	}

	while (delim)
	{
		argv[argc++] = buf;
		*delim = '\0';
		buf = delim + 1;
		while (*buf && (*buf == ' ')) /* ignore spaces */
			buf++;

		if (*buf == '\'')
		{
			buf++;
			delim = strchr(buf, '\'');
		}
		else
		{
			delim = strchr(buf, ' ');
		}
	}
	argv[argc] = NULL;

	if (argc == 0) /* ignore blank line */
		return 1;

	/* should the job run in the background? */
	if ((bg = (*argv[argc - 1] == '&')) != 0)
	{
		argv[--argc] = NULL;
	}
	return bg;
}

/*
 * builtin_cmd - If the user has typed a built-in command then execute
 *    it immediately.
 */
int builtin_cmd(char **argv)
{
	// fprintf(stderr, "In builtin_cmd\n");

	if (strcmp(argv[0], "quit") == 0)
	{
		// fprintf(stderr, "In builtin_cmd\n");
		exit(0);
	}
	return 0; /* not a builtin command */
}

/***********************
 * Other helper routines
 ***********************/

/*
 * usage - print a help message
 */
void usage(void)
{
	printf("Usage: shell [-hvp]\n");
	printf("   -h   print this message\n");
	printf("   -v   print additional diagnostic information\n");
	printf("   -p   do not emit a command prompt\n");
	exit(1);
}

/*
 * unix_error - unix-style error routine
 */
void unix_error(char *msg)
{
	fprintf(stdout, "%s: %s\n", msg, strerror(errno));
	exit(1);
}

/*
 * app_error - application-style error routine
 */
void app_error(char *msg)
{
	fprintf(stdout, "%s\n", msg);
	exit(1);
}
