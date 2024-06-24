

### 提交代码前

1. 写好commit message，简要概况所有的修改。
2. 检查添加的代码的注释和文档是否充足。


### push前多使用 Git Rebase

在push前尽量rebase到最新的分支。

- 本地开发时，创建以自己用户名开头的分支。比如我的用户名叫am009，就创建一个分支叫am009/dev

- 当main分支，或者其他人分支有新东西的时候，我们可能会使用git pull命令拉取下来，然而这个时候只会更新（fast-forward）自己当前的分支（checkout的）。

    但是反复切换分支再git pull很麻烦怎么办？可以使用下面这个命令，在不checkout的情况下，更新本地的对应分支。比如这里更新main分支

    ```
    git fetch origin main:main
    ```

- 比如现在我commit了一些东西，想要push上去，需要先按照上面的方法，把main分支的东西拉取到本地，然后通过`git rebase main`，让自己的分支基于最新的main分支，然后再push上去。

- （可选）确认自己的commit没有问题后，可以直接push到main分支。使用如下命令把自己的开发分支push到main分支。比如我本地开发分支叫am009/dev，我就使用这个命令：

    ```
    git push origin am009/dev:main
    ```

### 代码风格

遵循LLVM风格，以下是一些总结

- DEBUG_TYPE 小写，横线连接。


