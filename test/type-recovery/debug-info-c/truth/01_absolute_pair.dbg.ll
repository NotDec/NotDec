; ModuleID = '/sn640/NotDec/test/type-recovery/debug-info-c/cases/01_absolute_pair.c'
source_filename = "/sn640/NotDec/test/type-recovery/debug-info-c/cases/01_absolute_pair.c"
target datalayout = "e-m:e-p:32:32-p10:8:8-p20:8:8-i64:64-n32:64-S128-ni:1:10:20"
target triple = "wasm32-unknown-wasi"

%struct.Pair = type { i32, i32* }

; Function Attrs: noinline nounwind
define hidden i32 @use_pair() #0 !dbg !15 {
entry:
  %pair = alloca %struct.Pair*, align 4
  call void @llvm.dbg.declare(metadata %struct.Pair** %pair, metadata !19, metadata !DIExpression()), !dbg !20
  store %struct.Pair* inttoptr (i32 1024 to %struct.Pair*), %struct.Pair** %pair, align 4, !dbg !20
  %0 = load %struct.Pair*, %struct.Pair** %pair, align 4, !dbg !21
  %first = getelementptr inbounds %struct.Pair, %struct.Pair* %0, i32 0, i32 0, !dbg !22
  store i32 1, i32* %first, align 4, !dbg !23
  %1 = load %struct.Pair*, %struct.Pair** %pair, align 4, !dbg !24
  %second = getelementptr inbounds %struct.Pair, %struct.Pair* %1, i32 0, i32 1, !dbg !25
  store i32* inttoptr (i32 2048 to i32*), i32** %second, align 4, !dbg !26
  %2 = load %struct.Pair*, %struct.Pair** %pair, align 4, !dbg !27
  %first1 = getelementptr inbounds %struct.Pair, %struct.Pair* %2, i32 0, i32 0, !dbg !28
  %3 = load i32, i32* %first1, align 4, !dbg !28
  ret i32 %3, !dbg !29
}

; Function Attrs: nofree nosync nounwind readnone speculatable willreturn
declare void @llvm.dbg.declare(metadata, metadata, metadata) #1

attributes #0 = { noinline nounwind "frame-pointer"="none" "min-legal-vector-width"="0" "no-builtins" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="generic" }
attributes #1 = { nofree nosync nounwind readnone speculatable willreturn }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!11, !12, !13}
!llvm.ident = !{!14}

!0 = distinct !DICompileUnit(language: DW_LANG_C99, file: !1, producer: "Ubuntu clang version 14.0.0-1ubuntu1.1", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, retainedTypes: !2, splitDebugInlining: false, nameTableKind: None)
!1 = !DIFile(filename: "/sn640/NotDec/test/type-recovery/debug-info-c/cases/01_absolute_pair.c", directory: "/sn640/NotDec")
!2 = !{!3, !10}
!3 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !4, size: 32)
!4 = distinct !DICompositeType(tag: DW_TAG_structure_type, name: "Pair", file: !5, line: 1, size: 64, elements: !6)
!5 = !DIFile(filename: "test/type-recovery/debug-info-c/cases/01_absolute_pair.c", directory: "/sn640/NotDec")
!6 = !{!7, !9}
!7 = !DIDerivedType(tag: DW_TAG_member, name: "first", scope: !4, file: !5, line: 2, baseType: !8, size: 32)
!8 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!9 = !DIDerivedType(tag: DW_TAG_member, name: "second", scope: !4, file: !5, line: 3, baseType: !10, size: 32, offset: 32)
!10 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !8, size: 32)
!11 = !{i32 7, !"Dwarf Version", i32 4}
!12 = !{i32 2, !"Debug Info Version", i32 3}
!13 = !{i32 1, !"wchar_size", i32 4}
!14 = !{!"Ubuntu clang version 14.0.0-1ubuntu1.1"}
!15 = distinct !DISubprogram(name: "use_pair", scope: !5, file: !5, line: 6, type: !16, scopeLine: 6, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !0, retainedNodes: !18)
!16 = !DISubroutineType(types: !17)
!17 = !{!8}
!18 = !{}
!19 = !DILocalVariable(name: "pair", scope: !15, file: !5, line: 7, type: !3)
!20 = !DILocation(line: 7, column: 16, scope: !15)
!21 = !DILocation(line: 8, column: 3, scope: !15)
!22 = !DILocation(line: 8, column: 9, scope: !15)
!23 = !DILocation(line: 8, column: 15, scope: !15)
!24 = !DILocation(line: 9, column: 3, scope: !15)
!25 = !DILocation(line: 9, column: 9, scope: !15)
!26 = !DILocation(line: 9, column: 16, scope: !15)
!27 = !DILocation(line: 10, column: 10, scope: !15)
!28 = !DILocation(line: 10, column: 16, scope: !15)
!29 = !DILocation(line: 10, column: 3, scope: !15)
