#if 0
/* ###### Convert mixed IPv4/IPv6 addresses to IPv6-format ############### */
static struct sockaddr_in6* convertToIPv6(const struct sockaddr* addrs,
                                          const int              addrcnt)
{
   union sockaddr_union* addrArray = (union sockaddr_union*)addrs;
   struct sockaddr_in6*  newAddressArray;
   int                   i;

   newAddressArray = (struct sockaddr_in6*)malloc(sizeof(struct sockaddr_in6) * addrcnt);
   if(newAddressArray == NULL) {
      errno = ENOMEM;
      return(NULL);
   }

   for(i = 0;i < addrcnt;i++) {
      switch(addrArray->sa.sa_family) {
         case AF_INET:
            newAddressArray[i].sin6_family            = AF_INET6;
            newAddressArray[i].sin6_flowinfo          = 0;
            newAddressArray[i].sin6_port              = addrArray->in.sin_port;
            newAddressArray[i].sin6_addr.s6_addr32[0] = 0x0000;
            newAddressArray[i].sin6_addr.s6_addr32[1] = 0x0000;
            newAddressArray[i].sin6_addr.s6_addr32[2] = htonl(0x0000ffff);
            newAddressArray[i].sin6_addr.s6_addr32[3] = addrArray->in.sin_addr.s_addr;
#ifdef HAVE_SOCKLEN_T
            newAddressArray[i].sin6_socklen           = sizeof(struct sockaddr_in6);
#endif
            addrArray = (union sockaddr_union*)((long)addrArray + (long)sizeof(struct sockaddr_in));
          break;
         case AF_INET6:
            memcpy((void*)&newAddressArray[i], addrArray, sizeof(struct sockaddr_in6));
            addrArray = (union sockaddr_union*)((long)addrArray + (long)sizeof(struct sockaddr_in6));
          break;
         default:
            free(newAddressArray);
            errno = EPROTOTYPE;
            return(NULL);
          break;
      }
   }

   for(i = 0;i < addrcnt;i++) {
      printf("A%d=",i);
      fputaddress((struct sockaddr*)&newAddressArray[i], true, stdout);
      puts("");
   }

   return(newAddressArray);
}
#endif


/* ###### Wrapper for sctp_bindx() ######################################## */
static int my_sctp_bindx(int              sockfd,
                         struct sockaddr* addrs,
                         int              addrcnt,
                         int              flags)
{
#if 0
#warning Using Solaris-compatible sctp_bindx()!
   union sockaddr_union* addrArray = (union sockaddr_union*)addrs;
   struct sockaddr_in6*  newAddressArray;
   int                   result;

   result = sctp_bindx(sockfd, addrs, addrcnt, flags);
   return(result);
#else
   return(sctp_bindx(sockfd, addrs, addrcnt, flags));
#endif
}


/* ###### Wrapper for sctp_connectx() #################################### */
static int my_sctp_connectx(int              sockfd,
                            struct sockaddr* addrs,
                            int              addrcnt,
                            sctp_assoc_t*    id)
{
#if 0
#warning Using Solaris-compatible sctp_connectx()!
   struct sockaddr_in6*  newAddressArray;
   int                   result;

result=-1;
//   result = sctp_connectx(sockfd, addrs, addrcnt);
//   printf("1: sctp_connectx() %d errno=%d   inprogress=%d\n",result,errno,EINPROGRESS);
//   if((result < 0) && (errno == EPROTOTYPE)) {

      newAddressArray = convertToIPv6(addrs, addrcnt);
      if(newAddressArray) {
         result = sctp_connectx(sockfd, (struct sockaddr*)newAddressArray, addrcnt);
         printf("2: sctp_connectx() %d errno=%d   inprogress=%d\n",result,errno,EINPROGRESS);
         free(newAddressArray);
      }
      else {
         return(-1);
      }
//   }

   return(result);
#else
#ifdef HAVE_CONNECTX_WITH_ID
   int result = sctp_connectx(sockfd, addrs, addrcnt, id);
#else
   if(id) {
      *id = 0;
   }
   int result = sctp_connectx(sockfd, addrs, addrcnt);
#endif
   return(result);
#endif
}
