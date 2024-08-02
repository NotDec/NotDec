
%.wasm: %.c
	../../scripts/clang-wasi-link.sh ./$< -o ./$@

%.wat: %.wasm
	wasm2wat -f --enable-all ./$< > ./$@
