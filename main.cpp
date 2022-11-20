#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct{
    char* buffer; // the command line read in the terminal
    size_t buffer_length;
    ssize_t input_length;
}InputBuffer;

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

int main(int argc, char* argv[]){
	InputBuffer* input_buffer = new_input_buffer(); // initial input_buffer
	while(true){
		print_prompt(); // print the prompt when the terminal call the corresponding command
		read_input(input_buffer);

		// exit command in the terminal
		if(strcmp(input_buffer->buffer, ".exit") == 0){
		    close_input_buffer(input_buffer);
		    exit(EXIT_SUCCESS);
		}else{
		    printf("Unrecognized command '%s'. Please try again! \n", input_buffer->buffer);
		}
	}
}