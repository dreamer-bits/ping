#include "ping.h"

#define WAIT_TIME 5

char send_buffer[PING_SEND_BUFFER_SIZE];
char recv_buffer[PING_RECV_BUFFER_SIZE];
int n_recv = 0;
struct timeval first_send_time;
struct timeval last_recv_time;

double min = 0.0;
double avg = 0.0;
double max = 0.0;
double mdev = 0.0;

u_int16_t ping_compute_cksum(struct icmp *p_icmp)
{
    u_int16_t *data = (u_int16_t *)p_icmp;
    int len = PING_ICMP_LEN;
    u_int32_t sum = 0;

    while (len > 1) {
        sum += *data++;
        len -= 2;
    }

    if (len == 1) {
        u_int16_t tmp = *data;
        tmp &= 0xff00;
        sum += tmp;
    }

    while (sum >> 16) {
        sum = (sum >> 16) + (sum & 0x0000ffff);
    }

    sum = ~sum;

    return sum;
}

void ping_set_icmp(u_int16_t seq)
{
    struct icmp *p_icmp;
    struct timeval *d_time;

    p_icmp = (struct icmp *)send_buffer;

    p_icmp->icmp_type = ICMP_ECHO;
    p_icmp->icmp_code = 0;
    p_icmp->icmp_cksum = 0;
    p_icmp->icmp_seq = seq;
    p_icmp->icmp_id = getpid();

    gettimeofday(d_time, NULL);
    p_icmp->icmp_cksum = ping_compute_cksum(p_icmp);

    if (seq == 1) {
        d_time = (struct timeval *)malloc(sizeof(struct timeval));
        first_send_time = *d_time;
    }
}

void ping_send_packet(int sock_icmp, struct sockaddr_in *dest_addr, int n_send)
{
    ping_set_icmp(n_send);

    if (sendto(sock_icmp, send_buffer, PING_ICMP_LEN, 0, 
                (struct sockaddr *)dest_addr, sizeof(struct sockaddr_in)) < 0)
    {
        perror("sendto");
    }
}

double ping_get_rtt(struct timeval *recv_time, struct timeval *send_time)
{
    struct timeval sub = *recv_time;

    if ((sub.tv_usec -= send_time->tv_usec) < 0) {
        --(sub.tv_sec);
        sub.tv_usec += 1000000;
    }

    sub.tv_sec -= send_time->tv_sec;

    return sub.tv_sec * 1000.0 + sub.tv_usec / 1000.0;
}


int ping_unpack(struct timeval *recv_time)
{
    struct ip *p_ip = (struct ip *)recv_buffer;
    struct icmp *p_icmp;
    int ip_head_len;
    double rtt;

    ip_head_len = p_ip->ip_hl << 2;
    p_icmp = (struct icmp *)(recv_buffer + ip_head_len);

    if (p_icmp->icmp_type == ICMP_ECHOREPLY && p_icmp->icmp_id == getpid()) {
        struct timeval *send_time = (struct timeval *)p_icmp->icmp_data;
        rtt = ping_get_rtt(recv_time, send_time);

        printf("%u bytes from %s: icmp_seq=%u ttl=%u time=%.1f ms\n",
                ntohs(p_ip->ip_len) - ip_head_len,
                inet_ntoa(p_ip->ip_src),
                p_icmp->icmp_seq,
                p_ip->ip_ttl,
                rtt);

        if (rtt < min || min == 0) {
            min = rtt;
        }

        if (rtt > max) {
            max = rtt;
        }

        avg += rtt;
        mdev += rtt * rtt;

        return 0;
    }

    return -1;
}

void ping_statistics(int signo)
{
    double tmp;
    avg /= n_recv;
    tmp = mdev / n_recv - avg * avg;
    mdev = sqrt(tmp);

    if (p_host == NULL) {
        printf("--- %s  ping statistics ---\n", p_host->h_name);
    } else {
        printf("--- %s  ping statistics ---\n", IP);
    }

    printf("%d packets transmitted, %d received, %d%% packet loss, time %dms\n",
            n_send,
            n_recv,
            (n_send - n_recv) / n_send * 100,
            (int)ping_get_rtt(&last_recv_time, &first_send_time));
    
    printf("rtt min/avg/max/mdev = %.3f/%.3f/%.3f/%.3f ms\n",
            min, avg, max, mdev);

    close(sock_icmp);
    exit(0);
}

int ping_recv_packet(int sock_icmp, struct sockaddr_in *dest_addr)
{
    int recv_bytes = 0;
    int addrlen = sizeof(struct sockaddr_in);
    struct timeval recv_time;

    signal(SIGALRM, ping_statistics);

    alarm(WAIT_TIME);

    if ((recv_bytes = recvfrom(sock_icmp, recv_buffer, PING_RECV_BUFFER_SIZE, 
                    0, (struct sockaddr *)dest_addr, (socklen_t *)&addrlen)) < 0)
    {
        perror("recvfrom");
        return 0;
    }

    gettimeofday(&recv_time, NULL);

    last_recv_time = recv_time;

    if (ping_unpack(&recv_time) == -1) {
        return -1;
    }

    n_recv++;
    return 0;
}
