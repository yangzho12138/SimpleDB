#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define COLUMN_USERNAME_SIZE 32
#define COLUMN_EMAIL_SIZE 32

typedef enum {
    META_COMMAND_SUCCESS,
    META_COMMAND_UNRECOGNIZED_COMMAND
}MetaCommandResult;

typedef enum {
    PREPARE_SUCCESS,
    PREPARE_UNRECOGNIZED_STATEMENT,
    PREPARE_SYNTAX_ERROR,
}PrepareResult;

typedef enum {
    STATEMENT_INSERT,
    STATEMENT_SELECT
}StatementType;

typedef enum {
    EXECUTE_SUCCESS,
    EXECUTE_DUPLICATE_KEY,
    EXECUTE_TABLE_FULL,
} ExecuteResult;

typedef struct {
    char* buffer; // the command line read in the terminal
    size_t buffer_length;
    ssize_t input_length;
}InputBuffer;

typedef struct {
    uint32_t id;
    char username[COLUMN_USERNAME_SIZE];
    char email[COLUMN_EMAIL_SIZE];
}Row; // db schema

typedef struct {
    StatementType type;
    Row row_to_insert; // only used by insert statement
}Statement;

// the structure of a row
// 0: null pointer constant -> cast to a pointer of type Struct*
// the size of the Attribute member of the struct named Struct without having to have an object of that type
#define size_of_attribute(Struct, Attribute) sizeof(((Struct*)0)->Attribute)

const uint32_t ID_SIZE = size_of_attribute(Row, id);
const uint32_t USERNAME_SIZE = size_of_attribute(Row, username);
const uint32_t EMAIL_SIZE = size_of_attribute(Row, email);
const uint32_t ID_OFFSET = 0;
const uint32_t USERNAME_OFFSET = ID_OFFSET + ID_SIZE;
const uint32_t EMAIL_OFFSET = USERNAME_OFFSET + USERNAME_SIZE;
const uint32_t ROW_SIZE = ID_SIZE + USERNAME_SIZE + EMAIL_SIZE;

// Table structure: Table -> Page -> Row
const uint32_t PAGE_SIZE = 4096; // 4 kilobytes == page size used in most VM
#define TABLE_MAX_PAGES 100
const uint32_t ROWS_PER_PAGE = PAGE_SIZE / ROW_SIZE;
const uint32_t TABLE_MAX_ROWS = ROWS_PER_PAGE * TABLE_MAX_PAGES;

typedef struct {
    uint32_t num_rows;
    void* pages[TABLE_MAX_PAGES];
}Table;

Table* new_table(){
    Table* table = (Table*)malloc(sizeof(Table));
    table->num_rows = 0;
    for(uint32_t i = 0; i < TABLE_MAX_PAGES; i++){
        table->pages[i] = NULL;
    }
    return table;
}

void free_table(Table* table){
    for(uint32_t i = 0; table->pages[i]; i++){
        free(table->pages[i]);
    }
    free(table);
}

void serialize_row(Row* source, void* destination) {
    memcpy((int*)destination + ID_OFFSET, &(source->id), ID_SIZE);
    memcpy((int*)destination + USERNAME_OFFSET, &(source->username), USERNAME_SIZE);
    memcpy((int*)destination + EMAIL_OFFSET, &(source->email), EMAIL_SIZE);
}

void deserialize_row(void* source, Row* destination) {
    memcpy(&(destination->id), (int*)source + ID_OFFSET, ID_SIZE);
    memcpy(&(destination->username), (int*)source + USERNAME_OFFSET, USERNAME_SIZE);
    memcpy(&(destination->email), (int*)source + EMAIL_OFFSET, EMAIL_SIZE);
}

// get the target row's position in a table
void* row_slot(Table* table, uint32_t row_num){
    uint32_t page_num = row_num / ROWS_PER_PAGE; // this row in which page
    void* page = table->pages[page_num];
    if(page == NULL){
        page = table->pages[page_num] = malloc(PAGE_SIZE); // a new page -> malloc the memory
    }
    uint32_t row_offset = row_num % ROWS_PER_PAGE;
    uint32_t byte_offset = row_offset * ROW_SIZE;
    return (int*)page + byte_offset;
}


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
    if(strncmp(input_buffer->buffer, "insert", 6) == 0){
        statement->type = STATEMENT_INSERT;
        int arg_assigned = sscanf(input_buffer->buffer,
                "insert %d %s %s", &(statement->row_to_insert.id), statement->row_to_insert.username, statement->row_to_insert.email);
        if(arg_assigned < 3){
            return PREPARE_SYNTAX_ERROR;
        }
        return PREPARE_SUCCESS;
    }else if(strncmp(input_buffer->buffer, "select", 6) == 0){
        statement->type = STATEMENT_SELECT;
        return PREPARE_SUCCESS;
    }

    return PREPARE_UNRECOGNIZED_STATEMENT;
}

ExecuteResult execute_insert(Statement* statement, Table* table){
    if(table->num_rows >= TABLE_MAX_ROWS){
        return EXECUTE_TABLE_FULL;
    }
    Row* row_to_insert = &(statement->row_to_insert);
    serialize_row(row_to_insert, row_slot(table, table->num_rows)); // insert rows into table
    table->num_rows += 1;

    return EXECUTE_SUCCESS;
}

void print_row(Row* row) {
    printf("(%d, %s, %s)\n", row->id, row->username, row->email);
}

ExecuteResult execute_select(Statement* statement, Table* table){
    Row row;
    for(uint32_t i = 0; i < table->num_rows; i++){
        deserialize_row(row_slot(table, i), &row);
        print_row(&row);
    }
    return EXECUTE_SUCCESS;
}

// Virtual Machine
ExecuteResult execute_statement(Statement* statement, Table* table){
    switch (statement->type){
        case (STATEMENT_INSERT):
            return execute_insert(statement, table);
        case (STATEMENT_SELECT):
            return execute_select(statement, table);
    }
}


int main(int argc, char* argv[]){
    Table* table = new_table();
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
		    case (PREPARE_SYNTAX_ERROR):
		        printf("Syntax error. Could not parse statement.\n");
                continue;
		    case (PREPARE_UNRECOGNIZED_STATEMENT):
                printf("Unrecognized keyword at start of '%s'.\n", input_buffer->buffer);
                continue;
		}

		switch(execute_statement(&statement, table)){
		    case (EXECUTE_SUCCESS):
		        printf("Executed Successfully!\n");
		        break;
		    case (EXECUTE_TABLE_FULL):
		        printf("ERROR: Table Full!\n");
		        break;
		}

	}

}