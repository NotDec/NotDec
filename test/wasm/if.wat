(module
  (import "test" "test" (func $test (param f32 i32) (result i32)))
  (import "js" "mem" (memory (;0;) 1))
  (data (;0;) (i32.const 0) "Hello World from WebAssembly!")
  (func (;1;) (result i32)
    i32.const 1
    if (result i32)  ;; label = @1
      i32.const 1
    else
      i32.const 0
    end)
  (export "helloWorld" (func 1))
  (type (;0;) (func (param i32 i32)))
  (type (;1;) (func))
  (type (;2;) (func (result i32)))
  (type (;3;) (func (param f32 i32)))
  (type (;4;) (func (param f32 i32) (result i32 f32))))
