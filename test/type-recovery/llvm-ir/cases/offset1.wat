(module
  (type (;0;) (func (param i32)))
  (type (;1;) (func (param i32 i32) (result i32)))
  (type (;2;) (func))
  (type (;3;) (func (result i32)))
  (import "env" "__linear_memory" (memory (;0;) 1))
  (import "env" "__stack_pointer" (global (;0;) (mut i32)))
  (import "env" "printf" (func (;0;) (type 1)))
  (import "env" "__indirect_function_table" (table (;0;) 0 funcref))
  (func $do_printf (type 0) (param i32)
    (local i32 i32 i32 i32 i32 i32 i32 i32)
    (local.set 1
      (global.get 0))
    (local.set 2
      (i32.const 16))
    (local.set 3
      (i32.sub
        (local.get 1)
        (local.get 2)))
    (global.set 0
      (local.get 3))
    (i32.store offset=12
      (local.get 3)
      (local.get 0))
    (local.set 4
      (i32.const 12))
    (local.set 5
      (i32.add
        (local.get 3)
        (local.get 4)))
    (i32.store
      (local.get 3)
      (local.get 5))
    (local.set 6
      (i32.const 0))
    (drop
      (call 0
        (local.get 6)
        (local.get 3)))
    (local.set 7
      (i32.const 16))
    (local.set 8
      (i32.add
        (local.get 3)
        (local.get 7)))
    (global.set 0
      (local.get 8))
    (return))
  (func $get_s (type 1) (param i32 i32) (result i32)
    (local i32 i32 i32 i32 i32 i32 i32 i32 i32 i32)
    (local.set 2
      (global.get 0))
    (local.set 3
      (i32.const 16))
    (local.set 4
      (i32.sub
        (local.get 2)
        (local.get 3)))
    (i32.store offset=12
      (local.get 4)
      (local.get 0))
    (i32.store offset=8
      (local.get 4)
      (local.get 1))
    (local.set 5
      (i32.load offset=12
        (local.get 4)))
    (local.set 6
      (i32.const 2))
    (local.set 7
      (i32.add
        (local.get 5)
        (local.get 6)))
    (local.set 8
      (i32.load offset=8
        (local.get 4)))
    (local.set 9
      (i32.const 2))
    (local.set 10
      (i32.shl
        (local.get 8)
        (local.get 9)))
    (local.set 11
      (i32.add
        (local.get 7)
        (local.get 10)))
    (return
      (local.get 11)))
  (func $test1 (type 2)
    (local i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32)
    (local.set 0
      (global.get 0))
    (local.set 1
      (i32.const 32))
    (local.set 2
      (i32.sub
        (local.get 0)
        (local.get 1)))
    (global.set 0
      (local.get 2))
    (local.set 3
      (i32.const 28))
    (local.set 4
      (i32.add
        (local.get 2)
        (local.get 3)))
    (local.set 5
      (local.get 4))
    (call $do_printf
      (local.get 5))
    (local.set 6
      (i32.const 12))
    (local.set 7
      (i32.add
        (local.get 2)
        (local.get 6)))
    (local.set 8
      (local.get 7))
    (call $do_printf
      (local.get 8))
    (local.set 9
      (i32.const 0))
    (i32.store offset=8
      (local.get 2)
      (local.get 9))
    (block  ;; label = @1
      (loop  ;; label = @2
        (local.set 10
          (i32.load offset=8
            (local.get 2)))
        (local.set 11
          (i32.const 2))
        (local.set 12
          (local.get 10))
        (local.set 13
          (local.get 11))
        (local.set 14
          (i32.lt_s
            (local.get 12)
            (local.get 13)))
        (local.set 15
          (i32.const 1))
        (local.set 16
          (i32.and
            (local.get 14)
            (local.get 15)))
        (br_if 1 (;@1;)
          (i32.eqz
            (local.get 16)))
        (local.set 17
          (i32.const 16))
        (local.set 18
          (i32.add
            (local.get 2)
            (local.get 17)))
        (local.set 19
          (local.get 18))
        (local.set 20
          (i32.const 2))
        (local.set 21
          (i32.add
            (local.get 19)
            (local.get 20)))
        (local.set 22
          (i32.load offset=8
            (local.get 2)))
        (local.set 23
          (i32.const 2))
        (local.set 24
          (i32.shl
            (local.get 22)
            (local.get 23)))
        (local.set 25
          (i32.add
            (local.get 21)
            (local.get 24)))
        (call $do_printf
          (local.get 25))
        (local.set 26
          (i32.load offset=8
            (local.get 2)))
        (local.set 27
          (i32.const 1))
        (local.set 28
          (i32.add
            (local.get 26)
            (local.get 27)))
        (i32.store offset=8
          (local.get 2)
          (local.get 28))
        (br 0 (;@2;))))
    (local.set 29
      (i32.const 32))
    (local.set 30
      (i32.add
        (local.get 2)
        (local.get 29)))
    (global.set 0
      (local.get 30))
    (return))
  (func $test2 (type 2)
    (local i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32 i32)
    (local.set 0
      (global.get 0))
    (local.set 1
      (i32.const 32))
    (local.set 2
      (i32.sub
        (local.get 0)
        (local.get 1)))
    (global.set 0
      (local.get 2))
    (local.set 3
      (i32.const 28))
    (local.set 4
      (i32.add
        (local.get 2)
        (local.get 3)))
    (local.set 5
      (local.get 4))
    (call $do_printf
      (local.get 5))
    (local.set 6
      (i32.const 12))
    (local.set 7
      (i32.add
        (local.get 2)
        (local.get 6)))
    (local.set 8
      (local.get 7))
    (call $do_printf
      (local.get 8))
    (local.set 9
      (i32.const 0))
    (i32.store offset=8
      (local.get 2)
      (local.get 9))
    (block  ;; label = @1
      (loop  ;; label = @2
        (local.set 10
          (i32.load offset=8
            (local.get 2)))
        (local.set 11
          (i32.const 2))
        (local.set 12
          (local.get 10))
        (local.set 13
          (local.get 11))
        (local.set 14
          (i32.lt_s
            (local.get 12)
            (local.get 13)))
        (local.set 15
          (i32.const 1))
        (local.set 16
          (i32.and
            (local.get 14)
            (local.get 15)))
        (br_if 1 (;@1;)
          (i32.eqz
            (local.get 16)))
        (local.set 17
          (i32.load offset=8
            (local.get 2)))
        (local.set 18
          (i32.const 16))
        (local.set 19
          (i32.add
            (local.get 2)
            (local.get 18)))
        (local.set 20
          (local.get 19))
        (local.set 21
          (call $get_s
            (local.get 20)
            (local.get 17)))
        (call $do_printf
          (local.get 21))
        (local.set 22
          (i32.load offset=8
            (local.get 2)))
        (local.set 23
          (i32.const 1))
        (local.set 24
          (i32.add
            (local.get 22)
            (local.get 23)))
        (i32.store offset=8
          (local.get 2)
          (local.get 24))
        (br 0 (;@2;))))
    (local.set 25
      (i32.const 32))
    (local.set 26
      (i32.add
        (local.get 2)
        (local.get 25)))
    (global.set 0
      (local.get 26))
    (return))
  (func $__original_main (type 3) (result i32)
    (local i32)
    (call $test1)
    (call $test2)
    (local.set 0
      (i32.const 0))
    (return
      (local.get 0)))
  (func $main (type 1) (param i32 i32) (result i32)
    (local i32)
    (local.set 2
      (call $__original_main))
    (return
      (local.get 2)))
  (data $.L.str (i32.const 0) "%p\0a\00"))
