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
#include <fcntl.h>
#include <errno.h>

#define PACKET_SIZE sizeof(struct iphdr) + sizeof(struct icmphdr)

int sock;
int TTL = 1;
int flag_dst_reached = 0;


struct packet_list {
    char* packet;
    int size;
    struct packet_list* next;
};

struct address_list {
    char* address;
    struct address_list* next;
};


struct address_list* hstlst = NULL;
struct packet_list* pktlst = NULL;

/**
* function to free packet_list
*/

void free_list(struct packet_list*);

/**
* function to display packet list content
*/
void display_packet_list();

/**
* function to display buffer content
*/
void display_packet_content(void* buffer);

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

/**
 * delay function
 */
void delay();



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
    printf("Ip address %s\n", inet_ntoa((*((struct in_addr**)host->h_addr_list)[0])));//


    while(1) {
        ping(host);
        delay();
        trace(host);
        free_list(pktlst);
        pktlst = NULL;
        TTL++;
        if(flag_dst_reached == 1)
            break;
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

    const int val = TTL;
    if(-1 == setsockopt(sock, IPPROTO_IP, IP_TTL, &val, sizeof(val) )) {
        perror("Error setting TTL\n");
        exit(-1);
    }

    //set non blocking socket
    fcntl(sock, F_SETFL, O_NONBLOCK);
    //sending 10 packets
    for(int i = 0; i < 10; i++) {
        if(-1 == sendto(sock, icmp, sizeof(struct icmphdr), 0, (struct sockaddr*)&dest_addr, sizeof(dest_addr))) {
            perror("Error sending packet\n");
            exit(-1);
        }
        //printf("Sent %ld bytes\n", sizeof(struct iphdr) + sizeof(struct icmphdr));
        //printf("Packet type: %d\n", icmp->type);
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
    int received_ans = 0;
    struct packet_list* packet;

    buffer = malloc(sizeof(struct iphdr) + sizeof(struct icmphdr));
     //populating socket address
    //dest_addr.sin_port = 0;
    //dest_addr.sin_family = AF_INET;
    //dest_addr.sin_addr.s_addr = (*((struct in_addr**)host->h_addr_list)[0]).s_addr;

    //allocating packet list
    socklen_t len = sizeof(dest_addr);
    while(1) {
        recvlen = recvfrom(sock, buffer, sizeof(struct iphdr) + sizeof(struct icmphdr), 0, (struct sockaddr*)&dest_addr, &len);
        if(-1 == recvlen) {
            if( errno == EAGAIN || errno == EWOULDBLOCK)
                break;
            else {
                perror("Error receiving messages\n");
                exit(-1);
            }
        }
        else if(recvlen == 0) {
            printf("No more packets\n");
            break;
        }
        else {
            printf("Received packet\n");
            received_ans++;
            //printf("Packet size %d received packets %d\n", (PACKET_SIZE), received_ans);
            //allocating new packet
            packet = calloc(1, sizeof(struct packet_list));
            packet->packet = calloc(1, recvlen);
            packet->size = recvlen;
            //printf("Size to write %d\n", (PACKET_SIZE)*received_ans);
            //printf("Packet list pointer %p\n", packet_list);
            //printf("Point to write %p\n", packet_list+(received_ans-1)*(PACKET_SIZE));
            memcpy(packet->packet, buffer, recvlen);
            packet->next = pktlst;
            pktlst = packet;
        }
    }
    display_packet_list();
}

void delay() {
    for ( int c = 1 ; c < 1999967295 ; c++ )
        {}
}

void display_packet_list() {
    if(pktlst != NULL) {
        do {
            display_packet_content(pktlst->packet);
            pktlst = pktlst->next;
        }
        while (pktlst->next != NULL);
    }
    printf("\n");
}

void display_packet_content(void* buffer) {
    struct iphdr* ip;
    struct icmphdr *icmp;
    struct in_addr addr;



    ip = (struct iphdr*) buffer;
    icmp = buffer+ip->ihl*4;
    addr.s_addr = ip->saddr;

    //printf("IPv%d: hdr-size=%d pkt-size=%d protocol=%d TTL=%d ID=%d src=%s\n",
		//ip->version, ip->ihl*4, ntohs(ip->tot_len), ip->protocol,
		//ip->ttl, ip->id, inet_ntoa(addr));
	printf("src=%s ", inet_ntoa(addr));
    //printf("ICMP: type[%d/%d] checksum[%d] id[%d] seq[%d]\n",
		//	icmp->type, icmp->code, ntohs(icmp->checksum),
			//icmp->un.echo.id, icmp->un.echo.sequence);
    if(icmp->type == 0) {
        flag_dst_reached = 1;
    }
}

void free_list(struct packet_list* pktlst) {
    struct packet_list* temp;
    while(pktlst != NULL) {
        temp = pktlst->next;
        free(pktlst->packet);
        free(pktlst);
        pktlst = temp;
    }
}
