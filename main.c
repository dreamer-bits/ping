#include "ping.h"

// 保存主机信息
struct hostent *p_host = NULL;
// icmp套接字
int sock_icmp;
int n_send = 1;
char *IP = NULL;

void call(int argc, char *argv[]);

int main(int argc, char *argv[])
{
    call(argc, argv);
    return 0;
}

void call(int argc, char *argv[])
{
    struct protoent *protocol;
    struct sockaddr_in dest_addr;
    in_addr_t inaddr;

    if (argc < 2) {
        printf("Usage: %s [hostname/IP address]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    if ((protocol = getprotobyname("icmp")) == NULL) {
        perror("getprotobyname");
        exit(EXIT_FAILURE);
    }

    if ((sock_icmp = socket(PF_INET, SOCK_RAW, protocol->p_proto)) < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    dest_addr.sin_family = AF_INET;

    // 将点分十进制ip地址转换为网络字节序
    if ((inaddr = inet_addr(argv[1])) == INADDR_NONE) {
        // 转换失败则代表用户输入的是主机名，尝试通过主机名获取ip
        if ((p_host = gethostbyname(argv[1])) == NULL) {
            perror("gethostbyname()");
            exit(EXIT_FAILURE);
        }
        memmove(&dest_addr.sin_addr, p_host->h_addr_list[0], p_host->h_length);
    } else {
        memmove(&dest_addr.sin_addr, &inaddr, sizeof(struct in_addr));
    }

    if (p_host != NULL) {
        printf("PING %s\n", p_host->h_name);
    } else {
        printf("PING %s\n", argv[1]);
    }

    printf("PING %s(%s) %d bytes of data.\n", argv[1], 
            inet_ntoa(dest_addr.sin_addr), PING_ICMP_DATA_LEN);

    IP = argv[1];
    signal(SIGINT, ping_statistics);

    while (n_send < PING_SEND_NUM) {
        int unpack_ret;
        
        ping_send_packet(sock_icmp, &dest_addr, n_send);

        unpack_ret = ping_recv_packet(sock_icmp, &dest_addr);
        if (unpack_ret == -1) {
            ping_recv_packet(sock_icmp, &dest_addr);
        }

        sleep(1);
        n_send++;
    }

    ping_statistics(0);
}
