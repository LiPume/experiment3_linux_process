# 消息队列通信流程

## 消息类型

| msg_type | 方向 | 内容 |
|---|---|---|
| 1 | sender → receiver | 普通消息 / "end1" / "end2" |
| 2 | receiver → sender | "over1" / "over2" |

## 信号量

| 信号量 | 初始值 | 作用 |
|---|---|---|
| mutex | 1 | 互斥保护消息队列 |
| empty | 5 | 队列剩余空位 |
| full | 0 | 队列中待读消息数 |
| over | 0 | receiver 通知 sender 已处理 end |
| s_display | 1 | 控制 sender 先输出 |
| r_display | 0 | 控制 receiver 在 sender 发送后再输出 |

## 通信时序

```
sender1/sender2                   receiver
      |                               |
      | sem_wait(s_display/empty/mutex)
      | scanf -> msgsnd(type=1, msg)  |
      | sem_post(r_display/full/mutex)|
      |                               | sem_wait(r_display/full/mutex)
      |                               | msgrcv(type=1)
      |                               | printf("receive: msg")
      |                               | sem_post(mutex/empty/s_display)
      |      (循环，直到输入 exit)      |
      |                               |
      | msgsnd(type=1, "end1"/"end2") |
      | sem_wait(over) ← 阻塞         | msgrcv -> 检测到 "end1"/"end2"
      |                               | msgsnd(type=2, "over1"/"over2")
      |                               | sem_post(over) ──► 唤醒 sender
      | sem_wait(over) 通过            |
      | return NULL                   | flag1=0 / flag2=0
      |                               | (两个都为0后) return NULL
```

## 关键：full 必须在 mutex 之前 wait

```
错误（死锁）：receiver 先拿 mutex，再等 full=0 → 持锁阻塞 → sender 拿不到 mutex → 死锁
正确（本代码）：receiver 先等 full，有消息后再拿 mutex → 不会持锁阻塞
```

## 同步逻辑

- **发送节奏**：s_display=1 保证 sender 优先输出，sender 发完后 post r_display，receiver 才能读；receiver 读完后 post s_display，下一个 sender 才能继续输入。两者严格交替，不会乱序。
- **互斥访问**：mutex 保证同一时刻只有一个线程操作消息队列，防止并发读写冲突。
- **结束握手**：sender 发完 end 后 post r_display/full/mutex 并 break，然后 sem_wait(over) 阻塞等待；receiver 收到 end 后 post over 唤醒对应 sender，sender 收到信号后 return NULL 结束线程。over 信号量会被 post 两次（end1 一次、end2 一次），对应两个 sender 各自的 sem_wait(over)，不会混淆。
