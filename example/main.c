#include <stdint.h>
#include "stdio.h"
#include "../quilt.h"

/* Windows specific performance profiling (uncomment to measure)
#include <windows.h>
*/

#define MAX_RESULTS 20000

int main(int argc, char *argv[]) {
	if(argc < 3) {
		printf("Too few arguments\n");
		printf("Usage [OPTIONS] <file_path> <search_token>\n");
		return 1;
	}

	char* file_path = argv[1];
	char* search_token = argv[2];

	printf("searching for: \"%s\", in file: %s\n", search_token, file_path);
	
	/* Windows specific performance profiling (uncomment to measure)
	LARGE_INTEGER frequency;
	LARGE_INTEGER start, end;
	double elapsed_time = 0.0;
	QueryPerformanceFrequency(&frequency);
	QueryPerformanceCounter(&start);
	*/
	Quilt_State state = quilt_load(file_path);
	struct Quilt_Search_Result results[MAX_RESULTS];
	i32 result_count = quilt_find_all(&state, results, MAX_RESULTS, search_token);

	/* Windows specific performance profiling (uncomment to measure)
	QueryPerformanceCounter(&end);
	elapsed_time = (double)(end.QuadPart - start.QuadPart) / frequency.QuadPart;
	printf("elapsed time:%f\n", elapsed_time);
	*/

	printf("========\n");
	printf("result_count: %d\n", result_count);
	for(int i = 0; i < result_count; i++) {
		struct Quilt_Search_Result result = results[i];
		Quilt_Line* lines = (Quilt_Line*)state.lines.data;
		Quilt_Line line = lines[result.line];
		printf("%s:%d:%d:\n", file_path, result.line, result.column);
		quilt_print_line(&state, line);
	}

	quilt_cleanup(&state);

		
	printf("=== END QUILT TEST\n");

	return 0;
}
