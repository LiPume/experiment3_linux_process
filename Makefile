CC = gcc
CFLAGS = -Wall -g
PTHREAD_FLAGS = -lpthread

all: shell cmd1 cmd2 cmd3 pipe_comm msg_queue shm_sender shm_receiver

shell: shell/shell_main.c
	$(CC) $(CFLAGS) -o shell/shell_main shell/shell_main.c

cmd1: shell/cmd1.c
	$(CC) $(CFLAGS) -o shell/cmd1 shell/cmd1.c

cmd2: shell/cmd2.c
	$(CC) $(CFLAGS) -o shell/cmd2 shell/cmd2.c

cmd3: shell/cmd3.c
	$(CC) $(CFLAGS) -o shell/cmd3 shell/cmd3.c

pipe_comm: pipe_comm/pipe_main.c
	$(CC) $(CFLAGS) -o pipe_comm/pipe_main pipe_comm/pipe_main.c

msg_queue: msg_queue/msg_queue_main.c
	$(CC) $(CFLAGS) -o msg_queue/msg_queue_main msg_queue/msg_queue_main.c $(PTHREAD_FLAGS)

shm_sender: shared_memory/shm_sender.c
	$(CC) $(CFLAGS) -o shared_memory/shm_sender shared_memory/shm_sender.c

shm_receiver: shared_memory/shm_receiver.c
	$(CC) $(CFLAGS) -o shared_memory/shm_receiver shared_memory/shm_receiver.c

clean:
	rm -f shell/shell_main shell/cmd1 shell/cmd2 shell/cmd3
	rm -f pipe_comm/pipe_main
	rm -f msg_queue/msg_queue_main
	rm -f shared_memory/shm_sender shared_memory/shm_receiver
