// UCLA CS 111 Lab 1 command reading

#include "command.h"
#include "command-internals.h"

#include <error.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>    //for boolean flags
#include <ctype.h>      //for isalnum, isspaces, etc

/////////////       GENERAL NOTES     /////////////

// add_command_sequence adds a complete command
// add_command_normal adds a high level AND/OR command

/////////////       FUNCTION PROTOTYPES     /////////////

bool is_word_char( char c );      //returns true if legal word char
char skip_comment( int (*get_next_byte) (void *), void *stream );   //gets bytes until newline
command_t traverse_stream( command_t head, bool *subtree_complete );    //returns ptr to high lvl AND/OR in tree, and nullifies tree ptrs

command_t add_command_simple( int (*get_next_byte) (void *), void *stream );
command_t add_command_subshell( int (*get_next_byte) (void *), void *stream, bool subshell );
command_t add_command_normal( int (*get_next_byte) (void *), void *stream, enum command_type type, command_t prev_command );

/////////////       GLOBAL VARIABLES     /////////////

int error_line_number;  //for outputing the error line number

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

char skip_comment( int (*get_next_byte) (void *), void *stream)
{
    char byte;
    while(true)
    {
        byte = get_next_byte(stream);
        if(byte == EOF)
        {
            break;
        }
        else if(byte == '\n')
        {
            break;
        }
     }
     return byte;
}


command_t traverse_stream( command_t head, bool *subtree_complete )
{
    command_t command_ptr;
    
    if ( head == NULL )
    {
        return NULL;
    }
    else if ( head->type == SEQUENCE_COMMAND )
    {
        if (head->u.command[0] != NULL)
        {
            command_ptr = traverse_stream(head->u.command[0],subtree_complete);
            if( *subtree_complete )
            {
                free(head->u.command[0]);
                head->u.command[0] = NULL;
            }
            *subtree_complete = false;
            return command_ptr;
        }
        else if ( head->u.command[1] != NULL )
        {
            command_ptr = traverse_stream(head->u.command[1],subtree_complete);
            if( *subtree_complete )
            {
                free(head->u.command[1]);
                head->u.command[1] = NULL;
            }
            *subtree_complete = true;
            return command_ptr;
        }
        else    //empty sequence command
        {
            *subtree_complete = true;
            return NULL;
        }
    }
    else if (head->type == SUBSHELL_COMMAND)
    {
        if( head->u.subshell_command != NULL )
        {
            command_ptr = traverse_stream(head->u.subshell_command,subtree_complete);
            if( *subtree_complete )
            {
                free(head->u.subshell_command);
                head->u.subshell_command = NULL;
            }
            *subtree_complete = true;
            return command_ptr;
        }
        else    // empty subshell command
        {
            *subtree_complete = true;
            return NULL;
        }
    }
    else    // normal command
    {
        *subtree_complete = true;
        return head;
    }
}

int free_command(command_t head)   //frees high lvl and/ors
{
    if ( head->type == SIMPLE_COMMAND )
    {
        free(head);
        return true;
    }
    else if ( head->type == AND_COMMAND || 
              head->type == OR_COMMAND || 
              head->type == PIPE_COMMAND    )   // and, or, pipe
    {
        free_command(head->u.command[0]);
        free_command(head->u.command[1]);
        free(head);
        return true;
    }
    else    //this should never happen
    {
        fprintf(stderr,"free_command: incorrect type or misformatted tree.\n");
        return false;
    }
}

command_t add_command_simple( int (*get_next_byte) (void *), void *stream)
{
    //fprintf(stdout,"beginning add_command_simple\n");   //TODO:remove debugging print
    command_t command = malloc(sizeof(struct command));
    command->type = SIMPLE_COMMAND;
    
    const int default_num_words = 5;
    const int default_word_size = 20;
    
    int number_words = default_num_words;   //starting num strings in array
    int word_size = default_word_size;      //num chars in string
    int words_index = 0;                    //which word we are adding to
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
        if ( next_byte == '#')
        {
            next_byte = skip_comment(get_next_byte, stream);
            fgetpos(stream, &pos);
            if (next_byte == EOF)
                break;
        }
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
            if ( strlen( word ) != 0) 
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
            if ( input_flag == true || output_flag == true || strlen( word ) == 0)  // input syntax error
            {
                fprintf(stderr,"%d: Some error input has already occured.\n", error_line_number);
                exit(1);
            }
            if ( strlen(word) != 0 )
            {
                words[words_index] = word;
            }
            command->u.word = words;
            word_size = default_word_size;
            word = (char*) malloc( sizeof(char) * word_size );
            word[0] = '\0';
            input_flag = true;
            end_word_flag = false;
        }
        else if ( next_byte == '>' )
        {
            if ( output_flag == true )  //error
            {
                    fprintf(stderr,"%d: Output has already occurred.\n", error_line_number);
                    exit(1);
            }
            if ( strlen(word) != 0)
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
            else    //error
            {
                    fprintf(stderr,"%d: I/O Syntax error.\n", error_line_number);
                    exit(1);
            }
            word_size = default_word_size;
            word = (char *) malloc( sizeof(char) * word_size);
            word[0] = '\0';
            output_flag = true;
            end_word_flag = false;
        }
        else if ( is_word_char(next_byte) )
        {
            if (end_word_flag)
            {
                if ( output_flag || input_flag )    // error: both flags set
                {
                    fprintf(stderr,"%d: More than one word after an I/O token.\n", error_line_number);
                    exit(1);
                }
                if ( words_index == (number_words-1) )           //resize
                {
                    number_words *= 2;
                    words = realloc( words, sizeof(char*) * number_words );
                }
                words[words_index] = word;
                word_size = default_word_size;
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
            strcat(word, &next_byte);                       
        }
        else    //error: some strange character
        { 
                fprintf(stderr,"%d: Illegal character used.\n", error_line_number);
                exit(1);
        }

    }
    command->u.word = words;
    return command;
}    
        
command_t add_command_subshell( int (*get_next_byte) (void *), void *stream, bool subshell)
{
    //fprintf(stdout,"beginning add_command_subshell\n");//TODO:remove debugging print
    command_t prev_command = malloc(sizeof(struct command));
    prev_command->type = 999;
    char next_byte;
    enum command_type type;
    fpos_t pos;
    fgetpos(stream, &pos);

    for ( next_byte = get_next_byte(stream); next_byte != EOF; next_byte = get_next_byte(stream))
    {
        if(next_byte == '#')
        {
            next_byte = skip_comment( get_next_byte, stream );
            fgetpos( stream, &pos);
            if (next_byte == EOF)
            {
                fseek( stream, -1, 1);
                break;
            }
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
            else    //error
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
            else    //error
            {
                fprintf(stderr,"%d: Single '&' encountered.\n", error_line_number);
                exit(1);
            }
            prev_command = add_command_normal(get_next_byte, stream, type, prev_command);
        }
        else if (next_byte == '\n')
        {
            error_line_number++;
            fgetpos(stream, &pos);
            for (next_byte = get_next_byte(stream); next_byte != EOF; next_byte = get_next_byte(stream))
            {
                if (next_byte == '#')
                {
                    next_byte = skip_comment( get_next_byte, stream );
                    fgetpos( stream, &pos);
                    if (next_byte == EOF)
                    {
                        fseek( stream, -1, 1);
                        break;
                    }
                }
                if (next_byte == ' ' || next_byte == '\t')
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
            if( ! isspace(next_byte) && ! is_word_char(next_byte))  //error
            {
                fprintf(stderr,"%d: Newline followed by improper character.\n", error_line_number);
                exit(1);
            }
            else
            {
                fsetpos( stream, &pos );
                if (prev_command->type == 999)
                {
                    prev_command = add_command_simple(get_next_byte, stream);
                }
                else
                {
                    type = SEQUENCE_COMMAND;
                    prev_command = add_command_normal(get_next_byte, stream, type, prev_command);
                }
            }
        }
        else if (next_byte == ';')  //TODO: optional semicolon for statements
        {
            type = SEQUENCE_COMMAND;
            prev_command = add_command_normal(get_next_byte, stream, type, prev_command); 
        }
        else if ( is_word_char(next_byte) )
        {
            fsetpos( stream, &pos );
            prev_command = add_command_simple(get_next_byte, stream);
        }
        else    //some strange character  error
        {
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
    //fprintf(stdout,"beginning add_command_normal\n");//TODO:remove debugging print
    if(type == PIPE_COMMAND && (prev_command->type != SIMPLE_COMMAND ||
         prev_command->type != SUBSHELL_COMMAND) )
    {
        fprintf(stderr,"%d: Pipe command not preceeded by simple command or subshell command.\n", error_line_number);
        exit(1);
    }
 
    command_t command = malloc(sizeof(struct command));
    command->type = type;
    command->u.command[0] = prev_command;

    fpos_t pos;

    char next_byte;
    fgetpos( stream, &pos);
    for (next_byte = get_next_byte(stream); next_byte != EOF; next_byte = get_next_byte(stream))
    {
        if ( next_byte == '#')
        {
            next_byte = skip_comment(get_next_byte, stream);
            fgetpos( stream, &pos);
            if (next_byte == EOF)
            {
                fseek( stream, -1, 1);
                break;
            }
        }
        if (next_byte == ' ' || next_byte == '\t')
        {
            continue;
        }
        else if (next_byte == '(')
        {
            fsetpos( stream, &pos);
            command->u.command[1] = add_command_subshell(get_next_byte, stream, 1);
            break;
        }
        else if ( is_word_char(next_byte) )
        {
            fsetpos( stream, &pos);
            command->u.command[1] = add_command_simple(get_next_byte, stream);
            break;
        }
        else    //error
        {
            fprintf(stderr,"%d: Normal command not followed by simple command or subshell command.\n", error_line_number);
            exit(1);
        }

        fgetpos( stream, &pos );
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
   // fprintf(stdout,"beginning make_command_stream\n");//TODO:remove debugging print

    command_stream_t command_stream = malloc( sizeof(command_stream) );
    command_t head_command = malloc( sizeof(struct command) );
    
    head_command = add_command_subshell(get_next_byte, get_next_byte_argument, false);
    
    if( head_command != NULL)
    {
        command_stream->head = head_command;
        return command_stream;
    }
    else
    {
            fprintf(stderr,"%d: Error in construction of command stream.\n", error_line_number);
            exit(1);
    }
}

command_t read_command_stream (command_stream_t s)
{
    //fprintf(stdout,"beginning read_command_stream\n");//TODO:remove debugging print
    bool tree_complete;
    command_t command_ptr;
    command_ptr = traverse_stream( s->head, &tree_complete );
    if ( tree_complete ) 
    {   
        free(s->head);
        s->head = NULL;
    }
    return command_ptr;
}
