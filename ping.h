#ifndef __PING_H__
#define __PING_H__

#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/ip_icmp.h>
#include <unistd.h>
#include <sys/time.h>
#include <math.h>
#include <signal.h>

#define PING_ICMP_DATA_LEN 56
#define PING_ICMP_HEAD_LEN 8
#define PING_ICMP_LEN (PING_ICMP_DATA_LEN + PING_ICMP_HEAD_LEN)
#define PING_SEND_BUFFER_SIZE 128
#define PING_RECV_BUFFER_SIZE 128
#define PING_SEND_NUM 100
#define PING_MAX_WAIT_TIME 3

// 保存主机信息
extern struct hostent *p_host;
// icmp套接字
extern int sock_icmp;
extern int n_send;
extern char *IP;

u_int16_t ping_compute_cksum(struct icmp *p_icmp);

void ping_set_icmp(u_int16_t seq);

void ping_send_packet(int sock_icmp, struct sockaddr_in *dest_addr, 
        int n_send);

double ping_get_rtt(struct timeval *recv_time, struct timeval *send_time);

int ping_unpack(struct timeval *recv_time);

void ping_statistics(int signo);

int ping_recv_packet(int sock_icmp, struct sockaddr_in *dest_addr);

#endif
