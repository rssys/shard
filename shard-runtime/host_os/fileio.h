#ifndef FILE_IO

#define FILE_IO 1

// #define TIME_IT 1
#define DISABLE_LOGGING 1
#define DISABLE_SS_LOGGING 1

// #define APPLICATION_SPECIALIZATION 1
// #define DISABLE_SC_EXIT 1
// #define DISABLE_SC_ENTRY 1
// #define STOP_CHANGING_SCOPE 1
// #define CHANGE_SCOPE_TO_ORIGINAL 1


// #define PROFILING 1
// #define CTX_NCS_NHD 1 // APP_TRAP track context switches, dont change scope
// #define CTX_CS_NHD  1 // APP_EPT track context switches, change scope back to original (no hardening)

// #define SC_NCS_NHD  1 // SHARD_TRAP : track system calls, dont change scope
// #define SC_CS_NHD   1 //  SHARD_EPT : track system calls, change scope back to original (no hardening)
#define SC_CS_HD    1 // SHARD : track system calls, change scope to system call specialized (hardening)

#ifndef PROFILING
    #define DISABLE_SC_EXIT 1
    #define LARGE_FILE 1
#endif


#ifdef CTX_NCS_NHD
    #define DISABLE_SC_ENTRY 1
    #define APPLICATION_SPECIALIZATION 1
    #define STOP_CHANGING_SCOPE 1
#endif

#ifdef CTX_CS_NHD
    #define DISABLE_SC_ENTRY 1
    #define APPLICATION_SPECIALIZATION 1
    #define CHANGE_SCOPE_TO_ORIGINAL 1
#endif

#ifdef SC_NCS_NHD
    #define STOP_CHANGING_SCOPE 1
#endif

#ifdef SC_CS_NHD
    #define CHANGE_SCOPE_TO_ORIGINAL 1
#endif

#ifdef SC_CS_HD
#endif

// #ifdef STOP_CHANGING_SCOPE
//     #define CHANGE_SCOPE_TO_ORIGINAL 1
// #endif

struct file * file_open(const char * path, int flags, int rights)  {
    struct file * filp = NULL;
    mm_segment_t oldfs;
    int err = 0;
    oldfs = get_fs();
    set_fs(get_ds());
    filp = filp_open(path, flags, rights);
    set_fs(oldfs);
    if(IS_ERR(filp)) {
        err = PTR_ERR(filp);
        dt_printk("file_open : failed to open file : %d\n", err);
        return NULL;
    }
    return filp;
}

void file_close(struct file *file) {
    filp_close(file, NULL);
}

int file_read(struct file * file, unsigned char * data, unsigned int size) {
    mm_segment_t oldfs;
    int ret;
    oldfs = get_fs();
    set_fs(get_ds());
    ret = kernel_read(file, data, size, &file->f_pos);
    set_fs(oldfs);
    return ret;
}

int file_write(struct file *file, unsigned char *data, unsigned int size)  {
    mm_segment_t oldfs;
    int ret;
    oldfs = get_fs();
    set_fs(get_ds());
    ret = kernel_write(file, data, size, &file->f_pos);
    set_fs(oldfs);
    return ret;
}

bool dt_do_safe_read(struct file * fd, char * buffer, int size) {
    int bytes_read;
    bytes_read = file_read(fd, buffer, size);
    return bytes_read == size;
}

struct file * dt_init_log(char * path) {
    struct file * fd_log = file_open(path, O_CREAT |  O_RDWR | O_APPEND, S_IRWXU | S_IRWXG | S_IRWXO);
    dt_printk("fd_log is %p", fd_log);
    return fd_log;
}

void write_to_log(struct file * fd_log, char * str, int num_bytes) {
    #ifndef DISABLE_LOGGING
    if(fd_log) {
        file_write(fd_log, str, num_bytes);
    }
    #endif
}

void profile(struct file * fd_log, char * str, int num_bytes) {
    if(fd_log) {
        file_write(fd_log, str, num_bytes);
    }    
}

// void write_to_log_sprintf(struct file * fd_log, char * str, ...) {
//     #ifndef DISABLE_LOGGING
//     char log_buffer[1024];
//     va_list va;
//     va_start(va, str);
//     sprintf(log_buffer, str, va);
//     write_to_log(fd_log, log_buffer, strlen(log_buffer));
//     #endif
// }


// void profile_sprintf(struct file * fd_log, char * str, ...) {
//     char log_buffer[1024];
//     va_list va;
//     va_start(va, str);
//     sprintf(log_buffer, str, va);
//     profile(fd_log, log_buffer, strlen(log_buffer));
// }
char log_buffer[1024];

#ifndef DISABLE_LOGGING
#define write_to_log_sprintf(fd_log, str, ...) \
    sprintf(log_buffer, str, ##__VA_ARGS__); \
    write_to_log(fd_log, log_buffer, strlen(log_buffer));
#else
#define write_to_log_sprintf(fd_log, str, ...)
#endif

#define profile_sprintf(fd_log, str, ...) \
    sprintf(log_buffer, str, ##__VA_ARGS__); \
    profile(fd_log, log_buffer, strlen(log_buffer));


#endif