/**
* A simple traceroute implementation
*/
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include "traceroute.h"



extern int sock;
extern int TTL;
extern int flag_dst_reached;



extern struct address_list* hstlst;
extern struct packet_list* pktlst;




int main(int argc, char* argv[]) {

    struct hostent* host;
    uint32_t addr;

    TTL = 1;
    flag_dst_reached = 0;
    hstlst = NULL;
    pktlst = NULL;

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
        free_list(pktlst, packet_list);
        free_list(hstlst, address_list);
        TTL++;
        if(flag_dst_reached == 1)
            break;
    }



}





