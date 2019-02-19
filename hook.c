#define _GNU_SOURCE
#include <dlfcn.h>
#include <execinfo.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

static int output_fd = -1;
static void __attribute__((constructor)) init(void) {
  /*printf("we're in\n");*/
  output_fd = open("mallocs.log", O_CREAT | O_APPEND | O_WRONLY,
      S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
  if (output_fd == -1) exit(43);
}

static void __attribute__((destructor)) done(void) {
  /*write(0, "done\n", 5);*/
  close(output_fd);
}

static void load(void** fn, const char* const fn_name) {
  if (!*fn) {
    dlerror();
    void* sym = dlsym(RTLD_NEXT, fn_name);
    *fn = dlsym(RTLD_NEXT, fn_name);
    if (!*fn) {
      // print result of dlerror() ?
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
static unsigned long mem_pos = 0;
void* malloc(size_t size) {
  static void* (*real_malloc)(size_t) = NULL;
  static __thread int no_hook = 0;
  static int initializing = 0;

  /*fprintf(stderr, "malloc %zu\n", size);*/
  /*load((void**)&real_malloc, "malloc");*/
  if (!real_malloc) {
    if (!initializing) {
      initializing = 1;
      dlerror();
      void* sym = dlsym(RTLD_NEXT, "malloc");
      if (!sym) {
        exit(42);
      }
      real_malloc = sym;
      initializing = 0;
    } else if (mem_pos + size < sizeof(mem)){
      // if the function is not defined, we have to be careful calling dlsym,
      // as it may allocate, causing infinite recursion.
      // https://stackoverflow.com/a/10008252/1027966
      void* ret = mem + mem_pos;
      mem_pos += size;
      return ret;
    } else {
      /*fprintf(stderr, "Need more temp mem for allocations\n");*/
      exit(45);
    }
  }

  if (no_hook) return real_malloc(size);

  no_hook = 1;
  char message[32];
  int len = snprintf(message, sizeof(message), "malloc of %zu bytes\n", size);
  write(output_fd, message, len);
  show_backtrace();
  no_hook = 0;

  return real_malloc(size);
}

void free(void* ptr) {
  static void (*real_free)(void*) = NULL;
  static __thread int no_hook = 0;

  if (ptr >= (void*) mem && ptr <= (void*)(mem + mem_pos)) {
    // statically allocated memory, don't pass to real_free.
    return;
  }

  load((void**)&real_free, "free");

  if (no_hook) return real_free(ptr);

  no_hook = 1;
  /*puts("free");*/
  /*show_backtrace();*/
  no_hook = 0;

  real_free(ptr);
}
