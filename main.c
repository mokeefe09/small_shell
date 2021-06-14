// Author: Michael O'Keefe
// Date: 2/3/2021
// Description: Simple custom Linux shell. Capable of running
//              foreground and background processes, validating
//              input, parsing PIDs, cleaning up zombie processes,
//              redirecting input/output, and handling SIGINT and
//              SIGTSTP signals.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include "command_info.h"
#include "input_funcs.h"
#include "shell_process.h"
#include "list_node.h"

// Global variable. If 0, we are in normal mode where bg and fg commands
// can execute. If 1, only fg commands will execute. This value is changed
// by the handler for the SIGTSTP signal.
int fg_only_mode = 0;

int main(void){
	char* validated_str;
	struct command_info curr_command; 
	pid_t pid_result;
	struct pid_node* head = NULL;
	struct pid_node* tail = NULL;
	struct pid_node* new_node;

	int last_status = 0; 

	// Initialize sigaction structs for signals that affect the parent process
	struct sigaction sigtstp_action = {0};
	struct sigaction sigint_action = {0};

	// Create SIGINT signal handler (will be ignored by parent)
	make_sigint_struct(&sigint_action);
	sigaction(SIGINT, &sigint_action, NULL);

	// Create SIGTSTP signal handler (will be handled by parent, not ignored)	
	make_sigtstp_struct(&sigtstp_action);
	sigaction(SIGTSTP, &sigtstp_action, NULL);

	do{
		// Each time before the prompt is presented to the user, cleanup_bg
		// cleanups all background processes that have terminated.
		cleanup_bg(head, &head, &tail);

		// Present prompt to user
		printf(": ");
		fflush(stdout);

		// Get user's command-line input. If it is just white spaces
		// or a comment, go back to start of loop and re-prompt by
		// calling continue.
		if ((validated_str = arg_str()) == NULL)
			continue;

		// If this point is reached, a dynamically allocated user command
		// string is stored in validated_str. We call tokenize to break
		// the string into a structure that will hold the command args,
		// i/o redirection filenames and a background/foreground flag.
		tokenize(validated_str, &curr_command);

		// This if statement and the following two else if statements
		// are for built in functions. If the user's command was
		// status, call the status function. When status is done return
		// to prompt.
		if (strcmp(curr_command.args[0], "status") == 0)
		{
			status(last_status);
			continue;
		}

		// If user's command was cd, call the built in change_dir 
		// function, and when it's done return to prompt.
		else if (strcmp(curr_command.args[0], "cd") == 0)
		{
			change_dir(&curr_command);
			continue;
		}

		// If user's command was exit, call the built in exit_shell
		// function, which will exit the shell.
		else if (strcmp(curr_command.args[0], "exit") == 0)
			exit_shell(head);

		// If this point is reached, the user did NOT call a built in
		// function, so we have to fork off a new process. If we are
		// in normal more and the struct's background flag is set,
		// run a background process.
		if (curr_command.background && fg_only_mode == 0)
		{
			// Start the background command. The pid of the new bg command
			// is saved in pid_result.
			pid_result = bg_proc(&curr_command);

			// This will only happen if there is a fork error.
			if (pid_result == -1)
			{
				continue;	
			}

			// Create a new linked list node from the new bg pid.
			new_node = add_node(pid_result);	
			
			// Add the new node into the linked list of bg process IDs.
			if (head == NULL)
			{
				head = new_node;
				tail = new_node;
			}

			else
			{
				tail->next = new_node;
				tail = new_node;
			}
		}

		// If we are in foreground-only mode or if the background flag of the struct
		// was not set, we will run a foreground command.
		else
		{
			// The parent process must wait for the foreground process to terminate,
			// so last_status will hold the termination status of the child.
			last_status = fg_proc(&curr_command);
			fflush(stdout);
		}

	} while(1);

	return 0;
}

