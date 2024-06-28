.PHONY: all clean wat wasm

FUNCTIONALDIR = functional


SOURCES = $(basename $(wildcard $(FUNCTIONALDIR)/*.c))
# eg: functional => out_functional
PREFIXEDS = $(addprefix out_, $(SOURCES))
BINARY := $(shell pwd)/../../../build/bin/notdec

wat: $(addsuffix .wat, $(PREFIXEDS))
wasm: $(addsuffix .wasm, $(PREFIXEDS))
watll: $(addsuffix .wat.ll, $(PREFIXEDS))

#  $(addsuffix .clif, $(basename $(wildcard *.c))) $(addsuffix .wavm.ll, $(basename $(wildcard *.c))) $(addsuffix .wasm-decompile, $(basename $(wildcard *.c))) $(addsuffix .wasmdec, $(basename $(wildcard *.c))) $(addsuffix .wasm2c.cc, $(basename $(wildcard *.c)))
# emcc ./$< -g -s EXPORTED_FUNCTIONS=_main -s ERROR_ON_UNDEFINED_SYMBOLS=0 -o ./$@
# -g -O0 --no-standard-libraries -Wl,--export-all -Wl,--no-entry -Wl,--allow-undefined 
out_$(FUNCTIONALDIR)/%.wasm: $(FUNCTIONALDIR)/%.c
	/opt/wasi-sdk-16.0/bin/clang -I. -c -O0 -o ./$@ ./$<

%.wat: %.wasm
	wasm2wat -f --enable-all ./$< > ./$@

%.wasm2c.cc: %.wasm
	wasm2c ./$< > ./$@

%.clif: %.wasm
	/mnt/c/Users/warren/d/2022/wasm-decompiler/wasmtime/target/debug/clif-util.exe wasm --target x86_64-pc-windows-gnu --set opt_level=none -p ./$< > ./$@

%.wavm.ll: %.wasm
	/mnt/c/Users/warren/d/2022/wasm-decompiler/wavm/build/bin/wavm compile --format=unoptimized-llvmir ./$< ./$@

%.wasm-decompile: %.wasm
	wasm-decompile --enable-all ./$< -o ./$@

%.wasmdec: %.wasm
	LD_LIBRARY_PATH=./wasmdec ./wasmdec/wasmdec ./$< -o ./$@

%.wat.ll: %.wat
	$(BINARY) ./$< -o ./$@ --split-mem

clean:
	rm -r out_functional && mkdir out_functional 
	# rm *.wasm *.wat *.ll # *.clif *.wavm.ll *.wasm-decompile *.wasmdec
