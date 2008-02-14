#include "rserpoolmessage.h"
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

// void testASAPError()
// {
//    beginMsg(AHT_ERROR & 0xff, 0);
//    endMsg(PPID_ASAP);
//
//    beginMsg(AHT_ERROR & 0xff, 0);
//    rserpool_tlv_header* tlv = beginTLV(ATT_OPERATION_ERROR);
//    addErrorCauseParameter("Test");
//    endTLV(tlv);
//    endMsg(PPID_ASAP);
// }


void testASAPHandleResolution()
{
   beginMsg(AHT_HANDLE_RESOLUTION & 0xff, 0);
   addGenericParameter(ATT_POOL_HANDLE, "EchoPool");
   endMsg(PPID_ASAP);


   beginMsg(AHT_HANDLE_RESOLUTION & 0xff, 0);
   addGenericParameter(ATT_POOL_HANDLE, "EchoPool");
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

      testASAPHandleResolution();

      rserpoolMessageDelete(message);
   }

   finishLogging();
}
