#pragma once

// TYPES
typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

#define TRUE 1
#define FALSE 0
typedef int8_t bool;

#define KILOBYTE (1024ULL)
#define MEGABYTE (1024ULL * KILOBYTE)
#define GIGABYTE (1024ULL * MEGABYTE)
#define TERRABYTE (1024ULL * GIGABYTE)

#define ASSERT(Expression) if(!(Expression)) {*(volatile i32 *)0 = 0;}

struct Quilt_String {
	char *buffer;
	i64 length;
};

struct Arena {
	u8* data;
	i64 current_offset;
	i64 max_size;
};

struct Quilt_State {
	struct Arena temp_arena;

	i64 amount_of_lines;
	struct Arena lines;
	struct Arena lines_text;
};

struct Quilt_Search_Result {
	bool found;
	i32 line;
	i32 column;
};

typedef void* (*Alloc_Func)(i64);


//////////////////////////////
// ARENA
struct Arena create_arena(i64 max_size) {
	struct Arena result;
	result.data = malloc(max_size);
	result.max_size = max_size;
	result.current_offset = 0;
	return result;
}

void clear_arena(struct Arena *arena) {
	arena->current_offset = 0;
	return;
}

u8* arena_allocate(struct Arena *arena, i64 amount) {
	if(arena->current_offset + amount > arena->max_size) {
		printf("arena error: failed to allocate, amount exceeds memory left\n");
		return NULL;
	}

	u8* result = &arena->data[arena->current_offset];
	arena->current_offset += amount;
	return result;
}

//////////////////////////////
// STRING
i32 quilt_string_length(char *match) {
	i32 current = 0;
	while(1) {
		char c = match[current];
		if(c == '\0') break;

		current += 1;
	}
	return current;
}
struct Quilt_String add_quilt_string(char *value, struct Arena *arena) {
	struct Quilt_String result;
	int length = quilt_string_length(value);
	result.length = length;
	/* result.buffer = (char*)alloc_func(length); */
	result.buffer = (char*)arena_allocate(arena, length);
	sprintf(result.buffer, "%s", value);
	return result;
}
/* #define TEMP_STRING(value) add_quilt_string(value, temp_allocate) */

//////////////////////////////
// QUILT API
struct Quilt_State quilt_load(char *path) {
	struct Quilt_State state;
	FILE* file_ptr = fopen(path, "r");
	state.lines = create_arena(128*MEGABYTE);
	state.lines_text = create_arena(1*GIGABYTE);
	state.temp_arena = create_arena(512*MEGABYTE);
	state.amount_of_lines = 0;

	i64 last_line_start = 0;
	char c;
   
	// TODO (rhoe) need to support longer lines
	char string_buffer[128];
	while(fgets(string_buffer, 128, file_ptr) != NULL) {
		struct Quilt_String* string_ptr = (struct Quilt_String*)arena_allocate(&state.lines, sizeof(struct Quilt_String));

		*string_ptr = add_quilt_string(string_buffer, &state.lines_text);
		state.amount_of_lines += 1;

	}

	fclose(file_ptr);

	return state;
}

void quilt_print_string(struct Quilt_String value) {
	for(int i = 0; i < value.length; i++) {
		printf("%c", value.buffer[i]);
	}
}

void quilt_print_file(struct Quilt_State* state) {
	struct Quilt_String* lines = (struct Quilt_String*)state->lines.data;
	for(int i = 0; i < state->amount_of_lines; i++) {
		quilt_print_string(lines[i]);
	}
}

void quilt_cleanup(struct Quilt_State* state) {
	free(state->lines.data);
	free(state->lines_text.data);
	free(state->temp_arena.data);
}

/* BOOL quilt_find(struct File_State *state, struct Quilt_String value) { */
struct Quilt_Search_Result quilt_find_first(struct Quilt_State* state, char* value) {
	struct Quilt_Search_Result result;
	result.found = FALSE;

	BOOL found_matching = TRUE;
	i32 value_length = quilt_string_length(value);

	struct Quilt_String* lines = (struct Quilt_String*)state->lines.data;
	for(int l = 0; l < state->amount_of_lines; l++) {
		struct Quilt_String* line = &lines[l];

		for(int c = 0; c < line->length - value_length; c++) {
			found_matching = TRUE;

			for(int i = 0; i < value_length; i++) {
				int offset = c;
				if(line->buffer[offset + i] != value[i]) {
					found_matching = FALSE;
					break;
				}
			}

			if(found_matching) {
				result.found = TRUE;
				result.line = l;
				result.column = c;
				break;
			}
		}
	}



	return result;
}

/*

  quilt_find_all
  you need to allocate a buffer and pass it

*/
i32 quilt_find_all(struct Quilt_State* state, struct Quilt_Search_Result* results, i32 max_results, char* value) {
	ASSERT(results != NULL);

	if(max_results <= 0) {
		printf("quilt warning (quilt_find_all): max results needs to be higher than 0\n");
		return 0;
	}

	i32 value_length = quilt_string_length(value);
	ASSERT(value_length > 0);

	i32 result_count = 0;

	struct Quilt_String* lines = (struct Quilt_String*)state->lines.data;
	for(int l = 0; l < state->amount_of_lines; l++) {
		struct Quilt_String* line = &lines[l];

		for(int c = 0; c < line->length - value_length; c++) {
			BOOL found_matching = TRUE;

			for(int i = 0; i < value_length; i++) {
				int offset = c;
				if(line->buffer[offset + i] != value[i]) {
					found_matching = FALSE;
					break;
				}
			}

			if(found_matching) {
				results[result_count].found = TRUE;
				results[result_count].line = l;
				results[result_count].column = c;
				c += value_length;
				result_count += 1;
				if(result_count >= max_results) {
					return result_count;
				}
			}
		}
	}

	return result_count;
}
