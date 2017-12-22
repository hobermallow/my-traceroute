//
// Created by harlock on 12/22/17.
//

#ifndef TRACEROUTE_TRACEROUTE_H
#define TRACEROUTE_TRACEROUTE_H

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

#define free_list(list, type) { \
    struct type* temp; \
    while(list != NULL) { \
        temp = list->next; \
        free(list->buffer); \
        free(list); \
        list = temp; \
    } \
    list = NULL;\
}


struct packet_list {
    char* buffer;
    int size;
    struct packet_list* next;
};

struct address_list {
    char* buffer;
    struct address_list* next;
};

/**
* function to add address to list
*/

void add_address(struct address_list* lst, const char* addr);

/**
* function that returns 1 if address found, 0 if not
*/

int find_address(struct address_list*, const char*);


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



#endif //TRACEROUTE_TRACEROUTE_H
