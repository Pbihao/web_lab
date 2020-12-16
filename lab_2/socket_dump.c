#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <linux/if_ether.h>
#include <netinet/in.h>
#define BUFFER_MAX 2048
int main(int argc,char* argv[]){
    int fd;
    int proto;
    int str_len;
    char buffer[BUFFER_MAX];
    char *ethernet_head;
    char *ip_head;
    unsigned char *p;
    if((fd=socket(PF_PACKET,SOCK_RAW,htons(ETH_P_ALL)))<0)
    {
        printf("error create raw socket\n");
        return -1;
    }
    while(1){
        str_len = recvfrom(fd,buffer,2048,0,NULL,NULL);
        // 想一想,此处为什么要丢弃小于 42 字节的包?
        //最短为arp请求/应答arp请求和应答为42个字节2：类型 28：请求应答 18：PAD
        if(str_len < 42)
        {
            printf("error when recv msg \n");
            return -1;
        }
        ethernet_head = buffer;
        p = ethernet_head;
        printf("Dst MAC address: %02x:%02x:%02x:%02x:%02x:%02x\n",
            p[6],p[7],p[8],p[9],p[10],p[11]);
        printf("Src MAC address: %02x:%02x:%02x:%02x:%02x:%02x\n",
            p[0],p[1],p[2],p[3],p[4],p[5]);


        //如果是arp请求或者应答
        p = ethernet_head+12;
        
        if(p[0] == 0x08 && p[1] == 0x06){
            printf("Protocol:Arp\n");

            p = ethernet_head + 16;
            if(p[0] == 0x08 && p[1] == 0x00)
            printf("Protocol type:IPv4(0x0800)\n");

            p = ethernet_head + 18;
            printf("Hardware size:%d\n", p[0]);
            printf("Protocol size:%d\n", p[1]);

            p = ethernet_head + 20;
            if(p[0] == 0)printf("Opcode: request(1)\n");
            else printf("Opcode: ask(2)\n");

            p = ethernet_head + 22;
            printf("Sender Mac address:%.2x:%02x:%02x:%02x:%02x:%02x\n",
                p[0],p[1],p[2],p[3], p[4], p[5]);
            printf("Sender IP address: %d.%d.%d.%d\n",
                p[6],p[7],p[8],p[9]);
            p = p + 10;
            printf("Target Mac address:%.2x:%02x:%02x:%02x:%02x:%02x\n",
                p[0],p[1],p[2],p[3], p[4], p[5]);
            printf("Target IP address: %d.%d.%d.%d\n",
                p[6],p[7],p[8],p[9]);
            
            p = ethernet_head + 20;
            continue;
        }

        ip_head = ethernet_head+14;
        p = ip_head+12;
        printf("Dst IP: %d.%d.%d.%d\n",p[0],p[1],p[2],p[3]);
        printf("Src IP: %d.%d.%d.%d\n",p[4],p[5],p[6],p[7]);

        p = ip_head;
        int len = (p[0] & 0x0f) * 4;
        unsigned char * ip_data = ip_head + len;

        proto = (ip_head + 9)[0];
        printf("Protocol:");
        switch(proto){
            case IPPROTO_ICMP:
                printf("icmp\n");
                p = ip_data;
                if(*p == 0)
                    printf("Type:0(Echo replay)\n");
                else printf("Type:8(Echo ask)");

                p = p+1;
                printf("Code: %d\n", *p);

                p = p + 1;
                unsigned short* q = (unsigned short*)p;
                printf("Checksum: 0x%04x\n", *q);
                
                p = p + 2;
                q = (unsigned short*)p;
                printf("Identifier : %u(0x%04x)\n", *q, *q);

                p = p + 2;
                q = (unsigned short*)p;
                printf("Sequence number : %u(0x%04x)\n", *q, *q);
            break;
            case IPPROTO_TCP:printf("tcp\n");break;
            case IPPROTO_UDP:printf("udp\n");break;
            default:printf("\n");
            break;
        }
    }
    return -1;
}