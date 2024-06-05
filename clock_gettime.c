#include <stdio.h>
#include <time.h>
#include <unistd.h>

// 函数声明
void do_something();

int main()
{
    struct timespec start, end;

    // 获取开始时间
    clock_gettime(CLOCK_MONOTONIC, &start);

    // 执行一些任务
    do_something();

    // 获取结束时间
    clock_gettime(CLOCK_MONOTONIC, &end);

    // 计算时间差
    double elapsed_time = (end.tv_sec - start.tv_sec) +
                          (end.tv_nsec - start.tv_nsec) / 1e9;

    int secord_time = elapsed_time;

    // 打印结果
    printf("Elapsed time: %.9f seconds %d \n", elapsed_time, secord_time);

    return 0;
}

// 模拟执行任务的函数
void do_something()
{
    // 模拟一些耗时操作，比如睡眠2秒
    sleep(2);
}
