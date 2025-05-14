# cli.h
A simple header only library to parse command line tools in C. 

## Usage: 
```c
#include "cli.h"

int main(int argc, char *argv[])
{
  cli_str("title", "t", "Title");
  cli_bool("headless", "hd", "Headless mode");
  cli_int("njobs, j", "Number of jobs");
  cli(argc, argv);
  
  ...

  cli_cleanup();
  return 0;
}
```
