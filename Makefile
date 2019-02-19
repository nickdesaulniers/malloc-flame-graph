hook.so: hook.c
	clang -shared -fPIC -ldl -o hook.so hook.c
	#clang -shared -fPIC -lunwind -o hook.so hook.c

clean:
	rm -f hook.so

default: hook.so
