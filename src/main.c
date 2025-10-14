#include <stdint.h>
#include "stdio.h"
#include "thread.c"

// quick-filter
// quilt

// TYPES
typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

#define FILE_BUFFER_SIZE 128

#define TRUE 1
#define FALSE 0
typedef int8_t bool;

struct File_State {
	FILE *file_ptr;
	char file_buffer[FILE_BUFFER_SIZE];
};

struct File_State quilt_read_file(char *path) {
	struct File_State state;
	state.file_ptr = fopen(path, "r");
	fgets(state.file_buffer,FILE_BUFFER_SIZE, state.file_ptr);
	return state;
}

void quilt_print_file(struct File_State *state) {
	printf("%s\n", state->file_buffer);
}

void quilt_close_file(struct File_State *state) {
	fclose(state->file_ptr);
}

i32 quilt_string_length(char *match) {
	i32 current = 0;
	while(1) {
		char c = match[current];
		if(c == '\0') break;

		current += 1;
	}
	return current;
}


struct Quilt_String {
	char *value;
	int length;
};

BOOL quilt_find(struct File_State *state, char *value) {
	BOOL found_matching = TRUE;
	int length = quilt_string_length(value);
	for(int i = 0; i < FILE_BUFFER_SIZE-length; i++) {
		found_matching = TRUE;
		for(int j = 0; j < length; j++) {
			int offset = i;
			if(state->file_buffer[offset + j] != value[j]) {
				found_matching = FALSE;
				break;
			}
		}

		if(found_matching) {
			break;
		}
	}

	return found_matching;
}


int main() {
	printf("hello wurld\n");

	/* test_thread() */
	
	/* int length = quilt_string_length("my test string"); */
	/* printf("length:%d\n", length); */


	struct File_State state = quilt_read_file("test.txt");

	BOOL found = quilt_find(&state, "bla");
	printf("FOUND:%d\n", found);

	quilt_print_file(&state);
	quilt_close_file(&state);
		

	return 0;
}
