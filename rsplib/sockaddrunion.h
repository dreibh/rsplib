// ?????????????????????
#ifndef SOCKADDRUNION_H
#define SOCKADDRUNION_H

#include <sys/socket.h>
#include <netinet/in.h>


#ifdef HAVE_TEST

#define AF_TEST AF_MAX

struct sockaddr_testaddr {
   struct sockaddr _sockaddr;
   unsigned int     ta_addr;
   uint16_t         ta_port;
   double           ta_pos_x;
   double           ta_pos_y;
   double           ta_pos_z;
};

#define ta_family _sockaddr.sa_family

#endif


union sockaddr_union {
   struct sockaddr          sa;
   struct sockaddr_in       in;
   struct sockaddr_in6      in6;
#ifdef HAVE_TEST
   struct sockaddr_testaddr ta;
#endif
};


#endif
