import re

with open("mallocs.demangled", "r") as input_f:
    count = 0
    stack = []
    regex = re.compile("^malloc of ([0-9]+) bytes")
    hooks = re.compile("hook.so")
    for line in input_f:
        match = regex.search(line)
        if (match):
            print(";".join(stack) + " " + str(count))
            count = int(match.group(1))
            stack = []
        elif (hooks.search(line) is None):
            stack.insert(0, line.rstrip())
