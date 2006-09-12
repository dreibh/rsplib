#include "netutilities.h"


struct MessageBuffer
{
   char*  Buffer;
   size_t BufferSize;
   size_t BufferPos;
   size_t ExpectedMessageSize;
};


#define MBRead_Error   -1
#define MBRead_Partial -2

/**
  * Constructor.
  *
  * @param bufferSize Size of buffer to be allocated.
  * @param useEOR true to use MSG_EOR (SCTP), false otherwise (TCP, UDP).
  * @return MessageBuffer object or NULL in case of error.
  */
struct MessageBuffer* messageBufferNew(size_t bufferSize, const bool useEOR);


/**
  * Destructor.
  *
  * @param messageBuffer MessageBuffer.
  */
void messageBufferDelete(struct MessageBuffer* messageBuffer);


/**
  * Read message.
  *
  * @param messageBuffer MessageBuffer.
  * @param sockfd Socket descriptor.
  * @param flags Reference to store recvmsg() flags.
  * @param Bytes read or MBRead_Error in case of error or MBRead_Partial in case of partial read (ppid, assocID and streamID will be valid)!
  */
ssize_t messageBufferRead(struct MessageBuffer* messageBuffer,
                          int                   sockfd,
                          int*                  flags);

/**
  * Reset message buffer (set offset to 0).
  *
  * @param messageBuffer MessageBuffer.
  */
void messageBufferReset(struct MessageBuffer* messageBuffer);


/**
  * Check whether message buffer contains a message fragment.
  *
  * @param messageBuffer MessageBuffer.
  * @return true, if there is a fragment; false otherwise.
  */
bool messageBufferHasPartial(struct MessageBuffer* messageBuffer);





/* ###### Constructor #################################################### */
struct MessageBuffer* messageBufferNew(size_t bufferSize)
{
   struct MessageBuffer* messageBuffer = (struct MessageBuffer*)malloc(sizeof(struct MessageBuffer));
   if(messageBuffer) {
      messageBuffer->Buffer = (char*)malloc(bufferSize);
      if(messageBuffer->Buffer == NULL) {
         free(messageBuffer);
         return(NULL);
      }
      messageBuffer->BufferSize          = bufferSize;
      messageBuffer->BufferPos           = 0;
      messageBuffer->ExpectedMessageSize = 0;
   }
   return(messageBuffer);
}


/* ###### Destructor ##################################################### */
void messageBufferDelete(struct MessageBuffer* messageBuffer)
{
   if(messageBuffer->Buffer) {
      free(messageBuffer->Buffer);
      messageBuffer->Buffer = NULL;
   }
   messageBuffer->BufferSize          = 0;
   messageBuffer->BufferPos           = 0;
   messageBuffer->ExpectedMessageSize = 0;
   free(messageBuffer);
}


/* ###### Read message ################################################### */
ssize_t messageBufferRead(struct MessageBuffer* messageBuffer,
                          int                   sockfd,
                          char*                 buffer,
                          size_t                bufferSize,
                          int                   flags)
{
   ssize_t result;
   /* ssize_t i; */

   result = ext_recv(sockfd,
                     (char*)&messageBuffer->Buffer[messageBuffer->BufferPos],
                     messageBuffer->BufferSize - messageBuffer->BufferPos,
                     flags);
   if(result >= 0) {
      messageBuffer->BufferPos += (size_t)result;

      /* Header has not been read yet */
      if(messageBuffer->ExpectedMessageSize == 0) {
         if(messageBuffer->BufferPos >= 6) {
            messageBuffer->ExpectedMessageSize = 17;
         }
         else {
            return(MBRead_Partial);
         }
      }

      /* Header has been read already. Is the message already complete? */
      if(messageBuffer->BufferPos >= messageBuffer->ExpectedMessageSize) {
         if(messageBuffer->ExpectedMessageSize > bufferSize) {
            puts("INTERNAL ERROR: ExpectedMessageSize is greater than your buffer size!");
            exit(1);
         }
         memcpy(buffer, messageBuffer->Buffer, messageBuffer->ExpectedMessageSize);
         result = (ssize_t)messageBuffer->ExpectedMessageSize;

         /* Move data of next message */
         const size_t toMove = messageBuffer->BufferPos - messageBuffer->ExpectedMessageSize;
         for(size_t i = 0;i < toMove;i++) {
            messageBuffer->Buffer[i] = messageBuffer->Buffer[messageBuffer->ExpectedMessageSize + i];
         }
         messageBuffer->BufferPos           = toMove;
         messageBuffer->ExpectedMessageSize = 0;
         return(result);
      }
      else {
         return(MBRead_Partial);
      }
   }

   messageBuffer->ExpectedMessageSize = 0;
   result = MBRead_Error;
   return(result);
}


/* ###### Reset message buffer ########################################### */
void messageBufferReset(struct MessageBuffer* messageBuffer)
{
   messageBuffer->BufferPos = 0;
}


/* ###### Does message buffer contain a fragment? ######################## */
bool messageBufferHasPartial(struct MessageBuffer* messageBuffer)
{
   return(messageBuffer->BufferPos > 0);
}



int main(int argc, char** argv)
{
   union sockaddr_union dstAddress;
   int                  sd;
   char                 buffer[65536 + 2];

   if(string2address("127.0.0.1:19", &dstAddress) == false) {
      fprintf(stderr, "ERROR: Bad dst address <%s>\n", argv[1]);
      exit(1);
   }

   sd = ext_socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
   if(sd < 0) {
      perror("Unable to create socket");
      exit(1);
   }
   if(ext_connect(sd, (struct sockaddr*)&dstAddress, sizeof(dstAddress)) < 0) {
      perror("Unable to connect to dst");
      ext_close(sd);
      exit(1);
   }


   struct MessageBuffer* mb = messageBufferNew(65536);
   if(mb == NULL) {
      puts("ERROR: Out of memory!");
      exit(1);
   }
   for(;;) {
      ssize_t inBytes = messageBufferRead(mb, sd, (char*)&buffer, sizeof(buffer), 0);
      if(inBytes > 0) {
        buffer[inBytes] = 0x00;
        printf("IN: %d %s\n", inBytes, buffer);
      }
      else if(inBytes != MBRead_Partial) {
         break;
      }
      else puts("partial");
   }
   messageBufferDelete(mb);
}
