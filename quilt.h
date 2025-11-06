/*

  QUILT - quick text filter

  todo:
  - [ ] handle utf-8
  - [x] multithreading
  - [ ] simd
  - [x] linux/osx build
  - [ ] optimize loading (maybe load entire file to string and then split it in lines in an optimized way)

*/

#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

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


/////////////////////////////////
// WINDOWS IMPLEMENTATION
#if defined(_MSC_VER)
#include <windows.h>
#define THREAD_DATA LPVOID
#define THREAD_RESULT DWORD
#define THREAD_RETURN_OK 0
typedef int8_t bool;
#define THREAD HANDLE
#define CREATE_THREAD(thread, func, data) thread = CreateThread(NULL, 0, func, data, 0, NULL)
#define WAIT_FOR_THREADS() \
	WaitForMultipleObjects(state->number_of_threads, threads, TRUE, INFINITE); \
	for(int i = 0; i < state->number_of_threads; i++) {\
		CloseHandle(threads[i]);\
	}

i32 get_number_of_cores() {
	SYSTEM_INFO sysinfo;
	GetSystemInfo(&sysinfo);
	DWORD num_cores = sysinfo.dwNumberOfProcessors;
	return num_cores;
}
#else

/////////////////////////////////
// UNIX (PTHREAD) IMPLEMENTATION
#define THREAD_DATA void*
#define THREAD_RESULT void*
#define THREAD_RETURN_OK pthread_exit(NULL)
typedef int8_t bool;
#include <pthread.h>
#define THREAD pthread_t
#define CREATE_THREAD(thread, func, data) pthread_create(thread, NULL, func, data)
#define WAIT_FOR_THREADS() \
	for(int i = 0; i < state->number_of_threads; i++) {					\
		pthread_join(threads[i], NULL);										\
	}
#include <unistd.h>
i32 get_number_of_cores() {
	long num_cores = sysconf(_SC_NPROCESSORS_ONLN);
	return num_cores;
}
#endif

#define KILOBYTE (1024ULL)
#define MEGABYTE (1024ULL * KILOBYTE)
#define GIGABYTE (1024ULL * MEGABYTE)
#define TERRABYTE (1024ULL * GIGABYTE)

#define ASSERT(Expression) if(!(Expression)) {*(volatile i32 *)0 = 0;}

typedef struct {
	char *buffer;
	i64 length;
} Quilt_String;

typedef struct {
	u8* data;
	i64 current_offset;
	i64 max_size;
} Arena;

typedef struct {
	i64 offset, length;
} Quilt_Line;

typedef struct {
	bool initialized;
	Arena temp_arena;
	char* text_buffer;
	i64 amount_of_lines;
	Arena lines;
	i32 number_of_threads;
} Quilt_State;

typedef struct Quilt_Search_Result {
	bool found;
	i32 line;
	i32 column;
} Quilt_Search_Result;

typedef struct {
	Quilt_State* state;
	i32 line_start;
	i32 line_stop;
	struct Quilt_Search_Result* results;
	i32 result_count;
	i32 max_results;
	char* value;
	i32 lock_fails;
} Thread_State;

typedef void* (*Alloc_Func)(i64);



//////////////////////////////
// ARENA
Arena create_arena(i64 max_size) {
	Arena result;
	result.data = malloc(max_size);
	result.max_size = max_size;
	result.current_offset = 0;
	return result;
}

void clear_arena(Arena *arena) {
	arena->current_offset = 0;
	return;
}

// TODO (rhoe) should we implement automatic arena growth?
u8* arena_allocate(Arena *arena, i64 amount) {
	if(arena->current_offset + amount > arena->max_size) {
		printf("arena error: failed to allocate, amount exceeds memory left\n");
		printf("current size: %lld,  size after: %lld\n", arena->max_size / MEGABYTE, amount);
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
Quilt_String add_quilt_string(char *value, Arena *arena) {
	Quilt_String result;
	int length = quilt_string_length(value);
	result.length = length;
	result.buffer = (char*)arena_allocate(arena, length);
	snprintf(result.buffer, length, "%s", value);
	return result;
}
Quilt_String add_quilt_string_from_buffer(char* buffer, i64 start_index, i64 end_index, Arena *arena) {
	Quilt_String result;

	i32 length = end_index - start_index;
	result.length = length;
	result.buffer = (char*)arena_allocate(arena, length);
	snprintf(result.buffer, length, "%s", &buffer[start_index]);
	return result;
}


//////////////////////////////
// QUILT API
Quilt_State quilt_load(char *path) {
	Quilt_State state;
	FILE* file_ptr = fopen(path, "r");
	state.lines = create_arena(512*MEGABYTE);
	state.temp_arena = create_arena(512*MEGABYTE);
	state.amount_of_lines = 0;

	state.number_of_threads = get_number_of_cores();

	fseek(file_ptr, 0, SEEK_END);
	i64 size = ftell(file_ptr);
	fseek(file_ptr, 0, SEEK_SET);

	// TODO (rhoe) this buffer should be malloced as temp arena
	state.text_buffer = malloc(size + 1);
	if(!state.text_buffer) {
		printf("quilt error: failed to malloc buffer for file");
		fclose(file_ptr);
		state.initialized = FALSE;
		return state;
	}

	fread(state.text_buffer, 1, size, file_ptr);
	state.text_buffer[size] = '\0';


	fclose(file_ptr);

	i64 current_index = 0;
	i64 last_line_start = 0;
	while(current_index < size) {
		char current_token = state.text_buffer[current_index];
		if(current_token == '\n') {
			i64 length = current_index - last_line_start;
			if(length > 0) {
				Quilt_Line* line = (Quilt_Line*)arena_allocate(&state.lines, sizeof(Quilt_Line));
				line->offset = last_line_start;
				line->length = length;
				current_index += 1;
				last_line_start = current_index;
				state.amount_of_lines += 1;
			}
		}
		current_index += 1;
	}

	return state;
}

void quilt_print_line(Quilt_State* state, Quilt_Line line) {
	// TODO (rhoe) we can either use fputs and write one char at a time
	//             or we can write the segment into a temp buffer
	fwrite(state->text_buffer + line.offset, 1, line.length, stdout);
	printf("\n");
}

void quilt_print_string(Quilt_String value) {
	for(int i = 0; i < value.length; i++) {
		printf("%c", value.buffer[i]);
	}
}

void quilt_print_file(Quilt_State* state) {
	Quilt_String* lines = (Quilt_String*)state->lines.data;
	for(int i = 0; i < state->amount_of_lines; i++) {
		quilt_print_string(lines[i]);
	}
}

void quilt_cleanup(Quilt_State* state) {
	free(state->lines.data);
	free(state->temp_arena.data);
}

/*

  quilt_find_all
  results is a buffer of Quilt_Search_Result
  max_results is the size of the results buffer


*/
i32 quilt_find_all_single_thread(Quilt_State* state, Quilt_Search_Result* results, i32 max_results, char* value) {
	ASSERT(results != NULL);

	if(max_results <= 0) {
		printf("quilt warning (quilt_find_all): max results needs to be higher than 0\n");
		return 0;
	}

	i32 value_length = quilt_string_length(value);
	ASSERT(value_length > 0);

	i32 result_count = 0;

	Quilt_Line* lines = (Quilt_Line*)state->lines.data;
	for(int l = 0; l < state->amount_of_lines; l++) {
		Quilt_Line* line = &lines[l];

		for(int c = 0; c < line->length - value_length; c++) {
			bool found_matching = TRUE;

			for(int i = 0; i < value_length; i++) {
				int offset = c;
				if(state->text_buffer[line->offset + offset + i] != value[i]) {
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

THREAD_RESULT quilt_find_in_lines(THREAD_DATA param) {
	Thread_State* thread_state = (Thread_State*)param;

	if(thread_state->max_results <= 0) {
		printf("quilt warning (quilt_find_all): max results needs to be higher than 0\n");
		return 0;
	}

	i32 value_length = quilt_string_length(thread_state->value);
	ASSERT(value_length > 0);

	Quilt_Line* lines = (Quilt_Line*)thread_state->state->lines.data;
	for(int l = thread_state->line_start; l < thread_state->line_stop; l++) {
		Quilt_Line* line = &lines[l];

		for(int c = 0; c < line->length - value_length; c++) {
			bool found_matching = TRUE;

			for(int i = 0; i < value_length; i++) {
				int offset = c;
				if(thread_state->state->text_buffer[line->offset + offset + i] != thread_state->value[i]) {
					found_matching = FALSE;
					break;
				}
			}

			if(found_matching) {
				if(thread_state->result_count >= thread_state->max_results) {
					// TODO (rhoe) handle not having enough space for results
					// cannot add more results
				} else {
					thread_state->results[thread_state->result_count].found = TRUE;
					thread_state->results[thread_state->result_count].line = l;
					thread_state->results[thread_state->result_count].column = c;
					c += value_length;
					thread_state->result_count += 1;
				}
			}
		}
	}

	THREAD_RETURN_OK;
}

i32 quilt_find_all(Quilt_State* state, Quilt_Search_Result* results, i32 max_results, char* value) {
	ASSERT(results != NULL);

	if(max_results <= 0) {
		printf("quilt warning (quilt_find_all): max results needs to be higher than 0\n");
		return 0;
	}

	i32 value_length = quilt_string_length(value);
	ASSERT(value_length > 0);

	i32 base_tasks = state->amount_of_lines / state->number_of_threads;
	i32 remainder = state->amount_of_lines % state->number_of_threads;
	THREAD* threads = malloc(sizeof(THREAD) * state->number_of_threads);

	i32 max_result_per_thread = max_results / state->number_of_threads;
	Thread_State* thread_states = malloc(sizeof(Thread_State) * state->number_of_threads);
	i32 next_line_start = 0;
	for(int i = 0; i < state->number_of_threads; i++) {
		Quilt_Search_Result* search_results = (Quilt_Search_Result*)arena_allocate(&state->temp_arena, sizeof(Quilt_Search_Result) * max_result_per_thread);
		i32 add_remainder = 0;
		if(i < remainder){
			add_remainder = 1;
		}
		thread_states[i].state = state;
		thread_states[i].line_start = next_line_start;
		thread_states[i].line_stop = next_line_start + base_tasks + add_remainder;
		thread_states[i].results = search_results;
		thread_states[i].result_count = 0;
		thread_states[i].max_results = max_results;
		thread_states[i].value = value;
		next_line_start = thread_states[i].line_stop + 1;
		
		CREATE_THREAD(&threads[i], quilt_find_in_lines, &thread_states[i]);
	}

	WAIT_FOR_THREADS()

	Quilt_Search_Result* search_results_current = results;
	i32 result_count = 0;
	for(int i = 0; i < state->number_of_threads; i++) {
		Thread_State thread_state = thread_states[i];
		printf("Tread %d\tresults: %d\tfailed: %d\n", i, thread_state.result_count, thread_state.lock_fails);
		memcpy(search_results_current, thread_state.results, thread_state.result_count * sizeof(Quilt_Search_Result));
		
		result_count += thread_state.result_count;
		search_results_current = &results[result_count];
	}

	free(thread_states);
	free(threads);

	return result_count;
}
