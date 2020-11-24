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
        if(str_len < 42)
        {
            printf("error when recv msg \n");
            return -1;
        }
        ethernet_head = buffer;
        p = ethernet_head;
        printf("Dst MAC address: %.2x:%02x:%02x:%02x:%02x:%02x\n",
            p[6],p[7],p[8],p[9],p[10],p[11]);
        printf("Src MAC address: %.2x:%02x:%02x:%02x:%02x:%02x\n",
            p[0],p[1],p[2],p[3],p[4],p[5]);
        ip_head = ethernet_head+14;
        p = ip_head+12;
        printf("Dst IP: %d.%d.%d.%d\n",p[0],p[1],p[2],p[3]);
        printf("Src IP: %d.%d.%d.%d\n",p[4],p[5],p[6],p[7]);
        proto = (ip_head + 9)[0];
        p = ip_head +12;
        printf("Protocol:");
        switch(proto){
            case IPPROTO_ICMP:printf("icmp\n");break;
            case IPPROTO_TCP:printf("tcp\n");break;
            case IPPROTO_UDP:printf("udp\n");break;
            default:
            break;
        }
    }
    return -1;
}