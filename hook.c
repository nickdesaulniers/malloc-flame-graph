#define _GNU_SOURCE
#include <dlfcn.h>
#include <errno.h>
#include <execinfo.h>
#include <fcntl.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#ifdef DEBUG
void debug_print(const char *msg)
{
  write(0, "[MFG]: ", 7);
  write(0, msg, strlen(msg) + 1);
}
#else
#define debug_print(x)
#endif

#define MFG_ENV_OUTPUT "MFG_OUTPUT"
#define MFG_ENV_MIN_SIZE "MFG_MALLOC_MIN"
#define MFG_ENV_MAX_SIZE "MFG_MALLOC_MAX"

static size_t filter_min_size;
static size_t filter_max_size = SIZE_MAX;

static void safe_str_to_sizet(const char *s, size_t *value)
{
  if (!s)
    return;

  errno = 0;
  char *end_ptr;
  unsigned long int r = strtoul(s, &end_ptr, 10);
  if (errno == 0 && !(end_ptr == s && r == 0)) {
    *value = r;
  }
}

static int output_fd = -1;
static void __attribute__((constructor)) init(void) {

  debug_print("initialization\n");

  char* filename;
  if (!(filename = getenv(MFG_ENV_OUTPUT))) {
    filename = "mallocs.log";
  }

  output_fd = open(filename, O_CREAT | O_APPEND | O_WRONLY,
      S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
  if (output_fd == -1) exit(43);


  safe_str_to_sizet(getenv(MFG_ENV_MIN_SIZE), &filter_min_size);
  safe_str_to_sizet(getenv(MFG_ENV_MAX_SIZE), &filter_max_size);
}

static void __attribute__((destructor)) done(void) {
  debug_print("cleanup\n");
  close(output_fd);
}

static void load(void** fn, const char* const fn_name) {
  if (!*fn) {
    dlerror();
    *fn = dlsym(RTLD_NEXT, fn_name);
    if (!*fn) {
      debug_print("dl error\n");
      exit(42);
    }
  }
}

static void show_backtrace(void) {
#define STACK_DEPTH 100
  void* buf[STACK_DEPTH];
  const int n = backtrace(buf, STACK_DEPTH);
#undef STACK_DEPTH
  backtrace_symbols_fd(buf, n, output_fd);
}

static char mem[1024*1024];
static size_t mem_pos = 0;

void* malloc(size_t size) {
  static void* (*real_malloc)(size_t) = NULL;
  static __thread bool no_hook = false;
  static bool initializing = false;

  if (!real_malloc) {
    if (!initializing) {
      initializing = true;
      dlerror();
      void* sym = dlsym(RTLD_NEXT, "malloc");
      if (!sym) {
        exit(42);
      }
      real_malloc = sym;
      initializing = false;
    } else if (mem_pos + size < sizeof(mem)){
      // if the function is not defined, we have to be careful calling dlsym,
      // as it may allocate, causing infinite recursion.
      // https://stackoverflow.com/a/10008252/1027966
      void* ret = mem + mem_pos;
      mem_pos += size;
      return ret;
    } else {
      debug_print("Need more temp mem for allocations\n");
      exit(45);
    }
  }

  if (no_hook) return real_malloc(size);

  if (size >= filter_min_size && size <= filter_max_size) {
    no_hook = true;
    char message[32];
    int len = snprintf(message, sizeof(message), "malloc of %zu bytes\n", size);
    write(output_fd, message, len);
    show_backtrace();
    no_hook = false;
  }

  return real_malloc(size);
}

void free(void* ptr) {
  static void (*real_free)(void*) = NULL;
  static __thread bool no_hook = false;

  if (ptr >= (void*) mem && ptr <= (void*)(mem + mem_pos)) {
    // statically allocated memory, don't pass to real_free.
    return;
  }

  load((void**)&real_free, "free");

  if (no_hook) return real_free(ptr);

  no_hook = true;
  debug_print("free called\n");
  no_hook = false;

  real_free(ptr);
}
