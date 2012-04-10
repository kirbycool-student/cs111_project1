// UCLA CS 111 Lab 1 command reading

#include "command.h"
#include "command-internals.h"

#include <error.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* FIXME: You may need to add #include directives, macro definitions,
   static function definitions, etc.  */

command_t add_command_normal( int (*get_next_byte) (void *), void *stream, enum command_type type, command_t prev_command);
command_t add_command_subshell( int (*get_next_byte) (void *), void *stream);
command_t add_command_simple( int (*get_next_byte) (void *), void *stream);

command_t add_command_simple( int (*get_next_byte) (void *), void *stream)
{
    command_t command = malloc(sizeof(struct command));
    command->type = SIMPLE_COMMAND;

    char* word = malloc( sizeof(char) * 1024);      //TODO: implement dynamic resizing string
    char next_byte;
    fpos_t pos;
    fgetpos(stream, &pos);
    for (next_byte = get_next_byte(stream); next_byte != EOF; next_byte = get_next_byte(stream))
    {
        if ( next_byte =='|' || next_byte == '&' || next_byte == ';')
        {    
            fsetpos(stream, &pos);
            break;
        }
        else
            strcat(word, &next_byte);           //TODO: resize if array too short
    }

    command->u.word = &word;

    return command;
}

        
command_t add_command_subshell( int (*get_next_byte) (void *), void *stream)
{
    command_t prev_command;
    char next_byte;

    for ( next_byte = get_next_byte(stream); next_byte != EOF; next_byte = get_next_byte(stream))
    {
        if (next_byte == ' ')
            continue;
        else if (next_byte == ')')
            break;
        else if (next_byte == '(')
            prev_command = add_command_subshell(get_next_byte, stream);
        else if (next_byte == '|' )
        {
            enum command_type type;          //look at next byte for or command, if not command is pipe
            fpos_t pos;
            fgetpos(stream, &pos);
            next_byte = get_next_byte(stream);
            if (next_byte == '|')
                type = OR_COMMAND;
            else
            {
                type = PIPE_COMMAND;
                fsetpos(stream, &pos);   //move the file pointer back
            }
            prev_command = add_command_normal(get_next_byte, stream, type, prev_command);
        }
        else if (next_byte == '&')
        {
            enum command_type type;          //look at next byte for or command, if not command is pipe
            next_byte = get_next_byte(stream);
            if (next_byte == '&')
                type = AND_COMMAND;
            else
            {
                //TODO: some error
            }
            prev_command = add_command_normal(get_next_byte, stream, type, prev_command);
        }
        else if (next_byte == ';')
        {
            enum command_type type = SEQUENCE_COMMAND;
            prev_command = add_command_normal(get_next_byte, stream, type, prev_command); 
        }
    }

    return prev_command;
}



command_t add_command_normal ( int (*get_next_byte) (void *), void *stream, enum command_type type, command_t prev_command)
{
    command_t command = malloc(sizeof(struct command));
    command->type = type;
    command->u.command[0] = prev_command;

    char next_byte;
    for (next_byte = get_next_byte(stream); next_byte != EOF; next_byte = get_next_byte(stream))
    {
        if (next_byte == ' ')
            continue;
        else if (next_byte == '(')
        {
            command->u.command[1] = add_command_subshell(get_next_byte, stream);
            break;
        }
        else
        {
            command->u.command[1] = add_command_simple(get_next_byte, stream);
            break;
        }
    }

    
    return command;
}
    


/* FIXME: Define the type 'struct command_stream' here.  This should
   complete the incomplete type declaration in command.h.  */

command_stream_t
make_command_stream (int (*get_next_byte) (void *),
		     void *get_next_byte_argument)
{
  /* FIXME: Replace this with your implementation.  You may need to
     add auxiliary functions and otherwise modify the source code.
     You can also use external functions defined in the GNU C Library.  */

    command_t prev_command;
    char next_byte;
    enum command_type type;

    for ( next_byte = get_next_byte(get_next_byte_argument); next_byte != EOF; next_byte = get_next_byte(get_next_byte_argument))
    {
        if (next_byte == ' ')
		{
            continue;
		}
        else if (next_byte == '(')
		{
            prev_command = add_command_subshell(get_next_byte, get_next_byte_argument);
		}
    	else if (next_byte == '|' )
        {
            fpos_t pos;
            fgetpos(get_next_byte_argument, &pos);	//look at next byte for OR command, if not command is pipe
            next_byte = get_next_byte(get_next_byte_argument);
            if (next_byte == '|')
			{
                type = OR_COMMAND;
			}
            else
            {
                type = PIPE_COMMAND;
                fsetpos(get_next_byte_argument, &pos);   //move the file pointer back
            }
            prev_command = add_command_normal(get_next_byte, get_next_byte_argument, type, prev_command);
        }
        else if (next_byte == '&')
        {  
            next_byte = get_next_byte(get_next_byte_argument); //look at next byte for or command, if not command is pipe
            if (next_byte == '&')
			{
                type = AND_COMMAND;
			}            
			else
            {
                //TODO: some error
            }
            prev_command = add_command_normal(get_next_byte, get_next_byte_argument, type, prev_command);
        }
        else if (next_byte == ';')	// will need more cases for other character types... ex: #, * etc
        {
            type = SEQUENCE_COMMAND;
            prev_command = add_command_normal(get_next_byte, get_next_byte_argument, type, prev_command); 
        }
    }

  error (1, 0, "command reading not yet implemented");
  return 0;
}

command_t
read_command_stream (command_stream_t s)
{
  /* FIXME: Replace this with your implementation too.  */
  error (1, 0, "command reading not yet implemented");
  return 0;
}
