; ModuleID = '/sn640/NotDec/test/type-recovery/debug-info-c/cases/02_absolute_self_ref.c'
source_filename = "/sn640/NotDec/test/type-recovery/debug-info-c/cases/02_absolute_self_ref.c"
target datalayout = "e-m:e-p:32:32-p10:8:8-p20:8:8-i64:64-n32:64-S128-ni:1:10:20"
target triple = "wasm32-unknown-wasi"

%struct.Node = type { i32, %struct.Node* }

; Function Attrs: noinline nounwind
define hidden i32 @init_node() #0 !dbg !14 {
entry:
  %node = alloca %struct.Node*, align 4
  call void @llvm.dbg.declare(metadata %struct.Node** %node, metadata !18, metadata !DIExpression()), !dbg !19
  store %struct.Node* inttoptr (i32 1024 to %struct.Node*), %struct.Node** %node, align 4, !dbg !19
  %0 = load %struct.Node*, %struct.Node** %node, align 4, !dbg !20
  %value = getelementptr inbounds %struct.Node, %struct.Node* %0, i32 0, i32 0, !dbg !21
  store i32 7, i32* %value, align 4, !dbg !22
  %1 = load %struct.Node*, %struct.Node** %node, align 4, !dbg !23
  %next = getelementptr inbounds %struct.Node, %struct.Node* %1, i32 0, i32 1, !dbg !24
  store %struct.Node* inttoptr (i32 1024 to %struct.Node*), %struct.Node** %next, align 4, !dbg !25
  %2 = load %struct.Node*, %struct.Node** %node, align 4, !dbg !26
  %next1 = getelementptr inbounds %struct.Node, %struct.Node* %2, i32 0, i32 1, !dbg !27
  %3 = load %struct.Node*, %struct.Node** %next1, align 4, !dbg !27
  %cmp = icmp ne %struct.Node* %3, null, !dbg !28
  %conv = zext i1 %cmp to i32, !dbg !28
  ret i32 %conv, !dbg !29
}

; Function Attrs: nofree nosync nounwind readnone speculatable willreturn
declare void @llvm.dbg.declare(metadata, metadata, metadata) #1

attributes #0 = { noinline nounwind "frame-pointer"="none" "min-legal-vector-width"="0" "no-builtins" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="generic" }
attributes #1 = { nofree nosync nounwind readnone speculatable willreturn }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!10, !11, !12}
!llvm.ident = !{!13}

!0 = distinct !DICompileUnit(language: DW_LANG_C99, file: !1, producer: "Ubuntu clang version 14.0.0-1ubuntu1.1", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, retainedTypes: !2, splitDebugInlining: false, nameTableKind: None)
!1 = !DIFile(filename: "/sn640/NotDec/test/type-recovery/debug-info-c/cases/02_absolute_self_ref.c", directory: "/sn640/NotDec")
!2 = !{!3}
!3 = !DIDerivedType(tag: DW_TAG_pointer_type, baseType: !4, size: 32)
!4 = distinct !DICompositeType(tag: DW_TAG_structure_type, name: "Node", file: !5, line: 1, size: 64, elements: !6)
!5 = !DIFile(filename: "test/type-recovery/debug-info-c/cases/02_absolute_self_ref.c", directory: "/sn640/NotDec")
!6 = !{!7, !9}
!7 = !DIDerivedType(tag: DW_TAG_member, name: "value", scope: !4, file: !5, line: 2, baseType: !8, size: 32)
!8 = !DIBasicType(name: "int", size: 32, encoding: DW_ATE_signed)
!9 = !DIDerivedType(tag: DW_TAG_member, name: "next", scope: !4, file: !5, line: 3, baseType: !3, size: 32, offset: 32)
!10 = !{i32 7, !"Dwarf Version", i32 4}
!11 = !{i32 2, !"Debug Info Version", i32 3}
!12 = !{i32 1, !"wchar_size", i32 4}
!13 = !{!"Ubuntu clang version 14.0.0-1ubuntu1.1"}
!14 = distinct !DISubprogram(name: "init_node", scope: !5, file: !5, line: 6, type: !15, scopeLine: 6, flags: DIFlagPrototyped, spFlags: DISPFlagDefinition, unit: !0, retainedNodes: !17)
!15 = !DISubroutineType(types: !16)
!16 = !{!8}
!17 = !{}
!18 = !DILocalVariable(name: "node", scope: !14, file: !5, line: 7, type: !3)
!19 = !DILocation(line: 7, column: 16, scope: !14)
!20 = !DILocation(line: 8, column: 3, scope: !14)
!21 = !DILocation(line: 8, column: 9, scope: !14)
!22 = !DILocation(line: 8, column: 15, scope: !14)
!23 = !DILocation(line: 9, column: 3, scope: !14)
!24 = !DILocation(line: 9, column: 9, scope: !14)
!25 = !DILocation(line: 9, column: 14, scope: !14)
!26 = !DILocation(line: 10, column: 10, scope: !14)
!27 = !DILocation(line: 10, column: 16, scope: !14)
!28 = !DILocation(line: 10, column: 21, scope: !14)
!29 = !DILocation(line: 10, column: 3, scope: !14)
