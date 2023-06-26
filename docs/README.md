# NocDec 文档

文档基于[mdBook](https://rust-lang.github.io/mdBook)编写，并单独作为网站，便于阅读。

- `src/README.md` 软链接到项目的ReadMe(`../../README.md`)，都是markdown，正好和github主页的展示保持一致。
- `src/docs`目录下存放本项目主要文档
- `src/basics/`目录下存放补相关基础知识时的笔记。因为和项目关系不大，所以单独存放一个目录。

注意，由于链接是相对路径，所以移动文件的时候注意修一下。

## 部署

见`deploy.sh`。
