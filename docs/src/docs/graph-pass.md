## Graph Type Pass Algorithm

The `GraphTypePass` organizes pointer nodes in the constraint graph and assigns one of several structural type descriptions (simple pointer, array, struct, union). The core routine is `ConstraintsGenerator::organizeTypes`, which operates on the post-processed graph produced by earlier passes. The algorithm proceeds as follows:

1. **Graph Sanity Checks**: Iterate over every edge to assert that determinization already eliminated `one`, `forget`, and `store` labels, leaving only recall/load edges. The pass assumes there are no cycles through subtype/offset edges and that load/store edges are merged.
2. **DFS Type Construction**: Perform a recursive walk (`doBuild`) over pointer nodes. Memoization via `TypeInfos` prevents reprocessing and detects cycles.
3. **Simple Pointers**: If a pointer node has no offset edges and at most one load edge, classify it as a simple pointer, recording the dereferenced size when present.
4. **Single-Stride Arrays**: When a node has exactly one offset edge whose label starts at offset 0 and carries a single access size, treat it as an array. The element size equals the access size; recursion continues on the target node.
5. **Self-Offset Loops**: Detect self edges representing repeated strides. If the minimum stride exceeds the pointer size, create a synthetic array node, redirect outgoing edges through it, and recurse. Otherwise delete trivial self edges.
6. **Struct Normalization**:
   - Split load edges into dedicated field nodes so they appear as zero-offset recall edges followed by the load.
   - Collect remaining offset edges, grouping them by stride. For each stride group, sort by base offset and carve contiguous ranges. Each range becomes a field node; within the field node, residual access labels form nested array-like structures.
7. **Remaining Fields**: For leftover offset edges, recurse on their targets, fetch their sizes, and add struct fields based on offset and size.
8. **Union Detection**: Repeatedly scan field ranges for overlaps. When overlaps are found, expand to the minimal covering interval, create a union node, and partition members into non-overlapping "panels" (struct-like sequences). Each panel becomes its own struct node unless it contains a single field. The union node (or original pointer when appropriate) receives a `UnionInfo` with the member edges.
9. **Finalize Struct Info**: After unions are resolved, sort the remaining fields by offset, compute the enclosing size, and store a `StructInfo` entry.
10. **Entry Points**: Apply the recursive builder to every pointer node in the graph, forcing memory nodes to be treated as structs.

Two supporting routines simplify the graph after initial organization:

- `mergeArrayUnions` collapses unions that contain exactly two members when one member is an array and the other has the same size as the array element. The union node is removed, the other member is merged with the array element node, and type metadata is updated.
- `mergeNodeAndType` and `mergeFixTypeInfo` merge graph nodes while keeping `TypeInfos` consistent, handling cases where both nodes share compatible type categories (simple, struct, array) or when one lacks a type description.

Overall, the pass rewrites the constraint graph into a hierarchy of synthetic nodes that capture structs, arrays, and unions. These synthesized TypeInfos enable later stages to recover structured types from the low-level pointer graph.
