
%.wasm: %.c
	../../scripts/clang-wasi-simple.sh ./$< -o ./$@

%.wat: %.wasm
	wasm2wat -f --enable-all ./$< > ./$@
