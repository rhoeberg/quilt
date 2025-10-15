#include <stdint.h>
#include "stdio.h"
#include "thread.c"
#include "../quilt.h"

int main(int argc, char *argv[]) {

	if(argc < 3) {
		printf("too few arguments\n");
		return 1;
	}

	char* file_path = argv[1];
	char* search_token = argv[2];


	printf("searching for: \"%s\", in file: %s\n", search_token, file_path);

	struct Quilt_State state = quilt_load(file_path);

	struct Quilt_Search_Result result = quilt_find_first(&state, search_token);
	printf("FOUND:%d, line:%d, column:%d\n", result.found, result.line, result.column);


	#define MAX_RESULTS 1024
	struct Quilt_Search_Result results[MAX_RESULTS];
	i32 result_count = quilt_find_all(&state, results, MAX_RESULTS, search_token);
	printf("========\n");
	printf("result_count: %d\n", result_count);
	for(int i = 0; i < result_count; i++) {
		struct Quilt_Search_Result result = results[i];
		struct Quilt_String* lines = (struct Quilt_String*)state.lines.data;
		struct Quilt_String line = lines[result.line];
		printf("cimgui.h:%d:%d:\n", result.line, result.column);
		quilt_print_string(line);
	}



	quilt_cleanup(&state);
		
	printf("=== END QUILT TEST\n");

	return 0;
}
