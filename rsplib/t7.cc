#include "rserpoolmessage.h"
#include "netutilities.h"
#include "loglevel.h"


struct RSerPoolMessage* message;
size_t                  pos;



static char* get(size_t length)
{
   char* ptr = (char*)&message->Buffer[pos];
   pos += length;
   return(ptr);
}


static rserpool_tlv_header* beginTLV(uint16_t type)
{
   rserpool_tlv_header* tlv = (rserpool_tlv_header*)get(sizeof(rserpool_tlv_header));
   tlv->atlv_type = htons(type);
   return(tlv);
}


static void endTLV(rserpool_tlv_header* tlv)
{
   uint16_t len = (uint16_t)((unsigned long)&message->Buffer[pos] - (unsigned long)tlv);
   tlv->atlv_length = htons(len);
}


static char* beginMsg(uint8_t type, size_t customLength)
{
   rserpoolMessageClearAll(message);
   pos = 0;

   rserpool_header* h = (rserpool_header*)get(sizeof(rserpool_header) + customLength);
   printf("##### New message, type $%02x #####\n", type);
   h->ah_type  = type;
   h->ah_flags = 0x00;
   return( (char*)&message->Buffer[sizeof(rserpool_header)] );
}


static void endMsg(uint32_t ppid)
{
   rserpool_header* h = (rserpool_header*)&message->Buffer[0];
   h->ah_length = htons(pos);

   message->BufferSize = pos;


   fprintf(stdout, "Incoming message (%u bytes):\n", (unsigned int)message->BufferSize);
   size_t i, j;
   for(i = 0;i < message->BufferSize;i += 32) {
      fprintf(stdout, "%4u: ", (unsigned int)i);
      for(j = 0;j < 32;j++) {
         if(i + j < message->BufferSize) {
            fprintf(stdout, "%02x", (int)((unsigned char)message->Buffer[i + j]));
         }
         else {
            fputs("  ", stdout);
         }
         if(((j + 1) % 4) == 0) fputs(" ", stdout);
      }
      for(j = 0;j < 32;j++) {
         if(i + j < message->BufferSize) {
            if(isprint(message->Buffer[i + j])) {
               fprintf(stdout, "%c", message->Buffer[i + j]);
            }
            else {
               fputs(".", stdout);
            }
         }
         else {
            fputs("  ", stdout);
         }
      }
      fputs("\n", stdout);
   }

   RSerPoolMessage* m2 = NULL;
   unsigned int result = rserpoolPacket2Message(message->Buffer, NULL, 0, ppid, message->BufferSize, message->BufferSize, &m2);
   CHECK(result == RSPERR_OKAY);
   printf("error=%x\n", m2->Error);
   if(m2) {
      rserpoolMessageDelete(m2);
   }
}



void addGenericParameter(uint16_t type, char* data)
{
   rserpool_tlv_header* tlv = beginTLV(type);
   size_t l = strlen(data);
   if(l > 0) {
      char* s = get(l);
      memcpy(s, data, l);
   }
   endTLV(tlv);
}


/*
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |         Type = 0xa            |       Length=variable         |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                         PE Identifier                         |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                  Home ENRP Server Identifier                  |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                      Registration Life                        |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   :                      User Transport param                     :
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   :                 Member Selection Policy param                 :
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   :                      ASAP Transport param                     :
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/

void addRoundRobinParameter()
{
   rserpool_tlv_header* tlv = beginTLV(ATT_POOL_POLICY);
   rserpool_policy_roundrobin* rr = (rserpool_policy_roundrobin*)get(sizeof(rserpool_policy_roundrobin));
   rr->pp_rr_policy = PPT_ROUNDROBIN;;
   endTLV(tlv);
}

void addAddressParameter(char* addr)
{
   sockaddr_union a;
   CHECK(string2address(addr, &a));

   if(a.sa.sa_family == AF_INET) {
      rserpool_tlv_header* tlv = beginTLV(ATT_IPv4_ADDRESS);
      char* buf = get(4);
      memcpy(buf, &a.in.sin_addr.s_addr, 4);
      endTLV(tlv);
   }
   else if(a.sa.sa_family == AF_INET6) {
      rserpool_tlv_header* tlv = beginTLV(ATT_IPv6_ADDRESS);
      char* buf = get(16);
      memcpy(buf, &a.in6.sin6_addr.s6_addr32, 16);
      endTLV(tlv);
   }
   else { CHECK(false); }
}

void addSCTPTransportParameter(uint16_t port, uint16_t use, char* a1, char* a2, char* a3)
{
   rserpool_tlv_header* tlv = beginTLV(ATT_SCTP_TRANSPORT);
   rserpool_sctptransportparameter* sctptp = (rserpool_sctptransportparameter*)get(sizeof(rserpool_sctptransportparameter));
   sctptp->stp_port          = htons(port);
   sctptp->stp_transport_use = htons(use);
   if(a1) addAddressParameter(a1);
   if(a2) addAddressParameter(a2);
   if(a3) addAddressParameter(a3);
   endTLV(tlv);
}

void addPoolElementParameter(uint32_t peId, uint32_t homePRID)
{
   rserpool_tlv_header* tlv = beginTLV(ATT_POOL_ELEMENT);
   rserpool_poolelementparameter* pep = (rserpool_poolelementparameter*)get(sizeof(rserpool_poolelementparameter));
   pep->pep_identifier   = htonl(peId);
   pep->pep_homeserverid = htonl(homePRID);
   pep->pep_reg_life     = htonl(300000);

   addSCTPTransportParameter(1234, UTP_DATA_PLUS_CONTROL, "127.0.0.1:1234", NULL, NULL);
   addRoundRobinParameter();
//    addSCTPTransportParameter(1234, UTP_DATA_ONLY, "127.0.0.1:9999", NULL, NULL);

   endTLV(tlv);
}




void testASAPHandleResolutionResponse()
{
   beginMsg(AHT_HANDLE_RESOLUTION_RESPONSE & 0xff, 0);
   addGenericParameter(ATT_POOL_HANDLE, "EchoPool");
   addPoolElementParameter(0x1234,0x9999999);
   endMsg(PPID_ASAP);
}


void testASAPHandleResolution()
{
   beginMsg(AHT_HANDLE_RESOLUTION & 0xff, 0);
   addGenericParameter(ATT_POOL_HANDLE, "EchoPool");
   endMsg(PPID_ASAP);


   beginMsg(AHT_HANDLE_RESOLUTION & 0xff, 0);
   addGenericParameter(ATT_POOL_HANDLE, "EchoPool");
   addRoundRobinParameter();
   addGenericParameter(1234, "TEST");
   endMsg(PPID_ASAP);
}


int main(int argc, char** argv)
{
   for(int i = 1;i < argc;i++) {
      initLogging(argv[i]);
   }

   beginLogging();

   message = rserpoolMessageNew(NULL, 65536);
   if(message) {

//       testASAPHandleResolution();
      testASAPHandleResolutionResponse();

      rserpoolMessageDelete(message);
   }

   finishLogging();
}
