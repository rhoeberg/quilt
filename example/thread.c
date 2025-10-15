
#include <windows.h>

DWORD WINAPI ThreadFunc(LPVOID lpParam) {
	printf("hello from thread\n");
	return 0;
}

typedef DWORD (WINAPI *Thread_Func)(LPVOID);
/* typedef int (*Thread_Func)(LPVOID); */

/* DWORD WINAPI ThreadFunc(LPVOID lpParam) { */
/* 	printf("hello from thread\n"); */
/* } */

HANDLE create_thread() {
	HANDLE handle = CreateThread(NULL, 0, ThreadFunc, NULL, 0, NULL);
	return handle;
}

void test_thread() {
	HANDLE handle = create_thread();

	if(handle) {
		WaitForSingleObject(handle, INFINITE);
		CloseHandle(handle);
	}
}


/* #ifdef WIN */
// do windows threads here
/* #endif */

/* #ifdef UNIX */
/* // do unix threads here */
/* #endif */
