#!/bin/bash

# ! 首先通过mdbook serve命令查看效果，确认无误后再执行本脚本部署
# 根据 https://github.com/rust-lang/mdBook/wiki/Automated-Deployment%3A-GitHub-Pages-%28Deploy-from-branch%29 编写

# 初始化：首次执行可能找不到gh-pages分支，需要创建：（注意先git stash 保存分支内容）
# 参考 https://gist.github.com/renatoathaydes/75fcf8c5104134ae112f367d5e4f3f50
# git checkout --orphan gh-pages
# git reset --hard
# git commit --allow-empty -m "Initializing gh-pages branch"
# git push github gh-pages # 增加了一个remote叫github
# git checkout dev # 切换回原分支

echo "====> deploying to github"
# 这里增加了一个remote叫github
rm -r /tmp/book
mkdir -p /tmp/book
# https://stackoverflow.com/questions/41545293/branch-is-already-checked-out-at-other-location-in-git-worktrees
git worktree prune
git worktree add -B gh-pages /tmp/book github/gh-pages
mdbook build
rm -rf /tmp/book/*
cp -rp book/* /tmp/book/
cd /tmp/book && \
    git add --all && \
    git commit -m "deployed on $(shell date) by $(git config --get user.name)" && \
    git push github gh-pages
