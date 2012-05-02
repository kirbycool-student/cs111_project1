// UCLA CS 111 Lab 1 command execution

#include "command.h"
#include "command-internals.h"

#include <error.h>
#include <stdio.h>
#include <stdlib.h>     //exit
#include <unistd.h>     //execvp & fork
#include <sys/wait.h>   //waitpid
#include <sys/types.h>  //
#include <sys/stat.h>   //  } all needed for open
#include <fcntl.h>      //

int
command_status (command_t c)
{
    return c->status;
}

//TODO: implement time travel
void
execute_command (command_t c, int time_travel)
{
    pid_t pid;
    
    // command hasn't returned yet, default
    c->status = -1;
    
    // first case is simple command
    if (c->type == SIMPLE_COMMAND)
    {
		int childstatus = 0;
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
                    dup2(fd_input, 0);  // make stdin go to file
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
                //wait for child to finish before continuing
                //store childs exit status
                waitpid(pid,&childstatus,0);
                c->status = childstatus;
				break;
        }
    }
    else if ( c->type == AND_COMMAND || c->type == OR_COMMAND )
    {
        //execute left subtree
        execute_command(c->u.command[0],time_travel);
        if (c->type == AND_COMMAND)
        {
            //if true, execute right subtree
            if ( c->u.command[0]->status == 0 )
            {
                execute_command(c->u.command[1],time_travel);
                c->status = c->u.command[1]->status;
            }
            else
            {
                c->status = 1;
            }
        }
        else    //c->type == OR_COMMAND
        {
            // OR is opposite of AND
            if ( c->u.command[0]->status != 0 )
            {
                execute_command(c->u.command[1],time_travel);
                c->status = c->u.command[1]->status;
            }
            else
            {
                c->status = 0;
            }
        }
    }
    else if ( c->type == PIPE_COMMAND )
    {
        int pipe_fd[2];
        
        pipe(pipe_fd);
        
        switch (pid = fork())
        {
            case -1:
                // fork() has failed 
                fprintf(stderr,"fork failed!");
                break;
                
            case 0:     //child
            
                    //close read end
                close(pipe_fd[0]);
                    // send stdout to pipe     
                dup2(pipe_fd[1], 1);                 
                     
                    //execute left command
                execute_command(c->u.command[0],time_travel);
                    //close write end
                close(pipe_fd[1]);   
                break;
                
            default:    //parent
                            
                    //close write end
                close(pipe_fd[1]);
                    // stdin from pipe     
                dup2(pipe_fd[0], 0);    
      
                
                    //wait for left command to finish
                //waitpid(pid,&(c->status),0);     
                    //execute right command
                execute_command(c->u.command[1],time_travel);
                    //close read end
                close(pipe_fd[0]);
                
                c->status = c->u.command[1]->status;
                break;
        }  
        
    }
    else    //should never reach here
    {
        fprintf(stderr,"Improper command type to execute.\n");
        exit(1);
    }
    return;
}
