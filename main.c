// UCLA CS 111 Lab 1 main program

#include <errno.h>
#include <error.h>
#include <getopt.h>
#include <stdio.h>

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

#include "command.h"
#include "command-internals.h"

//TODO: dynamic resizing io arrays
int default_io_num = 10000;
int cur_inputs_index;
int cur_outputs_index;

struct high_lvl_command
{
    command_t command;
    char *inputs[10000];
    char *outputs[10000];
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
void consolidate_io (command_t command, char** inputs, char** outputs);

// return 1 if arrays have words in common, 0 otherwise
int common_word(char **arr1, char ** arr2);

// recursive command to do bulk work of parallel execution
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
    
   //define and allocate array of high lvl commands
	int num_hl_commands = 100;//default
	high_lvl_command_t command_list = malloc(sizeof(struct high_lvl_command) * num_hl_commands);

	int num_commands = 0;    
    
    //TODO:figure out how this whole thing will work???
	while ((command = read_command_stream (command_stream)))
    {
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
        else	//time travel
        {
      //populate high lvl commands with command ptrs and input/output list
			if( num_commands  >= num_hl_commands)
			{
				num_hl_commands += 100;
				command_list = realloc(command_list, 
						sizeof(struct high_lvl_command) * num_hl_commands);
            }
			command_list[num_commands].command = command;
			cur_inputs_index = 0;
			cur_outputs_index = 0;

			consolidate_io(command_list[num_commands].command,
				command_list[num_commands].inputs,
				command_list[num_commands].outputs);
        }
		num_commands++;
    }

    //if we're not in parallel mode we're done
    if( !time_travel )
    	return print_tree || !last_command ? 0 : command_status (last_command);
    
/**************** time travel logic **************/
    
    //set dependency graph
    int i,j;
    int* dep_graph[num_commands];
    for ( i = 0; i < num_commands; i++)
    {
        dep_graph[i] = malloc( sizeof(int) * num_commands );
        memset(dep_graph[i], 0, num_commands);
    }
	
	for( i = 0; i < num_commands; i++ )
	{
		for(j = 0; j < i; j++)
		{
			if(j == i)
			{
				continue;
			}
			if(common_word(command_list[i].outputs,
							command_list[j].inputs) == 1 ||
				common_word(command_list[i].inputs,
							command_list[j].outputs) == 1 ||
				common_word(command_list[i].outputs,
							command_list[j].outputs) == 1)
			{
				dep_graph[i][j] = 1;
			}
		}	
	}
    
	last_command = execute_command_parallel ( dep_graph, command_list, num_commands );

    return print_tree || !last_command ? 0 : command_status (last_command);
}

command_t execute_command_parallel ( int** dep_graph, high_lvl_command_t commands, int num_commands )
{
    command_t command;
    command_t last_command;
    
    pid_t pid;
    pid_t pid_array[num_commands];      //which processes are being run

    int i;
    int idx;
    int dependant_flag = 0;
    int run_again_flag = 0;
    for( idx = 0; idx < num_commands; idx++ )
    {
        if ( idx >= num_commands )
            break;
        if ( dep_graph[idx][idx] == 1)      //process has already run
        {
            continue;
        }
        command = commands[idx].command;
        last_command = command;
        dependant_flag = 0;

        //check for a dependancy
        for ( i = 0; i < idx; i++)
        {
            if ( dep_graph[idx][i] != 0 )
            {
                dependant_flag = 1;
                run_again_flag = 1;
            }
        }
        if (dependant_flag == 1)
        {
            continue;
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
                dep_graph[idx][idx] = 1; //mark this command has run
                pid_array[idx] = pid;
                continue;
        };
    }

    for ( i = 0; i < num_commands; i++)
    {
        if ( dep_graph[idx][idx] == 1 )
        {
            // unset any dependants on this process 
            for ( idx = i; i < num_commands; i++ )
            {
                dep_graph[idx][i] = 0;
            }
        }

        if ( pid_array[i] != 0 )
        {
            //wait for the process to finish    
            waitpid( pid_array[i], &(command->status), 0 );
            //remove dependancies on the process
            for ( idx = 0; idx < num_commands; idx++ )
            {
                if ( idx != i )     //don't overwrite run files
                    dep_graph[idx][i] = 0;
            }
        }
    }

    if ( run_again_flag == 1 )
    {
        last_command = execute_command_parallel( dep_graph, commands, num_commands );
    }


    return last_command;  
}

void consolidate_io (command_t command, char** inputs, char** outputs)
{
	switch(command->type)
	{
		case SIMPLE_COMMAND:
			if(cur_inputs_index > default_io_num || cur_outputs_index > default_io_num)
			{
				fprintf(stderr,"toooooo big");
				exit(1);
			}
			if(command->input != NULL)
			{
				inputs[cur_inputs_index] = command->input;
				cur_inputs_index++;
			}
			if(command->output != NULL)
			{
				outputs[cur_outputs_index] = command->output;
				cur_outputs_index++;
			}				
			break;	
		case SUBSHELL_COMMAND:
			consolidate_io(command->u.subshell_command,inputs,outputs);
			break;
		default:
			consolidate_io(command->u.command[0],inputs,outputs);
			consolidate_io(command->u.command[1],inputs,outputs);
			break;
	}
};


int common_word(char **arr1, char ** arr2)
{
	int i,j;
	for(i = 0; i < default_io_num; i++)
	{
		if( arr1[i] == NULL)
		{
			break;
		}
		for(j = 0; j < default_io_num; j++)
		{
			if(arr2[j] == NULL)
			{
				break;
			}
			if(strcmp(arr1[i],arr2[j]) == 0)
			{
				return 1;
			}
		}
	}
	return 0;
}






