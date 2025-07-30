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

# args.hpp

# Usage

```cpp
	auto cli = args::Cli_Parser(true);
	cli.add<std::vector<std::string>>("src", "Source files to compare against destination");
	cli.add<std::vector<std::string>>("dest", "Destination files to compare with source");

	if (!cli.parse(argc, argv)) {
		std::println(stderr, "ERROR: {}", cli.error_str());
		std::println(stderr, "{}", cli.usage_str(argv[0]));
		return 1;
	}

	auto dest_files = cli.get<std::vector<std::string>>("dest");
	
```

# Customization

```cpp
struct Vec2 { 
	int x, y;
};

#define ARGS_CUSTOM_TYPES \ 
	ARGS_TYPE(Vec2) \
	ARGS_TYPE(std::fstream)

ARGS_PARSE_FUNC(Vec2, 
{
	// defines args_str (parsed string of value) 
	// defines args_delim (string of value delimiter of argv)
	
    Vec2 vec;
    ... 
    return vec;
})

ARGS_PARSE_EXPR(std::fstream, std::fstream{args_str})
...


// then include the file
#include "args.h"
```
