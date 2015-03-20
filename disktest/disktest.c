#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <ctype.h>
#include <time.h>
#include <math.h>
#include <pthread.h>


enum {
    DIRECTORIES = 100,
    FILE_SIZE = 60*1024*1024,
    RANGE_SIZE = 32*1024,
};

int g_read_count = 100000;
uint64_t g_deploy_bytes = 4L*1024*1024*1024*1024;
const char* g_data_path = "./data/";
uint64_t g_read_bytes = 0;


void* thread(void* arg)
{
    int id = (int)arg;
    char filename[512];
    char buffer[RANGE_SIZE];
    int file_id;
    int range_id;
    int directory_id;
    int file_fd;
    int read_bytes;

    while (__sync_sub_and_fetch(&g_read_count, 1) >= 0) {
        file_id = rand()%(g_deploy_bytes/FILE_SIZE) + 1/*file id start by 1*/;
        range_id = rand()%(int)ceil((double)FILE_SIZE/RANGE_SIZE);
        directory_id = file_id%DIRECTORIES;
        snprintf(filename, sizeof(filename), "%s/%d/%d", g_data_path, directory_id, file_id);
        file_fd = open(filename, O_RDONLY);
        if (file_fd == -1) {
            fprintf(stderr, "Open file %s error: %d\n", filename, errno);
            return (void*)errno;
        }
        if (range_id) {
            if (-1 == lseek(file_fd, range_id*RANGE_SIZE, SEEK_SET)) {
                fprintf(stderr, "Seek file %s error: %d\n", filename, errno);
                return (void*)errno;
            }
        }
        read_bytes = read(file_fd, buffer, sizeof(buffer));
        if (-1 == read_bytes) {
            fprintf(stderr, "Read file %s range %d error: %d\n", filename, range_id, errno);
            return (void*)errno;
        }
        close(file_fd);
        __sync_add_and_fetch(&g_read_bytes, read_bytes);
    }

    return NULL;
}

int main(int argc, char *argv[])
{
    int optopt = 0;
    uint32_t thread_num = 8;

    while ((optopt = getopt(argc, argv, "d:p:t:c:h")) != -1) {
        switch (optopt) {
        case 'd':
            g_deploy_bytes = strtoull(optarg, NULL, 0);
            break;

        case 'p':
            g_data_path = optarg;
            break;

        case 't':
            thread_num = strtoul(optarg, NULL, 0);
            if (thread_num < 1) {
                fprintf(stderr, "Thread numbers must greater than 0\n");
                exit(EXIT_FAILURE);
            }
            break;

        case 'c':
            g_read_count = strtol(optarg, NULL, 0);
            break;

        case 'h':
            fprintf(stderr, "usage: %s [OPTION]...\n\t-d deploy bytes, default is %"PRIu64"\n\t-p data path, default is %s\n\t-t thread numbers, default is %"PRIu32"\n\t-c read count, default is %"PRIu32"\n\t-h display this help and exit\n", argv[0], g_deploy_bytes, g_data_path, thread_num, g_read_count);
            exit(EXIT_FAILURE);

        case '?':
            fprintf(stderr, "Illegal option:-%c\n", isprint(optopt) ? optopt : '#');
            break;

        default:
            fprintf(stderr, "Not supported option\n");
            break;
        }
    }

    time_t start_time = time(NULL);
    srand(start_time);

    pthread_t* threads = (pthread_t*)calloc(thread_num, sizeof(pthread_t));

    int i;
    int err;

    for (i = 0; i < thread_num; ++i) {
        err = pthread_create(&threads[i], NULL, thread, (void*)i);
        if (err != 0) {
            fprintf(stderr, "Create thread #%d error: %d\n", i, err);
            exit(EXIT_FAILURE);
        }
    }

    for (i = 0; i < thread_num; ++i) {
        pthread_join(threads[i], NULL);
    }

    time_t elapsed_time = time(NULL)  - start_time;
    printf("%"PRIu64" bytes readed, %"PRIu64" bytes per second\n", g_read_bytes, elapsed_time ? g_read_bytes/elapsed_time : 0);

    return 0;
}
