#ifndef __LIST_NODE_H__
#define __LIST_NODE_H__

#include <sys/types.h>

struct pid_node {
	pid_t pid;
	struct pid_node* next;
};

#endif
