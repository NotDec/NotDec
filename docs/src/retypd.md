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

- 首先阅读[这个PPT](http://www.lsv.fr/~schwoon/enseignement/verification/ws1112/c3.pdf)和这个论文《[Saturation algorithms for model-checking pushdown systems](https://arxiv.org/pdf/1405.5593)》，学会B¨uchi发现， Caucal提出的这个saturation算法。
- 然后阅读论文，学习其中基于saturation算法改进的部分。

### 自动机基础 Saturation algorithms

资料：

- 《[Pushdown systems](http://www.lsv.fr/~schwoon/enseignement/verification/ws1112/c3.pdf)》了解下推自动机
- 《[Saturation algorithms for model-checking pushdown systems](https://arxiv.org/abs/1405.5593)》了解Saturation algorithms相关的背景概念。

此外，一个重要的结论是下推系统可达的栈内容形成了一个正则语言，因此可以表示为有限状态自动机。但是最初提出的算法是指数级的。后面有人提出了多项式级的算法，核心思想就是引入一个saturation process，转换被逐渐地加入有限自动机。

**有限自动机**

- $\Sigma$ 是自动机的字符集。$\Sigma^{*}$ 是字符串的集合。 $\Gamma^{\leq n}$ 是长度最多为n的字符串集合。
- 一个在字符集 $\Sigma$ 上的自动机 $\mathcal{A}$ 是一个元组： $(\mathbb{S},\mathcal{I},\mathcal{F},\delta)$ 分别代表有限的状态集合，初始状态集合，最终状态集合，转换规则集合 $\delta \subseteq \mathbb{S} \times \Sigma \times \mathbb{S}$ 表示从一个状态遇到一个字符，转换到另外一个状态。
  - 有时会把字符集写进来 $(\mathbb{S},\Sigma,\mathcal{I},\mathcal{F},\delta)$ 变成五元组
- $s \overset{a}{\underset{\mathcal{A}}{\longrightarrow}} t$ 表示自动机 $\mathcal{A}$ 有这样一个转换 $(s,a,t)$
- $s \overset{w}{\underset{\mathcal{A}}{\Longrightarrow}} t$ 表示自动机读入字符串w之后可以转换到t。

#### 下推系统 pushdown system 与 P-自动机

**和下推自动机的区别**：pushdown system是自动机的简化版本。当我们用PDS建模程序的时候，有时不太关注读入和识别字符，而是关注栈状态的转移。

和自动机的区别在于没有识别字符串这种概念。自动机一般只能用来识别字符串，有初始态，终止态，遇到某个字符的时候触发转换规则。这里的下推系统则不是识别字符串的，即转换规则里不需要读入字符，直接改变状态和栈内容。这里下推系统直接主要关注栈上的内容字符串。

一个下推系统是四元组 $(Q,\Gamma,\bot,\Delta)$

- $Q$ 是有限的控制状态的集合
- $\Gamma$ 是有限的栈字符集
- $\bot \in \Gamma$ 是一个特殊的底部栈字符
- $\Delta \subseteq (Q \times \Gamma) \times (Q \times \Gamma^{\leq 2})$ 是转换的集合。表示在一个状态从栈上弹出一个字符，转换到另外一个状态并压入一个字符串。
- Configuration表示系统状态，是 当前状态 $\in Q$ 和 当前栈字符串 $\in \Gamma^{*}$ 的二元组。
- 不考虑弹出超过栈底的情况。所以有时会引入特殊底部栈字符，然后要求规则不会把这个符号弹出来。

**Pushdown system （PDS）可以用来分析程序**

- 程序表示
  - 状态Q保存程序的全局变量
  - 栈反映程序执行的调用栈
  - 栈上字符编码程序局部变量：比如编码为 当前program counter 和 局部变量的二元组。
- 一些例子
  - 状态 $pAw$, 这里 $p$ 表示状态，代表全局变量， $Aw$ 表示栈。其中 $A$ 表示当前的PC和局部变量，w表示调用栈中，被“暂停”执行的程序状态，包括返回地址和局部变量情况。
  - 状态转换 $pA \rightarrow qB$ 表示执行一个普通语句。
    - 全局变量可能发生修改，从 $p$ 变成 $q$ ，
    - 栈深度不变，但是从A变成B，表示当前PC和局部变量发生了变化。
  - 状态转换 $pA \rightarrow qBC$ 表示函数调用
    - 栈深度增加了，B里面包含了返回地址以及调用者的局部变量状态。现在压入了被调用者的栈。
  - $pA \rightarrow q\epsilon$ 表示函数返回
    - 栈上弹出了一个字符，表示弹出了一个调用栈。

**PDS的可达性问题**

给定一个PDS和两个configuration $c,c'$，是否能从 $c$ 走到 $c'$ 状态？

$$
\{ w \in \Gamma^{*} \mid  \exists q \in Q, (q_{0},\bot) \Rightarrow (q,w) \}
$$

即求出这样一个集合，从起始状态 $(q_{0},\bot)$ 出发能推导到的所有状态 $(q,w)$ (这里状态 $q \in Q$ 任意)，其中字符串w组成的集合。

**一个非常重要的结论是，这里可达的字符串集合形成了正则语言。** 这意味着函数调用时的状态转换可以用一个有限状态自动机表示。

**PDS的configuration的正则性：**

- **背景**：众所周知，能被确定有限状态自动机识别的语言是正则（regular）语言。而说到语言，我们会想到根据语法生成的规则，最后推导出很多可以被接受的字符串，但是语言本质也就是一种无限的字符串的集合。
  - 如果是有限的话，直接对每个字符串用or连接就完全匹配了。所以一般讨论的都是无限的字符串集合。
- **定义1**：如果某个配置集合 $C$ 是正则的，那么对任意的状态 $p \in Q$ （即不管状态），在这个配置集合里面的所有字符串w构成的集合， $\{ w \in \Gamma^{*} \mid (p,w) \in C\}$ 是正则的。即栈状态构成正则语言。
  - 简单来说，不管state，如果栈上字符的集合构成正则的语言，则这个configuration集合是正则的。
- **定理1，初始状态可达的配置集合的正则性**：
  - 定义：初始配置：任意状态，但是栈为空（仅有 $\bot$ 符号）的配置
  - 任意pushdown system，从初始配置出发，可达的配置是正则的。
- **定理2：后推闭包的正则性**：给定一个正则配置集合C，基于自动机规则任意后推形成的闭包集合也是正则的。
  - 写作：$Post^*_{P}(C) = \{ c'  \mid \exists c \in C, c \underset{P}{\Longrightarrow} c' \}$ 从任意的C中配置开始，在自动机P下能推导出c'，则c'属于集合。
  - 这一定理能从定理1推出：构建一个新自动机P'，增加很多新的state，使得当前配置集合都是初始配置。这里定理里的后推可得到的配置集合其实就是可达的配置集合。
- **倒推闭包的正则性**：正则集合的倒推集合，即另外一个配置集合Pre*(C)，能够推理得到当前集合C。当前集合是正则的，则Pre*(C)也是正则的。
  - **原因**：有时候想要基于程序的错误状态，倒退前面的可达状态。
  - **证明**：思想是，构建新的自动机P'，规则和P是反着来的。
    - 如果P有个规则是 $qA \rightarrow p$ 即弹出了字符A。我们P'增加反着来，压入字符A的规则 $pX \rightarrow qAX$。即对于任意字符X，都允许压入AX。
    - 如果P有个规则是 $qA \rightarrow pBC$ 弹出A压入BC。则我们需要增加两个规则。
      - $pB \rightarrow r_{(C,q,A)}$
      - $r_{(C,q,A)} C  \rightarrow  q A$
      - 直接插入规则的问题在于，下推自动机左边一般只能匹配一个栈顶字符，右边则可以为空，也可以压入两个字符。这里我们需要考虑同时按顺序存在BC字符的情况。
      - 借助中间状态 $r_{(C,q,A)}$ 我们
    - **一致性**：如果在P中有两个状态可以互推，当且仅当我们P'中这两个状态可以反着互推。而且P'满足上面定理的要求，然后应用上面正推的定理，成功证明反推定理。


**PDS对应的自动机**

有一个PDS $\mathcal{P}=(P, \Gamma, \Delta)$，我们对应有一个有限自动机 $\mathcal{A}=(Q, \Gamma, P, T, F)$

- PDS的栈字符集 $\Gamma$ 被用作有限自动机的字符集。PDS栈字符原本用来表示局部变量和PC。
- PDS的控制状态（表示全局变量），被用作有限自动机的初始状态。
- 我们称，有限自动机接受配置 $pw$ ，如果自动机从状态p开始，接受字符串w之后能停止在终结态。而且对应每个PDS都能构造出这样的自动机。
  - 直观上理解：对任意的state（代表了程序的全局变量状态），此时可能的栈情况是什么样的？这些所有可能的栈情况可以用一个有限自动机来表达。
  - 上面提到，不考虑状态，单把栈上字符串拿出来，形成的也是正则语言。这里其实表达的是一个意思。每个configuration的状态作为初始状态，然后存在一个P-有限自动机匹配栈上的内容，其实就是说明了栈上形成的是正则语言。

**Pre(S)的Saturation Algorithm**

- 相关符号总结
  - $\Delta$: pushdown system的下推规则
  - $\delta$: P-自动机的状态推理规则。后面会不断增加规则，构成一个递增的规则序列。
  - $Q$: pushdown system的状态集合，也是P-自动机的初始状态
  - $\mathcal{F}$: P-自动机的终止状态集合
  - $\mathcal{A}$: P-自动机。包括状态集合 $\mathbb{S}$, 初始状态 $Q$, 推理规则 $\delta$ 和终止状态 $\mathcal{F}$。
  - $\mathcal{L}(\mathcal{A})$: 表示A接受的语言。
  - $\mathcal{B}$: 需要构建的新自动机，接受 $Pre^{*}_P (\mathcal{L}(\mathcal{A}))$
- Saturation Algorithm算法
  - 前提：已知一个下推系统 $P=(Q,\Gamma,\bot,\Delta)$ ,和对应的P-自动机 $\mathcal{A}=(\mathbb{S},Q,\delta,\mathcal{F})$
  - 目标：构建出一个新的自动机 $\mathcal{B}$ 接受语言 $Pre^{*}_P (\mathcal{L}(\mathcal{A}))$
  - 过程：构建一个自动机的序列 $(\mathcal{A}_{i})_{i \in [0,N]}$ 
    - 其中其他部分都是相同的，状态数量是相同的，只有转换规则 $\delta_i$ 不同。
    - 保证转换规则只会增加新的规则： $i \in [0,N-1]$, $\delta_{i} \subseteq \delta_{i+1}$
    - 最后转换规则会收敛，达到不动点： $\delta_{i+1}=\delta_{i}$
    - 每次至少增加一个规则，因此程序最多走 $|Q|^{2} |\Gamma|$ 步
      - 作为一个有限状态自动机，这里只对Q状态增加边，每两个状态之间最多 $|\Gamma|$ 个规则。
  - 算法：如果有一个规则 $pA \rightarrow qw$ ，同时现在的P-自动机存在一个路径 $q \overset{w}{\underset{\mathcal{A}_{i}}{\Longrightarrow}} s$ 我们给P-自动机增加一个新的转换规则 $p \xrightarrow[]{A} s$
    
    ![saturaion](imgs/saturation.drawio.png)

    `p Au -> q wu`。原有自动机为黑色部分，u代表未知任意字符串，即代表的是路径，内部省略了具体的其他节点。意味着从状态为q栈为空的状态，能够接受一个字符串wu。

    然后我们新增了这样一条规则，使得状态为p，字符串为Au也被接受了。

    规则的构建使得，某个下推系统的前驱规则的节点被覆盖到了。而最后目的自动机 $\mathcal{B}$ 由于达到了不动点，因此关于下推系统P的前驱规则封闭。
  - 证明过程的不变量：在这个自动机转换规则的序列中，存在一些始终为真的条件。如果自动机存在转换规则 $p \xrightarrow[]{A} s$ ，那么以下两个性质始终满足满足。
    - 如果状态s属于下推系统P的状态：在下推系统P中存在规则 $pA \overset{}{\underset{P}{\Longrightarrow}} s$。这是一个弹出栈字符的规则。
    - 如果状态s属于其他状态，即不属于下推系统P的状态：对于任何从状态s出发能被接受的字符串u，配置 $(p,Au)$ 属于 $Pre^{*}_P (\mathcal{L}(\mathcal{A}))$
    - 从这个不变关系，可以推出， $\mathcal{L}(\mathcal{A}_i) \subseteq Pre^{*}_P (\mathcal{L}(\mathcal{A}))$ ，即每个A的语言都是我们目标的子集。 特别地， $\mathcal{L}(\mathcal{B}) \subseteq Pre^{*}_P (\mathcal{L}(\mathcal{A}))$ 最终迭代的不动点也是这样。
  - 最小关系：我们如果只关注状态在Q中（下推系统的状态）的部分，那么关系只在 $(Q \times \{\varepsilon\}) \times (Q \times \Gamma)$ 上被添加 1. 左边都是初始状态; 2. 转换规则都是接受了一个字符。 这里增加的关系可以被看作是一个最小关系 $\mathcal{R}$
    - $pA \; \mathcal{R} \; q$ 如果存在规则 $pA \rightarrow q$
    - $pA \; \mathcal{R} \; q$ 如果 $rB \; \mathcal{R} \; q$ 并且 $pA \rightarrow rB$ 在规则中。
      - 这里关系 $\mathcal{R}$ 似乎会对每个带栈字符的状态创建一个无字符的对应状态？比如这里为 $rB$ 引入中间状态
    - $pA \; \mathcal{R} \; q$ 如果存在规则 $pA \rightarrow rBC$，并且存在状态 $s\in Q$，关系 $rB \; \mathcal{R} \; s$ 和 $sC \; \mathcal{R} \; q$
      - 这里 $q$ 仿佛直接代表了 $rBC$

#### 描述派生（可达）关系的 Transducer

基础概念：

- Finite State Transducer 其实就是带有输出的有限状态自动机（finite state automaton）。
- 派生关系 $Deriv_{P}$：描述任意两个字符串之间的关系： $Deriv_{P} =\{ (u,v) \in \Gamma^{*} \mid (q_{0},u) \overset{}{\underset{P}{\Longrightarrow}} (q_{f},v) \}.$
  - $Deriv_{P} \subseteq \Gamma^{*} \times \Gamma^{*}$ 。是字符串之间的关系。
- 操作：$A_{+}$ 表示压入单个符号的操作 push A（$A \in \Gamma$ 是字符）。 $A_{-}$ 同理。
  - 操作集合 $\Gamma_{+}=\{ A_{+} \mid A \in \Gamma\}$ push操作的集合，  $\Gamma_{-}$ 同理。 $\overline{\Gamma}=\Gamma_{+} \cup \Gamma_{-}$ 表示两个的并集。
  - 操作序列： $\alpha=\alpha_{1} \ldots \alpha_{n} \in \overline{\Gamma}^{*}$ 表示一系列操作。
    - 例如， $pA \rightarrow qBC$ 的操作序列是 $A_{-}C_{+}B_{+}$
  - 操作转换关系： $u \overset{\alpha}{\underset{}{\leadsto}} v$ 表示栈状态 $u \in \Gamma^{*}$ 在操作 $\alpha$ 下变成了栈状态 $v$。
  - non-productive序列：无法被应用的栈序列，常常是这种 $B_{+} C_{-}$ 先push再pop一个不同符号的序列，刚压入B，怎么可能栈顶弹出C呢？
    - 就像下面对比图先走橙色边再走蓝色边一样。
    
- 对比自动机和transducer
  
  ![对比自动机和transducer](imgs/saturation3.drawio.png)

  这里可以看到，transducer的状态和PDS是完全对应的，前面的自动机则仅是初始状态和PDS相同，只有栈为空的时候才对应PDS对应的状态。

  Transducer 每条边上也只能push或pop一个字符，这里是简略写法，隐藏了中间的状态。但是是否打破了Transducer和PDS状态的对应关系？并没有，如果要求PDS每次也只能增加或者减少一个栈字符，则两边还是对应的。

- 行为集合 $Behaviour_{P}$ 表示下推系统P，不管是否non-productive，可能走出的所有栈操作序列的集合。
  - 即我现在只需要管状态转换，不需要管栈操作的应用。
  - 即画出上面的转换图，然后随便沿着边走
- $Behaviour_{P}$ 和 $Deriv_{P}$ 的关系：对于任何在行为集合的操作序列， $\alpha \in Behaviour_{P}$ ， 两个字符串属于派生关系集合 $(u,v) \in Deriv_{P}$ 当且仅当 这两个字符串存在操作关系 $u \overset{\alpha}{\underset{}{\leadsto}} v$ 。
  - 然而这个定义不太行，因为 (1) $Behaviour_{P}$ 有很多不合法的序列。(2) 存在刚push就pop的冗余序列，如 $A_{-} B_{+} A_{+} A_{-} C_{+} C_{-}=A_{-} B_{+}$ 。
- 简化关系 $\mapsto$ ：表示一个栈操作序列被简化为另外一个栈操作序列。消除了上面的这种刚push就pop的冗余行为
  - 例如 $B_{-}A_{+}A_{-}C_{+} \mapsto B_{-}C_{+}$
- 操作序列集合在简化操作下维持正则性：如果一个操作序列集合R内的每个操作都被 $\mapsto$ 简化，那么新的集合 $Red(R) = \{ Red(\alpha)  \mid \alpha \in R \}$ 也是正则的
- 操作序列自动机的简化算法： $Red(R)$ 能在 $\mathcal{O}(|\mathcal{A}|^{3})$ 的复杂度构建出来。
- 简化并移除non-productive序列的操作集合 $RP_{P}$ 
  - 移除non-productive序列：简化后很容易识别，直接扫描找先push再pop的序列。因为如果push的字符和pop的字符不同则不合法，相同则说明化简没有结束。
  - 性质：可以推出里面的序列必然只会先pop再push。
- 最终的结论： 
  - $Deriv_{P}$ 的关系 $(w_{1},w_{2})$ 对应到transducer 必然是，找出两个串的公共后缀，然后先pop w1的前缀，然后push w2的前缀。
  - $Deriv_{P}$ 对应的 Transducer 可以在多项式时间内被构建出来。

核心在于，将状态转换改为了一个push pop栈的序列。即，如果有一个 $pA \rightarrow rw$ 规则，那么我们这里构建一个序列：pop A，push ...(构成w的几个字符)。表示栈上的变化关系。

然后使用饱和算法。这里需要每条边上的操作只能压入或者弹出单个字符。即找到两个路径上能化简的状态，然后直接连过去，标上化简后的操作序列。

### retypd 基础符号

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


## 规约算法：概述（Section 5）

### 约束的简化

**类型，以及函数的类型到底应该怎么表示？**

- 格表示的类型：对于单个固定大小的基本类型，可以使用lattice结构以及一个偏序关系表示类型。子类型为格上的偏序关系
- 对于复杂的具有能力的类型，比如访问某个偏移字段的能力，加载出值的能力，则类似结构体的子类型关系，子类型允许具有更多能力，安全代换父类型。
- 函数的类型则涉及到泛型的类型方案的表示 $\forall{\overline{\alpha}}.\,{C}\!\Rightarrow\!{\tau}$ 。为函数的输入和输出类型创建类型变量，然后得到一个变量的最小约束集合表示这个函数的类型。
  - 例如通用的恒等函数，直接将参数返回，表示为，对任意的类型X，返回值的类型也是X。对应我们的表示可能是 $F.in \sqsubseteq F.out$
- PDS = （未化简，或者简化后的）约束 = 类型方案 type scheme

**我们为什么要简化约束？** 令 $\mathcal{C}$ 表示由抽象解释生成的过程的约束集，并且 $\overline{\alpha}$ 是 $\mathcal{C}$ 中的自由类型变量集。我们其实已经可以使用 $\forall{\overline{\alpha}}.\,{\mathcal{C}}\!\Rightarrow\!{\tau}$ 作为过程类型方案中的约束集，因为合法调用 $f$ 时使用的输入和输出类型显然是满足 $\mathcal{C}$ 的。然而，实际上我们不能直接使用这个约束集，因为这会导致在嵌套过程中产生很多无用的自由变量，并且约束集的增长率很高。

这个简化算法的输入是，从一个函数的推断得到的一个类型方案 $\forall{\overline{\alpha}}.\,{C}\!\Rightarrow\!{\tau}$ （包括自由类型变量，约束，和泛型），并创建一个较小的约束集 $\mathcal{C}'$，使得任何由 $\mathcal{C}$ 对 $\tau$ 的约束也被 $\mathcal{C}'$ 所蕴含。

相反，我们寻求生成一个**简化的约束集** $\mathcal{C}'$，使得如果 $c$ 是一个“有趣”的约束，并且 $\mathcal{C} \;\vdash\; c$，那么 $\mathcal{C}' \;\vdash\; c$ 也同样成立。但什么让一个约束变得有趣呢？

- 能力约束，表示某个dtv有某个field label
- 递归类型约束： $\tau.u {\;\sqsubseteq\;} \tau.v$
- 涉及常量类型的约束： $\tau.u {\;\sqsubseteq\;} \overline{\kappa}$ 或者 $\overline{\kappa} {\;\sqsubseteq\;} \tau.u$ 其中 $\overline{\kappa}$ 是类型常量.

### Roadmap

1. （A.）收集初始约束。
2. （F.1）约束简化算法。简化后的约束就是type schemes？
3. （F.2）构建sketches（为每个类型变量，比如函数类型）（自底向上遍历call graph的顺序），同时细化具体类型。
4. （4.3）最后转换sketches到C类型。


### 无约束的下推系统 Unconstrained Pushdown Systems

**无约束的含义**：TODO。可能是表示没有限制栈符号和转换规则的有限性？

核心思路： 下推系统 $\mathcal{P}_\mathcal{C}$ 的转换序列，可以直接对应上基于约束集合 $\mathcal{C}$ 上的子类型推导判断的推导树。

定义：一个**无约束下推系统**是由三个部分组成的元组 $\mathcal{P} = (\mathcal{V}, \Sigma, \Delta)$，其中 $\mathcal{V}$ 是**控制位置**的集合，$\Sigma$ 是**栈符号**的集合，而 $\Delta$ 是包含在 $(\mathcal{V} \times \Sigma^*)^2$ 内的（可能无限）**转换规则**的集合。转换规则表示为 $\langle X; u \rangle \hookrightarrow \langle Y;v\rangle$，其中 $X,Y \in \mathcal{V}$ 且 $u,v \in \Sigma^*$。我们定义**配置**的集合为 $\mathcal{V} \times \Sigma^*$。在配置 $(p,w)$ 中，$p$ 称为**控制状态**，$w$ 称为**栈状态**。

注意到，我们既不要求栈符号的集合也不要求转换规则的集合是有限的。这种自由度是为了模拟推导规则 S-Pointer， 正如图3的推导规则 S-Pointer 所示，它对应于一个无限的转换规则集。

**为什么要用下推系统？** 下推系统能很好地反映类型关系关于能力的传递关系，反映在下推系统上就是后缀子串的关系。

- 转换关系定义：一个无约束的下推系统 $\mathcal{P}$ 确定了一个**转换关系** $\longrightarrow$ 在配置集合上： $(X,w) \longrightarrow (Y,w')$ 如果存在一个后缀 $s$ 和一个规则 $\langle {X}; {u} \rangle \hookrightarrow 	\langle {Y}; {v} \rangle$，使得 $w = us$ 和 $w' = vs$。$\longrightarrow$ 的传递闭包表示为 $\stackrel{*}{\longrightarrow}$。
  - 这里的公共后缀s，就可以想象为类型的能力。比如各种偏移里的字段。然后前缀类型变量，比如两个结构体类型，如果符合子类型关系，则对应的访问相同的字段得到的类型变量，则也存在子类型关系。

有了这个定义，我们可以陈述我们简化算法背后的主要定理。这里一个类型对应一个 $(\mathcal{V} \cup \Sigma)^*$ 字符串。

设 $\mathcal{C}$ 是一个约束集合，$\mathcal{V}$ 是一组基类型变量集合。定义 在类型变量和标签集合构成的两个字符串之间的关系 $(\mathcal{V} \cup \Sigma)^* \times (\mathcal{V} \cup \Sigma)^*$ 的一个子集 $S_\mathcal{C}$，
通过 $(Xu, Yv) \in S_\mathcal{C}$ 当且仅当 $\mathcal{C} \;\vdash\; X.u {\;\sqsubseteq\;} Y.v$。
那么 $S_\mathcal{C}$ 是一个正则集合，并且可以在 $O(|\mathcal{C}|^3)$ 时间内构造一个识别 $S_\mathcal{C}$ 的自动机 $Q$。

证明：基本思想是将每个 $X.u {\;\sqsubseteq\;} Y.v \in \mathcal{C}$
作为下推系统 $\mathcal{P}$ 中的一个转换规则 $\langle {X}; {u} \rangle \hookrightarrow	\langle {Y}; {v} \rangle$。
此外，我们为每个 $X \in \mathcal{V}$ 添加控制状态 ${^\#{Start}}, {^\#{End}}$ 及其转换 $\langle {{^\#{Start}}}; {X} \rangle \hookrightarrow \langle {X}; {\varepsilon} \rangle$
和 $\langle {X}; {\varepsilon} \rangle \hookrightarrow \langle {{^\#{End}}}; {X} \rangle$。
目前，
假设所有标签都是协变的，并且忽略规则 S-Pointer。
通过构造，$({^\#{Start}}, Xu) \stackrel{*}{\longrightarrow} ({^\#{End}}, Yv)$ 在 $\mathcal{P}$ 中当且仅当 $\mathcal{C} \;\vdash\; X.u {\;\sqsubseteq\;} Y.v$。
Büchi [27] 保证，对于任何标准（非无约束）下推系统中的两个控制状态 $A$ 和 $B$，
所有满足 $(A, u) \stackrel{*}{\longrightarrow} (B, v)$ 的 $(u,v)$ 对组成的集合是一个正则语言；
Caucal [8] 给出了一个构造识别这种语言的自动机的饱和算法。

在完整的证明中，我们增加了两个创新之处：首先，我们通过将variance数据编码到控制状态和转换规则中，支持逆变堆栈符号。
第二个创新之处涉及到 S-Pointer 规则；这个规则是有问题的，因为自然的编码将导致无限多的转换规则。
我们将 Caucal 的构造扩展为在饱和过程中懒惰实例化所有必要的 S-Pointer 应用。
详情见 Appendix D。

由于 $\mathcal{C}$ 通常涉及到无限多的约束，
这个定理特别有用：它告诉我们由 $\mathcal{C}$ 引发的完整约束集合可以通过自动机 $Q$ 的有限编码来实现。
对约束闭包的进一步操作，如有效的最小化，可以在 $Q$ 上进行。通过限制与 ${^\#{Start}}$ 和 ${^\#{End}}$ 的转换，使用相同的算法
消去类型变量，生成所需的约束简化。

**推理的整体复杂性**

用于执行约束集简化类型方案构造的饱和算法，
在最坏情况下，是关于简化子类型约束数量的三次方。由于一些著名的指针分析方法也具有三次方复杂度（如 Andersen [4]），
因此很自然地会怀疑 Retypd 的“无需指向”分析是否真的比基于指向分析数据的类型推理系统提供优势。

为了理解 Retypd 的效率所在，首先考虑 $O(n^3)$ 中的 $n$。
Retypd 的核心饱和算法在子类型约束的数量上是三次方的；由于机器代码指令的简单性，每条指令大约会产生一个子类型约束。
此外，Retypd 在每个独立的过程中应用约束简化以消除该过程局部的类型变量，
从而得到只涉及过程形参、全局变量和类型常量的约束集。在实践中，这些简化的约束集很小。

由于每个过程的约束集是独立简化的，因此三次方的 $n^3$ 因子由最大过程大小控制，而不是整个二进制文件的大小。
相比之下，像 Andersen 这样的源代码指向分析通常与指针变量的总数呈三次方，并且根据用于上下文敏感性的调用字符串深度呈

## 算法：细节（Appendix D）

**基础符号**

- $\amalg$ 表示集合的不交并。表示某个集合可以分割为不同的组成部分。
  - $\mathcal{V} = \mathcal{V}_i \amalg \mathcal{V}_u$ 表示类型变量集合被分割为interesting和uninteresting两部分。
- 一个证明是elementary的，如果证明的结论的子类型关系里没有uninteresting的变量，且证明过程中，uninteresting变量都只在内部。
  - $\mathcal{C} {\;\vdash\;}^{\mathcal{V}_i}_\text{elem} X.u {\;\sqsubseteq\;} Y.v$ 表示约束集合 $\mathcal{C}$ 上能够推理出这样一个子类型约束。其中类型变量都定义在 ${\mathcal{V}_i}$ 上，并且这样的关系是elementary的。

**自动机**

对应关系：

- 一个类型（sketch）就是一个有限状态自动机。
  - 本来用树就够了，但是递归类型会导致无限长的树。根据子树的有限性，用自动机处理递归的情况。典型的例子是复杂的递归结构体类型。
- 下推自动机工作的过程就是我们类型推理的过程
  - 当前状态表示基础的类型变量。
  - 栈状态表示标签，比如`.load`，field访问
  - 自动机配置：状态+栈状态。表示一个派生变量。
  - 状态转换规则：子类型关系。
    - 比如，随便写一个规则 $a.\sigma\mathsf{N@k} \sqsubseteq b.load$ 表示可以从状态为 $a$ 栈内容为 $\sigma\mathsf{N@k}$ 的配置转换到 状态为 $b$ 栈内容为 $load$ 的配置。
  - 可达性：派生的子类型关系。
    - 然后，如果 $b.load$ 又是 $c$ 的子类型，两个规则合起来，在自动机上，状态 $a.\sigma\mathsf{N@k}$ 到 $c$ 也是可达的（走了两步）。因此也具有子类型关系。 $a.\sigma\mathsf{N@k} \sqsubseteq c$。
- P-自动机（从下推系统P转换而来）对应了类型的自动机结构sketch。
  - P-自动机从PDS的状态开始，对应栈为空的PDS配置，到终态能匹配的语言代表所有可能的栈字符的集合构成的语言。表达了一个类型所有可能的能力。
  - P-自动机接受一个字符串，表示这个派生类型变量存在。
- 饱和算法和类型推断的对应关系：
  - 下推系统中每个状态下栈组成的语言是正则的。对应我们每个基本类型变量可能存在的能力标签集合是正则的。这个很显然吧。
  - 可以多项式时间构建出，每个状态下，对应的有限状态自动机，识别栈可能出现的字符串集合。对应：可以构建出有限状态自动机表达某个类型所有可能的能力。
    - 跨基础类型变量的子类型关系呢？这里可以想象，子类型关系只是用来传递能力的。把每个基础变量初始状态拿出来单独看从这里可达的自动机部分，子类型关系反映在不同基础变量的这个“树”的交叉的部分。
      ![](imgs/subtypeFSM.drawio.png)

Transducer：

- Transducer可以表示下推自动机的所有的推断关系！任意两个dtv字符串之间的子类型关系！
- Transducer和类型推断的对应关系。

重要概念：

- $\mathcal{P}_\mathcal{C}$ 表示我们这里构建的pushdown system。包含三部分 $(\widetilde{\mathcal{V}}, \widetilde{\Sigma}, \Delta)$
  - 状态集合： $\widetilde{\mathcal{V}} = \left(\mathsf{lhs}(\mathcal{V}_i) \amalg \mathsf{rhs}(\mathcal{V}_i) \amalg \mathcal{V}_u\right) \times \{ \oplus, \ominus \} \cup \{ {^\#{Start}}, {^\#{End}} \}$ 
    - 额外增加的两个特殊状态 start 和 end
    - 带有variance标签的状态，包括三部分
      - 带有L或R标签的interesting变量
      - uninteresting变量
  - 栈字符集： $\widetilde{\Sigma} = \Sigma \cup \{ v^\oplus ~|~ v \in \mathcal{V}_i \} \cup \{ v^\ominus ~|~ v \in \mathcal{V}_i \}$
    - 包含普通的field label
    - 带有variance标记的有趣变量。TODO这表示什么意思
  - 转换规则包括四部分 $\Delta = \Delta_\mathcal{C} \amalg \Delta_\mathsf{ptr} \amalg \Delta_\mathsf{start} \amalg \Delta_\mathsf{end}$
    - $\Delta_\mathcal{C}$ 现有的规则，经过rule函数转换后的结果
    - $\Delta_\mathsf{ptr}$ PTR规则，经过rule函数转换后的结果
    - $\Delta_\mathsf{start} = \left\{\langle {{^\#Start}}; {v^\oplus} \rangle \hookrightarrow \langle {v^\oplus_\mathsf{L}}; {\varepsilon} \rangle~|~v \in \mathcal{V}_i \right\} \cup \left\{\langle {{^\#Start}}; {v^\ominus} \rangle \hookrightarrow \langle {v^\ominus_\mathsf{L}}; {\varepsilon} \rangle~|~v \in \mathcal{V}_i \right\}$ 
      - 表示start状态可以把栈上的唯一变量标签转换为当前状态，栈为空
    - $\Delta_\mathsf{end} = \left\{\langle {v^\oplus_\mathsf{R}}; {\varepsilon} \rangle \hookrightarrow \langle {{^\#End}}; {v^\oplus} \rangle~|~v \in \mathcal{V}_i \right\} \cup \left\{\langle {v^\ominus_\mathsf{R}}; {\varepsilon} \rangle \hookrightarrow \langle {{^\#End}}; {v^\ominus} \rangle~|~v \in \mathcal{V}_i \right\}$
      - 表示当前状态为某个变量，栈为空的时候，可以转换到End状态，把变量放到标签。
- $\mathsf{Deriv}_{\mathcal{P_C}}$ 表示 $\mathcal{P}_\mathcal{C}$ 上派生得到的约束
- $\mathsf{Deriv}_{\mathcal{P_C}}' = \left\{ (X.u, Y.v) ~|~ (X^{\langle u \rangle}u, Y^{\langle v \rangle} v)\in \mathsf{Deriv}_{\mathcal{P_C}}\right\}$ 
  - 表示 $\mathcal{P}_\mathcal{C}$ 上删去variance标签得到的约束。
- rule辅助函数，对普通的规则，生成我们内部使用的，带variance标签的规则形式
  - $\mathsf{rule}^\oplus(p.u \sqsubseteq q.v) = \langle {\mathsf{lhs}(p)^{\langle u \rangle}}; {u} \rangle \hookrightarrow \langle {\mathsf{rhs}(q)^{\langle v \rangle}}; {v} \rangle \\$ 
    - 首先观察到分别给左边和后边的有趣的基本类型变量，通过lhs和rhs函数带上了L/R标记
    - 其次将field label的variance标签标记到了类型变量上
  - $\mathsf{rule}^\ominus(p.u \sqsubseteq q.v) = \langle {\mathsf{lhs}(p)^{\ominus \cdot \langle u \rangle}}; {u} \rangle \hookrightarrow \langle {\mathsf{rhs}(q)^{\ominus \cdot \langle v \rangle}}; {v} \rangle \\$ 
    - 这里的点运算符就是variance的叠加运算。
    - TODO：这里的规则有什么实际的含义吗？
  - $\mathsf{rules}(c) = \{ \mathsf{rule}^\oplus(c),~ \mathsf{rule}^\ominus(c)\}$
    - 表示对每个约束生成两种类型的约束，带有不同的variance标记。

- 状态上的variance标签的作用：控制状态上的 $\{ \oplus, \ominus \}$ 上标用于追踪栈状态的当前variance，这使得我们能够区分在协变和逆变位置使用公理的情况。
- 标签 操作 lhs 和 rhs 的作用：用于防止推导中使用来自 $\mathcal{V}_i$ 的变量，防止 $\mathcal{P}_\mathcal{C}$ 接受代表非基本证明的推导。
  - 例如，我们写一个递归的无限约束 $var.load \sqsubseteq var$，推导为 $var.load.load \sqsubseteq var.load \sqsubseteq var$ ，在增加标签之后就变成了 $var_{L}.load \sqsubseteq var_{R}$ 从而不会被递归推导。

## TODO

- monoid: 读《Haskell趣学指南》的Monoids一节，理解一下。

**unification-based type inference algorithms**

https://www.cs.cornell.edu/courses/cs3110/2011sp/Lectures/lec26-type-inference/type-inference.htm
