#include <stdint.h>
#include "stdio.h"
#include "thread.c"
#include "../quilt.h"

// TODO (rhoe) make multiplatform
#include <windows.h>

int main(int argc, char *argv[]) {

	LARGE_INTEGER frequency;
	LARGE_INTEGER start, end;
	double elapsed_time = 0.0;
	QueryPerformanceFrequency(&frequency);

	if(argc < 3) {
		printf("too few arguments\n");
		return 1;
	}

	char* file_path = argv[1];
	char* search_token = argv[2];


	printf("searching for: \"%s\", in file: %s\n", search_token, file_path);

	
	QueryPerformanceCounter(&start);
	struct Quilt_State state = quilt_load(file_path);
	QueryPerformanceCounter(&end);
	elapsed_time = (double)(end.QuadPart - start.QuadPart) / frequency.QuadPart;

	printf("elapsed time:%f\n", elapsed_time);

	/* struct Quilt_Search_Result result = quilt_find_first(&state, search_token); */
	/* printf("FOUND:%d, line:%d, column:%d\n", result.found, result.line, result.column); */


	
	QueryPerformanceCounter(&start);
	#define MAX_RESULTS 20000
	struct Quilt_Search_Result results[MAX_RESULTS];
	i32 result_count = quilt_find_all(&state, results, MAX_RESULTS, search_token);
	printf("========\n");
	printf("result_count: %d\n", result_count);
	for(int i = 0; i < result_count; i++) {
		struct Quilt_Search_Result result = results[i];
		struct Quilt_Line* lines = (struct Quilt_Line*)state.lines.data;
		struct Quilt_Line line = lines[result.line];
		printf("%s:%d:%d:\n", file_path, result.line, result.column);
		quilt_print_line(&state, line);
		/* quilt_print_string(line); */
	}
	QueryPerformanceCounter(&end);
	elapsed_time = (double)(end.QuadPart - start.QuadPart) / frequency.QuadPart;

	printf("elapsed time:%f\n", elapsed_time);




	quilt_cleanup(&state);
		
	printf("=== END QUILT TEST\n");

	return 0;
}
