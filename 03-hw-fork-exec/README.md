# `fork` and `exec`

The purpose of this assignment is to give you hands-on experience with
`fork()`, `execve()`, and `pipe()` system calls, by walking you through various
iterative exercises and examining the resulting output.  Read the entire
assignment before beginning!


# Maintain Your Repository

 Before beginning:
 - [Mirror the class repository](../01a-hw-private-repo-mirror), if you haven't
   already.
 - [Merge upstream changes](../01a-hw-private-repo-mirror#update-your-mirrored-repository-from-the-upstream)
   into your private repository.

 As you complete the assignment:
 - [Commit changes to your private repository](../01a-hw-private-repo-mirror#commit-and-push-local-changes-to-your-private-repo).


# Preparation

 1. Read the following in preparation for this assignment:

    - Sections 8.2 - 8.4 and 10.8 - 10.10 in the book

    Additionally, man pages for the following are also referenced throughout
    the assignment:

    - `fork(2)`
    - `execve(2)`
    - `pipe(2)`, `pipe(7)`
    - `dup2(2)`

 2. Run `make` to build two executables: `fork` and `exec`.  These are programs
    that illustrate the system calls `fork()` and `execve()`.

 3. Start a [tmux session](../01c-hw-remote).  Create two panes, such that the
    window looks like this:

    ```
    ----------------------------
    |  command   |   system    |
    | execution  |  analysis   |
    |            |             |
    ----------------------------
    ```


# Part 1: `fork()` Overview

Open `fork.c`, and look at what it does.  Then answer the following questions.
Note that you will be _testing_ the behavior of `fork.c` in Part 2, so you will
want to consider these questions as you go through that part.

 1. *Briefly describe the behavior of `fork.c`.*
it forks the program into two and checks for which process is the parent and child. The child process sleeps for 30 seconds and the parent sleeps for 60.


 2. *Which sections (i.e., of "A", "B", "C", and "D") are run by the parent
    process?*
A,C

 3. *Which sections (i.e., of "A", "B", "C", and "D") are run by the child
    process?*
A,B

# Part 2: `fork()` Experimentation

The `ps` command prints information about processes on the system.  Which
processes are included and what information is printed about them depends on
the options you specify on the command line.

In the next steps, you will be using the `ps` command to examine how a process
associated with the `fork` program changes over time. Because of this, you
should read all of problems 4 through 11 before you start.

 4. In the left pane of your tmux window, run the `fork` program.  In the
    right pane, run the `ps` command _twice_, according to the following
    timing:
    - during the 30 seconds of sleep immediately following the printing of
      "Section B" and "Section C".
    - after "Section B done sleeping" is printed.

    Use the `-p`, `-o`, and `--forest` options when you run `ps` so that,
    respectively:

    - only the processes with the PIDs output by `fork` are shown
    - the `ps` output is formatted to have the following fields:
      - `user`: the username of the user running the process
      - `pid`: the process ID of the process
      - `ppid`: the process ID of the parent process
      - `state`: the state of the process, e.g., "Running", "Sleep", "Zombie"
      - `ucmd`: the command executed
    - the process ancestry is illustrated

    Use the man page for `ps(1)` for more on how to use these options.

    *Show the two `ps` commands you used, each followed by its respective
    output.*

    ```
	[SSH] 03-hw-fork-exec git:(master U:1 ?:4) ✗ ➜ ps --forest -o user,pid,ppid,state,ucmd -p 317876,317877
	USER         PID    PPID S CMD
	cf436     317876  316307 S fork
	cf436     317877  317876 S  \_ fork
	[SSH] 03-hw-fork-exec git:(master U:1 ?:4) ✗ ➜ ps --forest -o user,pid,ppid,state,ucmd -p 317876,317877
	USER         PID    PPID S CMD
	cf436     317876  316307 S fork
	cf436     317877  317876 Z  \_
    ```
 

    Note: to copy the command and output from only a single tmux pane, do the
    following:

    - Select the pane from which you want to copy.
    - Push `ctrl`+`b` then `z` to "zoom" into (i.e., show only) the selected
      pane.
    - Hold down `shift` and select with your mouse the area you want to copy.
    - Copy (`ctrl`+`c` or `ctrl`+`shift`+`c`) the hightlighted text.
    - Push `ctrl`+`b` then `z` to "un-zoom" the selected pane.

 5. *What is different between the output of the two `ps` commands?  Briefly
    explain.*
	After the child exits, it is a zombie process

 6. If you were to run the `fork` and `ps` commands from question 4 again at
    the same times as you did before, *what special line of code could you add
    to `fork.c` to eliminate the process with state "Z" from the output of the
    second `ps` command?*
    `waitpid(-1,NULL,0);`


 7. Referring to the previous question, *where would this line most
    appropriately go?*
    at the beginning of section C before it calls sleep.

 8. Add the line of code referenced in question 6 to the location referenced
    in question 7.  Then call `make` to re-compile `fork.c`. (Note that you
    may also need to add a few `#include` statements at the top of the file
    for it to compile and run properly.  See the man page for the function to
    learn which to include.)

    Follow the instructions from question 4 again to verify your answers to
    questions 6 and 7.

    *Show the two `ps` commands you used, each followed by its respective
    output.*
```
[SSH] 03-hw-fork-exec git:(master U:2 ?:4) ✗ ➜ ps --forest -o user,pid,ppid,state,ucmd -p 318879,318880
USER         PID    PPID S CMD
cf436     318879  316307 S fork
cf436     318880  318879 S  \_ fork
[SSH] 03-hw-fork-exec git:(master U:2 ?:4) ✗ ➜ ps --forest -o user,pid,ppid,state,ucmd -p 318879,318880
USER         PID    PPID S CMD
```


 9. *What is different between the output of the two `ps` commands?  Briefly
    explain.*
    The child process is both terminated and reaped by the parent and so it is deleted by the system.	

 10. Modify `fork.c` according to the following:

     - Comment out the line of code you added in question 8.
     - Replace the single 30-second call to `sleep()` in Section B with two
       30-second `sleep()` calls, back-to-back.
     - Replace the 60-second `sleep()` in Section C with a 30-second `sleep()`
       call.

     Re-`make`, then run `fork` in the left pane of your tmux window.  In the
     right pane, run the `ps` command _twice_, with the same options as in
     question 4, according to the following timing:
     - during the 30 seconds of sleep immediately following the printing of
       "Section B" and "Section C".
     - after "Section C done sleeping" is printed.

     *Show the two `ps` commands, each followed by its respective output.*
```
[SSH] 03-hw-fork-exec git:(master U:2 ?:4) ✗ ➜ ps --forest -o user,pid,ppid,state,ucmd -p 319169,319170
USER         PID    PPID S CMD
cf436     319169  316307 S fork
cf436     319170  319169 S  \_ fork
[SSH] 03-hw-fork-exec git:(master U:2 ?:4) ✗ ➜ ps --forest -o user,pid,ppid,state,ucmd -p 319169,319170
USER         PID    PPID S CMD
cf436     319170       1 S fork
```

 11. *What is different between the output of the two `ps` commands?  Briefly
     explain.*
The first one is the same and it shows the both the parent and the child, then after the parent is done sleeping, it dissapears and the child process is the only one left and it's cmd is fork.

You can now close the right pane in tmux.  Further commands will only use a
single pane.

Now would be a good time to review questions 1 through 3, both to confirm or
update your answers and to check your understanding.


# Part 3: File Descriptor Inheritance and File Description Sharing

In this section, you will learn hands-on how file descriptors are inherited by
child processes, and how two different processes with descriptors referencing
the same system-wide file description can write to the same open file.

 12. Modify `fork.c` according to the following:

     - Comment out _all_ calls to `sleep()`.
     - Comment out _all_ `printf()` calls that print "...done sleeping".
     - Before the call to `fork()`, open the file `fork-output.txt` for writing
       (see the man page for `fopen(3)`).
     - Write "BEFORE FORK (%d)\n" to the file before the call to `fork()`,
       replacing "%d" with the file descriptor of the newly opened file (see
       the man page for `fileno(3)`).
     - Call `fflush()` on the file stream immediately after writing.

       Note that any data buffered in a file stream is part of the user-space
       memory that is copied to the newly-created process created by `fork()`.
       Thus, any such data would be printed twice--once each process flushes
       the buffer associated with its own copy of that file stream.  Calling
       `fflush()` makes sure the buffer is completely flushed before `fork()`
       to avoid this potentially confusing scenario.
     - In section B, do the following, in order:
       - Sleep for 5 seconds
       - write "SECTION B (%d)\n" to the file, replacing "%d" with the
         file descriptor of the newly opened file (see the man page for
         `fileno(3)`).
     - In section C, do the following, in order:
       - write "SECTION C (%d)\n" to the file you opened, replacing "%d" with
         the file descriptor of the newly opened file.
       - Immediately after writing to the file, call `fclose()` on the file.

         Note that `fclose()` calls `close()` on the file descriptor, after
         flushing the buffer of the file stream (see the man page for
         `fclose(3)`).
       - sleep for 5 seconds.

     Re-`make` and run the newly recompiled `fork`. *Using `cat`, show the
     contents of the `fork-output.txt` file you created.*

 13. *Based on the contents of `fork-output.txt`, which process(es) wrote to
     the file?*
	Both the parent and the child wrote to the file.

 14. *Based on both the contents of `fork-output.txt` and what was written to
     the terminal, which file descriptor(s) were inherited by the child
     process?*  (Hint: See "Note the following further points" in the man page
     for `fork(2)`.)
	stdout(2) was used by both for printing to the terminal and a new file descriptor for the file (3) was used.	

 15. Consider the timing of the `fprintf()` calls made 1) before the `fork()`,
     2) in section B, and 3) in section C.  In each call a process wrote to
    `fork-output.txt`, and the writes were orchestrated such that they happened
     in a specific order.

     *Based on the content of `fork-output.txt`, did the second write pick up
     where the first write left off, or did it start over at the beginning?
     Why?*  (Hint: See "Note the following further points" in the man page
     for `fork(2)` and the section titled "Open file descriptions" in the man
     page for `open(2)`.)

It wrote where the first left off. Because the FDs are copied completely which includes the IO offset.

 16. *Based on the content of `fork-output.txt`, did the third write pick up
     where the second write left off, or did it start over at the beginning?
     Why?*  (Hint: See "Note the following further points" in the man page
     for `fork(2)` and the section titled "Open file descriptions" in the man
     page for `open(2)`.)
	It also wrote where the second one left off. They share the same file table statuses

 17. Consider the timing of the `fclose()` call in relation to the third write
     to `fork-output.txt`.  The execution of `fclose()` clearly occurred before
     the third call.

     *Based on the content of `fork-output.txt`, did this third write succeed?
     Why or why not?*  (Hint: See "Note the following further points" in the
     man page for `fork(2)` and the second paragraph in the "DESCRIPTION"
     section of the man page for `close(2)`.)

	It did succeed because the child process has a copy of the file descriptor that points to the file descripTION. Close simply closes the file descriptor but the active status flags are still valid in the child process's file descriptor.

# Part 4: Pipes

In this section, you will learn how pipes are created and used to communicate
between different processes.

 18. Modify `fork.c` according to the following:

     - Comment out _all_ calls to `sleep()`.
     - Prior to the call to `fork()`, open a pipe (see the man page for
       `pipe(2)`).
     - In section B:
       - Close the file descriptor corresponding to the _read_ end of the pipe
         (see the man pages for `pipe(2)` and `close(2)`).
       - Write "hello from Section B\n" to the file descriptor corresponding to
         the _write_ end of the pipe (see the man page for `write(2)`).  Note
         that unlike `fprintf()`, which takes a null-terminated string (`char *`)
         as input and writes to a buffered file stream (`FILE *`), `write()`
         simply takes a file descriptor, a pointer to a memory location and a
         number of bytes.  Thus, you will need to specify the length of the
         string.  If the length is incorrect, the command will yield unexpected
         results.
       - Call `close()` on the write end of the pipe.
     - In section C:
       - Close the file descriptor corresponding to the _write_ end of the
         pipe (see the man pages for `pipe(2)` and `close(2)`).
       - Read from file descriptor corresponding to the _read_ end of the pipe
         (see the man page for `read(2)`) into a buffer that you have declared.
         Save the number of bytes read (return value of `read()`), and use that
         value to add a null character after them, so string operations can be
         performed on it (see the man page for `string(3)`).
       - Print the number of bytes received from calling `read()` on the pipe.
       - Print the string retrieved from `read()` to stdout.  Note that
         `printf()` and `fprintf()` require a null-terminated string, i.e., to
         know where the string ends.  If you have not properly added the null
         character, the command will yield unexpected results.  See an example
         of adding the null byte
         [here](../01d-hw-strings-io-env#part-5---inputoutput) (i.e., after
         `read()` was used to read bytes from the file).

     Re-`make` and run the newly recompiled `fork`.  *Show the output of your
program.*

```
[SSH] 03-hw-fork-exec git:(master U:2 ?:5) ✗ ➜ ./fork
Starting program; process has pid 245116
Section A;  pid 245116
Section C
Section A;  pid 245117
Section B
Read (21) bytes
hello from Section B
```

 19. *Is the ordering of `pipe()` and `fork()` important?  Why or why not?*
     (Hint: See "Note the following further points" in the man page for
     `fork(2)`)
	yes. if pipe is run before fork, then both processes will have the same FDs for the pipe, but if it is run after, then they will be unique pipes.


 20. The way that you have set things up, one process is is writing to its end
     of the pipe, while the other is reading from its end of the pipe.  *Can
     the communication also go in reverse direction, such that each process is
     both reading and writing from its end of the pipe?*  (Hint: see the man
     page for `pipe(7)`.)
	no. a pipe is unidirectional.

 21. Modify `fork.c` according to the following:

     - In section B:
       - Immediately before writing "hello..." to the write end of the pipe,
         sleep for 10 seconds.
       - Immediately before calling `close()` on the write end of the pipe,
         sleep for 10 seconds.
     - In section C:
       - Immediately after reading from the pipe and printing the results,
         perform a second read from the pipe, and again print the number of
         bytes read from the pipe.  All this should happen before calling
         `close()` on the read end of the pipe.

     Re-`make` and run the newly recompiled `fork`.

*How many bytes were received as a result of the second read from the
     pipe?*
	0

 22. Consider the timing of both the first and the second calls to `read()` on
     the pipe, in relation to calls made by the other process.  *What happens
     when `read()` is called on an empty pipe (i.e., where no data has been
     written to it)?*  (Hint: See the man page for `pipe(7)`.)
	It returns because it sees an end of file signal

 23. *What call in the other process caused the first call to `read()` to
     return?*  (Hint: See the man page for `pipe(7)`.)
	the byte count of 21

 24. *What call in the other process caused the second call to `read()` to
     return?*  (Hint: See the man page for `pipe(7)`.)
	the close(fd[1])

 25. *What was the effect of the call referred to in the previous question, as
     evidenced by the number of bytes returned?*  (Hint: See the man pages for
     `pipe(7)` and `pipe(2)`)
	it closes the pipe

# Part 5: `execve()` Overview

Open `exec.c`, and look at what it does.  Then answer the following questions.
Note that you will be _testing_ the behavior of `exec.c` in Part 6, so you
might want to revisit these questions after you go through that part.

 26. *Briefly describe the behavior of `exec.c`.*
	the user passes a binary path as an argument and the program runs that binary with custom arguments and env.

 27. *At what point will the final `printf()` statement get executed?*
After the exec program exits.


# Part 6: `execve()` Experimentation

In the next steps, you will be using the `ps` command to examine how a process
associated with the `exec` program changes over time. Because of this, you
should read all of problems 28 through 31 before you start.

 28. In the left ("command execution") pane of your tmux window, run the `exec`
     program, passing `/bin/cat` as the first command-line argument. *Show your
     terminal commands and the output.*
```
[SSH] 03-hw-fork-exec git:(master U:2 ?:4) ✗ ➜ ./exec /bin/cat
Program "./exec" has pid 246899. Sleeping.
Running exec of "/bin/cat"
```

 29. In the right ("system analysis") pane of your tmux window, run the `ps`
     command, first during the initial 30-second `sleep()` call, then again
     after the first 30 seconds is over, but before the end of the program.

     Use the `-p` and `-o` options when you run `ps` so that, respectively:

     - only the process with the PID output by `exec` is shown
     - the `ps` output is formatted to have the following fields:
       - `user`: the username of the user running the process
       - `pid`: the process ID of the process
       - `ppid`: the process ID of the parent process
       - `state`: the state of the process, e.g., "Running", "Sleep", "Zombie"
       - `ucmd`: the command executed

     Use the man page for `ps(1)` for more on how to use these options.

     *Show your terminal commands and the output.*
```
[SSH] 03-hw-fork-exec git:(master U:2 ?:4) ✗ ➜ ps --forest -o user,pid,ppid,state,ucmd -p 246899
USER         PID    PPID S CMD
cf436     246899  241820 S exec
```
 30. *Which fields (if any) have changed in the output of the two ps commands?
     Briefly explain.*
	None they are the same process

     (You can use `ctrl`+`d` to signal end of file (EOF), so the program will
     run to completion)

 31. Run the `exec` program again, but this time using a non-existent program
     (e.g., `/does-not-exist`) as an argument.  *Show the output, and briefly
     explain what happened.*
	Now it creates a process at first, but when it tries to run it fails.

Now would be a good time to review questions 26 and 27, both to confirm or
update your answers and to check your understanding.


# Part 7: Combining `fork()` and `execve()`

In this section, you will learn hands-on how file descriptors are inherited by
child processes after a call to `fork()` and maintained after a call to
`execve()`.

 32. Modify `fork.c` according to the following:

     - Copy the contents of the `main()` function in `exec.c` into `fork.c` in
       such a way that the _child_ process created with the call to `fork()`
       runs the `execve()` call with the first command-line argument passed to
       `fork`. The contents you paste should immediately precede the `exit(0)`
       statement called by the child process.
     - Comment out the 30-second call to `sleep()` copied over from `exec.c`.

     Re-`make` and execute the following:

     ```bash
     echo foobar | ./fork /bin/cat
     ```

     *Show the output from running the pipeline.*

```
[SSH] 03-hw-fork-exec git:(master U:2 ?:5) ✗ ➜ echo foobar | ./fork /bin/cat
Starting program; process has pid 247961
Section A;  pid 247961
Section C
Section A;  pid 247962
Program "./fork" has pid 247962. Sleeping.
Running exec of "/bin/cat"
foobar
Read (0) bytes
@@yURead (0) bytes
```

     Note that you might find that the buffer associated with the open file
     stream (i.e., `FILE *`) is destroyed--as part of the re-initialization
     of the stack and heap, in connection with `execve()`, before it is ever
     flushed.  In this case, the statements you wrote to `fork-output.txt`
     in the child process_before_ calling `execve()` will never make it to the
     file!  You could fix this by using `fflush()` immediately after `fprintf()`.


# Part 8: File Descriptor Duplication

In this section, you will learn hands-on how file descriptors can be duplicated
using `dup2()`.

 33. First make a copy of `fork-output.txt` with the following:

     ```bash
     cp fork-output.txt fork-output-old.txt
     ```

     Modify `fork.c` according to the following:

     - Immediately before calling `execve()`, duplicate the file descriptor
       associated with the file stream you opened in connection with
       `fork-output.txt` such that the standard output of the child process
       goes to file descriptor associated with that stream instead (see the man
       pages for `fileno()` and `dup2()`).  Pay special attention to the
       ordering of the arguments passed to `dup2()`, or this will not work
       properly.

     Here is a brief explanation about `dup2()`.  The man page gives the
     following synopsis:
     ```c
     int dup2(int oldfd, int newfd);
     ```
     The man page explains that "the file descriptor `newfd` is adjusted so
     that it now refers to the same open file description as `oldfd`."  In
     other words when `dup2(4, 5)` is called, file descriptor 5 is closed and
     re-opened to point to the same file description pointed to by file
     descriptor 4.

     Re-make and execute the following to show that it works:

     ```bash
     echo foobar | ./fork /bin/cat
     ```

     *Show the output from running the pipeline. Also show the contents of
     `fork-output.txt`.*

 34. *What is now the difference between `fork-output-old.txt` and
     `fork-output.txt`.*
