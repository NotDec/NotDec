.PHONY: all clean wat wasm 

FUNCTIONALDIR = functional

%.c.ll: %.c
	clang -S -O -emit-llvm ./$< -o ./$@

clean:
	rm *.ll.c *.c.ll
