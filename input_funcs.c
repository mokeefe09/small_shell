#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h> 
#include <unistd.h>
#include "command_info.h"
#include "input_funcs.h"

char* arg_str(void){
/*
Gets a line of a user's input. If the line is just a newline char,
starts with '#' or is all spaces, return NULL. Otherwise, strip
the newline from the line.

Receivs: Nothing
Returns: -Char pointer to allocated string.
         -NULL if all spaces, started with #, or just newline.
*/
	char* line = NULL; // Will store getline() input
	size_t num = 0;    // Simply used as an arg for getline()
	ssize_t line_len;  // Will hold return val from getline()

	// Get the user's command input from the terminal stored in line. The
	// line_len variable records the number of chars the user entered.
	line_len = getline(&line, &num, stdin);

	// Handle error if there is a signal interrupts getline(), then just
	// return NULL, which will take the user back to a new prompt.
	if (line_len == -1)
	{
		clearerr(stdin);
		return NULL;	
	}

	// Special case: If the user entered the enter key without any
	// other characters, the length of the string will just be 1, as it
	// only contains the newline char. In this case return null, which will 
	// re-prompt the user for input in the main function.
	if (line_len == 1){
		free(line);
		return NULL;
	}

	// Max char len is 2048. If user entered too many chars, exit with error.
	if (line_len > 2049){
		free(line);
		exit(1);
	}

	// Remove the newline char at the end of the string.
	strip_newline(line, line_len);

	// If the user's input was a comment or just all spaces, return null
	// which will re-prompt the user for input in the main function.
	if (comment_or_space(line)){
		free(line);
		return NULL;
	}

	// If this point is reached, the user's input was not empty, just spaces, or
	// a comment, so we return a pointer to the string.
	return line;
}

void strip_newline(char* string, ssize_t length){
/*
Strips the newline off of a string. Assumes it is not an empty string.
and that it has a newline at the end.

Receives: -char* string: string we will remove newline from.
          -ssize_t length: Length of string, not including null-byte
*/

	// Decrement length by one and add it to the start of the string to find
	// the last character in the string, which will be a newline. 
	string += --length;

	// Change last character in the string from '\n' to '\0'
	*string = '\0';

	return;
}

bool comment_or_space(char* string){
/*
Determines if a string is a comment (starts with '#') or all spaces.
Assumes string is not of length 0.

Receives: char* string: String we will be analyzing
Returns: bool: true if a comment or all spaces, false otherwise
*/

	// Check if first character of the string is a comment char.
	if (*string == '#')
		return true;
	
	// Iterate through each char of the string. As long as each char
	// is a space, keep going. When we exit the loop, there are two possibilities:
	// 1) Every char was a space and str will point to the null-byte
	// 2) Not every char was a space and str will not point to the null-byte
	while (*string == ' ')
		string++;

	// If we're at the null-byte, it's all spaces, otherwise it's not.
	if (*string == '\0')
		return true;
	else
		return false;
}

char* expand_pid(char* token){
/*
Allocates memory for the token and expands the "$$" sub-string to the
process id if encountered.

Receives: char* token: The input string that will be expanded. Does
                       not contain any spaces.
Returns: char*: Pointer to dynamically allocated, expanded string.
*/
	char* dollar_ptr;
	char* expanded_str;
	char* expanded_str_cpy;
	char* pid_str;
	char* token_cpy;
	int procID;
	int num_dollars;
	int num_chars;
	int temp;
	int expanded_size;

	// Find how many instances of $$ exist in the token.
	// At the start there are no $$ instances, so set num_dollars to 0.
	// Make a copy of the token ptr that we'll use to iterate through
	// the string.
	num_dollars = 0;
	token_cpy = token;

	// Use strstr to find "$$" sub-string. If found, returns ptr to
	// that sub-string.
	while ((dollar_ptr = strstr(token_cpy, "$$")) != NULL)
	{
		// Found sub_string. Increment num_dollars, and increment
		// token_cpy by two so we now have a ptr to the char immediately
		// after the second '$' char. If we reached the end of the
		// string, break out.
		num_dollars++;
		token_cpy = dollar_ptr + 2;
		if (*token_cpy == '\0')
			break;
	}

	// If there were no double dollar signs, no expansion needed. Return
	// a malloc'd version of the original token.
	if (num_dollars == 0)
	{
		expanded_str = malloc((strlen(token)+1) * sizeof(char));
		strcpy(expanded_str, token);
		return expanded_str;
	}
	
	// Get the proc ID, then calculate the # of chars of storage needed 
	// for the pid by successively dividing by ten and taking the floor.
	procID = (int) getpid();
	temp = procID;
	num_chars = 0;
	while (temp > 0)
	{
		temp /= 10;	
		num_chars++;
	}

	// Allocate space to hold string version of procID with malloc. Then,
	// store procID as a string in the malloc'd memory.
	pid_str = malloc((num_chars+1) * sizeof(char));
	sprintf(pid_str, "%d", procID);
	

	// Calculate the size of the new, expanded string. We subtract the 
	// $$ chars (2*num_dollars), then add the chars needed for the pid with
	// (num_chars*num_dollars). Then, allocate memory for the expanded string.
	expanded_size = strlen(token) - (2 * num_dollars) + (num_chars * num_dollars) + 1;
	expanded_str = malloc(expanded_size * sizeof(char));
	expanded_str_cpy = expanded_str;
	
	// If the size of the expanded string is more than the max of 2048 chars, 
	// return failure.
	if (expanded_size > 2049)
		exit(1);

	// This is where we actually copy the expanded string into memory
	while (*token)
	{
		// If we encounter $$ in the original string, copy the PID string into 
		// the expanded string, then increment token by 2 to pass the second $
		// char, and increment the expanded string ptr by the number of chars
		// in the PID string.
		if (*token == '$' && *(token+1) == '$')
		{
			memcpy(expanded_str_cpy, pid_str, num_chars);
			token += 2;
			expanded_str_cpy += num_chars;
		}

		// If we don't encounter $$, simply add the token character to the
		// corresponding character in the expanded string and increment both
		// pointers.
		else
		{
			*expanded_str_cpy = *token;
			expanded_str_cpy++;
			token++;
		}
	}

	// Add null-byte to the end of the expanded string.
	*expanded_str_cpy = '\0';

	// Free memory for the pid string.
	free(pid_str);

	// Return pointer to the start of the malloc'd expansion string.
	return expanded_str;	
}

void tokenize(char* inp_str, struct command_info* command_struct){
/*
Takes a string of command input as input and parses it into a 
struct containing arguments, input and output redirection files,
and a background flag. Uses teh expand_pid() function to change
the "$$" sub-string into a string representation of the pid.

Receives: -char* inp_str: The string to be parsed.
          -struct command_info* command_struct: Pointer to struct
		   that will hold parsed data.

Returns: Nothing. Data will be stored in the struct whose pointer
         was passed as input.
*/
	char* token;
	char* saveptr;
	int i, j;
	command_struct->stdin_file = NULL;
	command_struct->stdout_file = NULL;
	command_struct->background = 0;

	// Get the first argument. The expand_pid function returns a pointer
	// to dynamically allocated memory.
	token = strtok_r(inp_str, " ", &saveptr);
	command_struct->args[0] = expand_pid(token);

	// Loop through each argument. If an argument is preceded by "<" or by
	// ">", add it to the struct member stdin_file or stdout_file. i keeps
	// track of how many args were given.
	i = 1;
	while ((token = strtok_r(NULL, " ", &saveptr)) != NULL)
	{
		// Expand token to substitute PID for $$.
		command_struct->args[i] = expand_pid(token);

		// If the previous token was "<" fill the stdin_file with the
		// current token
		if (strcmp(command_struct->args[i-1], "<") == 0)
			command_struct->stdin_file = command_struct->args[i];

		// If the previous token was ">" fill the stdout_file with the
		// current token
		else if (strcmp(command_struct->args[i-1], ">") == 0)
			command_struct->stdout_file = command_struct->args[i];

		// Increment i to point to the next member of the args array
		i++;
	}

	// Fill the last arg with NULL. Will be useful when calling exec funcs.
	command_struct->args[i] = NULL;

	// If the last member of args is &, set the background flag.
	if (strcmp(command_struct->args[i-1], "&") == 0)
		command_struct->background = 1;

	// At this point, we have our struct filled, but our args array is over-filled.
	// We need to remove the trailing & character, if it exists, as well as <, >, 
	// and their associated filenames. These must be removed because they should
	// not be in the array that is passed to exec. We start by looping through
	// the array. Note that i is the index of the NULL ptr marking the array's end.
	for (j=0; j<i; j++)	
	{
		// If we find an &, treat it as a normal arg and don't delete it
		// unless it's the last arg in the array.
		if (strcmp(command_struct->args[j], "&") == 0)
		{
			if (command_struct->args[j+1] == NULL)
			{
				free(command_struct->args[j]);
				command_struct->args[j] = NULL;
			}
		}

		// If we find a < or >, according to the assignment rubric nothing
		// after those symbols will be an arg, so we can delete every array
		// element until the end of the array.
		else if ((strcmp(command_struct->args[j], "<") == 0) ||
			(strcmp(command_struct->args[j], ">") == 0))
		{
			// This loop executes if we found < or >, setting all args after
			// to NULL.
			while (j<i)
			{
				command_struct->args[j] = NULL;
				j++;
			}
		}
	}
}
