(module
  (import "console" "log" (func $log (param i32 i32)))
  (import "js" "mem" (memory (;0;) 1))
  (data (;0;) (i32.const 0) "Hello World from WebAssembly!")
  (func (;1;)
    i32.const 0
    i32.const 29
    call $log)
  (export "helloWorld" (func 1))
  (type (;0;) (func (param i32 i32)))
  (type (;1;) (func))
  (type (;2;) (func (result i32))))
