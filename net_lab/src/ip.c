#include "ip.h"
#include "arp.h"
#include "icmp.h"
#include "udp.h"
#include <string.h>
#include <stdio.h>

/**
 * @brief 处理一个收到的数据包
 *        你首先需要做报头检查，检查项包括：版本号、总长度、首部长度等。
 * 
 *        接着，计算头部校验和，注意：需要先把头部校验和字段缓存起来，再将校验和字段清零，
 *        调用checksum16()函数计算头部检验和，比较计算的结果与之前缓存的校验和是否一致，
 *        如果不一致，则不处理该数据报。
 * 
 *        检查收到的数据包的目的IP地址是否为本机的IP地址，只处理目的IP为本机的数据报。
 * 
 *        检查IP报头的协议字段：
 *        如果是ICMP协议，则去掉IP头部，发送给ICMP协议层处理
 *        如果是UDP协议，则去掉IP头部，发送给UDP协议层处理
 *        如果是本实验中不支持的其他协议，则需要调用icmp_unreachable()函数回送一个ICMP协议不可达的报文。
 *          
 * @param buf 要处理的包
 */
void ip_in(buf_t *buf)
{
    //
    // TODO 
    ip_hdr_t ip_head;
    memcpy(&ip_head, buf->data, sizeof(ip_hdr_t));
    ip_head.total_len = swap16(ip_head.total_len);
    ip_head.id = swap16(ip_head.id);
    ip_head.flags_fragment = swap16(ip_head.flags_fragment);
    ip_head.hdr_checksum = swap16(ip_head.hdr_checksum);

    if(!ip_head.version == 0b0100 || ip_head.hdr_len < 5)return;
    uint16_t checksum = ip_head.hdr_checksum;
    memset(buf->data + 10, 0, 2);
    if(checksum16((uint16_t*)buf->data, ip_head.hdr_len * 2) != checksum)return;
    uint8_t my_ip[4] = DRIVER_IF_IP;
    if(memcmp(ip_head.dest_ip, my_ip, 4))return;

    
    switch(ip_head.protocol){
        case NET_PROTOCOL_ICMP:
        buf_remove_header(buf, ip_head.hdr_len * 4);
        icmp_in(buf, ip_head.src_ip);
        break;
        case NET_PROTOCOL_UDP:
        buf_remove_header(buf, ip_head.hdr_len * 4);
        udp_in(buf, ip_head.src_ip);
        break;
        default:
        icmp_unreachable(buf, ip_head.src_ip, ICMP_CODE_PROTOCOL_UNREACH);
        break;
    }
}

/**
 * @brief 处理一个要发送的ip分片
 *        你需要调用buf_add_header增加IP数据报头部缓存空间。
 *        填写IP数据报头部字段。
 *        将checksum字段填0，再调用checksum16()函数计算校验和，并将计算后的结果填写到checksum字段中。
 *        将封装后的IP数据报发送到arp层。
 * 
 * @param buf 要发送的分片
 * @param ip 目标ip地址
 * @param protocol 上层协议
 * @param id 数据包id
 * @param offset 分片offset，必须被8整除
 * @param mf 分片mf标志，是否有下一个分片
 */
void ip_fragment_out(buf_t *buf, uint8_t *ip, net_protocol_t protocol, int id, uint16_t offset, int mf)
{
    // TODO

    // printf("Debug: begin to ip_fragment_out\n");

    buf_add_header(buf, 20);

    // for(int i = 0; i < 20; i++)printf("Debug %d\n", buf->data[i]);

    ip_hdr_t ip_head;
    ip_head.hdr_len = 5;
    ip_head.version = 0b0100;
    ip_head.tos = 0;
    ip_head.total_len = swap16(buf->len);
    ip_head.id = swap16(id);
    ip_head.flags_fragment = swap16(((offset << 3) >> 3) | (mf << 13));
    ip_head.ttl = 64;
    ip_head.protocol = protocol;
    ip_head.hdr_checksum = 0;

    // printf("Debug: complete ip_head\n");
    
    uint8_t my_ip[4] = DRIVER_IF_IP;
    memcpy(ip_head.src_ip, my_ip, 4);

    memcpy(ip_head.dest_ip, ip, 4);

    // printf("Debug: buf->data point %p\n", buf->data);
    // printf("Debug: buf len %d\n", buf->len);

    memcpy(buf->data, &ip_head, 20);

    // printf("Debug: begin to checksum16\n");
    
    ip_head.hdr_checksum = checksum16((uint16_t*)buf->data, 10); ////////////////////////////////////////////////////std forgot to swap
    memcpy(buf->data, &ip_head, 20);
    

    // printf("Debug: begin to arp_out\n");

    arp_out(buf, ip, 0x0800);
}

/**
 * @brief 处理一个要发送的ip数据包
 *        你首先需要检查需要发送的IP数据报是否大于以太网帧的最大包长（1500字节 - 以太网报头长度）。
 *        
 *        如果超过，则需要分片发送。 
 *        分片步骤：
 *        （1）调用buf_init()函数初始化buf，长度为以太网帧的最大包长（1500字节 - 以太网报头长度）
 *        （2）将数据报截断，每个截断后的包长度 = 以太网帧的最大包长，调用ip_fragment_out()函数发送出去
 *        （3）如果截断后最后的一个分片小于或等于以太网帧的最大包长，
 *             调用buf_init()函数初始化buf，长度为该分片大小，再调用ip_fragment_out()函数发送出去
 *             注意：id为IP数据报的分片标识，从0开始编号，每增加一个分片，自加1。最后一个分片的MF = 0
 *    
 *        如果没有超过以太网帧的最大包长，则直接调用调用ip_fragment_out()函数发送出去。r
 * 
 * @param buf 要处理的包
 * @param ip 目标ip地址
 * @param protocol 上层协议
 */
uint16_t ip_id = 0x00;
void ip_out(buf_t *buf, uint8_t *ip, net_protocol_t protocol)
{
    // TODO 
    int MAX_LENGTH = ETHERNET_MTU - 20;
    int offest = 0;
    while(buf->len > MAX_LENGTH){

        // printf("Debug:  %d\n", buf->len);

        buf_init(&rxbuf, MAX_LENGTH);

        // printf("Degug: buf_init succeed!\n");

        
        // for(int i = 0; i < 20; i++)printf("Debug: ip_out %d\n", rxbuf.data[i]);
        memcpy(rxbuf.data, buf->data, MAX_LENGTH);

        // printf("Degug: memcpy succeed!\n");
        // printf("Debug: rxbuf.len %d\n", rxbuf.len);

        ip_fragment_out(&rxbuf, ip, protocol, ip_id, offest, 1);

        // printf("Degug: ip_fragment_out succeed!\n");

        offest += (MAX_LENGTH >> 3);
        buf_remove_header(buf, MAX_LENGTH);
    }
  //  fprintf(stderr, "Debug:protocol %x\n", protocol);
    ip_fragment_out(buf, ip, protocol, ip_id, offest, 0);
    ip_id += 1;
}
//45 00 00 46 00 00 00 00 40 11 1c f3 c0 a8 a3 67 c0 a8 a3 0a
//45 00 00 46 00 00 00 00 40 11 b2 e4 c0 a8 a3 67 c0 a8 a3 0a