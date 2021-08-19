#ifndef SYSCALL_INFO
#define SYSCALL_INFO 1
#define num_ts_frames 2048
struct StartEnd {
int start;
int end;
};

struct FrameInfo {
struct StartEnd * func_bounds;
int num_funcs;
};

struct SyscallInfo {
    char handler[100];
    struct FrameInfo frames[num_ts_frames];
};
#endif