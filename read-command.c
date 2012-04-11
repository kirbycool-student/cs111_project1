// UCLA CS 111 Lab 1 command reading

#include "command.h"
#include "command-internals.h"

#include <error.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>    //for boolean flags
#include <ctype.h>      //for isalnum, isspaces, etc

/////////////       FUNCTION PROTOTYPES     /////////////

bool is_word_char( char c );
command_t add_command_simple( int (*get_next_byte) (void *), void *stream);
command_t add_command_subshell( int (*get_next_byte) (void *), void *stream, int subshell);
command_t add_command_normal( int (*get_next_byte) (void *), void *stream, enum command_type type, command_t prev_command);

/////////////       GLOBAL VARIABLES     /////////////

int error_line_number;

/////////////       FUNCTION DEFINITIONS     /////////////

bool is_word_char( char c )    //checks if c is in the subset of command word characters
{
    if( isalnum(c) )    //alphanumeric == letter or digits
    {
        return true;
    }
    else if (    c == '!' || c == '%' || c ==  '+' || c ==  ','  || c == '-'  || c == '.'  || 
                c == '/'  || c == ':'  || c == '@'  || c == '^' || c ==  '_'    )
    {
        return true;
    }
    else    // c cannot be part of a word
    {    
        return false;
    }
}

command_t add_command_simple( int (*get_next_byte) (void *), void *stream)
{
    command_t command = malloc(sizeof(struct command));
    command->type = SIMPLE_COMMAND;

    char* word = malloc( sizeof(char) * 1024);      //TODO: implement dynamic resizing string
    char next_byte;
    fpos_t pos;
    fgetpos(stream, &pos);
    
    //TODO: implement I/O
    // change word implementation
    
    bool input_flag = false;
    bool output_flag = false;

    for ( next_byte = get_next_byte(stream); next_byte != EOF; next_byte = get_next_byte(stream) )
    {
        if ( next_byte =='|' || next_byte == '&' || next_byte == ';')
        {    
            fsetpos(stream, &pos);
            break;
        }
        else if ( next_byte == '<' )
        {
            if ( input_flag == true )
            {
                ; //TODO: some error input has already occured
            }
            char* input = malloc( sizeof(char) * strlen(word) );
            strcpy( input, word );
            command->input = input;
            word[0] = '\0';
            input_flag = false;
        }
        else if ( next_byte == '>' )
        {
            if ( output_flag == true )
            {
                ; //TODO: some error output has already occured
            }
            char* command_word = malloc( sizeof(char) * strlen(word) );
            strcpy( command_word, word ); 
            command->u.word = &command_word;
            word[0] = '\0';
            output_flag = true;
        }
        else if ( is_word_char(next_byte) )
        {
            strcat(word, &next_byte);           //TODO: resize if array too short
        }
        else
        { 
            //TODO: error
        }
    }
    if ( output_flag == true )
    {
        command->output = word;
    }
    else
    {
        command->u.word = &word;
    }
    return command;
}    
        
command_t add_command_subshell( int (*get_next_byte) (void *), void *stream, int subshell)
{
    command_t prev_command;
    char next_byte;
    enum command_type type;

    for ( next_byte = get_next_byte(stream); next_byte != EOF; next_byte = get_next_byte(stream))
    {
        if (next_byte == ' ' || next_byte == '\t')
        {
            continue;
        }
        else if (next_byte == ')')
        {
            if (subshell)
            {
                break;
            }
            else
            {
                ;                //TODO: some error
            }
        }
        else if (next_byte == '(')
        {
            prev_command = add_command_subshell(get_next_byte, stream, 1);
        }
        else if (next_byte == '|' )
        {
            //look at next byte for or command, if not command is pipe
            fpos_t pos;
            fgetpos(stream, &pos);
            next_byte = get_next_byte(stream);
            if (next_byte == '|')
            {
                type = OR_COMMAND;
            }
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
            {
                type = AND_COMMAND;
            }
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
                if (next_byte == ' ' || next_byte == '\t' || next_byte == '\n')
                {
                    continue;
                }
                else
                {
                    break;
                }
            }
            if( ! isspace(next_byte) && ! is_word_char(next_byte))
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
        else if (next_byte == ';')  //TODO: optional semicolon for statements
        {
            type = SEQUENCE_COMMAND;
            prev_command = add_command_normal(get_next_byte, stream, type, prev_command); 
        }
        else if ( is_word_char(next_byte) )
        {
            prev_command = add_command_simple(get_next_byte, stream);
        }
        else    //some strange character
        {
               //TODO: error
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
    {
        return prev_command;
    }
}



command_t add_command_normal ( int (*get_next_byte) (void *), void *stream, enum command_type type, command_t prev_command)
{
    // normal command is anything other than simple or subshell
    command_t command = malloc(sizeof(struct command));
    command->type = type;
    command->u.command[0] = prev_command;

    char next_byte;
    for (next_byte = get_next_byte(stream); next_byte != EOF; next_byte = get_next_byte(stream))
    {
        if (next_byte == ' ' || next_byte == '\t')
        {
            continue;
        }
        else if (next_byte == '(')
        {
            command->u.command[1] = add_command_subshell(get_next_byte, stream, 1);
            break;
        }
        else if ( is_word_char(next_byte) )
        {
            command->u.command[1] = add_command_simple(get_next_byte, stream);
            break;
        }
        else
        {
            //TODO:ERROR
        }
    }
    return command;
}

/////////////       COMMAND STREAM STRUCT AND FUNCTIONS     /////////////

struct command_stream
{
    command_t head;
};

command_stream_t
make_command_stream (int (*get_next_byte) (void *),
		     void *get_next_byte_argument)
{
    command_stream_t command_stream = malloc( sizeof(command_stream) );
    
    command_t head_command = malloc( sizeof(struct command) );
    head_command = add_command_subshell(get_next_byte, get_next_byte_argument, false);
    command_stream->head = head_command;

    return command_stream;
}

command_t read_command_stream (command_stream_t s)
{
    return s->head;
}
