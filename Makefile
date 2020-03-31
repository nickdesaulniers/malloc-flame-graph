hook.so: hook.c
	clang -shared -fPIC -ldl -Wall -Wextra -o hook.so hook.c

clean:
	rm -f hook.so

default: hook.so
