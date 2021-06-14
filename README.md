Custom Linux shell project for Operating Systems course at Oregon State University.
Make file included to build the project. 

The program simulates the basics of a Bash shell, allowing the user to run processes,
redirect input/output, and interact with the shell and its child processes through
signals. SIGTSTP toggles foreground only mode, which, when active, prohibits 
background processes from running. SIGINT does not affect the parent process, but 
will terminate the currently running foreground child process.


