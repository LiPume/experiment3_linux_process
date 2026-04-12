CC = gcc
CFLAGS = -Wall -O2
PTHREAD_FLAGS = -lpthread

.PHONY: all clean shell pipe msg shm benchmark

all: shell pipe msg shm

# ===== 实验1：Shell =====
shell: my_shell_lab/myshell my_shell_lab/cmd1 my_shell_lab/cmd2 my_shell_lab/cmd3

my_shell_lab/myshell: my_shell_lab/myshell.c
	$(CC) $(CFLAGS) -o my_shell_lab/myshell my_shell_lab/myshell.c

my_shell_lab/cmd1: my_shell_lab/cmd1.c
	$(CC) $(CFLAGS) -o my_shell_lab/cmd1 my_shell_lab/cmd1.c

my_shell_lab/cmd2: my_shell_lab/cmd2.c
	$(CC) $(CFLAGS) -o my_shell_lab/cmd2 my_shell_lab/cmd2.c

my_shell_lab/cmd3: my_shell_lab/cmd3.c
	$(CC) $(CFLAGS) -o my_shell_lab/cmd3 my_shell_lab/cmd3.c

# ===== 实验2：管道 =====
pipe: pipe_comm/pipe_main pipe_comm/pipe_block_write

pipe_comm/pipe_main: pipe_comm/pipe_main.c
	$(CC) $(CFLAGS) -o pipe_comm/pipe_main pipe_comm/pipe_main.c

pipe_comm/pipe_block_write: pipe_comm/pipe_block_write.c
	$(CC) $(CFLAGS) -o pipe_comm/pipe_block_write pipe_comm/pipe_block_write.c

# ===== 实验3：消息队列 =====
msg: msg_queue/msg_queue

msg_queue/msg_queue: msg_queue/msg_queue_main.c
	$(CC) $(CFLAGS) -o msg_queue/msg_queue msg_queue/msg_queue_main.c $(PTHREAD_FLAGS)

# ===== 实验4：共享内存 =====
shm: shared_memory/sender shared_memory/receiver

shared_memory/sender: shared_memory/shm_sender.c
	$(CC) $(CFLAGS) -o shared_memory/sender shared_memory/shm_sender.c

shared_memory/receiver: shared_memory/shm_receiver.c
	$(CC) $(CFLAGS) -o shared_memory/receiver shared_memory/shm_receiver.c

# ===== 可选：性能测试程序 =====
benchmark: pipe_comm/pipe_benchmark msg_queue/msg_queue_benchmark shared_memory/shm_benchmark_sender shared_memory/shm_benchmark_receiver

pipe_comm/pipe_benchmark: pipe_comm/pipe_benchmark.c
	$(CC) $(CFLAGS) -o pipe_comm/pipe_benchmark pipe_comm/pipe_benchmark.c

msg_queue/msg_queue_benchmark: msg_queue/msg_queue_benchmark.c
	$(CC) $(CFLAGS) -o msg_queue/msg_queue_benchmark msg_queue/msg_queue_benchmark.c

shared_memory/shm_benchmark_sender: shared_memory/shm_benchmark_sender.c
	$(CC) $(CFLAGS) -o shared_memory/shm_benchmark_sender shared_memory/shm_benchmark_sender.c

shared_memory/shm_benchmark_receiver: shared_memory/shm_benchmark_receiver.c
	$(CC) $(CFLAGS) -o shared_memory/shm_benchmark_receiver shared_memory/shm_benchmark_receiver.c

# ===== 清理 =====
clean:
	rm -f my_shell_lab/myshell
	rm -f my_shell_lab/cmd1 my_shell_lab/cmd2 my_shell_lab/cmd3
	rm -f pipe_comm/pipe_main pipe_comm/pipe_block_write pipe_comm/pipe_benchmark
	rm -f msg_queue/msg_queue msg_queue/msg_queue_benchmark
	rm -f shared_memory/sender shared_memory/receiver
	rm -f shared_memory/shm_benchmark_sender shared_memory/shm_benchmark_receiver
