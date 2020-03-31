# librf 2.9.7

### librf  - 协程库
 * librf是一个基于C++ Coroutines提案 ‘Stackless Resumable Functions’编写的非对称stackless协程库。

支持以下平台和编译器:

	Windows: 使用VS2017/VS2019/clang 9编译
	Android: 使用NDK 20.1 自带的clang编译
	Mac: 使用XCode 11.2.1 自带的apple-clang编译

librf有以下特点：

 * 1.基于C++20提案'Stackless Resumable Functions'编写的非对称stackless协程库，可以以同步的方式编写简单的代码，同时获得异步的性能
 * 2.理论上支持海量协程, 创建 **10,000,000** 个协程只需使用 **2.2G** 物理内存(使用clang编译)
 * 3.拥有极小的协程调度，在I7 8100 3.6GHz的CPU上，**1000** 个协程的平均切换开销是 **27** 纳秒(使用clang编译)
 * 4.提供协程锁(mutex), 定时器, channel, event等特性, 帮助用户更加容易地编写程序
 * 5.可以很好的跟asio, libuv等库结合，能跟现有的callback范式的异步/延迟代码很好的结合
 * 6.目前已处于较为完善状态，已经小规模在生产项目中使用。不出意外，2.8以上版本就是C++20 Coroutines对应的版本

如果你发现了任何bug、有好的建议、或使用上有不明之处，可以提交到issue，也可以直接联系作者:

	email: tearshark@163.net
	QQ交流群: 296561497



2020-03-31 更新：

	使用Doxygen自动生成文档，并完善文档内容。
	支持cmake。(目前仅VS2019测试通过)。

2020-03-26 更新：

	兼容xcode 11.2.1。
2020-03-18 更新：

	更新event/mutex/when_all/when_any实现。至此，2.x版本完整恢复1.x版本的所有功能。
	版本号提升至 2.8.0。
	3.0之前，只打算做修复BUG相关的工作。
	3.0的目标，是根据executor的设计，重写scheduler代码。
2020-03-08 更新：

	更新channel实现，效率提高了近三倍。
	channel的新的实现方法，为event/mutex指明了新的修改方向。
2020-02-16 更新：

	更新调度器算法，深入应用Coroutines的特性，以期获得更高调度性能。
	不再支持C++14。




 * 注一：doc目录下有作者搜集的一些关于C++协程的资料
 * 注二：tutorial目录下有针对每个特性的范例代码，让用户可以循序渐进的了解librf库的特性
