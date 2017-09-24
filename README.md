# librf

### librf  - 协程库

librf是一个基于C++ Coroutines提案 ‘Stackless Resumable Functions’编写的非对称stackless协程库。

目前仅支持:

    Windows (使用VS2015/2017编译)


librf有以下特点：

 *   1.基于C++17提案'Stackless Resumable Functions'编写的非对称stackless协程库，可以以同步的方式编写简单的代码，同时获得异步的性能
 *   2.理论上支持海量协程, 创建100万个协程只需使用<待测试>物理内存
 *   3.提供协程锁(mutex), 定时器, channel等特性, 帮助用户更加容易地编写程序 
 *   4.可以很好的跟asio,libuv等库结合，能更现有的callback范式的异步/延迟代码结合
 *   5.目前还处于实验状态，不对今后正式的C++ Coroutines支持有任何正式的承诺
 
 *   如果你发现了任何bug、有好的建议、或使用上有不明之处，可以提交到issue，也可以直接联系作者:
      email: tearshark@163.net  QQ交流群: 296561497

 *   **doc目录下有作者搜集的一些关于C++协程的资料**
 *   **tutorial目录下有针对每个特性的范例代码，让用户可以循序渐进的了解librf库的特性**