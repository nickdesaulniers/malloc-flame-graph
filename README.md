# Malloc Flame Graph

A fake malloc hook (using runtime function interposition) that logs backtraces
of allocation sites, and tools to preprocess the log into a flamegraph.

The # of samples is actually the size of the allocation. Allocation sites are
grouped by call frame.

Also, for more meaningful stack frames, you probably want debug builds (debug
info and unstripped binary names).

## Usage

```sh
$ rm -f mallocs.log
$ LD_PRELOAD=./hook.so clang -fuse-ld=lld hello_world.c
$ c++filt < mallocs.txt > mallocs.demangled
$ python flatten.py > mallocs.txt
$ path/to/FlameGraph/flamegraph.pl mallocs.txt > mallocs.svg

$ # use alternative output file, e.g. malloc.log
$ MFG_OUTPUT=malloc.log LD_PRELOAD=./hook.so ...
```

hook.so appends to a file named mallocs.txt. flatten.py reads a file named
mallocs.demangled.

## Dependencies

- [FlameGraph](https://github.com/brendangregg/FlameGraph)
- c++filt (optional, if you'd like demangled C++ function names)
- python

## Links

- https://www.cs.cmu.edu/afs/cs/academic/class/15213-s03/src/interposition/mymalloc.c
- https://eli.thegreenplace.net/2015/programmatic-access-to-the-call-stack-in-c/
- https://stackoverflow.com/a/10008252/1027966
