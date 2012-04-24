// UCLA CS 111 Lab 1 command execution

#include "command.h"
#include "command-internals.h"

#include <error.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

/* FIXME: You may need to add #include directives, macro definitions,
   static function definitions, etc.  */

int
command_status (command_t c)
{
    return c->status;
}

void
execute_command (command_t c, int time_travel)
{
    pid_t pid;
    
    if (c->type == SIMPLE_COMMAND)
    {
        switch (pid = fork())
        {
            case -1:
                // fork() has failed 
                fprintf(stderr,"fork failed!");
                break;
                
            case 0:
                // processed by the child 
                //check command input
                if (c->input != NULL)
                {
                    int fd_input = open(c->input, O_RDWR , 0666);
                    if (fd_input == -1)
                    {
                        fprintf(stderr,"input open failed!");
                    }
                    dup2(fd_input, 0);// make stdin go to file
                    close(fd_input);
                }
                //check command output
                if (c->output != NULL)
                {
                    int fd_output = open(c->output, O_RDWR | O_CREAT, 0666);
                    if (fd_output == -1)
                    {
                        fprintf(stderr,"output open failed!");
                    }
                    dup2(fd_output, 1);   // make stdout go to file
                    close(fd_output);
                }
                
                //execute shell command
                execvp(c->u.word[0], c->u.word);
                
                //should never reach this point
                fprintf(stderr,"execvp() has failed!");
                exit(1);
                break;
                
            default:
                // processed by the parent 
                //TODO: this 1b implementation is linear
                //wait for child to finish before continuing
                waitpid(pid,NULL,0);
                break;
        }
    }    
    return;
}
