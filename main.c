// UCLA CS 111 Lab 1 main program

#include <errno.h>
#include <error.h>
#include <getopt.h>
#include <stdio.h>

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "command.h"
#include "command-internals.h"

struct high_lvl_command
{
    command_t command;
    char **inputs;
    char **outputs;
};

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

// traverse high lvl command to prep io for dependency graph
void consolidate_io(char **input, char**output)
{
    return;
};

command_t execute_command_parallel ( int** dep_graph, high_lvl_command_t commands, int num_commands );

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
    
    
    
    //TODO:figure out how this whole thing will work???
	int num_commands = 0;    
	while ((command = read_command_stream (command_stream)))
    {
        num_commands++;
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

    //if we're not in parallel mode we're done
    if( !time_travel )
        exit(0);
    
/**************** time travel logic **************/
    //define and allocate array of high lvl commands
    high_lvl_command_t command_list = malloc(sizeof(struct high_lvl_command) * num_commands);
    
    int k;
    //populate high lvl commands with command ptrs and input/output lists
    for (k = 0; (command = read_command_stream (command_stream)); k++)
    {
            command_list[k].command = command;
            consolidate_io(command_list[k].inputs,command_list[k].outputs);
    } 
    
    
    //set dependency graph
    int dep_graph[num_commands][num_commands];
    last_command = execute_command_parallel ( (int**) dep_graph, command_list, num_commands );


    return print_tree || !last_command ? 0 : command_status (last_command);
}

command_t execute_command_parallel ( int** dep_graph, high_lvl_command_t commands, int num_commands )
{
    command_t command;
    command_t last_command;
    
    pid_t pid;
    pid_t pid_array[num_commands];      //which processes are being run
    int pid_index = 0;

    int i;
    int idx;
    int dependant_flag = 0;
    int run_flag = 0;
    for( idx = 0; idx <= num_commands; idx++ )
    {
        command = commands[idx].command;
        last_command = command;
        dependant_flag = 0;

        //check for a dependancy
        for ( i = 0; i <= pid_index; i++)
        {
            if ( dep_graph[idx][i] != 0 )
            {
                dependant_flag = 1;
            }
        }
        if (dependant_flag == 1)
        {
            continue;
        }
        run_flag = 1;
        // unset any dependants on this process 
        for ( i = idx; i < num_commands; i++ )
        {
            dep_graph[i][idx] = 0;
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

    if ( run_flag == 1 )
    {
        last_command = execute_command_parallel( dep_graph, commands, num_commands );
    }


    return last_command;  
}
