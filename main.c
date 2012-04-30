// UCLA CS 111 Lab 1 main program

#include <errno.h>
#include <error.h>
#include <getopt.h>
#include <stdio.h>

#include "command.h"
#include "command-internals.h"
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

static char const *program_name;
static char const *script_name;

static void
usage (void)
{
    error (1, 0, "usage: %s [-pt] SCRIPT-FILE", program_name);
}

static int
get_next_byte (void *stream)
{
    return getc (stream);
}

command_t execute_command_parallel ( int** dep_graph, command_stream_t stream, int number_commands );


int
main (int argc, char **argv)
{
    int opt;
    int command_number = 1;
    int print_tree = 0;
    int time_travel = 0;
    program_name = argv[0];

    for (;;)
        switch (getopt (argc, argv, "pt"))
        {
            case 'p': print_tree = 1; break;
            case 't': time_travel = 1; break;
            default: usage (); break;
            case -1: goto options_exhausted;
        }
        options_exhausted:;

  // There must be exactly one file argument.
    if (optind != argc - 1)
        usage ();

    script_name = argv[optind];
    FILE *script_stream = fopen (script_name, "r");
    if (! script_stream)
    error (1, errno, "%s: cannot open", script_name);
    command_stream_t command_stream =
        make_command_stream (get_next_byte, script_stream);

    command_t last_command = NULL;
    command_t command;

    int number_commands = 0;
    while ((command = read_command_stream (command_stream)))
    {
        number_commands++;
        if (print_tree)
        {
            printf ("# %d\n", command_number++);
            print_command (command);
            free_command(command);    //unallocate all commands beneath this one
        }
        else if (!time_travel)
        {
            last_command = command;
            execute_command (command, time_travel);
        }
        else
        {
            continue;
        }
    }
      
    /******** time travel logic **************/
    
    
    //set dependency graph
    int dep_graph[number_commands][number_commands];
    last_command = execute_command_parallel ( (int**) dep_graph, command_stream, number_commands );


    return print_tree || !last_command ? 0 : command_status (last_command);
}

command_t execute_command_parallel ( int** dep_graph, command_stream_t stream, int number_commands )
{
    command_t command;
    command_t last_command;
    
    pid_t pid;
    pid_t pid_array[number_commands];
    int pid_index = 0;

    int idx = 0;
    int i;
    int dependant_flag = 0;
    while( (command = read_command_stream(stream)) )
    {
        last_command = command;
        dependant_flag = 0;
        //check for a dependancy
        for ( i = 0; i < idx + 1; i++)
        {
            if ( dep_graph[idx][i] != 0 )
            {
               dependant_flag = 1;
            }
        }
        switch( pid = fork() )
        {
            case -1:
                //some error
                ;
            case 0:
                //execute the command
                execute_command( command, 0 );
                exit(0);
            default:
                pid_array[pid_index] = pid;
                pid_index++;
                continue;
        };
    }

    for ( i = 0; i <= pid_index; i++)
    {
        waitpid( pid_array[pid_index], &(command->status), 0 );
    }

    return last_command;  
}
