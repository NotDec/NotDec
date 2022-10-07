
### CMake 学习资料

1. https://eliasdaler.github.io/using-cmake/
1. https://github.com/kigster/cmake-project-template 找一个模板项目作为参考

### External Project

基本就是先添加ExternalProject_add，然后add_library指定IMPORTED，设置import路径。最后使用的时候额外增加一个add_dependencies关联到ExternalProject_add的target。

1. https://cmake.org/cmake/help/latest/module/ExternalProject.html
1. https://stackoverflow.com/questions/51564251/correct-way-to-use-third-party-libraries-in-cmake-project/51567322#51567322
1. https://stackoverflow.com/questions/51661637/having-cmake-build-but-not-install-an-external-project
1. https://stackoverflow.com/questions/29533159/what-is-install-dir-useful-for-in-externalproject-add-command
1. ninja报错找不到import的库的构建方法： https://stackoverflow.com/questions/50400592/using-an-externalproject-download-step-with-ninja
