# 模块名称：消息队列通信

## 1. 功能目标

利用 Linux System V 消息队列，实现三个线程（sender1、sender2、receiver）之间的通信与同步：

- **sender1**：循环从终端读取字符串，通过消息队列发给 receiver（消息后附加 `--s1` 标记）；输入 `exit` 后发 `end1`，等待 receiver 的 over 信号后结束线程。
- **sender2**：与 sender1 完全对称，发 `end2`，等待 over 信号后结束线程。
- **receiver**：接收并显示来自两个 sender 的消息；收到 `end1` 时回复 `over1` 并 post over 信号量；收到 `end2` 时回复 `over2` 并 post over 信号量；两者都收到后结束线程。

---

## 2. 实现思路

### 消息类型设计

只用两种 msg_type，通过消息内容区分 sender1 和 sender2：

| msg_type | 方向 | 内容 |
|---|---|---|
| 1 | sender → receiver | 普通消息 / "end1" / "end2" |
| 2 | receiver → sender | "over1" / "over2" |

### 同步机制：参考生产者-消费者模型

用 6 个 POSIX 信号量实现线程同步与互斥：

| 信号量 | 初始值 | 作用 |
|---|---|---|
| mutex | 1 | 互斥保护消息队列的读写 |
| empty | 5 | 队列空位数，sender 发前检查 |
| full | 0 | 队列消息数，receiver 收前检查 |
| over | 0 | receiver 处理完 end 后唤醒对应 sender |
| s_display | 1 | sender 先输出（初始=1，sender 优先） |
| r_display | 0 | receiver 等 sender 发完再输出 |

**关键顺序**：receiver 必须先 `sem_wait(full)` 再 `sem_wait(mutex)`，否则会发生死锁。

---

## 3. 核心系统调用

```c
// 创建消息队列
int msgid = msgget(key_t key, IPC_CREAT | 0666);

// 发送消息（flag=0：阻塞式）
msgsnd(msgid, &buf, sizeof(buf.msg), 0);

// 接收消息（msgtyp=1：只收类型1，flag=0：阻塞式）
msgrcv(msgid, &buf, sizeof(buf.msg), 1, 0);

// 删除消息队列
msgctl(msgid, IPC_RMID, NULL);

// POSIX 信号量
sem_init(&sem, 0, value);   // 初始化（pshared=0：线程间共享）
sem_wait(&sem);              // P 操作（-1，为0时阻塞）
sem_post(&sem);              // V 操作（+1，唤醒等待线程）
```

---

## 4. 核心代码逻辑

### sender 发送普通消息
```c
sem_wait(&s_display);        // 等轮到自己输出
sem_wait(&empty);            // 等队列有空位
sem_wait(&mutex);            // 加锁
scanf("%s", buf.msg);
strcat(buf.msg, " --s1");    // 附加来源标记
buf.msg_type = 1;
msgsnd(msgid, &buf, sizeof(buf.msg), 0);
sem_post(&r_display);        // 通知 receiver 可以读了
sem_post(&full);             // 消息数 +1
sem_post(&mutex);            // 解锁
```

### sender 发送 end 并等待结束
```c
strcpy(buf.msg, "end1");
msgsnd(msgid, &buf, sizeof(buf.msg), 0);
sem_post(&r_display);
sem_post(&full);
sem_post(&mutex);
// break 出循环后
sem_wait(&over);             // 阻塞，等 receiver 处理完 end1
return NULL;
```

### receiver 接收消息并应答
```c
sem_wait(&r_display);        // 等 sender 发完
sem_wait(&full);             // 等有消息（必须在 mutex 前！）
sem_wait(&mutex);            // 加锁
msgrcv(msgid, &buf, sizeof(buf.msg), 1, 0);
printf("receive: %s\n", buf.msg);
if (strcmp(buf.msg, "end1") == 0) {
    strcpy(buf.msg, "over1");
    buf.msg_type = 2;
    msgsnd(msgid, &buf, sizeof(buf.msg), 0);
    sem_post(&over);         // 唤醒 sender1
    flag1 = 0;
}
sem_post(&mutex);
sem_post(&empty);            // 空位 +1
sem_post(&s_display);        // 让 sender 继续
```

---

## 5. 运行结果说明

### 编译
```bash
make msg_queue
# 或
gcc -Wall -g -o msg_queue/msg_queue_main msg_queue/msg_queue_main.c -lpthread
```

### 运行
```bash
./msg_queue/msg_queue_main
```

### 典型输出示例
```
sender1> hello
receive: hello --s1
-----------------------------
sender2> world
receive: world --s2
-----------------------------
sender1> exit
receive: end1
-----------------------------
sender2> exit
receive: end2
-----------------------------
All senders finished!
```

### 查看/清理消息队列
```bash
ipcs -q                 # 查看当前消息队列
ipcrm -q <msqid>        # 异常退出后手动清理
```

---

## 6. 常见问题与解决方法

| 问题 | 原因 | 解决 |
|---|---|---|
| `msgget error` | 同 key 的消息队列已存在 | `ipcrm -q <msqid>` 清理后重跑 |
| 程序卡死不动 | 信号量顺序写错导致死锁 | 检查 full 是否在 mutex 之前 wait |
| sender1/sender2 提示符交替出现 | s_display 控制两个 sender 轮流等待 | 正常现象，按提示输入即可 |

---

## 7. 2分钟讲解稿

> 我负责的是消息队列通信模块，用一个文件 `msg_queue_main.c` 实现，核心是三个 POSIX 线程通过 System V 消息队列通信。
>
> 消息类型设计上只用了两种 msg_type：类型1是 sender 发给 receiver 的消息，类型2是 receiver 发回的应答。两个 sender 发的消息类型相同，receiver 靠消息内容里的 "end1"/"end2" 来区分是谁结束了。
>
> 同步这块参考了生产者-消费者模型，用了6个信号量。mutex 保证消息队列互斥访问；full 和 empty 控制发送和接收的节奏；over 用来通知 sender receiver 已经处理了它的 end 信号；s_display 和 r_display 保证 sender 和 receiver 交替显示，输出不会混乱。
>
> 有一个关键点是 receiver 里 full 必须在 mutex 之前 wait，不能调换。如果先拿 mutex 再等 full=0，receiver 就会持着锁阻塞，sender 拿不到锁也无法 post full，两边都卡住，就死锁了。这是生产者-消费者模型的经典陷阱。
>
> 最后 main 线程 pthread_join 等三个线程全部结束，再调用 msgctl(IPC_RMID) 删除消息队列，释放内核资源。
