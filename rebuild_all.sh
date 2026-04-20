#!/bin/bash

set -e

PROJECT_ROOT=~/os_experiment/experiment3_linux_process

DIRS=(
    "my_shell_lab"
    "pipe_comm"
    "msg_queue"
    "shared_memory"
)

echo "项目根目录: $PROJECT_ROOT"
echo

for dir in "${DIRS[@]}"; do
    TARGET_DIR="$PROJECT_ROOT/$dir"

    if [ ! -d "$TARGET_DIR" ]; then
        echo "跳过不存在的目录: $TARGET_DIR"
        continue
    fi

    echo "========== 处理目录: $TARGET_DIR =========="

    cd "$TARGET_DIR"

    found_c_file=false

    for src in *.c; do
        if [ ! -f "$src" ]; then
            continue
        fi

        found_c_file=true
        exe="${src%.c}"

        if [ -f "$exe" ]; then
            echo "删除旧可执行文件: $exe"
            rm -f "$exe"
        fi

        echo "编译: $src -> $exe"
        gcc "$src" -o "$exe"
    done

    if [ "$found_c_file" = false ]; then
        echo "这个目录下没有 .c 文件"
    fi

    echo
done

echo "全部重新编译完成。"