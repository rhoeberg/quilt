# quilt (quick text filtering)

a single header library for quickly searching large text files using multithreading.

## Status 
Quilt is functional and can be used for text filtering, but the project is still in development.
Use at your own risk.

## Building example
### with MSVC
make sure in `build_example.bat` that the path to visual studio is correct
run:
`build_example.bat`

### with GCC
make sure GCC is installed, run:
`./build_example.sh`

## Performance 
Example benchmark: processing a 500 MB (19240926 lines) text file on an AMD Ryzen 9 5900X 12-Core CPU: 1.857589s

## How to use
see `example/main.c` for usage example.


## LICENES
This project is licensed under the **MIT License**.
See `quilt.h` for the full license text.
