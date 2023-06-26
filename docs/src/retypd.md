# retypd

#### retypd+ghidra 配置

1.安装retypd 

```Python
git clone https://github.com/GrammaTech/retypd
cd retypd
pip install .
```

2.编译`GhidraRetypd`  安装ghidra_retypd_provider

```Python
git clone https://github.com/GrammaTech/retypd-ghidra-plugin
cd retypd-ghidra-plugin
make
pip install .
```

3.修改 GhidraRetypd.zip中的extension.properties，添加

```Python
version=10.2.3
```

4.安装插件

1. 打开 Ghidra 软件，点击 "File" 菜单，选择 "Install Extensions" 选项。
2. 在弹出的 "Install Extensions" 窗口中，点击 "Browse" 按钮选择你要安装的扩展程序。
3. 选中你要安装的扩展程序文件（通常是一个 zip 压缩文件），然后点击 "Open" 按钮。
4. 点击 "OK" 按钮开始安装扩展程序。在安装过程中，Ghidra 软件会自动解压缩扩展程序文件，并将它们安装到正确的目录中。
5. 安装完成后，重启 Ghidra 软件。

为Ghidra 10.2.3 版本成功编译的retypd[下载](GhidraRetypd.zip)
