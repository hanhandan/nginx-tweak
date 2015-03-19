#include <unistd.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <math.h>
#include <inttypes.h>


enum {
    DIRECTORIES = 100,
    FILE_SIZE = 60*1024*1024,
    RANGE_SIZE = 32*1024,
    RESPONSE_BUFFER_SIZE = 1024+RANGE_SIZE,
    FD_MAX = 100000
};

const char* g_server_ip = "127.0.0.1";
const char* g_server_domain = "localhost";
unsigned short g_server_port = 80;
uint64_t g_server_deploy_bytes = 4L*1024*1024*1024*1024;
int g_max_connect = 1000;

int g_epoll_fd = -1;
int g_current_connect = 0;
int g_total_success = 0;
int g_total_failed = 0;
time_t g_start_time;
uint64_t g_recv_bytes = 0;


struct fd_data_t
{
    char* request;              /**< 请求缓冲区. */
    int request_pos;            /**< 请求缓冲区待发送数据位置. */
    char* response;             /**< 响应缓存区. */
    int response_pos;           /**< 响应缓冲区待接收数据位置. */
};

struct fd_data_t* g_fd_data[FD_MAX] = {NULL};

void init_fd_data(int fd)
{
    if(g_fd_data[fd] == NULL)
    {
        static const char* request_format = "GET /data/%d/%d HTTP/1.1\r\nhost: %s\nRange: bytes=%d-%d\r\nConnection: close\r\n\r\n";
        static char request[1024];
        g_fd_data[fd] = (fd_data_t*)malloc(sizeof(struct fd_data_t));
        memset(g_fd_data[fd], 0, sizeof(struct fd_data_t));
        int file_id = rand()%(g_server_deploy_bytes/FILE_SIZE) + 1/*file id start by 1*/;
        int range_id = rand()%int(ceil(double(FILE_SIZE)/RANGE_SIZE));
        snprintf(request, sizeof(request), request_format, file_id%DIRECTORIES, file_id, g_server_domain, range_id*RANGE_SIZE, (range_id+1)*RANGE_SIZE-1);
        g_fd_data[fd]->request = strdup(request);
    }
}

void free_fd_data(int fd)
{
    if(g_fd_data[fd])
    {
        if(g_fd_data[fd]->response)
        {
            char* body = strstr(g_fd_data[fd]->response, "\r\n\r\n");
            if(body && ((body + 4 - g_fd_data[fd]->response) + RANGE_SIZE) == g_fd_data[fd]->response_pos)
            {
                ++g_total_success;
            }
            else
            {
                ++g_total_failed;
            }
            g_recv_bytes += g_fd_data[fd]->response_pos;
        } else
        {
            ++g_total_failed;
        }

        if(epoll_ctl(g_epoll_fd, EPOLL_CTL_DEL, fd, NULL) == -1)
        {
            perror("epoll_ctl");
        }
        close(fd);
        free(g_fd_data[fd]->request);
        free(g_fd_data[fd]->response);
        free(g_fd_data[fd]);
        g_fd_data[fd] = NULL;
        --g_current_connect;
        assert(g_current_connect >= 0);
    }
}

void async_connect()
{
    int fd = socket(PF_INET, SOCK_STREAM, 0);
    if(fd < 0)
    {
        perror("socket error");
        return;
    }

    int opt = fcntl(fd, F_GETFL);
    if(opt < 0)
    {
        perror("fcntl error");
        close(fd);
        return;
    }

    if(fcntl(fd, F_SETFL, opt | O_NONBLOCK) < 0)
    {
        perror("fcntl error");
        close(fd);
        return;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(g_server_port);
	if(0 == inet_aton(g_server_ip, (struct in_addr*) &addr.sin_addr.s_addr))
    {
        perror("invalid ip");
        close(fd);
        return;
    }

    struct epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLOUT|EPOLLET;
    if(epoll_ctl(g_epoll_fd, EPOLL_CTL_ADD, fd, &event) < 0)
    {
        perror("epoll_ctl error");
        close(fd);
        return;
    }

    if(connect(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1)
    {
        if(errno != EINPROGRESS)
        {
            perror("connect error");
            close(fd);
            return;
        }
    }

    ++g_current_connect;
}

int main(int argc, char *argv[])
{
    if(argc == 2 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0))
    {
        printf("usage: %s [server ip default=%s] [server port default=%"PRIu16"] [server domain default=%s] [server deploy bytes default=%"PRIu64"] [max connection default=%"PRIu32"]\n", argv[0], g_server_ip, g_server_port, g_server_domain, g_server_deploy_bytes, g_max_connect);
        exit(EXIT_FAILURE);
    }

    if (argc > 1)
    {
        g_server_ip = argv[1];
    }

    if (argc > 2)
    {
        g_server_port = atoi(argv[2]);
    }

    if (argc > 3)
    {
        g_server_domain = argv[3];
    }

    if(argc > 4)
    {
        g_server_deploy_bytes = strtoull(argv[4], NULL, 0);
    }

    if(argc > 5)
    {
        g_max_connect = strtoull(argv[5], NULL, 0);

        if (g_max_connect > FD_MAX)
        {
            std::cerr << "max connection must not greater than " << FD_MAX << std::endl;
            exit(EXIT_FAILURE);
        }
    }

    std::cerr << "start test. " << std::endl
              << "                 ip    " << g_server_ip << std::endl
              << "               port    " << g_server_port << std::endl
              << "             domain    " << g_server_domain << std::endl
              << "       deploy bytes    " << g_server_deploy_bytes << std::endl
              << "    max connections    " << g_max_connect << std::endl << std::endl;

    srand(time(NULL));

    g_epoll_fd = epoll_create(1);
    if(g_epoll_fd == -1)
    {
        perror("epoll_create error");
        exit(EXIT_FAILURE);
    }

    g_start_time = time(NULL);
    async_connect();

    int ready;
    struct epoll_event events[1024];
    while(ready = epoll_wait(g_epoll_fd, &events[0], sizeof(events)/sizeof(events[0]), -1))
    {
        for(int i = 0; i < ready; ++i)
        {
            if(g_fd_data[events[i].data.fd] == NULL)
            {
                init_fd_data(events[i].data.fd);
            }

            if(events[i].events & EPOLLOUT)
            {
                const int request_len = strlen(g_fd_data[events[i].data.fd]->request);
                int n = 0;
                while((n = send(events[i].data.fd, g_fd_data[events[i].data.fd]->request + g_fd_data[events[i].data.fd]->request_pos, request_len - g_fd_data[events[i].data.fd]->request_pos, 0)) > 0)
                {
                    g_fd_data[events[i].data.fd]->request_pos += n;
                }
                if(n < 0 && errno != EAGAIN)
                {
                    perror("send error");
                    free_fd_data(events[i].data.fd);
                    continue;
                }
                if(g_fd_data[events[i].data.fd]->request_pos != request_len)
                {
                    std::cerr << "request send not completion" << std::endl;
                    continue;
                }
                events[i].events = EPOLLIN|EPOLLET;
                if(epoll_ctl(g_epoll_fd, EPOLL_CTL_MOD, events[i].data.fd, &events[i]) < 0)
                {
                    perror("epoll_ctl error");
                    free_fd_data(events[i].data.fd);
                    continue;
                }
            }

            if(events[i].events & EPOLLIN)
            {
                if(g_fd_data[events[i].data.fd]->response == NULL){
                    g_fd_data[events[i].data.fd]->response = (char*)malloc(RESPONSE_BUFFER_SIZE);
                    memset(g_fd_data[events[i].data.fd]->response, 0, RESPONSE_BUFFER_SIZE);
                }
                int n = 0;
                while((n = recv(events[i].data.fd, g_fd_data[events[i].data.fd]->response + g_fd_data[events[i].data.fd]->response_pos, RESPONSE_BUFFER_SIZE - g_fd_data[events[i].data.fd]->response_pos, 0)) > 0)
                {
                    g_fd_data[events[i].data.fd]->response_pos += n;
                }
                if(n == 0 || errno != EAGAIN)
                {
                    if(n != 0)
                    {
                        perror("recv error");
                    }
                    free_fd_data(events[i].data.fd);
                    continue;
                }
            }

            if(events[i].events & EPOLLERR)
            {
                free_fd_data(events[i].data.fd);
                continue;
            }
        }

        bool connected = false;
        while(g_current_connect < g_max_connect)
        {
            async_connect();
            connected = true;
        }
        if(connected)
        {
            time_t elapsed_time = time(NULL)  - g_start_time;
            printf("%d connect, %d success(qps: %d), %d failed, %"PRIu64" received(speed: %"PRIu64")\n", g_current_connect, g_total_success, elapsed_time ? g_total_success/elapsed_time : 0, g_total_failed, g_recv_bytes, elapsed_time ? g_recv_bytes/elapsed_time : 0);
        }
    }

    return 0;
}
