#include "udp.h"
#include "ip.h"
#include "icmp.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/**
 * @brief udp处理程序表
 * 
 */
static udp_entry_t udp_table[UDP_MAX_HANDLER];

/**
 * @brief udp伪校验和计算
 *        1. 你首先调用buf_add_header()添加UDP伪头部
 *        2. 将IP头部拷贝出来，暂存被UDP伪头部覆盖的IP头部
 *        3. 填写UDP伪头部的12字节字段
 *        4. 计算UDP校验和，注意：UDP校验和覆盖了UDP头部、UDP数据和UDP伪头部
 *        5. 再将暂存的IP头部拷贝回来
 *        6. 调用buf_remove_header()函数去掉UDP伪头部
 *        7. 返回计算后的校验和。  
 * 
 * @param buf 要计算的包
 * @param src_ip 源ip地址
 * @param dest_ip 目的ip地址
 * @return uint16_t 伪校验和
 */
static uint16_t udp_checksum(buf_t *buf, uint8_t *src_ip, uint8_t *dest_ip)
{
    uint16_t udp_length = swap16(*(uint16_t*)(buf->data + 4));

    // TODO
    buf_add_header(buf, 12);
    uint8_t buff_ip[12];
    memcpy(buff_ip, buf->data, 12);

    udp_peso_hdr_t ip_head;
    memcpy(ip_head.src_ip, src_ip, 4);
    memcpy(ip_head.dest_ip, dest_ip, 4);

    ip_head.placeholder = 0;
    ip_head.protocol = 0x11;

    ip_head.total_len = swap16(udp_length); ////////////////////////swap?sub 12?////////////////////////

    uint8_t src_sum[2];
    memcpy(src_sum, buf->data + 18, 2);
    memset(buf->data + 18, 0, 2);
    memcpy(buf->data, &ip_head, 12);

    uint16_t* p = (uint16_t*)buf->data;
    uint32_t sum = 0;
    for(int i = 0; i < buf->len / 2; i++){
        sum += p[i];
    }

    if(buf->len % 2)sum += buf->data[buf->len - 1];

    sum = (sum >> 16) + (sum & 0xffff);
    sum = (sum >> 16) + (sum & 0xffff);
    sum = ~sum;

    memcpy(buf->data + 18, src_sum, 2);
    memcpy(buf->data, buff_ip, 12);
    buf_remove_header(buf, 12);

    return sum;
}

/**
 * @brief 处理一个收到的udp数据包
 *        你首先需要检查UDP报头长度
 *        接着计算checksum，步骤如下：
 *          （1）先将UDP首部的checksum缓存起来
 *          （2）再将UDP首都的checksum字段清零
 *          （3）调用udp_checksum()计算UDP校验和
 *          （4）比较计算后的校验和与之前缓存的checksum进行比较，如不相等，则不处理该数据报。
 *       然后，根据该数据报目的端口号查找udp_table，查看是否有对应的处理函数（回调函数）
 *       
 *       如果没有找到，则调用buf_add_header()函数增加IP数据报头部(想一想，此处为什么要增加IP头部？？)
 *       然后调用icmp_unreachable()函数发送一个端口不可达的ICMP差错报文。
 * 
 *       如果能找到，则去掉UDP报头，调用处理函数（回调函数）来做相应处理。
 * 
 * @param buf 要处理的包
 * @param src_ip 源ip地址
 */
void udp_in(buf_t *buf, uint8_t *src_ip)
{
    // TODO

    // fprintf(stderr, "Debug: udp in\n");
    // fprintf(stderr, "Debug: Ip is ->\n");
    // for(int i = 0; i < 4; i++)
    // fprintf(stderr, " %x", src_ip[i]);
    // fprintf(stderr, " \n");

    if(buf->len < 8)return;
    udp_hdr_t udp_head;
    memcpy(&udp_head, buf->data, 8);
    uint8_t my_ip[4] = DRIVER_IF_IP;

    // printf("Debug:");
    // for(int i = 0; i < buf->len; i++)printf("%02x ", buf->data[i]);
    // printf("\n");

    // printf("Debug:checksum-> %04x\n", udp_checksum(buf, src_ip, my_ip));
    // printf("Debug:checksum-> %04x\n", udp_head.checksum);

    if(udp_head.checksum != udp_checksum(buf, src_ip, my_ip))return;

    // printf("Debug:");
    // for(int i = 0; i < buf->len; i++)printf("%02x ", buf->data[i]);
    // printf("\n");
    // printf("Debug:checksum %x\n", udp_head.checksum);

    udp_head.total_len = swap16(udp_head.total_len);
    udp_head.src_port = swap16(udp_head.src_port);
    udp_head.dest_port = swap16(udp_head.dest_port);

    int ok = 0;
    for(int i = 0; i < UDP_MAX_HANDLER; i++){
        if(udp_table[i].port == udp_head.dest_port){
            ok = 1;
            buf_remove_header(buf, 8);
            udp_table[i].handler(&udp_table[i], src_ip, udp_head.src_port, buf);
            break;
        }
    }
    if(!ok){
        buf_add_header(buf, 8);
        fprintf(stderr,"icmp code port unreach\n");
        icmp_unreachable(buf, src_ip, ICMP_CODE_PORT_UNREACH);
    }
}

/**
 * @brief 处理一个要发送的数据包
 *        你首先需要调用buf_add_header()函数增加UDP头部长度空间
 *        填充UDP首部字段
 *        调用udp_checksum()函数计算UDP校验和
 *        将封装的UDP数据报发送到IP层。    
 * 
 * @param buf 要处理的包
 * @param src_port 源端口号
 * @param dest_ip 目的ip地址
 * @param dest_port 目的端口号
 */
void udp_out(buf_t *buf, uint16_t src_port, uint8_t *dest_ip, uint16_t dest_port)
{
    // TODO
    // fprintf(stderr, "Debug: udp out\n");
    buf_add_header(buf, 8);
    udp_hdr_t udp_head;
    udp_head.src_port = swap16(src_port);
    udp_head.dest_port = swap16(dest_port);
    udp_head.total_len = swap16(buf->len);
    uint8_t src_ip[4] = DRIVER_IF_IP;
    udp_head.checksum = 0;
    
    memcpy(buf->data, &udp_head, 12);
    udp_head.checksum = udp_checksum(buf, src_ip, dest_ip);
    memcpy(buf->data, &udp_head, 12);

    ip_out(buf, dest_ip, NET_PROTOCOL_UDP);
}

/**
 * @brief 初始化udp协议
 * 
 */
void udp_init()
{
    for (int i = 0; i < UDP_MAX_HANDLER; i++)
        udp_table[i].valid = 0;
}

/**
 * @brief 打开一个udp端口并注册处理程序
 * 
 * @param port 端口号
 * @param handler 处理程序
 * @return int 成功为0，失败为-1
 */
int udp_open(uint16_t port, udp_handler_t handler)
{
    for (int i = 0; i < UDP_MAX_HANDLER; i++) //试图更新
        if (udp_table[i].port == port)
        {
            udp_table[i].handler = handler;
            udp_table[i].valid = 1;
            return 0;
        }

    for (int i = 0; i < UDP_MAX_HANDLER; i++) //试图插入
        if (udp_table[i].valid == 0)
        {
            udp_table[i].handler = handler;
            udp_table[i].port = port;
            udp_table[i].valid = 1;
            return 0;
        }
    return -1;
}

/**
 * @brief 关闭一个udp端口
 * 
 * @param port 端口号
 */
void udp_close(uint16_t port)
{
    for (int i = 0; i < UDP_MAX_HANDLER; i++)
        if (udp_table[i].port == port)
            udp_table[i].valid = 0;
}

/**
 * @brief 发送一个udp包
 * 
 * @param data 要发送的数据
 * @param len 数据长度
 * @param src_port 源端口号
 * @param dest_ip 目的ip地址
 * @param dest_port 目的端口号
 */
void udp_send(uint8_t *data, uint16_t len, uint16_t src_port, uint8_t *dest_ip, uint16_t dest_port)
{
    buf_init(&txbuf, len);
    memcpy(txbuf.data, data, len);
    udp_out(&txbuf, src_port, dest_ip, dest_port);
}



