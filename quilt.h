/*

  QUILT - quick text filter

  todo:
  - [ ] handle utf-8
  - [x] multithreading
  - [ ] simd
  - [ ] linux/osx build
  - [ ] optimize loading (maybe load entire file to string and then split it in lines in an optimized way)


*/

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

// TODO (rhoe) should be multiplatform
#include <windows.h>

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

i32 get_number_of_cores_windows() {
	SYSTEM_INFO sysinfo;
	GetSystemInfo(&sysinfo);
	DWORD num_cores = sysinfo.dwNumberOfProcessors;
	return num_cores;
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
	/* state.lines_text = create_arena(1*GIGABYTE); */
	state.temp_arena = create_arena(512*MEGABYTE);
	state.amount_of_lines = 0;

	state.number_of_threads = get_number_of_cores_windows();

	/* i64 last_line_start = 0; */
	/* char c; */
   
	// TODO (rhoe) need to support longer lines
	/* char string_buffer[1024]; */
	/* while(fgets(string_buffer, 1024, file_ptr) != NULL) { */
	/* 	struct Quilt_String* string_ptr = (struct Quilt_String*)arena_allocate(&state.lines, sizeof(struct Quilt_String)); */

	/* 	*string_ptr = add_quilt_string(string_buffer, &state.lines_text); */
	/* 	state.amount_of_lines += 1; */

	/* } */

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


	/* free(state.text_buffer); */
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
	/* free(state->lines_text.data); */
	free(state->temp_arena.data);
}

/* bool quilt_match_line(Quilt_State* state, Qilt_Line* line, char *value) { */
/* 	struct Quilt_Line* line = &lines[l]; */

/* 	bool result = FALSE; */

/* 	for(int c = 0; c < line->length - value_length; c++) { */
/* 		found_matching = TRUE; */

/* 		for(int i = 0; i < value_length; i++) { */
/* 			int offset = c; */
/* 			if(state->text_buffer[line->offset + i] != value[i]) { */
/* 				found_matching = FALSE; */
/* 				break; */
/* 			} */
/* 		} */

/* 		if(found_matching) { */
/* 			result.found = TRUE; */
/* 			result.line = l; */
/* 			result.column = c; */
/* 			return result; */
/* 		} */
/* 	} */
	
/* } */

/* struct Quilt_Search_Result quilt_find_first(struct Quilt_State* state, char* value) { */
/* 	struct Quilt_Search_Result result; */
/* 	result.found = FALSE; */

/* 	BOOL found_matching = TRUE; */
/* 	i32 value_length = quilt_string_length(value); */

/* 	struct Quilt_Line* lines = (struct Quilt_Line*)state->lines.data; */
/* 	for(int l = 0; l < state->amount_of_lines; l++) { */
/* 		struct Quilt_Line* line = &lines[l]; */

/* 		for(int c = 0; c < line->length - value_length; c++) { */
/* 			found_matching = TRUE; */

/* 			for(int i = 0; i < value_length; i++) { */
/* 				int offset = c; */
/* 				if(state->text_buffer[line->offset + i] != value[i]) { */
/* 					found_matching = FALSE; */
/* 					break; */
/* 				} */
/* 			} */

/* 			if(found_matching) { */
/* 				result.found = TRUE; */
/* 				result.line = l; */
/* 				result.column = c; */
/* 				return result; */
/* 			} */
/* 		} */
/* 	} */

/* 	return result; */
/* } */


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
			BOOL found_matching = TRUE;

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

/* i32 quilt_find_in_lines(struct Quilt_State* state, i32 line_start, i32 line_stop, struct Quilt_Search_Result* results, i32 max_results, char* value) { */
i32 quilt_find_in_lines(LPVOID param) {
	Thread_State* thread_state = (Thread_State*)param;

	/* printf("starting thread\n"); */
	/* printf("line_start, line_stop: %d, %d\n", thread_state->line_start, thread_state->line_stop); */


	/* ASSERT(thrresults != NULL); */

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
			BOOL found_matching = TRUE;

			for(int i = 0; i < value_length; i++) {
				int offset = c;
				if(thread_state->state->text_buffer[line->offset + offset + i] != thread_state->value[i]) {
					found_matching = FALSE;
					break;
				}
			}

			if(found_matching) {
				/* BOOL waiting_for_free_mutex = TRUE; */
				/* while(waiting_for_free_mutex) { */
					// TODO (rhoe) blocking method
					/* DWORD dwWaitResult = WaitForSingleObject(ghMutex, INFINITE); */
					// TODO (rhoe) blocking method
					/* DWORD dwWaitResult = WaitForSingleObject(ghMutex, 0); */
					/* if(dwWaitResult == WAIT_OBJECT_0) { */
						if(thread_state->result_count >= thread_state->max_results) {
							// cannot add more results
						} else {
							thread_state->results[thread_state->result_count].found = TRUE;
							thread_state->results[thread_state->result_count].line = l;
							thread_state->results[thread_state->result_count].column = c;
							c += value_length;
							thread_state->result_count += 1;
							/* if(*thread_state->result_count >= thread_state->max_results) { */
							/* 	return *thread_state->result_count; */
							/* } */
						}

						/* waiting_for_free_mutex = FALSE; */
						/* ReleaseMutex(ghMutex); */
					/* } else if (dwWaitResult == WAIT_TIMEOUT) { */
						// locked by another thread
						/* thread_state->lock_fails++; */
					/* } */
			}
		}
	}

	return thread_state->result_count;
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
	HANDLE* threads = malloc(sizeof(HANDLE) * state->number_of_threads);

	/* #define MAX_RESULTS_PER_THREAD 4096 */
	i32 max_result_per_thread = max_results / state->number_of_threads;
	/* i32* ids = malloc(sizeof(i32) * state->number_of_threads); */
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
		
		threads[i] = CreateThread(NULL, 0, quilt_find_in_lines, &thread_states[i], 0, NULL);
		if(threads[i] == NULL) {
			printf("CreateThread failed (%lu)\n", GetLastError());
		}
	}

	WaitForMultipleObjects(state->number_of_threads, threads, TRUE, INFINITE);
	for(int i = 0; i < state->number_of_threads; i++) {
		CloseHandle(threads[i]);
	}

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

	

	/* struct Quilt_Line* lines = (struct Quilt_Line*)state->lines.data; */
	/* for(int l = 0; l < state->amount_of_lines; l++) { */
	/* 	struct Quilt_Line* line = &lines[l]; */

	/* 	for(int c = 0; c < line->length - value_length; c++) { */
	/* 		BOOL found_matching = TRUE; */

	/* 		for(int i = 0; i < value_length; i++) { */
	/* 			int offset = c; */
	/* 			if(state->text_buffer[line->offset + offset + i] != value[i]) { */
	/* 				found_matching = FALSE; */
	/* 				break; */
	/* 			} */
	/* 		} */

	/* 		if(found_matching) { */
	/* 			results[result_count].found = TRUE; */
	/* 			results[result_count].line = l; */
	/* 			results[result_count].column = c; */
	/* 			c += value_length; */
	/* 			result_count += 1; */
	/* 			if(result_count >= max_results) { */
	/* 				return result_count; */
	/* 			} */
	/* 		} */
	/* 	} */
	/* } */

	return result_count;
}


/* i32 quilt_find_first */
