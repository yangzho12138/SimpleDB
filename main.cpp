#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
    META_COMMAND_SUCCESS,
    META_COMMAND_UNRECOGNIZED_COMMAND
}MetaCommandResult;

typedef enum {
    PREPARE_SUCCESS,
    PREPARE_UNRECOGNIZED_STATEMENT
}PrepareResult;

typedef enum {
    STATEMENT_INSERT,
    STATEMENT_SELECT
}StatementType;

typedef struct {
    char* buffer; // the command line read in the terminal
    size_t buffer_length;
    ssize_t input_length;
}InputBuffer;

typedef struct {
    StatementType type;
}Statement;

// return a pointer -> InputBuffer type variable
InputBuffer* new_input_buffer(){
    InputBuffer* input_buffer = (InputBuffer*)malloc(sizeof(InputBuffer)); // allocate space
    input_buffer->buffer = NULL;
    input_buffer->buffer_length = 0;
    input_buffer->input_length = 0;

    return input_buffer;
}

void print_prompt(){
    printf("db > ");
}

// linux system programming method (https://man7.org/linux/man-pages/man3/getline.3.html)
// lineptr: a pointer to the variable we use to point to the buffer containing the read line
// n: a pointer to the variable we use to save the size of allocated buffer.
// stream:  the input stream to read from
// return: the number of bytes read, which may be less than the allocated buffer (*n)
ssize_t getline(char **lineptr, size_t *n, FILE *stream);

// get the standard input (stdin)
void read_input(InputBuffer* input_buffer){
    ssize_t bytes_read = getline(&(input_buffer->buffer), &(input_buffer->buffer_length), stdin);

    if(bytes_read <= 0){
        printf("Error reading input\n");
        exit(EXIT_FAILURE);
    }

    // ignore the trailing newline
    input_buffer->input_length = bytes_read - 1;
    input_buffer->buffer[bytes_read - 1] = 0;
}

// free the memory allocated (getline method) for an instance of InputBuffer * and the buffer element of the structure
void close_input_buffer(InputBuffer* input_buffer){
    free(input_buffer->buffer);
    free(input_buffer);
}

MetaCommandResult do_meta_command(InputBuffer* input_buffer){
    if(strcmp(input_buffer->buffer, ".exit") == 0){
        exit(EXIT_SUCCESS);
    }else{
        return META_COMMAND_UNRECOGNIZED_COMMAND;
    }
}

// SQL Compiler
PrepareResult prepare_statement(InputBuffer* input_buffer, Statement* statement){
    if(strncmp(input_buffer->buffer, "insert", 6) == 0 || strncmp(input_buffer->buffer, "INSERT", 6) == 0){
        statement->type = STATEMENT_INSERT;
        return PREPARE_SUCCESS;
    }else if(strncmp(input_buffer->buffer, "select", 6) == 0 || strncmp(input_buffer->buffer, "SELECT", 6) == 0){
        statement->type = STATEMENT_SELECT;
        return PREPARE_SUCCESS;
    }

    return PREPARE_UNRECOGNIZED_STATEMENT;
}

// Virtual Machine
void execute_statement(Statement* statement){
    switch (statement->type){
        case (STATEMENT_INSERT):
            // insert
            printf("do insert\n");
            break;
        case (STATEMENT_SELECT):
            // select
            printf("do select\n");
            break;
    }
}


int main(int argc, char* argv[]){
	InputBuffer* input_buffer = new_input_buffer(); // initial input_buffer
	while(true){
		print_prompt(); // print the prompt when the terminal call the corresponding command
		read_input(input_buffer);

		// non-sql statements starts with '.', like .exit
		if(input_buffer->buffer[0] == '.'){
		    switch (do_meta_command(input_buffer)){
		        case (META_COMMAND_SUCCESS):
                    continue; // begin the next while loop
		        case (META_COMMAND_UNRECOGNIZED_COMMAND):
                    printf("Unrecognized command '%s'\n", input_buffer->buffer);
                    continue;
		    }
		}

		// sql statement
		Statement statement;
		switch (prepare_statement(input_buffer, &statement)){
		    case (PREPARE_SUCCESS):
		        break;
		    case (PREPARE_UNRECOGNIZED_STATEMENT):
                printf("Unrecognized keyword at start of '%s'.\n", input_buffer->buffer);
                continue;
		}

		execute_statement(&statement);
		printf("Executed successfully\n");
	}

}