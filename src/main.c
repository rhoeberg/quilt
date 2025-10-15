#include <stdint.h>
#include "stdio.h"
#include "thread.c"
#include "quilt.h"

// quick-filter
// quilt


int main() {
	printf("BEGIN QUILT TEST\n");

	/* test_thread() */
	
	/* int length = quilt_string_length("my test string"); */
	/* printf("length:%d\n", length); */
	/* temp_arena = create_arena(1 * GIGABYTE); */
	/* memory = create_arena(1 * GIGABYTE); */



	struct Quilt_State state = quilt_load("test.txt");

	/* quilt_print_file(&state); */

	struct Quilt_Search_Result result = quilt_find_first(&state, "is");
	printf("FOUND:%d, line:%d, column:%d\n", result.found, result.line, result.column);


	#define MAX_RESULTS 128
	struct Quilt_Search_Result results[MAX_RESULTS];
	i32 result_count = quilt_find_all(&state, results, MAX_RESULTS, "here");
	printf("========\n");
	printf("result_count: %d\n", result_count);
	for(int i = 0; i < result_count; i++) {
		struct Quilt_Search_Result result = results[i];
		struct Quilt_String* lines = (struct Quilt_String*)state.lines.data;
		struct Quilt_String line = lines[result.line];
		quilt_print_string(line);
	}



	/* quilt_close_file(&state); */
	quilt_cleanup(&state);
		
	printf("END QUILT TEST\n");

	return 0;
}
