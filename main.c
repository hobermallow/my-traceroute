/**
* A simple traceroute implementation
*/
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/ip.h>
#include <netinet/in.h>
#include <netinet/ip_icmp.h>
#include <malloc.h>
#include <string.h>
#include <resolv.h>

int sock;
/**
* function to display buffer content
*/
void display_packet_content(void* buffer, uint32_t size);

/**
* function to ping destination
*/
void ping(struct hostent*);

/**
* function to listen responses
*/
void trace(struct hostent*);

/**
* checksum function
*/
unsigned short in_cksum(unsigned short *, int );



int main(int argc, char* argv[]) {

    struct hostent* host;
    uint32_t addr;

    if(argc != 2) {
        perror("Usage traceroute <destination>\n");
        exit(-1);
    }

    //read destination host and convert to ip address
    if(0 == inet_pton(AF_INET, argv[1], &addr)) {
        //try to convert string to host
        host = gethostbyname(argv[1]);
        if(host == NULL) {
            perror("Enter valid host\n");
            exit(-1);
        }
        addr = (*((struct in_addr**)host->h_addr_list)[0]).s_addr;
    }
    else {
        host = gethostbyaddr(&addr, sizeof(addr), AF_INET);
    }
    printf("Host name %s\n", host->h_name);
    printf("Ip address %s\n", inet_ntoa((*((struct in_addr**)host->h_addr_list)[0])));

    ping(host);

    while(1) {
        trace(host);
    }



}

void ping(struct hostent* host) {


    int optval;
    struct sockaddr_in dest_addr;
    struct iphdr *ip;
    struct icmphdr *icmp;

    sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if(-1 == sock) {
        perror("Can't create socket\n");
        exit(-1);
    }
    printf("Socket created\n");
    //populating socket address
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_port = 0;
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_addr.s_addr = (*((struct in_addr**)host->h_addr_list)[0]).s_addr;

    //create icmp packet
    /*ip = malloc(sizeof(struct iphdr) + sizeof(struct icmphdr));
    memset(ip, 0, sizeof(struct iphdr) + sizeof(struct icmphdr));
    icmp = (struct icmphdr*) (ip + sizeof(struct iphdr));
    ip->ttl         = 255;
    ip->ihl         = 5;
    ip->version     = 4;
    ip->tot_len     = (sizeof(struct iphdr) + sizeof(struct icmphdr));
    printf("total length %d\n", ip->tot_len);
    ip->protocol    = htons(1);
    ip->saddr       = inet_addr("192.168.1.15");
    ip->daddr       = dest_addr.sin_addr.s_addr;
    ip->check = in_cksum((unsigned short *)ip, sizeof(struct iphdr));
    */

    icmp = malloc(sizeof(struct icmphdr));
    icmp->code = 0;
    icmp->type = ICMP_ECHO;
    icmp->un.echo.sequence = rand();
    icmp->un.echo.id = rand();

    icmp->checksum = in_cksum((unsigned short *)icmp, sizeof(struct icmphdr));


    /* IP_HDRINCL must be set on the socket so that the kernel does not attempt
     *  to automatically add a default ip header to the packet*/
    /*if(-1 == setsockopt(sock, IPPROTO_IP, IP_HDRINCL, &optval, sizeof(int))) {
        perror("Error setsockopt\n");
        exit(-1);
    }*/


    //sending 10 packets
    for(int i = 0; i < 10; i++) {
        if(-1 == sendto(sock, icmp, sizeof(struct icmphdr), 0, (struct sockaddr*)&dest_addr, sizeof(dest_addr))) {
            perror("Error sending packet\n");
            exit(-1);
        }
        printf("Sent %ld bytes\n", sizeof(struct iphdr) + sizeof(struct icmphdr));
        printf("Packet type: %d\n", icmp->type);
    }
}

unsigned short in_cksum(unsigned short *addr, int len)
{
    register int sum = 0;
    u_short answer = 0;
    register u_short *w = addr;
    register int nleft = len;
    /*
     * Our algorithm is simple, using a 32 bit accumulator (sum), we add
     * sequential 16 bit words to it, and at the end, fold back all the
     * carry bits from the top 16 bits into the lower 16 bits.
     */
    while (nleft > 1)
    {
      sum += *w++;
      nleft -= 2;
    }
    /* mop up an odd byte, if necessary */
    if (nleft == 1)
    {
      *(u_char *) (&answer) = *(u_char *) w;
      sum += answer;
    }
    /* add back carry outs from top 16 bits to low 16 bits */
    sum = (sum >> 16) + (sum & 0xffff);     /* add hi 16 to low 16 */
    sum += (sum >> 16);             /* add carry */
    answer = ~sum;              /* truncate to 16 bits */
    return (answer);
}

void trace(struct hostent* host) {
    struct sockaddr_in dest_addr;
    char* buffer;
    int recvlen;
    struct iphdr* ip;
    struct icmphdr* icmp;
    buffer = malloc(sizeof(struct iphdr) + sizeof(struct icmphdr));
     //populating socket address
    //dest_addr.sin_port = 0;
    //dest_addr.sin_family = AF_INET;
    //dest_addr.sin_addr.s_addr = (*((struct in_addr**)host->h_addr_list)[0]).s_addr;

    socklen_t len = sizeof(dest_addr);
    if(-1 == (recvlen = recvfrom(sock, buffer, sizeof(struct iphdr) + sizeof(struct icmphdr), 0, (struct sockaddr*)&dest_addr, &len))) {
        printf("Error receiving packets\n");
        exit(-1);
    }
    else {
        printf("Received packet\n");
        ip = (struct iphdr*) buffer;
        icmp = (struct icmphdr*) (buffer + sizeof(struct iphdr));
        printf("Packet received from %s\n", inet_ntoa(dest_addr.sin_addr));
        char * cp = (char *)&ip->saddr;
        printf("Received %d byte reply from %u.%u.%u.%u:\n", ntohs(ip->tot_len), cp[0]&0xff,cp[1]&0xff,cp[2]&0xff,cp[3]&0xff);
        printf("ID: %d\n", ntohs(ip->id));
        printf("TTL: %d\n", ip->ttl);
        printf("Packet type: %d \n", icmp->type);
    }
}
