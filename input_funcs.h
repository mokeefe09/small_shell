#ifndef __INPUT_FUNCS_H__
#define __INPUT_FUNCS_H__

#include "command_info.h"
#include <stdbool.h>

char* arg_str(void);
void strip_newline(char* string, ssize_t length);
bool comment_or_space(char* string);
char* expand_pid(char* token);
void tokenize(char* inp_str, struct command_info* command_struct);

#endif // __INPUT_FUNCS_H__
