#ifndef _H_CLI
#define _H_CLI

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

// TODO: Trim string values from arguments 
// TODO: Add optional and default values
// TODO: Add possible values
// TODO: Add better description formatting, placeholders and stuff
// TODO: Add single name options with --
// TODO: Add 'enable help flag' option
// TODO: Add 'on_cleanup' callback function
// TODO: Add proper error reporting with some kind of ERR_NO

void cli(int argc, char *argv[]);
void cli_cleanup();

#define cli_str(name, flag, desc)  cli_add(Arg_str, (name), (flag), (desc), false)
#define cli_int(name, flag, desc)  cli_add(Arg_int, (name), (flag), (desc), false)
#define cli_bool(name, flag, desc) cli_add(Arg_bool, (name), (flag), (desc), false)

#define cli_get_str(name)  (const char *)cli_get_data(name)
#define cli_get_int(name)        *((int *)cli_get_data(name))
#define cli_get_bool(name)      *((bool *)cli_get_data(name))

typedef enum Arg_Type {
  Arg_bool,
  Arg_str,
  Arg_int
} Arg_Type;

typedef struct cli_arg {
  const char *flag, *name, *desc;
  bool optional;
  Arg_Type type;

  void* data;
  struct cli_arg *next;
} cli_arg;

typedef struct cli_arg_list {
  cli_arg *head, *tail;
  size_t size;
} cli_arg_list;

static cli_arg_list arg_list;

static cli_arg *cli_build(int argc, char *argv[]);
static void cli_help(const char* exec_name);
static void cli_usage(const char *exec_name);
static void cli_details();

static void cli_print_inline(cli_arg* arg);
static void cli_print_extended(cli_arg* arg);

static void *cli_get_data(const char *name);
static int cli_get_arg_data(cli_arg* arg, int start, int argc, char *argv[]);
static int cli_get_arg_str(char *start, int arg0, int argc, char *argv[], char **dest);

static int cli_match_arg(cli_arg* arg, int start, int argc, char *argv[]);

static void cli_add(Arg_Type type, const char *name, const char *flag, const char *desc, bool optional);
static void cli_list_add(cli_arg *arg);
static void *cli_arg_alloc();

void cli(int argc, char *argv[])
{
  cli_arg* err = cli_build(argc, argv);
  if (err) {
	fputs("Error: Argument: '", stderr);
	cli_print_inline(err);
	fputs("' not specified properly\n", stderr);
	
	cli_help(argv[0]);
	cli_cleanup();
	exit(1);
  }
}

void cli_cleanup()
{
  cli_arg *rmv, *arg = arg_list.head;
  while (arg) {
	rmv = arg;
	arg = arg->next;
	free(rmv->data);
	free(rmv);
	arg_list.size--;
  }
}


static void *cli_get_data(const char *name)
{
  assert(name && "Name must be a string");
  cli_arg *arg = arg_list.head;

  while (arg && strcmp(name, arg->name))
	arg = arg->next;

  assert(arg && "Argument must be registered");
  assert(arg->data && "Could not get arg data");
  return arg->data;
}

cli_arg *cli_build(int argc, char *argv[])
{
  cli_arg *arg = arg_list.head;
  int i = 1;

  while (arg && i < argc) {
	int found = cli_match_arg(arg, i, argc, argv);
	int advance = 0;
	  
	if (arg->type == Arg_bool) {
	  arg->data = malloc(sizeof(bool));
	  *(bool *)arg->data = (bool)found;
	  advance = 1;
	} else if (found) 
	  advance = cli_get_arg_data(arg, found, argc, argv);

	if (!advance) break;
	i += advance;
	arg = arg->next;
  }

  return arg;
}

static void cli_help(const char *exec_name)
{
  cli_usage(exec_name);
  cli_details();
}

static void cli_usage(const char* exec_name)
{
  fprintf(stderr, "Usage: %s ", exec_name);

  cli_arg *arg = arg_list.head;
  
  for (; arg; arg = arg->next) {
	cli_print_inline(arg);
	fputc(' ', stderr);
  }

  fprintf(stderr, "\n");
}

static void cli_details()
{
  fputs("Command line options:\n", stderr);
  cli_arg *arg = arg_list.head;
  for (; arg; arg = arg->next) {
	cli_print_extended(arg);
	fputc(' ', stderr);
  }

  fprintf(stderr, "\n");  
}

static void cli_print_inline(cli_arg* arg)
{
  assert(arg);
  
  if (arg->optional) fputc('[', stderr);

  if (arg->type == Arg_bool) 
	fprintf(stderr, "-%s", arg->flag ? arg->flag : arg->name);
  else if (arg->name && !arg->flag)
	fprintf(stderr, "-%s %s", arg->name, arg->name);
  else {
	if (arg->flag) fprintf(stderr, "-%s ", arg->flag);
	if (arg->name) fputs(arg->name, stderr);
  }

  if (arg->optional) fputc(']', stderr);  
}

static void cli_print_extended(cli_arg* arg)
{
  assert(arg);

  /* fprintf(stderr, "\t\t\033[1m-%s\033[0m ", arg->flag ? arg->flag : arg->name); */
  fprintf(stderr, "\t\t-%s ", arg->flag ? arg->flag : arg->name);

  if (arg->type != Arg_bool) { 
	if (arg->optional) fputc(']', stderr);
	
	switch (arg->type) {
	case Arg_str:
	  fputs("string ", stderr);
	  break;
	case Arg_int:
	  fputs("integer", stderr);
	  break;
	case Arg_bool:
	default:
	  fputs("      ", stderr);
	  break;
	}
	
	if (arg->optional) fputc('[', stderr);
  }

  if (arg->desc) fprintf(stderr, "\t\t%s: %s", arg->name, arg->desc);
  
  fputc('\n', stderr);
}


static int cli_match_arg(cli_arg* arg, int start, int argc, char *argv[])
{
  assert(start < argc && arg);
  
  char *ptr;
  while (start < argc) {
	ptr = argv[start];
	if (*(ptr++) == '-' &&
		strstr(ptr, arg->flag) ||
		strstr(ptr, arg->name))
	  break;

	start++;
  }

  return start < argc ? start : 0;
}

static int cli_get_arg_data(cli_arg* arg, int start, int argc, char *argv[])
{
  assert(start < argc && arg);

  size_t vlen = strlen(argv[start]);
  size_t len = strlen(arg->flag ? arg->flag : arg->name);
  char *ptr = argv[start];

  if (vlen < len) return false;
  ptr += len + 1; // skip the '-' and the flag name

  int advanced = 0;
  switch (arg->type) {
	
  case Arg_str: {
	char *buf = {0};
	advanced = cli_get_arg_str(ptr, start, argc, argv, &buf);
	if (advanced) arg->data = (void *)buf;
	break;
  }
  case Arg_int: {
	arg->data = malloc(sizeof(int));
	*(int *)arg->data = atoi(ptr);
	advanced = 1;
	break;
  }
  case Arg_bool:
  default:
	break;
  }
  
  return advanced;
}

static int cli_get_arg_str(char *start, int arg0, int argc, char *argv[], char **dest)
{
  assert(dest);
  size_t size = 0;
  char *ptr = start ? start : argv[arg0];
  
  for (int i = arg0;
	   i < argc && *ptr != '-';
	   i++, ptr = argv[i])
	size += strlen(ptr) + 1;

  char *buf = malloc(sizeof(char) * size);
  if (!size || !buf) {
	*dest = 0;
	return 0;	
  }
  
  char *carry = buf;
  ptr = start ? start : argv[arg0];
  
  int i = arg0;
  for (; i < argc && *ptr != '-';
	   i++, ptr = argv[i]) {
	carry = strcat(carry, ptr);
	carry = strncat(carry, " ", 1);	
  }

  *dest = buf;
  return i - arg0;
}

static void cli_add(Arg_Type type, const char *name, const char *flag, const char *desc, bool optional)
{
  assert(name && "Arguments must have names");
  cli_arg *arg = cli_arg_alloc();
  assert(arg);

  *arg = (cli_arg){
	.flag = flag,
	.name = name,
	.desc = desc,
	.optional = optional,
	.type = type
  };

  cli_list_add(arg);  
}

static void cli_list_add(cli_arg *arg)
{
  if (!arg_list.head)
	arg_list.head = arg_list.tail = arg;
  else {
	arg_list.tail->next = arg;
	arg_list.tail = arg;
  }

  arg_list.size++;
}

static void *cli_arg_alloc()
{
  return malloc(sizeof(cli_arg));
}


#endif //_H_CLI
