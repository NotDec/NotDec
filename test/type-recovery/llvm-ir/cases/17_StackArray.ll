; ModuleID = '/sn640/NotDec/experiment/dataset/out_dataset-SAC-2022/src/CWE121_Stack_Based_Buffer_Overflow__CWE129_fgets_01.wasm'
source_filename = "/sn640/NotDec/experiment/dataset/out_dataset-SAC-2022/src/CWE121_Stack_Based_Buffer_Overflow__CWE129_fgets_01.wasm"
target datalayout = "e-m:e-p:32:32-i64:64-n32:64-S128"
target triple = "wasm32-unknown-wasi"

; /* goodG2B uses the GoodSource with the BadSink */
; static void goodG2B()
; {
;     int data;
;     /* Initialize data */
;     data = -1;
;     /* FIX: Use a value greater than 0, but less than 10 to avoid attempting to
;      * access an index of the array in the sink that is out-of-bounds */
;     data = 7;
;     {
;         int i;
;         int buffer[10] = { 0 };
;         /* POTENTIAL FLAW: Attempt to write to an index of the array that is above the upper bound
;         * This code does check to see if the array index is negative */
;         if (data >= 0)
;         {
;             buffer[data] = 1;
;             /* Print the array values */
;             for(i = 0; i < 10; i++)
;             {
;                 printIntLine(buffer[i]);
;             }
;         }
;         else
;         {
;             printLine("ERROR: Array index is negative.");
;         }
;     }
; }

@__stack_pointer = internal global i32 66720

declare void @printLine(i32)

declare void @printIntLine(i32)

; 栈大小为48，-4为data，-8为i，-48(=stack_end)为buffer
define internal void @goodG2B() {
allocator:
  %stack26 = alloca [48 x i8], align 1; stack_1_stack26⊕-Pointer 89 <- (null, )
  %stack_addr = ptrtoint ptr %stack26 to i32; new_3_stack_addr⊕-Pointer 89 <- (stack_1_stack26⊕-Pointer 89, )
  %stack_end = add i32 %stack_addr, -48; new_6_stack_end⊕-Pointer 89 <- (new_3_stack_addr⊕-Pointer 89, #Int#@-48_0x60c000024d40⊕-Number 5, )
  %calcOffset = add i32 %stack_addr, -4; new_9_calcOffset⊕-Pointer 89 <- (new_3_stack_addr⊕-Pointer 89, #Int#@-4_0x60c000011900⊕-Number 8, )
  %new_11 = inttoptr i32 %calcOffset to ptr; new_11⊕-Pointer 89 <- (new_9_calcOffset⊕-Pointer 89, )
  store i32 7, ptr %new_11, align 4; (#Int#@7_0x60c000011c00⊕-Number 55, new_11⊕-Pointer 89, )
  %new_15 = inttoptr i32 %stack_end to ptr; new_15⊕-Pointer 89 <- (new_6_stack_end⊕-Pointer 89, )
  store i64 0, ptr %new_15, align 8; (#Int#@0_0x60c000011f00⊕-int64 17, new_15⊕-Pointer 89, )
  %new_20 = add i32 %stack_addr, -16; new_20⊕-Pointer 89 <- (new_3_stack_addr⊕-Pointer 89, #Int#@-16_0x60c000012080⊕-Number 19, )
  %new_22 = inttoptr i32 %new_20 to ptr; new_22⊕-Pointer 89 <- (new_20⊕-Pointer 89, )
  store i64 0, ptr %new_22, align 8; (#Int#@0_0x60c0000122c0⊕-int64 24, new_22⊕-Pointer 89, )
  %new_27 = add i32 %stack_addr, -24; new_27⊕-Pointer 89 <- (new_3_stack_addr⊕-Pointer 89, #Int#@-24_0x60c000012440⊕-Number 26, )
  %new_29 = inttoptr i32 %new_27 to ptr; new_29⊕-Pointer 89 <- (new_27⊕-Pointer 89, )
  store i64 0, ptr %new_29, align 8; (#Int#@0_0x60c000012680⊕-int64 31, new_29⊕-Pointer 89, )
  %new_34 = add i32 %stack_addr, -32; new_34⊕-Pointer 89 <- (new_3_stack_addr⊕-Pointer 89, #Int#@-32_0x60c000012800⊕-Number 33, )
  %new_36 = inttoptr i32 %new_34 to ptr; new_36⊕-Pointer 89 <- (new_34⊕-Pointer 89, )
  store i64 0, ptr %new_36, align 8; (#Int#@0_0x60c000012a40⊕-int64 38, new_36⊕-Pointer 89, )
  %new_41 = add i32 %stack_addr, -40; new_41⊕-Pointer 89 <- (new_3_stack_addr⊕-Pointer 89, #Int#@-40_0x60c000012bc0⊕-Number 40, )
  %new_43 = inttoptr i32 %new_41 to ptr; new_43⊕-Pointer 89 <- (new_41⊕-Pointer 89, )
  store i64 0, ptr %new_43, align 8; (#Int#@0_0x60c000012e00⊕-int64 45, new_43⊕-Pointer 89, )
  %calcOffset7 = add i32 %stack_addr, -4; new_48_calcOffset7⊕-Pointer 89 <- (new_3_stack_addr⊕-Pointer 89, #Int#@-4_0x60c000012ec0⊕-Number 47, )
  %new_50 = inttoptr i32 %calcOffset7 to ptr; new_50⊕-Pointer 89 <- (new_48_calcOffset7⊕-Pointer 89, )
  %loadResult = load i32, ptr %new_50, align 4; new_53_loadResult⊕-Number 55 <- (new_50⊕-Pointer 89, )
  %brif_val.not = icmp sgt i32 %loadResult, -1; new_56_brif_val.not⊕-int1 57 <- (new_53_loadResult⊕-Number 55, #Int#@-1_0x60c000022700⊕-Number 55, )
  br i1 %brif_val.not, label %brif_next, label %blk_exit8; (new_56_brif_val.not⊕-int1 57, null, null, )

blk_exit:                                         ; preds = %loop_entry, %blk_exit8
  ret void; ()

blk_exit8:                                        ; preds = %allocator
  call void @printLine(i32 1060); (#Int#@1060_0x60d000026ee0⊕-Unknown 60, null, )
  br label %blk_exit; (null, )

brif_next:                                        ; preds = %allocator
  %new_63 = shl i32 %loadResult, 2; new_63⊕-Number 64 <- (new_53_loadResult⊕-Number 55, #Int#@2_0x60c000013ac0⊕-Number 65, )
  %new_66 = add i32 %stack_end, %new_63; new_66⊕-Pointer 89 <- (new_6_stack_end⊕-Pointer 89, new_63⊕-Number 64, )
  %new_68 = inttoptr i32 %new_66 to ptr; new_68⊕-Pointer 89 <- (new_66⊕-Pointer 89, )
  store i32 1, ptr %new_68, align 4; (#Int#@1_0x60c000013f40⊕-Unknown 70, new_68⊕-Pointer 89, )
  %calcOffset12 = add i32 %stack_addr, -8; new_73_calcOffset12⊕-Pointer 89 <- (new_3_stack_addr⊕-Pointer 89, #Int#@-8_0x60c0000140c0⊕-Number 72, )
  %new_75 = inttoptr i32 %calcOffset12 to ptr; new_75⊕-Pointer 89 <- (new_73_calcOffset12⊕-Pointer 89, )
  br label %loop_entry; (null, )

loop_entry:                                       ; preds = %brif_next17, %brif_next
  %storemerge = phi i32 [ 0, %brif_next ], [ %new_101, %brif_next17 ]; phi_78_storemerge⊕-Number 99 <- (#Int#@0_0x607000005a48⊕-Number 99, new_101⊕-Number 99, )
  store i32 %storemerge, ptr %new_75, align 4; (phi_78_storemerge⊕-Number 99, new_75⊕-Pointer 89, )
  %new_81 = icmp sgt i32 %storemerge, 9; new_81⊕-int1 82 <- (phi_78_storemerge⊕-Number 99, #Int#@9_0x60c000022940⊕-Number 99, )
  br i1 %new_81, label %blk_exit, label %brif_next17; (new_81⊕-int1 82, null, null, )

brif_next17:                                      ; preds = %loop_entry
  %new_83 = shl i32 %storemerge, 2; new_83⊕-Number 84 <- (phi_78_storemerge⊕-Number 99, #Int#@2_0x60c000014e40⊕-Number 85, )
  %new_86 = add i32 %stack_end, %new_83; new_86⊕-Pointer 89 <- (new_6_stack_end⊕-Pointer 89, new_83⊕-Number 84, )
  %new_88 = inttoptr i32 %new_86 to ptr; new_88⊕-Pointer 89 <- (new_86⊕-Pointer 89, )
  %loadResult21 = load i32, ptr %new_88, align 4; new_91_loadResult21⊕-Unknown 70 <- (new_88⊕-Pointer 89, )
  call void @printIntLine(i32 %loadResult21); (new_91_loadResult21⊕-Unknown 70, null, )
  %loadResult23 = load i32, ptr %new_75, align 4; new_98_loadResult23⊕-Number 99 <- (new_75⊕-Pointer 89, )
  %new_101 = add i32 %loadResult23, 1; new_101⊕-Number 99 <- (new_98_loadResult23⊕-Number 99, #Int#@1_0x60c000015500⊕-Number 100, )
  br label %loop_entry; (null, )
}
