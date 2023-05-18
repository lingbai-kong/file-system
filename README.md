# file-system
同济大学CS《操作系统》课程设计: 文件系统TongJi University CS OS assignment: file system
## 概述

本项目是为2021年同济大学计算机系操作系统课程设计。实现了类UNIX文件系统。

## 功能

1. 具有高速缓存
2. 实现了多用户多用户组的管理和访问权限控制
3. 实现了一个简单的控制台编辑器，可以直接在文件系统内对文本文件进行编辑

## 快速开始

1. 打开文件系统.exe
2. 输入help命令查看可用命令
3. 输入命令login root登录系统，初始密码为root

## 重要注意事项

1. 退出程序时禁止直接关闭程序，必须使用exit命令退出文件系统，否则可能导致文件系统错误或崩溃
2. 慎重修改/etc/users.txt和/etc/groups.txt文件，这两个文件与用户管理相关，如果修改格式不当可能会导致文件系统无法登录或其他权限问题
3. 如果以上两条问题发生请尝试格式化文件系统

## 运行环境

本文件系统运行与64位Windows操作系统下，文件系统的模拟文件卷大小为128MB，请确保程序目录下有足够的可用空间

## 备注

本学期最简单的大作业（主要是没有GUI），开发该文件系统耗时10天

如果本仓库有帮助到你，就送我一颗star吧🤗
