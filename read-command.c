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
command_t add_command_subshell( int (*get_next_byte) (void *), void *stream, int subshell);
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

//todo: factor our enum again...
        
command_t add_command_subshell( int (*get_next_byte) (void *), void *stream, int subshell)
{
    command_t prev_command;
    char next_byte;
    enum command_type type;

    for ( next_byte = get_next_byte(stream); next_byte != EOF; next_byte = get_next_byte(stream))
    {
        if (next_byte == ' ')
            continue;
        else if (next_byte == ')')
        {
            if (subshell)
            {
                break;
            }
            else
            {
                ;
            }
                //TODO: some error
        }
        else if (next_byte == '(')
            prev_command = add_command_subshell(get_next_byte, stream, 1);
        else if (next_byte == '|' )
        {
            //look at next byte for or command, if not command is pipe
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
            //look at next byte for or command, if not command is pipe
            next_byte = get_next_byte(stream);
            if (next_byte == '&')
                type = AND_COMMAND;
            else
            {
                //TODO: some error
            }
            prev_command = add_command_normal(get_next_byte, stream, type, prev_command);
        }
        else if (next_byte == '\n')
        {
            fpos_t pos;
            fgetpos(stream, &pos);
            for (next_byte = get_next_byte(stream); next_byte != EOF; next_byte = get_next_byte(stream))
            {
                if (next_byte == ' ' || next_byte == '\n')
                {
                    continue;
                }
                else
                {
                    break;
                }
            }
            if(next_byte == '|' || next_byte == '&' || next_byte == ';' || next_byte == '<' || next_byte == '>')
            {
                //TODO: some error
            }
            else
            {
                type = SEQUENCE_COMMAND;
                prev_command = add_command_normal(get_next_byte, stream, type, prev_command);
            }
            fsetpos(stream, &pos);
        }

        else if (next_byte == ';')
        {
            type = SEQUENCE_COMMAND;
            prev_command = add_command_normal(get_next_byte, stream, type, prev_command); 
        }
    }
    if ( subshell )
    {
        command_t command = malloc( sizeof(struct command) );
        command->type = SUBSHELL_COMMAND;
        command->u.subshell_command = prev_command;
        return command;
    }
    else
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
            command->u.command[1] = add_command_subshell(get_next_byte, stream, 1);
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

struct command_stream
{
    command_t head;
};

command_stream_t
make_command_stream (int (*get_next_byte) (void *),
		     void *get_next_byte_argument)
{
  /* FIXME: Replace this with your implementation.  You may need to
     add auxiliary functions and otherwise modify the source code.
     You can also use external functions defined in the GNU C Library.  */
    command_stream_t command_stream = malloc( sizeof(command_stream) );
    
    command_t head_command = malloc( sizeof(struct command) );
    head_command = add_command_subshell(get_next_byte, get_next_byte_argument, 0);
    command_stream->head = head_command;

    return command_stream;

    	// write function for determining if character is in set of possible chars for word

  //error (1, 0, "command reading not yet implemented");
  //return 0;
}

command_t
read_command_stream (command_stream_t s)
{
  /* FIXME: Replace this with your implementation too.  */
  error (1, 0, "command reading not yet implemented");
  return 0;
}
