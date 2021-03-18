# Calculation_acceleration

#### 介绍
大数据与算法课程作业
利用多线程、多机通信、SSE、openMP等进行加速
@ForAgony
tju

#### 软件架构
本项目在windows x64平台下，使用C++编程实现了通过遍历求和、求最大值以及通过快排算法进行排序三个功能函数。
并通过对三种不同加速方法（openmp，SSE，openmp&SSE、多线程）的加速比的比较，选择了openmp作为sum函数的加速
方式，SSE作为MAX函数的加速方式，多线程作为排序函数的加速方式。并通过socket函数实现了基于TCP的双机稳定通信，
并编写了响应的sever端和client端程序。


#### 使用说明

1. 在使用时，请在client端输入IP地址（tcp.cpp第37行），sever端无需改动。
2. 在使用时，如果要修改输入数据，请分别修改服务端87-92行代码，客户端85-90行代码。
3. 若要修改使用函数，请在client端代码134行和164行，sever端148和167行分别进行修改。
4. 如果要修改排序算法所用线程数，可以修改TREAD常量，但实测8线程最快。
5. 如果在重现时发现排序出现问题，可能是未能正确打开关闭socket导致的，可以重新打开程序重试。


#### 参与贡献

1.  Fork 本仓库
2.  新建 Feat_xxx 分支
3.  提交代码
4.  新建 Pull Request


#### 特技

1.  使用 Readme\_XXX.md 来支持不同的语言，例如 Readme\_en.md, Readme\_zh.md
2.  Gitee 官方博客 [blog.gitee.com](https://blog.gitee.com)
3.  你可以 [https://gitee.com/explore](https://gitee.com/explore) 这个地址来了解 Gitee 上的优秀开源项目
4.  [GVP](https://gitee.com/gvp) 全称是 Gitee 最有价值开源项目，是综合评定出的优秀开源项目
5.  Gitee 官方提供的使用手册 [https://gitee.com/help](https://gitee.com/help)
6.  Gitee 封面人物是一档用来展示 Gitee 会员风采的栏目 [https://gitee.com/gitee-stars/](https://gitee.com/gitee-stars/)
