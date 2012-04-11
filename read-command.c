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
command_t add_command_subshell( int (*get_next_byte) (void *), void *stream, bool subshell);
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
    fprintf(stdout,"beginning add_command_simple\n");   //TODO:remove debugging print
    command_t command = malloc(sizeof(struct command));
    command->type = SIMPLE_COMMAND;
    
    int number_words = 5;   //staring num strings in array
    int word_size = 20;     //num chars in string
    int words_index = 0;       //which word we are adding to
    char** words = (char**) malloc( sizeof(char*) * number_words );      //DONE: implement dynamic resizing string
    char* word = malloc( sizeof(char) * word_size);
    word[0] = '\0';
    char next_byte;
    fpos_t pos;
    fgetpos(stream, &pos);
    
    //TODO: implement I/O
    // change word implementation
    
    bool input_flag = false;
    bool output_flag = false;
    bool end_word_flag = false;
    
    for ( next_byte = get_next_byte(stream); next_byte != EOF; next_byte = get_next_byte(stream) )
    {
        fgetpos(stream, &pos);
        if ( next_byte == ' ' || next_byte == '\t' )        //word ended
        {
            if ( strlen(word) == 0 || end_word_flag)
            {
                continue;
            }
            else
            {
                end_word_flag = true;
            }
        }
        else if ( next_byte =='|' || next_byte == '&' || next_byte == ';' || next_byte == '\n')
        {   
            if ( next_byte == '\n')
            {
                error_line_number++;
            }
            if ( end_word_flag) 
            {
                if (output_flag)
                {
                    command->output = word;
                }
                else if (input_flag)
                {
                    command->input = word;
                }
                else
                {
                    words[words_index] = word;
                }
            }
            fsetpos(stream, &pos);
            break;
        }
        else if ( next_byte == '<' )
        {
            if ( input_flag == true || output_flag == true || strlen( word ) == 0)
            {
                ; //TODO: some error input has already occured
            }
            if ( end_word_flag )
            {
                words[words_index] = word;
            }
            command->u.word = words;
            word_size = 20;
            word = (char*) malloc( sizeof(char) * word_size );
            word[0] = '\0';
            input_flag = false;
            end_word_flag = false;
        }
        else if ( next_byte == '>' )
        {
            if ( output_flag == true )
            {
                ; //TODO: some error output has already occured
            }
            if ( end_word_flag )
            {
                if ( input_flag )
                {
                    command->input = word;
                }
                else
                {
                    words[words_index] = word;
                    command->u.word = words;
                }            
            }
            else
            {
                ; //TODO: some error
            }
            word_size = 20;
            word = (char *) malloc( sizeof(char) * word_size);
            word[0] = '\0';
            output_flag = true;
            end_word_flag = false;
        }
        else if ( is_word_char(next_byte) )
        {
            if (end_word_flag)
            {
                if ( output_flag || input_flag )
                {
                    //TODO: some error. there is more than one word after an I/O token;
                }
                if ( words_index == (number_words-1) )           //resize
                {
                    number_words *= 2;
                    words = realloc( words, sizeof(char*) * number_words );
                }
                words[words_index] = word;
                word_size = 20;
                word = (char*) malloc(sizeof(char) * word_size);
                word[0] = '\0';                 //rest stuff for the next word
                words_index++;
                end_word_flag = false;
            }
            //TODO: resize if array too short
            if ( (int) strlen( word ) == word_size-1)    //resize word
            {
                word_size *= 2;
                words[words_index] = realloc( words[words_index], sizeof(char*) * word_size );
            }
            strcat(words[words_index], &next_byte);                       
        }
        else
        { 
            //TODO: error invalid char
        }

    }
    command->u.word = words;
    return command;
}    
        
command_t add_command_subshell( int (*get_next_byte) (void *), void *stream, bool subshell)
{
    fprintf(stdout,"beginning add_command_subshell\n");//TODO:remove debugging print
    command_t prev_command;
    char next_byte;
    enum command_type type;

    for ( next_byte = get_next_byte(stream); next_byte != EOF; next_byte = get_next_byte(stream))
    {
        if(next_byte == '#')
        {
            char byte;
            while(true)
            {
                byte = get_next_byte(stream);
                if(byte == EOF)
                {
                    next_byte = EOF;
                    break;
                }
                else if(byte == '\n')
                {
                    error_line_number++;
                    break;
                }
            }
            continue;
        }
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
                fprintf(stderr,"%d: Character ')' encountered outside of a subshell.\n", error_line_number);
                exit(1);
            }
        }
        else if (next_byte == '(')
        {
            prev_command = add_command_subshell(get_next_byte, stream, true);
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
                //done: some error
                fprintf(stderr,"%d: Single '&' encountered.\n", error_line_number);
                exit(1);
            }
            prev_command = add_command_normal(get_next_byte, stream, type, prev_command);
        }
        else if (next_byte == '\n')
        {
            error_line_number++;
            fpos_t pos;
            fgetpos(stream, &pos);
            for (next_byte = get_next_byte(stream); next_byte != EOF; next_byte = get_next_byte(stream))
            {
                if (next_byte == ' ' || next_byte == '\t' )
                {
                    continue;
                }
                else if ( next_byte == '\n' )
                {
                    error_line_number++;
                    continue;
                }
                else
                {
                    break;
                }
            }
            if( ! isspace(next_byte) && ! is_word_char(next_byte))
            {
                //done: some error
                fprintf(stderr,"%d: Newline followed by improper character.\n", error_line_number);
                exit(1);
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
               //done: error
            fprintf(stderr,"%d: Illegal character used.\n", error_line_number);
            exit(1);
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
    fprintf(stdout,"beginning add_command_normal\n");//TODO:remove debugging print

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
            //done:ERROR
            fprintf(stderr,"%d: Normal command not followed by simple command or subshell command.\n", error_line_number);
            exit(1);
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
    fprintf(stdout,"beginning make_command_stream\n");//TODO:remove debugging print

    command_stream_t command_stream = malloc( sizeof(command_stream) );
    
    command_t head_command = malloc( sizeof(struct command) );
    head_command = add_command_subshell(get_next_byte, get_next_byte_argument, false);
    command_stream->head = head_command;

    return command_stream;
}

command_t read_command_stream (command_stream_t s)
{
    fprintf(stdout,"beginning read_command_stream\n");//TODO:remove debugging print
    return s->head;
}
