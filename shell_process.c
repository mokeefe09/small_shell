#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
#include "shell_process.h"
#include "command_info.h"
#include "list_node.h"

// Defined in main.c. Used by sigtstp_handler function to toggle between
// foreground_only and regular modes.
extern int fg_only_mode;

void sigtstp_handler(int signo){
/*
Executes if a SIGTSTP signal is received by the parent process.
Toggles the fg_only_mode on or off depending on its current state and
writes a message to STDOUT.
*/
	// If fg_only_mode is on, turn it off
	if (fg_only_mode)
	{
		fg_only_mode = 0;
		char* message = "\nExiting foreground-only mode\n";
		write(STDOUT_FILENO, message, 31);
		fflush(stdout);
	}

	// If fg_only_mode is off, turn it on
	else
	{
		fg_only_mode = 1;
		char* message = "\nEntering foreground-only mode (& is now ignored)\n";
		write(STDOUT_FILENO, message, 51);
		fflush(stdout);
	}
}

void make_sigtstp_struct(struct sigaction * sig){
/*
Short function that creates a sigaction struct for the parent process,
called at the beginning of main.c

Receives: struct sigaction* sig: Pointer to struct that will be filled.
Returns: Nothing, just fills the struct.
*/
	// When SIGTSTP signal received in parent process, the sigtstp_handler
	// function, defined in this file, will execute.
	sig->sa_handler = sigtstp_handler;
	sigfillset(&sig->sa_mask);
	sig->sa_flags = 0;
}

void make_sigint_struct(struct sigaction * sig){
/*
Short function that creates a sigaction struct for the parent process,
called at the beginning of main.c

Receives: struct sigaction* sig: Pointer to struct that will be filled.
Returns: Nothing, just fills the struct.
*/
	// Handler is just SIG_IGN, which means the process just ignores the signal.
	// This sigaction struct will be used with the SIGINT signal.
	sig->sa_handler = SIG_IGN;
	sigfillset(&sig->sa_mask);
	sig->sa_flags = 0;
}

void cleanup_bg(struct pid_node* list, struct pid_node** head, struct pid_node** tail){
/*
Searches through a linked list of process IDs, searching for any IDs representing
child processes that have not been cleaned up. If found, it cleans up the terminated
process and removes its corresponding node from the linked list.

Receives: -struct pid_node* list: The head of the linked list.
          -struct pid_node** head: This is a pointer to the head pointer in the main
		                           function. When dereferencing this double pointer,
								   we can change the value of the head pointer that
								   resides in main.

          -struct pid_node** tail: This is a pointer to the tail pointer in the main
		                           function. When dereferencing this double pointer,
								   we can change the value of the tail pointer that
								   resides in main.

Returns: Nothing.
*/
	struct pid_node* prev = NULL;
	int wstatus;
	int remove_flag = 0;
	pid_t childPID;

	// Iterate through linked list
	while (list != NULL)
	{
		// If childPID isn't 0, there was a child that terminated.
		if (childPID = waitpid(list->pid, &wstatus, WNOHANG))
		{
			// Flag to be used below, indicates a node is being removed from the linked list.
			remove_flag = 1;

			// If exited normally, print exit status, otherwise print signal that caused termination.
			if (WIFEXITED(wstatus))
			{
				printf("background pid %d is done: exit value %d\n", childPID, WEXITSTATUS(wstatus));
				fflush(stdout);
			}
			
			else
			{
				printf("background pid %d is done: terminated by signal %d\n", childPID, WTERMSIG(wstatus));
				fflush(stdout);
			}


		// Since a process exited and we cleaned it up, we now must remove the linked list node
		// associated with that process. The if statements below determine which node needs to be
		// removed and remove it. Note, if the node is at the head or tail, it must be removed
		// differently from a node in the middle of the list. The head and tail variables are
		// double pointers received as input to the function. When dereferenced they allow us
		// to change the head and tail variables defined in the main function.

			// If the node to be removed is the only one in the list:
			if (list == *head && list == *tail)
			{
				// Free node and change head and tail pointers to NULL.
				free(list);
				*head = NULL;
				*tail = NULL;

				// Exit from function as there are was only one node in the list, don't have
				// to check for more
				return;
			}

			// Multiple items in the list. Just have to remove head.
			else if (list == *head && list != *tail)
			{
				*head = (*head)->next;	
				free(list);
			}

			// Multiple items in the list. Just have to remove tail.
			else if (list != *head && list == *tail)
			{
				*tail = prev;
				(*tail)->next = NULL;
				free(list);
			}

			// Current node to remove is neither head nor tail.
			else
			{
				prev->next = list->next;
				free(list);
			}
		}

		// This flag is set above if a process is terminated. It lets us know
		// a node was removed from the linked list. If so, simply re-start iteration
		// through the list starting at the head of the list (which may have changed
		// if the head was the node that was removed).
		if (remove_flag == 1)
		{
			list = *head;
			prev = NULL;
			remove_flag = 0;
		}

		// No nodes were removed, so simply iterate to the next node in the list.
		else
		{
			prev = list;
			list = list->next;
		}
	}

	return;
}

void change_dir(struct command_info* command){
/*
Changes the working directory of the current process. Note, this does not
update the env variable PWD.

Receives: -struct command_info* command: Pointer to a struct that holds
           command info. Only uses the arg at index 1 of the args array,
		   regardless of how many args are in the struct.

Returns: Nothing
*/
	int result;
	char* home_dir;

	// If the arg at index 1 is null, that means the command only contained
	// one arg, cd. Which means we change to the home directory.
	if (command->args[1] == NULL)
	{
		// Get the value for the HOME env variable. And use it to change
		// to home dir.
		home_dir = getenv("HOME");
		result = chdir(home_dir);
	}

	else
		// Change dir to path specified in user's argument.
		result = chdir(command->args[1]);

	// If chdir failed, print an error message.
	if (result == -1)
	{
		printf("Error opening directory\n");
		fflush(stdout);
	}
}

void exit_shell(struct pid_node* list){
/*
This function execute when the user enters the "exit" command.
It terminates and cleans up all background child processes
then terminates itself.

Receives: struct pid_node* list: head of linked list containing
          pids of running background processes.
*/
	pid_t childPID;
	int wstatus;

	// Iterate through linked list
	while (list != NULL)
	{
		// Send the SIGTERM signal with the node's pid, then
		// go to the next node in the list.
		kill(list->pid, SIGTERM);
		list = list->next;
	}

	// At this point, all background children have received a 
	// termination signal. Clean them all up with a WAITPID loop.
	while ((childPID = waitpid(-1, &wstatus, WNOHANG)) != -1)
		continue;
	
	// When waitpid returns -1, if the error type was ECHILD, we know
	// there are no more children left to clean up, so we can
	// exit normally. Any other error type means there was an error
	// with waitpid.
	if (errno == ECHILD)
		exit(0);
	else
	{
		printf("Error during waitpid() in exit function\n");
		fflush(stdout);
		exit(1);
	}
}

void status(int fg_status){
/*
Prints the status of the most recently terminated fg process.

Receives: int fg_status: holds the raw status received from the
                         last terminated fg process.
Returns: Nothing
*/
	// If terminated normally, prints the status number.
	if (WIFEXITED(fg_status))
	{
		printf("Exit value %d\n", WEXITSTATUS(fg_status));
		fflush(stdout);
	}
	
	// If terminated abnormally, prints the signal number.
	else
	{
		printf("Terminated by signal %d\n", WTERMSIG(fg_status));
		fflush(stdout);
	}
}

struct pid_node* add_node(pid_t processID){
/*
Creates a linked list node.

Receives: pid_t processID: Will be "pid" member of node
Returns: Pointer to the newly created node.
*/
	// Dynamically allocate memory for the node.
	struct pid_node* new_node = malloc(sizeof(struct pid_node));

	// Set pid member to the input PID and the next pointer to NULL.
	new_node->pid = processID;
	new_node->next = NULL;

	return new_node;
}

pid_t bg_proc(struct command_info* command){
/*
Uses fork and exec to create a background process. Since this is
a bg process, there is no waitpid to clean up the process. It is
eventually cleaned up outside the function.

Receives: struct command_info* command: Pointer to struct with 
          information for command (args, i/o redirection files).

Returns: If succesful, returns pid of new bg process.
         If failure, returns -1.
*/

	// Background processes ignore the SIGTSTP signal. This differs from
	// the parent process, which does not ignore SIGTSTP and has a handler
	// for it.
	struct sigaction sigtstp_action = {0};

	int infile;
	int outfile;
	int dup_result;
	int exec_result;
	char devnull[] = "/dev/null";
	pid_t childPID;

// Create child process
	switch (childPID = fork())
	{

// Handle fork error
		case -1:
			printf("Error during fork\n");
			fflush(stdout);
			return -1;

// Child process
		case 0:
	// Ignore SIGTSTP. This must be done in the bg process because
	// the parent process does not ignore SIGTSTP
			sigtstp_action.sa_handler = SIG_IGN;
			sigfillset(&sigtstp_action.sa_mask);
			sigtstp_action.sa_flags = 0;
			sigaction(SIGTSTP, &sigtstp_action, NULL);
	
	// stdin redirection
			// If user redirected input, open the stdin_file, otherwise
			// open /dev/null.
			if (command->stdin_file != NULL)
				infile = open(command->stdin_file, O_RDONLY);
			else
				infile = open(devnull, O_RDONLY);

			// if there was an error opening either file exit failure.
			if (infile == -1)
			{
				exit(1);
			}

			// Redirect the stdin_file descriptor to point to the file
			// we just opened.
			dup_result = dup2(infile, 0);

			// If there was an error during dup2 exit failure.
			if (dup_result == -1)
			{
				exit(1);
			}

	// stdout redirection
			// If user redirected output, open the stdout_file, otherwise open
			// /dev/null
			if (command->stdout_file != NULL)
				outfile = open(command->stdout_file, O_WRONLY | O_CREAT | O_TRUNC, 0660);
			else
				outfile = open(devnull, O_WRONLY | O_CREAT | O_TRUNC, 0660);

			// Error when opening file, exit failure
			if (outfile == -1)
			{
				exit(1);
			}

			// Redirect the stdout_file descriptor to point to the file we just
			// opened
			dup_result = dup2(outfile, 1);

			// Error during dup2, exit failure.
			if (dup_result == -1)
			{
				exit(1);
			}

	// Execute the process
			exec_result = execvp(command->args[0], command->args);

			if (exec_result == -1)
			{
				exit(1);
			}

// Parent Process	
		default:
			// Since this is a background process, do not wait for the child.
			printf("Background pid is %d\n", childPID);	
			fflush(stdout);
			return childPID;	
	}
}

int fg_proc(struct command_info* command){
/*
Uses fork and exec to create a foreground process, and uses waitpid for
the parent processes to wait until the fg child has terminated.

Receives: struct command_info* command: Pointer to struct with 
          information for command (args, i/o redirection files).

Returns:  If successful, termination status of fg process.
          If failure, returns -1.
*/
	
	// Initialize two sigaction structs, one for SIGTSTP and one
	// for SIGINT.
	struct sigaction sigint_action = {0};
	struct sigaction sigtstp_action = {0};

	int infile;
	int outfile;
	int wstatus;
	int dup_result;
	int exec_result;
	pid_t wait_result;
	pid_t childPID;
	sigset_t sigtstp_set;

	// Normally the parent process has a signal handler for sigtstp. However,
	// during a fg process, it must ignore sigtstp. This signal set contains
	// only one signal, SIGTSTP, which will be used below, immediately before
	// and after waitpid.
	sigemptyset(&sigtstp_set);
	sigaddset(&sigtstp_set, SIGTSTP); 
	
// Fork child process
	switch (childPID = fork())
	{
// Handle fork error
		case -1:
			printf("Error during fork\n");
			fflush(stdout);
			return -1;

// Child process
		case 0:
	// Set SIGINT handler to default, as we want the child to terminate on
	// receipt of SIGINT. This is different from the parent, which ignores
	// SIGINT.
			sigint_action.sa_handler = SIG_DFL;	
			sigfillset(&sigint_action.sa_mask);
			sigint_action.sa_flags = 0;
			sigaction(SIGINT, &sigint_action, NULL);

	// Set SIGTSTP handler to ignore. All fg processes ignore SIGTSTP.
			sigtstp_action.sa_handler = SIG_IGN;
			sigfillset(&sigtstp_action.sa_mask);
			sigtstp_action.sa_flags = 0;
			sigaction(SIGTSTP, &sigtstp_action, NULL);

	// stdin redirection
			// Checks if user specified stdin redirection
			if (command->stdin_file != NULL)
			{
				// Open file that stdin will be redirected to.
				infile = open(command->stdin_file, O_RDONLY);

				// Error opening file. 
				if (infile == -1)
				{
					printf("Error opening file for stdin redirection\n");
					fflush(stdout);
					exit(1);
				}

				// Redirect stdin fd to the file we just opened.
				dup_result = dup2(infile, 0);

				// Error during dup2
				if (dup_result == -1)
				{
					printf("Error while redirecting stdin: dup2() function\n");
					fflush(stdout);
					exit(1);
				}
			}

	// stdout redirection
			// Checks if user specified stdout redirection
			if (command->stdout_file != NULL)
			{
				// Open file that stdout will be redirected to.
				outfile = open(command->stdout_file, O_WRONLY | O_CREAT | O_TRUNC, 0660);

				// Error opening file.
				if (outfile == -1)
				{
					perror("open");
					fflush(stdout);
					exit(1);
				}

				// Redirect stdout fd to the file we just opened.
				dup_result = dup2(outfile, 1);

				// Error during dup2
				if (dup_result == -1)
				{
					perror("dup2");
					fflush(stdout);
					exit(1);
				}
			}

	// Execute command
			exec_result = execvp(command->args[0], command->args);

			if (exec_result == -1)
			{
				perror(command->args[0]);
				fflush(stdout);
				exit(1);
			}

// Parent process
		default:
			// Use the signal set defined above (signal set only contains SIGTSTP) to block
			// SIGTSTP in the parent process while waiting for the fg process to terminate.
			sigprocmask(SIG_BLOCK, &sigtstp_set, NULL);

			// Will wait to execute until child fg process terminates.
			wait_result = waitpid(childPID, &wstatus, 0);

			// Use the signal set with SIGTSTP to unblock SIGTSTP. Now, the parent process
			// will handle a SIGTSTP signal like normal (enter/exit fg-only mode).
			sigprocmask(SIG_UNBLOCK, &sigtstp_set, NULL);

			// handle waitpid error.
			if (wait_result == -1)
			{
				printf("Error during waitpid\n");
				fflush(stdout);
				return -1;
			}

			// Immediately print message if fg process was terminated by signal.
			else if (WIFSIGNALED(wstatus))
			{
				printf("terminated by signal %d\n", WTERMSIG(wstatus));
				fflush(stdout);
				return wstatus;
			}

			else
			{
				return wstatus;
			}
	}
}
