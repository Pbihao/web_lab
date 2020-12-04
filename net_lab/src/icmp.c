#include "icmp.h"
#include "ip.h"
#include <string.h>
#include <stdio.h>

/**
 * @brief 处理一个收到的数据包
 *        你首先要检查ICMP报头长度是否小于icmp头部长度
 *        接着，查看该报文的ICMP类型是否为回显请求，
 *        如果是，则回送一个回显应答（ping应答），需要自行封装应答包。
 * 
 *        应答包封装如下：
 *        首先调用buf_init()函数初始化txbuf，然后封装报头和数据，
 *        数据部分可以拷贝来自接收到的回显请求报文中的数据。
 *        最后将封装好的ICMP报文发送到IP层。  
 * 
 * @param buf 要处理的数据包
 * @param src_ip 源ip地址
 */
void icmp_in(buf_t *buf, uint8_t *src_ip)
{
    // TODO
    if(buf->len < 8)return;
    icmp_hdr_t head;
    memcpy(&head, buf->data, 8);
    head.checksum = swap16(head.checksum);
    head.id = swap16(head.id);
    head.seq = swap16(head.seq);
    if(head.type == 8 && head.code == 0){

        buf_init(&txbuf, buf->len);
        memcpy(txbuf.data, buf->data, buf->len);

        icmp_hdr_t replay;
        replay.type = 0;
        replay.code = 0;
        replay.checksum = 0;
        replay.id = 1;
        replay.seq = 1;
        replay.id = swap16(replay.id);
        replay.seq = swap16(replay.seq);

        memcpy(txbuf.data, &replay, 8);


        replay.checksum = checksum16((uint16_t*)txbuf.data, buf->len);

        memcpy(txbuf.data, &replay, 8);

        fprintf(stderr, "Debug: ");
        for(int i = 0; i < txbuf.len; i++)fprintf(stderr, "%02x ", txbuf.data[i]);
        fprintf(stderr, "\n");

        ip_out(&txbuf, src_ip, NET_PROTOCOL_ICMP);
    }

}

/**
 * @brief 发送icmp不可达
 *        你需要首先调用buf_init初始化buf，长度为ICMP头部 + IP头部 + 原始IP数据报中的前8字节 
 *        填写ICMP报头首部，类型值为目的不可达
 *        填写校验和
 *        将封装好的ICMP数据报发送到IP层。
 * 
 * @param recv_buf 收到的ip数据包
 * @param src_ip 源ip地址
 * @param code icmp code，协议不可达或端口不可达
 */
void icmp_unreachable(buf_t *recv_buf, uint8_t *src_ip, icmp_code_t code)
{

    // TODO
    buf_init(&txbuf, 36);
    icmp_hdr_t sd;
    sd.type = 3;
    sd.code = code;
    sd.checksum = 0;
    sd.id = 0;
    sd.seq = 0;
    
    memcpy(txbuf.data + 8, recv_buf->data, 28);
    memcpy(txbuf.data, &sd, 8);

    sd.checksum = checksum16((uint16_t*)txbuf.data, 36);
    memcpy(txbuf.data, &sd, 8);

    fprintf(stderr, "Debug: ");
    for(int i = 0; i < txbuf.len; i++)fprintf(stderr, "%02x ", txbuf.data[i]);
    fprintf(stderr, "\n");

    ip_out(&txbuf, src_ip, NET_PROTOCOL_ICMP);
}