# 反编译器后端 - 结构分析

结构分析负责将IR转换为伪C代码。

## (最简单的)fallback实现

<!-- - 判断每个IR语句是否有副作用。如果不存在副作用，且仅有一个user，则作为表达式折叠到其他语句。否则生成局部变量储存它的值。 -->
- 给所有的基本块前添加标号，对于任意控制流跳转，使用goto实现。最后删除无用的标号。

Fallback实现可以考虑作为现有算法的辅助，将其他算法无法处理的边转换为Goto语句。

## Structural Analysis 和 Reducible Control Flow

- [PPT(结尾介绍了Reducibility)](http://baishakhir.github.io/class/2020_Fall/5_control_flow_analysis.pdf)
- [PPT(介绍了Reducibility编译原理相关的内容)](https://www.cs.cmu.edu/afs/cs/academic/class/15745-s08/www/lectures/lect5-loops.pdf)
- [PPT:介绍了structural analysis](http://www.cs.cmu.edu/afs/cs/academic/class/15745-s05/www/lectures/t1t2.pdf) 特别说明了
- [PPT:更详细地介绍了structural analysis](https://www.cs.princeton.edu/courses/archive/spr04/cos598C/lectures/10-Structural.pdf)
- [介绍了T1 T2转换](http://misailo.web.engr.illinois.edu/courses/526-sp17/lec1.pdf)

一个CFG如果是Reducible的，则我们可以将图上所有边分割为两部分，前向边，和反向边
- 前向边构成有向无环图，每个节点可以从entry到达。
- 反向边的目标节点都支配源节点。

定义2：T1 T2转换
一个CFG如果是reducible的，可以通过规则将图中的节点递归缩减为单个节点。
- T1 自环可以被缩减
- T2 如果某个节点仅有一个前驱，则可以将该节点缩入这个前驱节点

然而，有个PPT里面说，这种T1 T2转换，对控制流的缩减，从而对控制流划分的层次结构，不一定规范地符合真实源码里面的划分。T1 T2转换是最早的，最简单的一种interval analysis.

## Structural Analysis

- [反编译的结构分析](https://www.cs.tufts.edu/comp/150FP/archive/cristina-cifuentes/structuring-algorithm.pdf)
- [《Advanced Compiler Design and Implementation》](https://www.cs.princeton.edu/courses/archive/spr04/cos598C/lectures/10-Structural.pdf) 在203页更详细地介绍了structural analysis。建议从196页的Reducibility开始阅读。（书里面的页码，不是pdf的页码）

**背景1 深度优先遍历与逆后序遍历：** Depth First Spanning Tree 是在DFS遍历过程中生成的一个树。基本思想是，DFS遍历过程中，会出现一种情况：判断当前节点指向其他节点的一条边的时候，发现这个边指向的目标节点已经被访问过了，所以就不需要沿着这条边过去了。因此可以将边划分为遍历使用了的边，和遍历过程中没有使用的边。

在深度优先遍历时，怎么才算是反向边？当然是这条边指向了已经被访问的节点。即遍历时发现指向的地方已经访问过了。也有可能出现，根据选择的子节点不同导致反向边不同的情况。比如两个子树交叉指向隔壁更高的节点。深度优先遍历的时候，走过的边属于前向边。如果某条边反过来就属于前向边，则它是反向边（和某个前向边形成2节点的小环）。剩下的边属于交叉边。



