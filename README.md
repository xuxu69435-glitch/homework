# xv6-riscv 实践周：内核运行机制

课程仓库对应 2026 春季实践周任务（系统调用路径、调度与内存观察、系统调用扩展、生产者–消费者模型）。视频参考：[xv6 Kernel 系列（Bilibili）](https://www.bilibili.com/video/BV1w94y1a7i8/)。

## 实验内容与完成层级

| 层级 | 内容 | 状态 |
|------|------|------|
| 第一层（必做） | 任务1：`write` 在用户态包装、`syscall()` 与 `sys_write` 中的内核日志 | 已完成 |
| 第一层 | 任务2：`scheduler()` 中打印 `[SCHED] switch to pid=...` | 已完成 |
| 第一层 | 任务3：`kalloc()` 中打印 `[MEM] alloc page at ...` | 已完成 |
| 第二层（选做） | 新增系统调用 `getpid_plus()`（返回 `pid+1`），用户程序 `getpidtest` 验证 | 已完成 |
| 第二层（选做） | 任务 3：两进程竞争输出；`syncdemo` 演示无同步交错 vs 管道握手串行化（对照自旋锁保护的临界区思想） | 已完成 |
| 第三层（挑战） | 内核环形缓冲区 + `spinlock`，`sleep`/`wakeup` 阻塞；系统调用 `pcput`/`pcget`；用户程序 `pcdemo` | 已完成 |

## 第二层任务实现说明：`getpid_plus`

1. 在 `kernel/syscall.h` 中增加 `SYS_getpid_plus` 编号。  
2. 在 `kernel/sysproc.c` 中实现 `sys_getpid_plus()`，返回 `myproc()->pid + 1`。  
3. 在 `kernel/syscall.c` 中注册该调用。  
4. 在 `user/usys.pl` 与 `user/user.h` 中增加用户态接口。  
5. `user/getpidtest.c` 中调用 `getpid()` 与 `getpid_plus()` 并打印结果。

## 第二层任务 3：`syncdemo`（无锁竞争 vs 有同步）

用户程序无法直接使用内核 `spinlock`，本实验用 **双管道 ping-pong** 模拟“进入临界区前必须取得令牌”：父进程写 `p2c` 唤醒子进程打印，子进程再通过 `c2p` 把令牌还给父进程，从而保证每次只有一方输出完整一行。前半段 `nosync()` 则让父子各自快速输出 `P`/`C` 字符，便于在 QEMU 里观察 **交错乱序** 与 **严格交替** 的差异。

## 第三层任务实现说明：生产者–消费者

- `kernel/pcbuf.c`：固定长度环形队列，用自旋锁 `pclock` 保护 `wi`/`ri`/`count`；缓冲区满或空时通过 `sleep`/`wakeup` 在条件变量 `&count` 上阻塞与唤醒（避免用户态忙等）。  
- 系统调用 `pcput(int)` / `pcget()` 暴露给两个用户进程。  
- `user/pcdemo.c`：`fork` 后子进程作消费者循环 `pcget`，父进程依次 `pcput(1..8)`，观察有序消费。

### 运行输出摘录（示意）

在 xv6 shell 中执行 `getpidtest`，可看到 `getpid` 与 `getpid_plus` 相差 1。执行 `pcdemo`，可看到 `consumer: got 1` … `got 8` 以及内核中（若未关闭）调度与 `write` 跟踪日志。本地需在安装 RISC-V 工具链与 QEMU 后执行 `make qemu`。

## 遇到的问题与解决

1. **用户态在 `write()` 里再打日志会递归**  
   若用 `printf` 打印 “calling write”，会再次进入 `write`。解决：用内联 `ecall` 封装 `kern_write`，先向 **stderr（fd=2）** 输出一行跟踪信息，再对原始 `fd` 调用 `write`，避免经过包装函数造成无限递归。

2. **Windows 下 Git 与脚本**  
   PowerShell 中 `$U` 在 Makefile 相关字符串里会被误展开。后续提交改为在仓库内直接维护文件，避免在脚本里对含 `$` 的 Makefile 行做错误替换。

## 实践心得

本次实践把上学期已经接触过的“从 bootloader 到 main”的启动视角，延伸到了运行期的三条主线：系统调用、调度与物理页分配。在 `write` 上打通用户包装、`syscall` 分发和 `sys_write` 之后，对“用户态参数如何进入内核、内核如何再落到文件层”有了更具体的认识；原先抽象的 trap 编号与 `a7` 寄存器，变成了可以打印、可以单步对照代码的路径。调度器日志让我们直观看到多核下多个进程在 `RUNNABLE` 与 `RUNNING` 之间轮转，同一 PID 连续出现多次说明时间片与就绪队列策略会反复选中同一进程，这与“并发交替执行”的直觉需要放在一起理解。`kalloc` 的打印则把空闲链表分配的形状暴露出来：启动阶段大量连续地址分配与后续按需分配、释放后再分配造成的复用，能对应到课堂上讲的 freelist 管理。扩展 `getpid_plus` 走完了从 `syscall.h` 到 `usys.pl` 的完整链条，体会到 xv6 把系统调用表集中注册、编号与实现分离的设计。第三层的环形缓冲区在加锁临界区内配合 `sleep`/`wakeup`，比单纯自旋更符合内核习惯用法，也加深了对“锁与睡眠必须配对、条件判断在循环里”的印象。整体上，视频与代码对照仍然是最省时间的学法；调试输出一旦加得过多会影响时序与性能，后续作业里会更注意用编译开关或宏包起来。若能在本机跑通 `make grade` 或官方 `usertests`，还可以进一步验证改动没有破坏原有语义。

（以上心得超过 300 字。）

## 构建与运行

```text
make
make qemu
```

在 shell 中可运行：`getpidtest`、`pcdemo`、`syncdemo`。退出 QEMU：按 `Ctrl-a` 再按 `x`（与 xv6 文档一致）。

### 关于“运行截图”

仓库在无图形 QEMU 环境下整理；验收时可在本机运行 `make qemu`，对上述命令 **截屏终端窗口**，或将控制台文本复制到实验报告。截图对应位置：执行 `getpidtest` 的一屏、`pcdemo` 消费 1–8 的一屏、`syncdemo` 两段输出各一屏即可。
