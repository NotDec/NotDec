# NotDec: Bringing the sparse pointer analysis into assembly

## Abstract (TODO)

**Contributions** (Probably should split into different papers)
- Investigated the difficulties to apply source-level analysis to binary code.
- The first tool that supports pointer analysis on Webassembly.
- A Webassembly analysis platform based on LLVM IR.
    - supports many analysis: symbolic execution, 
- A Webassembly decompiler that out-performs other decompiler.

## Introduction

There are many existing work that focus on the analysis of binary code, like disassembly. <二进制分析的历史也很久>. However, some analyses, even on non-obfuscated binary, are still way more difficult than that on the source level, especially with recent advances in sparse pointer analysis.

Some analysis views the code as a black or gray box, and is easier to perform on binary code. Some analysis is closely related to the high-level semantics of the language, and usually requires source code or high-level IR. According to this, we classify them as low-level analysis or high-level analysis, correspondingly.

- High level analysis: Pointer analysis, dataflow/taint tracking analysis.

- Low level analysis: Fuzzing, symbolic execution, binary instrument, DNN-based discriminability analysis.

The difficulties is not solely caused by the fact that assembly and high-level languages (e.g., C/C++) are different languages. The IR produced by existing lifters do not perform well in the pointer analysis task `SoK_Demystifying_Binary_Lifters_Through_the_Lens_of_Downstream_Applications`. This suggests that there are other key differences between the lifted IR and compiler IR.

We believe that, due to the low-level nature of binary code, the transformation from source code to assembly code complicates analysis as a side effect. By investigating on the problem, we find two reasons.
- the usage of activation record/stack frame.
- the mixture of integer type and pointer type.

We believe that these difficulties can be, and should be solved by some decompiler algorithms. 

Some may argue that, decompiler's correctness is hard to be proved, and when it performs bad, the correctness of subsequent analysis will also be tamed.

However, with the [advance of decompiler][1], the

Pointer analysis serves as an important basis for other analyses like taint/dataflow analysis.

<!-- decompiler algorithm that focus on correctness? First, if the analysis is sure, it will leave the function unchanged. -->

<!-- 画一个图，从二进制代码到其他low level 分析是直接打勾，到其他高层分析就标灰色(未探索过)，中间经过我们的关注正确性的反编译算法。 -->

[1]: <> (How Far Have we come, test decompiler.)

webassembly is similar to other assembly, without some already mature problem, but with the same (easier) problem that prevents the analysis.

**Motivation of this project**

1. existing lifter is only capable of doing "low level" analysis, but performs badly on source level analysis, most importantly, pointer analysis. <> 

    

1. Most existing work focus on the low level analysis, , we can utilize existing source-level analysis and extend them to support binary.


~~1. Existing decompiler only offer as a whole part, however, the decompilation algorithm can be useful to boost the scalability and precision of existing binary analysis techniques.~~


## Background


**formally verified decompilation is not possible** There are attempts on formally verified decompilation `BIRD: A Binary Intermediate Representation for Formally Verified Decompilation of X86-64 Binaries`, but it currently only reaches formally verified disassembly. 

We attempt to demonstrate the difficulty of formally verified stack variable recovery by an example in webassembly:
- In webassembly, the heap, the stack and the global variables shares a continuous address space (at maximum 4GB). If the stack keeps growing, it will overlap with the heap and the global variable region.
- The source-level program, will not have this effect. So if the stack grows above the limit of overlapping, the semantic between webassembly and the source code differ.
- even if it is not the intension of program author, the process of formal verification may not ignore this difference.
- At least in this case, formally verified decompilation is not possible.



~~- It is foreseeably hard for the formal verifier to prove if one program will not infinite recursion.~~


### webassembly stack in memory


## Motivating example


## Challenge/Roadmap


### roadmap to high level analysis

1. no pointer type
1. 

stack allocation recovery/pointer type recovery/high-type recovery



## Design/Approach


## Motivation


