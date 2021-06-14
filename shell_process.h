#ifndef __SHELL_PROCESS_H__
#define __SHELL_PROCESS_H__

#include <signal.h>
#include <sys/types.h>
#include "list_node.h"
#include "command_info.h"

void change_dir(struct command_info* command);
void make_sigtstp_struct(struct sigaction * sig);
void sigtstp_handler(int signo);
void exit_shell(struct pid_node* list);
void make_sigint_struct(struct sigaction * sig);
void cleanup_bg(struct pid_node* list, struct pid_node** head, struct pid_node** tail);
void status(int fg_status);
struct pid_node* add_node(pid_t processID);
int fg_proc(struct command_info* command);
pid_t bg_proc(struct command_info* command);

#endif // __SHELL_PROCESS_H__
