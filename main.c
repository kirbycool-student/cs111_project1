// UCLA CS 111 Lab 1 main program

#include <errno.h>
#include <error.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

#include "command.h"

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
    
    int k;
    //populate high lvl commands with command ptrs and input/output lists
    for (k = 0; (command = read_command_stream (command_stream)); k++)
    {
            if( k >= num_hl_commands)
            {
                num_hl_commands += 100;
                command_list = realloc(command_list, sizeof(struct high_lvl_command) * num_hl_commands);
            }
            command_list[k].command = command;
            consolidate_io(command_list[k].inputs,command_list[k].outputs);
    }
    
    //TODO:figure out how this whole thing will work???
    while ((command = read_command_stream (command_stream)))
    {
        if (print_tree)
        {
            printf ("# %d\n", command_number++);
            print_command (command);
            free_command(command);    //unallocate all commands beneath this one
        }
        else
        {
            last_command = command;
            execute_command (command, time_travel);
        }
    }

    return print_tree || !last_command ? 0 : command_status (last_command);
}
