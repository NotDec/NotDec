.PHONY: all clean wat wasm dec

FUNCTIONALDIR = dataset-SAC-2022/src

# 先初始化创建文件夹
# mkdir -p ./out_dataset-SAC-2022/src/
# 然后直接make -j`nproc`

SOURCES = $(basename $(wildcard $(FUNCTIONALDIR)/CWE*.c))
# eg: functional => out_functional
PREFIXEDS = $(addprefix out_, $(SOURCES))

all: dec wat
dec: $(addsuffix .dec.ll, $(PREFIXEDS))
wat: $(addsuffix .wat, $(PREFIXEDS))
wasm: $(addsuffix .wasm, $(PREFIXEDS))

#  $(addsuffix .clif, $(basename $(wildcard *.c))) $(addsuffix .wavm.ll, $(basename $(wildcard *.c))) $(addsuffix .wasm-decompile, $(basename $(wildcard *.c))) $(addsuffix .wasmdec, $(basename $(wildcard *.c))) $(addsuffix .wasm2c.cc, $(basename $(wildcard *.c)))
# emcc ./$< -g -s EXPORTED_FUNCTIONS=_main -s ERROR_ON_UNDEFINED_SYMBOLS=0 -o ./$@
# -g -O0 --no-standard-libraries -Wl,--export-all -Wl,--no-entry -Wl,--allow-undefined 
out_$(FUNCTIONALDIR)/%.wasm: $(FUNCTIONALDIR)/%.c
# -fno-builtin -fno-lto -Wl,--lto-O0 -I. -g -O0 --no-standard-libraries -Wl,--entry=main -lc -Wl,--allow-undefined
# /opt/wasi-sdk-20.0/bin/clang -fno-builtin -fno-lto -Wl,--lto-O0 -I. -g -O0 --no-standard-libraries -Wl,--entry=main -lc -Wl,--allow-undefined -o ${out}.wasm $file
	/opt/wasi-sdk-16.0/bin/clang -D INCLUDEMAIN -I. -g -O0 --no-standard-libraries -Wl,--entry=__main_argc_argv -fno-builtin -Wl,--allow-undefined -o ./$@ ./$<

%.dec.ll: %.wasm
	../../build/bin/notdec --compat-mode -o $@ -i $<

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

clean:
	rm ./out_$(FUNCTIONALDIR)/*
# rm *.wasm *.wat *.ll # *.clif *.wavm.ll *.wasm-decompile *.wasmdec
