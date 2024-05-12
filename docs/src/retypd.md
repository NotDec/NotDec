# retypd

**Intro**

自此《[Polymorphic Type Inference for Machine Code](https://arxiv.org/abs/1603.05495)》论文以来，在反编译器的类型分析就不再有进展。

跨函数的类型传播算法，可以将库函数的类型传递到代码内部。即使没有库函数识别时，也可以识别处函数内的结构体信息。

**资源**

[retypd](https://github.com/GrammaTech/retypd) 是一个高级的反编译算法，有一篇[论文](https://github.com/GrammaTech/retypd/blob/master/reference/paper.pdf): 《[Polymorphic Type Inference for Machine Code](https://arxiv.org/abs/1603.05495)》

资源：
- 优先看[这个介绍](https://github.com/GrammaTech/retypd/blob/master/reference/type-recovery.rst)。
- 这个[PPT](https://github.com/GrammaTech/retypd/blob/master/reference/presentation_slides.pdf)比论文容易懂很多


## 背景

**规则生成**
- 复制操作：x := y，此时保守地认为，有可能是子类型赋值给了父类型变量：Y ⊑ X。
    - 由复制操作带来的数据流，类型方向是父类型。子类型赋值给了父类型。
- 指针加载：x := *p，生成：P.load.σ32@0 ⊑ X 。
- 指针赋值：*q := y，生成 Y ⊑ Q.store.σ32@0 。
- 函数调用：如果有调用y := f(x)，生成 X ⊑ F.in 和 F.out ⊑ Y 。
- icmp：两值进行比较的时候：1 bool类型是结果的子类型。2 两个被比较的值直接，随便生成一个子类型关系？

## 开发与使用

如何使用当前开源的代码呢？代码是一个python模块。当前开源的两个相关的使用代码有：[retypd-ghidra-plugin](https://github.com/GrammaTech/retypd-ghidra-plugin)和[gtirb-ddisasm-retypd](https://github.com/GrammaTech/gtirb-ddisasm-retypd)。

首先分析[retypd-ghidra-plugin](https://github.com/GrammaTech/retypd-ghidra-plugin)是如何使用retypd的。内部代码主要分为ghidra插件的java代码，和封装模块，[ghidra_retypd_provider](https://github.com/GrammaTech/retypd-ghidra-plugin/tree/master/ghidra_retypd_provider)。Java代码部分通过Ghidra提供的API，从Ghidra的反编译器的IR中提取出相关的类型约束，提取为json文件。然后调用python封装模块读取并求解，结果也表示为json文件。然后Ghidra插件部分的java代码读取结果，并设置相应的类型。（注，无论是上次分析lua虚拟机，还是这次分析`/bin/ls`，花的时间特别久，半小时往上）

输入ghidra_retypd_provider的样例json约束文件如下。可以观察到，每个函数的约束单独分开，同时还包含一个call graph部分。

```json
{
  "language": "x86/little/64/default",
  "constraints": {
    "FUN_00109d00": [
        "v_7456 ⊑ v_7780",
        "v_997 ⊑ int64",
        "v_1441 ⊑ FUN_00109b50.in_13",
        "v_4504 ⊑ v_1242.store.σ8@0",
        "v_6777 ⊑ FUN_00109b50.in_5",
        "bool ⊑ v_542",
        "v_2301 ⊑ null",
        "v_7379.load.σ8@0*[nobound] ⊑ v_1441",
        "v_4396 ⊑ v_1671.store.σ8@0*[nobound]",
        "v_1188.load.σ8@0 ⊑ v_1191",
        "v_1671.load.σ8@0*[nobound] ⊑ v_1720",
    ],
    "FUN_00110e10": [
        ...
    ]
  }
  "callgraph": {
    "FUN_00109d00": [
      "FUN_001158c0",
      "FUN_00115920",
      "FUN_00109b50",
      "FUN_00115b30"
    ],
    "FUN_00110e10": [
      "strcmp",
      "strlen",
      "FUN_001158c0",
      "getgrnam",
      "strcpy"
    ],
    ...
  }
}
```

输出ghidra_retypd_provider的样例json结果文件如下。可以观察到，包含两种类型的结果，分别是结构体和函数。结构体包含内部的成员及类型。函数块描述了函数的各个参数的类型。在[这里](https://github.com/am009/retypd-ghidra-plugin/blob/e4f587a2560148f0d0ebbdb2f26fc9977587661f/ghidra_retypd_provider/type_serialization.py#L51)的自定义encoder中定义了转json的函数。

这一点其实很奇怪，似乎该插件关注的核心是函数参数类型。这里[后续解析和类型设置](https://github.com/am009/retypd-ghidra-plugin/blob/7c2547574cd86100e02793ae2c8c0cc8f88c3990/GhidraRetypd/src/main/java/ghidraretypd/RetypdTypes.java#L272)也说明了这一点。

```json
[
  {
    "type": "struct",
    "name": "struct_545",
    "fields": [
      {
        "name": "field_0",
        "type": "char1_t*",
        "offset": 0
      },
      {
        "name": "field_1",
        "type": "char1_t[4]",
        "offset": 168
      }
    ]
  },
  {
    "type": "function",
    "name": "function_260",
    "params": [
      {
        "index": 0,
        "type": "char1_t[4]"
      }
    ],
    "ret": "char1_t[0]"
  },
  ...
]
```

接着我们看ghidra_retypd_provider内部是如何调用retypd的。

- 使用`SchemaParser.parse_constraint`解析每个约束项（`SubtypeConstraint`），它保存子类型关系左右两边的变量（`DerivedTypeVariable`）。每个函数的约束项放到一个集合里，再按函数名字放到map里，然后构建`Program`：
    ```python
    program = Program(
        CLattice(),
        {},
        parsed_constraints,
        callgraph,
    )
    ```
    这里`parsed_constraints`就是准备好的那个map。callgraph都不用动，就是`Dict[str, List[str]]`。前两个参数分别是`types: Lattice[DerivedTypeVariable]`和`global_vars: Iterable[MaybeVar]`。
- 使用Solver去求解约束：
    ```python
    config = SolverConfig(top_down_propagation=True)
    solver = Solver(program, config, verbose=LogLevel.DEBUG)
    _, sketches = solver()
    ```
    查看solver的`__call__`方法，可以发现返回类型是`Dict[DerivedTypeVariable, ConstraintSet]`和`Dict[DerivedTypeVariable, Sketch]`。
- 传入`CTypeGenerator`，得到最终的类型结果。
    ```python
    gen = CTypeGenerator(
        sketches,
        CLattice(),
        CLatticeCTypes(),
        int_size,
        pointer_size,
    )
    return list(gen().values())
    ```
    `CTypeGenerator`的`__call__`方法的返回类型是`Dict[DerivedTypeVariable, CType]`。

根据[这里](https://github.com/GrammaTech/retypd/blob/e2c2adac5b123aa68b573192519f0d01e365527d/src/parser.py#L111)，每个规则大致就是`var1 ⊑ var2`或者`var1 <= var2`，然后两边的变量就是DerivedTypeVariable类型的。因此，上面结果里返回的map其实就能够用来查每个变量的类型。

**S-Pointer and S-Field⊕/⊖** 通过搜索代码，可以得知，至少在[retypd-ghidra-plugin](https://github.com/GrammaTech/retypd-ghidra-plugin)和[gtirb-ddisasm-retypd](https://github.com/GrammaTech/gtirb-ddisasm-retypd)中是没有和这个相关的规则的生成的。

#### retypd ghidra 插件

在我的[fork](https://github.com/am009/retypd-ghidra-plugin)仓库里可以直接下载到构建好的插件，修改版Ghidra，以及docker镜像。同时提供了给retypd的样例输入和输出。

通过修改 GhidraRetypd.zip中的extension.properties，可以绕过ghidra插件安装时的版本检查

```Python
version=10.2.3
```

安装Ghidra插件：

    1. 打开 Ghidra 软件，点击 "File" 菜单，选择 "Install Extensions" 选项。
    2. 在弹出的 "Install Extensions" 窗口中，点击 "Browse" 按钮选择你要安装的扩展程序。
    3. 选中你要安装的扩展程序文件（通常是一个 zip 压缩文件），然后点击 "Open" 按钮。
    4. 点击 "OK" 按钮开始安装扩展程序。在安装过程中，Ghidra 软件会自动解压缩扩展程序文件，并将它们安装到正确的目录中。
    5. 安装完成后，重启 Ghidra。

## 算法详解

### 基础

**使用的符号**

- $\mathcal{V}$: 类型变量的集合
  - 在这个集合里包含一系列类型常量，作为一种符号描述。这些类型常量可能形成一个lattice，但是我们不解释它们。
- $\Sigma$: 字段标签 field label。不一定是有限的。主要的符号如下：
  - $\mathsf{.in}_L$ 函数在位置L的输入
  - $\mathsf{.out}_L$ 函数在L位置的输出
  - $\mathsf{.load}$ 可读的指针
  - $\mathsf{.store}$ 可写的指针
  - $.\sigma\mathsf{N@k}$ 在偏移k处有一个N bit的成员。
- 函数 $\langle \cdot \rangle : \Sigma \to \{ \oplus, \ominus \}$: 从结构体label映射到 $\{ \oplus, \ominus \}$ 表示variance，协变和逆变属性
- 派生的类型变量 derived type variable (定义3.1)：形为 $\alpha w$ ，其中类型变量 $\alpha \in \mathcal{V}$ and 字段标签 $w \in \Sigma^*$.
- 标签 $\ell$ 的variance (定义3.2)，指的是前面的类型变量的类型如果发生变化时，带标签的派生类型变量的variance的变化方向。 $\alpha.\ell$ 和 $\beta.\ell$ 中，如果 $\alpha$ 是 $\beta$ 的子类型。
- 约束 $c$ (定义3.3)有两种形式。约束的集合用 $\mathcal{C}$ 表示
  - 存在形式: (派生)类型变量X存在
  - 子类型形式：(派生)类型变量X是Y的子类型。
  - 约束的推导 $\mathcal{C} \vdash c$ 表示约束能从原约束集合中，由那些规则派生出来。
  - 约束中的自由变量定义 $\exists \tau . \mathcal{C}$ 表示，存在变量 $\tau$ 满足了约束集合。
- Type Scheme类型方案，表示一个泛型的函数。 $\forall{\overline{\alpha}}.{\mathcal{C}}\Rightarrow{\beta}$ 表示在约束C的条件下，带有模板变量集合 $\overline{\alpha}$ 的泛型类型 $\beta$
  - 仅增加约束： $\forall \tau . C \Rightarrow \tau$ 表示仅对类型 $\tau$ 增加约束。 
    - 例如 $\forall \tau . (\tau.\mathsf{in}.\mathsf{load}.\sigma\mathsf{32@4} \sqsubseteq \tau.\mathsf{out}) \Rightarrow \tau$ 表示函数返回了参数在4字节offset位置的成员。
  

**常见术语**

- pushdown system: 在基本的自动机的基础上，额外增加了一个栈结构。
- non-structural subtyping: 即子类型关系不一定非要结构完全相同（在structural subtyping中只能叶子节点不同）。尤其是在有结构体和对象这种情况。见["Type Inference with Non-structural Subtyping"](https://web.cs.ucla.edu/~palsberg/paper/fac97.pdf)


**规约规则**

- **T-Left**/**T-Right**/**T-Prefix**: 如果存在约束 $\alpha \sqsubseteq \beta$ ，则 $\alpha$ 和 $\beta$ 存在。如果存在一个带field label的派生变量，则原始变量存在。
  - 这意味着在算法中我们在访问约束时会创建对应变量节点。
- **T-InheritL** / **T-InheritR**: 子类型能安全代换父类型。父类型如果能带一个field label，则子类型带有相同的field label的派生变量也存在。
- **S-Refl**: 反射性，自己是自己的子类型。
- **S-Field$_\oplus$** / **S-Field$_\ominus$**: 如果field label的variance是协变 $\oplus$，则原变量子类型关系在带上标签后保持。否则反过来。
- **S-Pointer**: 指针存入的类型是取出的子类型。

**sketches** 约束的求解结果被表示为sketches。每个value关联上一个sketch，包含该value的所有能力，即能否被store，能否访问指定的偏移。同时sketch还包含一个可自定义的lattice，用来传播类似于typedef这种类型。

我们分析的不是具体的程序中的变量，而是他们的类型和类型之间的关系。因为复杂的约束关系，我们会把类型再设为一个类型变量，称为DataTypeVariable，DTV。

**什么是Sketches**：一个派生类型变量DTV，可能有各种各样的能力，比如可以在offset为4的地方load出一个四字节的值 （`.load.σ32@4`）。首先可以遍历所有的约束关系，比如`v_4504 ⊑ v_1242.store.σ8@0`，对每个关系单独看两边的变量，然后看比如`v_1242`是否被直接这样取过offset，然后把这些操作收集起来。但是这样还不够，因为可能因为约束的存在，其他变量能做的操作，它因为约束，应该也能做。这些都求解出来，得到的数据结构就是一个Sketch。

一个Sketch才是真正直接代表一个具体的类型。是一个树状的类型结构。这个树的边上标记了field label，节点上标记了类型lattice上的元素。

**基于程序操作的约束生成**

- 变量复制/赋值：要么两边类型相同，要么根据安全代换原则，子类型被赋值为父类型。
- 指针读取：增加field label。
  - 指针的读和写能力分开考虑。子类型方面特殊处理。
- 函数调用：参数父类型，返回值子类型。
  - 单独的类型变量规则是structural的，即子类型和父类型的能力必须一致。但是在函数调用时，可以遗忘一些能力。

## 第五章 The Simplification Algorithm

In this section, we sketch an outline of the simplification algorithm at the core of
the constraint solver.  The complete algorithm appears in \aautoref{construction}{Appendix D of \citep{arxiv}}.

本节中，我们将概述约束求解器核心的简化算法。完整的算法见 \aautoref{construction}{Appendix D of \citep{arxiv}}。\label{simplify}

### 推断类型方案 Inferring a Type Scheme

The goal of the simplification algorithm is to take an inferred type scheme
$\scheme{\overline{\alpha}}{C}{\tau}$ for a procedure and create a smaller constraint set
$\mathcal{C}'$, such that any constraint on $\tau$ implied by $\mathcal{C}$
is also implied by $\mathcal{C}'$.

这个简化算法的目标是接受一个推断出的类型方案 $\scheme{\overline{\alpha}}{C}{\tau}$ 用于一个过程，并创建一个较小的约束集 $\mathcal{C}'$，使得任何由 $\mathcal{C}$ 对 $\tau$ 的约束也被 $\mathcal{C}'$ 所蕴含。

Let $\mathcal{C}$ denote the constraint set generated by abstract interpretation
of the procedure being analyzed, and let $\overline{\alpha}$ be the set of
free type variables in $\mathcal{C}$.  We could already use
$\scheme{\overline{\alpha}}{\mathcal{C}}{\tau}$ as the
constraint set in the procedure's type scheme, since the
input and output types used in a valid invocation of {\verb|f|}
are tautologically those that satisfy $\mathcal{C}$.
Yet, as a practical matter, we cannot  use the constraint set directly, since this would result in
constraint sets with many useless free variables and a high growth rate over
nested procedures.

令 $\mathcal{C}$ 表示由抽象解释生成的过程的约束集，并且 $\overline{\alpha}$ 是 $\mathcal{C}$ 中的自由类型变量集。我们其实已经可以使用 $\scheme{\overline{\alpha}}{\mathcal{C}}{\tau}$ 作为过程类型方案中的约束集，因为合法调用 {\verb|f|} 时使用的输入和输出类型显然是满足 $\mathcal{C}$ 的。然而，实际上我们不能直接使用这个约束集，因为这会导致在嵌套过程中产生很多无用的自由变量，并且约束集的增长率很高。

Instead, we seek to generate a {\em simplified constraint set} $\mathcal{C}'$,
such that if $c$ is an ``interesting'' constraint
and $\mathcal{C} \proves c$ then $\mathcal{C}' \proves c$ as well.
But what makes a constraint interesting?

相反，我们寻求生成一个**简化的约束集** $\mathcal{C}'$，使得如果 $c$ 是一个“有趣”的约束，并且 $\mathcal{C} \proves c$，那么 $\mathcal{C}' \proves c$ 也同样成立。但什么让一个约束变得有趣呢？

- 能力约束，表示某个dtv有某个field label
- 递归类型约束： $\tau.u \subtype \tau.v$
- 涉及常量类型的约束： $\tau.u \subtype \overline{\kappa}$ or $\overline{\kappa} \subtype \tau.u$ 其中 $\overline{\kappa}$ 是类型常量.

### 无约束的下推系统 Unconstrained Pushdown Systems

约束简化算法通过对约束集合 $\mathcal{C}$ 构建一个下推系统 $\mathcal{P}_\mathcal{C}$ 来工作，其转换序列表示有效的子类型推导判断。此处我们快速回顾一下下推系统及其一些必要的一般化。

定义：一个**无约束下推系统**是由三个部分组成的元组 $\mathcal{P} = (\mathcal{V}, \Sigma, \Delta)$，其中 $\mathcal{V}$ 是**控制位置**的集合，$\Sigma$ 是**栈符号**的集合，而 $\Delta$ 是包含在 $(\mathcal{V} \times \Sigma^*)^2$ 内的（可能无限）**转换规则**的集合。转换规则表示为 $\langle X; u \rangle \hookrightarrow \langle Y;v\rangle$，其中 $X,Y \in \mathcal{V}$ 且 $u,v \in \Sigma^*$。我们定义**配置**的集合为 $\mathcal{V} \times \Sigma^*$。在配置 $(p,w)$ 中，$p$ 称为**控制状态**，$w$ 称为**栈状态**。

注意到，我们既不要求栈符号的集合也不要求转换规则的集合是有限的。这种自由度是为了模拟推导规则 S-Pointer， 正如图3的推导规则 S-Pointer 所示，它对应于一个无限的转换规则集。

Definition: An unconstrained pushdown system $\mathcal{P}$ determines a {\em transition relation} $\rewrite$ on
the set of configurations:
$(X,w) \rewrite (Y,w')$ if there is a suffix $s$ and a rule $\pdsrule{X}{u}{Y}{v}$,
such that $w = us$ and $w' = vs$. The transitive closure of $\rewrite$ is denoted $\rewrites$.

With this definition, we can state the primary theorem behind our simplification algorithm.
\begin{theorem}
\label{mod-sat}
Let $\mathcal{C}$ be a constraint set and $\mathcal{V}$ a set of base type variables. Define a
subset $S_\mathcal{C}$ of $(\mathcal{V} \cup \Sigma)^* \times (\mathcal{V} \cup \Sigma)^*$
by $(Xu, Yv) \in S_\mathcal{C}$ if and only if $\mathcal{C} \proves X.u \subtype Y.v$.
Then $S_\mathcal{C}$ is a regular set, and an automaton $Q$ to recognize $S_\mathcal{C}$ can
be constructed in $O(|\mathcal{C}|^3)$ time.
\end{theorem}
\begin{proof}The basic idea is to treat each $X.u \subtype Y.v \in \mathcal{C}$
as a transition rule $\pdsrule{X}{u}{Y}{v}$ in the pushdown system $\mathcal{P}$.  In addition,
we add control states $\stStart, \stEnd$ with transitions $\pdsrule{\stStart}{X}{X}{\varepsilon}$
and $\pdsrule{X}{\varepsilon}{\stEnd}{X}$ for each $X \in \mathcal{V}$.
For the moment,
assume that (1) all labels are covariant, and (2) the rule $\textsc{S-Pointer}$ is ignored.
By construction, $(\stStart, Xu) \rewrites (\stEnd, Yv)$ in $\mathcal{P}$ if and only if $\mathcal{C} \proves X.u \subtype Y.v$.  A theorem of B\"uchi \cite{richard1964regular} ensures that  for any two control states $A$ and $B$ in a standard (not unconstrained) pushdown system, the set of all pairs $(u,v)$
with $(A, u) \rewrites (B, v)$ is a regular language; \citet{caucal1992regular} gives a saturation
algorithm that constructs an automaton to recognize this language.

In the full proof, we add two novelties: first, we support contravariant stack symbols by
encoding variance data into the control states and transition rules. The second novelty
involves the rule $\textsc{S-Pointer}$; this rule is problematic since the natural
encoding would result in infinitely many transition rules. We extend Caucal's construction to lazily instantiate all necessary applications of
$\textsc{S-Pointer}$ during saturation.  For details, see \aautoref{construction}{Appendix D of \citep{arxiv}}.
\end{proof}

Since $\mathcal{C}$ will usually entail an infinite number of constraints,
this theorem is particularly useful: it tells us that the full set of
constraints entailed by $\mathcal{C}$ has a finite encoding by an automaton $Q$.
Further manipulations on the constraint closure, such as efficient minimization, can be carried out on $Q$. By restricting the transitions to and from $\stStart$ and $\stEnd$, the same algorithm
is used to eliminate type variables, producing the desired constraint simplifications.

 

\subsection{Overall Complexity of Inference}
The saturation algorithm used to perform constraint-set simplification and type-scheme construction
is, in the worst case, cubic in the number of subtype constraints to simplify.  Since some well-known pointer analysis methods also have cubic complexity (such as \citet{andersen1994program}), it is reasonable to wonder if Retypd's ``points-to free'' analysis really offers a benefit over a type-inference system built on top of points-to analysis data.

To understand where Retypd's efficiencies are found, first consider the $n$ in $O(n^3)$.  Retypd's core saturation algorithm is cubic in the number of subtype constraints; due to the simplicity of machine-code instructions, there is roughly one subtype constraint generated per instruction.  Furthermore, Retypd applies constraint simplification on each procedure in isolation to eliminate the procedure-local type variables, resulting in constraint sets that only relate procedure formal-ins, formal-outs, globals, and type constants. In practice, these simplified constraint sets are small.

Since each procedure's constraint set is simplified independently, the $n^3$ factor is controlled by the largest procedure size, not the overall size of the binary.  By contrast, source-code points-to analysis such as Andersen's are generally cubic in the overall number of pointer variables, with exponential duplication of variables depending on the call-string depth used for context sensitivity.  The situation is even more difficult for machine-code points-to analyses such as VSA, since there is no syntactic difference between a scalar and a pointer in machine code. In effect, every program variable must be treated as a potential pointer.

On our benchmark suite of real-world programs, we found that execution time for Retypd scales slightly below $O(N^{1.1})$, where $N$ is the number of program instructions.  The following back-of-the-envelope calculation can heuristically explain much of the disparity between the $O(N^3)$ theoretical complexity and the $O(N^{1.1})$ measured complexity.  On our benchmark suite, the maximum procedure size $n$ grew roughly like $n \approx N^{2/5}$. We could then expect that a per-procedure analysis would perform worst when the program is partitioned into $N^{3/5}$ procedures of size $N^{2/5}$.
On such a program, a per-procedure $O(n^k)$ analysis may be expected to behave more like an $O(N^{3/5} \cdot (N^{2/5})^k) = O(N^{({3 + 2k})/{5}})$ analysis overall.  In particular, a per-procedure cubic analysis like Retypd could be expected to scale like a global $O(N^{1.8})$ analysis.  The remaining differences in observed versus theoretical execution time can be explained by the facts that real-world constraint graphs do not tend to exercise the simplification algorithm's worst-case behavior, and that the distribution of procedure sizes is heavily weighted towards small procedures.

## TODO

- monoid: 读《Haskell趣学指南》的Monoids一节，理解一下。

**unification-based type inference algorithms**

https://www.cs.cornell.edu/courses/cs3110/2011sp/Lectures/lec26-type-inference/type-inference.htm
