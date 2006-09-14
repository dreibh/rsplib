#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "debug.h"
#include "netutilities.h"
#include "timeutilities.h"
#include "simpleredblacktree.h"


class StreamNode;

class ChunkNode
{
   // ====== Public methods =================================================
   public:
   ChunkNode(const char*    buffer,
             const size_t   length,
             const uint32_t seqNumber);
   ~ChunkNode();

   // ====== Public data ====================================================
   public:
   uint32_t ChunkSeqNumber;
   size_t   Length;
   size_t   Position;
   char*    Buffer;

   // ====== Private methods ================================================
   private:
   friend class StreamNode;

   inline static ChunkNode* getChunkNode(void* nodePtr) {
       const ChunkNode* dummy = (ChunkNode*)nodePtr;
       long n = (long)nodePtr - ((long)&dummy->Node - (long)dummy);
       return((ChunkNode*)n);
   }
   static int ComparisonFunction(const void* nodePtr1, const void* nodePtr2);
   static void PrintFunction(const void* nodePtr, FILE* fh);

   // ====== Private data ===================================================
   SimpleRedBlackTreeNode Node;
};


// ###### Constructor #######################################################
ChunkNode::ChunkNode(const char*    buffer,
                     const size_t   length,
                     const uint32_t seqNumber)
{
   simpleRedBlackTreeNodeNew(&Node);
   ChunkSeqNumber = seqNumber;
   Length         = length;
   Position       = 0;
   Buffer         = new char[length];
   CHECK(Buffer != NULL);
   memcpy(Buffer, buffer, length);
}


// ###### Destructor ########################################################
ChunkNode::~ChunkNode()
{
   delete [] Buffer;
   Buffer = NULL;
   simpleRedBlackTreeNodeDelete(&Node);
}


// ###### Compare chunk nodes ###############################################
int ChunkNode::ComparisonFunction(const void* nodePtr1, const void* nodePtr2)
{
   const ChunkNode* node1 = getChunkNode((void*)nodePtr1);
   const ChunkNode* node2 = getChunkNode((void*)nodePtr2);

   uint64_t s1 = (1ULL << 63) | node1->ChunkSeqNumber;
   uint64_t s2 = (1ULL << 63) | node2->ChunkSeqNumber;

   if( ((int32_t)node1->ChunkSeqNumber < 0) &&
       ((int32_t)node2->ChunkSeqNumber >= 0) ) {
      s1 &= ~(1ULL << 63);
   }
   else if( ((int32_t)node1->ChunkSeqNumber >= 0) &&
            ((int32_t)node2->ChunkSeqNumber < 0) ) {
      s2 &= ~(1ULL << 63);
   }

   if(s1 < s2) {
      return(-1);
   }
   else if(s1 > s2) {
      return(1);
   }
   return(0);
}


// ###### Print chunk node ##################################################
void ChunkNode::PrintFunction(const void* nodePtr, FILE* fh)
{
   const ChunkNode* chunkNode = getChunkNode((void*)nodePtr);
   fprintf(fh, "$%08x [%u bytes]\n",
           chunkNode->ChunkSeqNumber, chunkNode->Length);
}


class Dissector
{
   // ====== Public methods =================================================
   public:
   Dissector();
   virtual ~Dissector();

   virtual size_t getHeaderLength(int                   protocol,
                                  const sockaddr_union* srcAddress,
                                  const sockaddr_union* dstAddress) const;
   virtual size_t getPacketLength(int                   protocol,
                                  const sockaddr_union* srcAddress,
                                  const sockaddr_union* dstAddress,
                                  const char*           data,
                                  const size_t          dataLength);

   // ====== Public data ====================================================
   static Dissector* Default;
};


Dissector gDefaultDissector;
Dissector* Dissector::Default = &gDefaultDissector;


// ###### Constructor #######################################################
Dissector::Dissector()
{
}


// ###### Destructor ########################################################
Dissector::~Dissector()
{
}


// ###### Get header length #################################################
size_t Dissector::getHeaderLength(int                   protocol,
                                  const sockaddr_union* srcAddress,
                                  const sockaddr_union* dstAddress) const
{
   return(1);
}


// ###### Get packet length #################################################
size_t Dissector::getPacketLength(int                   protocol,
                                  const sockaddr_union* srcAddress,
                                  const sockaddr_union* dstAddress,
                                  const char*           data,
                                  const size_t          dataLength)
{
   return(dataLength);
}




class TLVDissector : public Dissector
{
   // ====== Public methods =================================================
   public:
   TLVDissector();
   virtual ~TLVDissector();

   virtual size_t getHeaderLength(int                   protocol,
                                  const sockaddr_union* srcAddress,
                                  const sockaddr_union* dstAddress) const;
   virtual size_t getPacketLength(int                   protocol,
                                  const sockaddr_union* srcAddress,
                                  const sockaddr_union* dstAddress,
                                  const char*           data,
                                  const size_t          dataLength);

   // ====== Public data ====================================================
   struct TLVHeader {
      uint8_t  Type;
      uint8_t  Flags;
      uint16_t Length;
   };
};


// ###### Constructor #######################################################
TLVDissector::TLVDissector()
{
}


// ###### Destructor ########################################################
TLVDissector::~TLVDissector()
{
}


// ###### Get header length #################################################
size_t TLVDissector::getHeaderLength(int                   protocol,
                                     const sockaddr_union* srcAddress,
                                     const sockaddr_union* dstAddress) const
{
   return(sizeof(TLVHeader));
}


// ###### Get packet length #################################################
size_t TLVDissector::getPacketLength(int                   protocol,
                                     const sockaddr_union* srcAddress,
                                     const sockaddr_union* dstAddress,
                                     const char*           data,
                                     const size_t          dataLength)
{
   CHECK(dataLength >= sizeof(TLVHeader));
   const TLVHeader* tlvHeader = (const TLVHeader*)data;
   return(ntohs(tlvHeader->Length));
}




class StreamTree;

class StreamNode
{
   // ====== Public methods =================================================
   public:
   StreamNode();
   StreamNode(int                   protocol,
              const sockaddr_union* srcAddress,
              const sockaddr_union* dstAddress,
              const uint32_t        initSeq,
              Dissector*            dissector);
   ~StreamNode();

   bool handleData(const char*    buffer,
                   const size_t   length,
                   const uint32_t seqNumber);
   size_t readFromBuffer(char*        buffer,
                         const size_t bufferSize,
                         bool         peek = false);
   size_t readMessage(char*        buffer,
                      const size_t bufferSize,
                      bool         peek = false);
   inline size_t bytesAvailable(const size_t maxSize = ~0) {
      return(readFromBuffer(NULL, maxSize));
   }


   // ====== Private methods ================================================
   private:
   friend class StreamTree;

   inline static StreamNode* getStreamNodeFromFlowNode(void* flowNodePtr) {
       const StreamNode* dummy = (StreamNode*)flowNodePtr;
       long n = (long)flowNodePtr - ((long)&dummy->FlowNode - (long)dummy);
       return((StreamNode*)n);
   }
   inline static StreamNode* getStreamNodeFromTimerNode(void* timerNodePtr) {
       const StreamNode* dummy = (StreamNode*)timerNodePtr;
       long n = (long)timerNodePtr - ((long)&dummy->TimerNode - (long)dummy);
       return((StreamNode*)n);
   }
   static int FlowNodeComparisonFunction(const void* nodePtr1, const void* nodePtr2);
   static void FlowNodePrintFunction(const void* nodePtr, FILE* fh);
   static int TimerNodeComparisonFunction(const void* nodePtr1, const void* nodePtr2);
   static void TimerNodePrintFunction(const void* nodePtr, FILE* fh);

   inline ChunkNode* getFirstChunkNode() {
      SimpleRedBlackTreeNode* node = simpleRedBlackTreeGetFirst(&ChunkSet);
      if(node) {
         return(ChunkNode::getChunkNode(node));
      }
      return(NULL);
   }

   inline ChunkNode* getNextChunkNode(ChunkNode* chunkNode) {
      SimpleRedBlackTreeNode* node = simpleRedBlackTreeGetNext(&ChunkSet, &chunkNode->Node);
      if(node) {
         return(ChunkNode::getChunkNode(node));
      }
      return(NULL);
   }


   // ====== Private data ===================================================
   SimpleRedBlackTreeNode FlowNode;
   SimpleRedBlackTreeNode TimerNode;
   SimpleRedBlackTree     ChunkSet;
   Dissector*             PacketDissector;

   int                    Protocol;
   sockaddr_union         SrcAddress;
   sockaddr_union         DstAddress;
   unsigned long long     LastUpdate;
   uint32_t               SeqNumber;
};


class StreamTree
{
   // ====== Public methods =================================================
   public:
   StreamTree();
   ~StreamTree();

   void addStream(int                   protocol,
                  const sockaddr_union* srcAddress,
                  const sockaddr_union* dstAddress,
                  const uint32_t        initSeq,
                  Dissector*            dissector = Dissector::Default);
   StreamNode* findStream(int                   protocol,
                          const sockaddr_union* srcAddress,
                          const sockaddr_union* dstAddress);
   void removeStream(StreamNode* streamNode);
   void removeStream(int                   protocol,
                     const sockaddr_union* srcAddress,
                     const sockaddr_union* dstAddress);
   StreamNode* handleData(int                   protocol,
                          const sockaddr_union* srcAddress,
                          const sockaddr_union* dstAddress,
                          const char*           buffer,
                          const size_t          length,
                          const uint32_t        seqNumber);
   size_t purge(const unsigned long long oldestTimeStamp);
   void dump();

   inline StreamNode* getFirstStreamNodeFromTimerNode() {
      SimpleRedBlackTreeNode* node = simpleRedBlackTreeGetFirst(&TimerTree);
      if(node) {
         return(StreamNode::getStreamNodeFromTimerNode(node));
      }
      return(NULL);
   }

   inline StreamNode* getNextStreamNodeFromTimerNode(StreamNode* streamNode) {
      SimpleRedBlackTreeNode* node = simpleRedBlackTreeGetNext(&TimerTree, &streamNode->TimerNode);
      if(node) {
         return(StreamNode::getStreamNodeFromTimerNode(node));
      }
      return(NULL);
   }

   // ====== Private data ===================================================
   private:
   SimpleRedBlackTree FlowTree;
   SimpleRedBlackTree TimerTree;
};


// ###### Default constructor ###############################################
StreamNode::StreamNode()
{
}


// ###### Constructor #######################################################
StreamNode::StreamNode(int                   protocol,
                       const sockaddr_union* srcAddress,
                       const sockaddr_union* dstAddress,
                       const uint32_t        initSeq,
                       Dissector*            dissector)
{
   PacketDissector = dissector;
   Protocol        = protocol,
   SrcAddress      = *srcAddress;
   DstAddress      = *dstAddress;
   LastUpdate      = getMicroTime();
   SeqNumber       = initSeq;

   simpleRedBlackTreeNodeNew(&FlowNode);
   simpleRedBlackTreeNodeNew(&TimerNode);
   simpleRedBlackTreeNew(&ChunkSet, &ChunkNode::PrintFunction, &ChunkNode::ComparisonFunction);
}


// ###### Destructor ########################################################
StreamNode::~StreamNode()
{
   simpleRedBlackTreeDelete(&ChunkSet);
   simpleRedBlackTreeNodeDelete(&FlowNode);
   simpleRedBlackTreeNodeDelete(&TimerNode);
}


// ###### Compare flow nodes ################################################
int StreamNode::FlowNodeComparisonFunction(const void* nodePtr1, const void* nodePtr2)
{
   const StreamNode* node1 = getStreamNodeFromFlowNode((void*)nodePtr1);
   const StreamNode* node2 = getStreamNodeFromFlowNode((void*)nodePtr2);
   int               result;

   if(node1->Protocol < node2->Protocol) {
      return(-1);
   }
   else if(node1->Protocol > node2->Protocol) {
      return(1);
   }

   result = addresscmp(&node1->SrcAddress.sa, &node2->SrcAddress.sa, true);
   if(result != 0) {
      return(result);
   }

   result = addresscmp(&node1->DstAddress.sa, &node2->DstAddress.sa, true);
   return(result);
}


// ###### Print flow node ###################################################
void StreamNode::FlowNodePrintFunction(const void* nodePtr, FILE* fh)
{
   const StreamNode* node = getStreamNodeFromFlowNode((void*)nodePtr);
   switch(node->Protocol) {
      case IPPROTO_TCP:
        fprintf(fh, "TCP ");
       break;
      case IPPROTO_UDP:
        fprintf(fh, "UDP ");
       break;
      case IPPROTO_SCTP:
        fprintf(fh, "SCTP ");
       break;
      default:
        fprintf(fh, "Protocol #%d ", node->Protocol);
       break;
   }
   fputaddress(&node->SrcAddress.sa, true, fh);
   fprintf(fh, " <-> ");
   fputaddress(&node->DstAddress.sa, true, fh);
   fprintf(fh, " lastUpdate=%Ld\n", node->LastUpdate);

   simpleRedBlackTreePrint(&node->ChunkSet, stdout);
}


// ###### Compare timer nodes ###############################################
int StreamNode::TimerNodeComparisonFunction(const void* nodePtr1, const void* nodePtr2)
{
   const StreamNode* node1 = getStreamNodeFromTimerNode((void*)nodePtr1);
   const StreamNode* node2 = getStreamNodeFromTimerNode((void*)nodePtr2);
   if(node1->LastUpdate < node2->LastUpdate) {
      return(-1);
   }
   else if(node1->LastUpdate > node2->LastUpdate) {
      return(1);
   }

   return(StreamNode::FlowNodeComparisonFunction(&node1->FlowNode, &node2->FlowNode));
}


// ###### Print timer node ##################################################
void StreamNode::TimerNodePrintFunction(const void* nodePtr, FILE* fh)
{
   const StreamNode* node = getStreamNodeFromTimerNode((void*)nodePtr);
   StreamNode::FlowNodePrintFunction(&node->FlowNode, fh);
}


// ###### Handle incoming data ##############################################
bool StreamNode::handleData(const char*    buffer,
                            const size_t   length,
                            const uint32_t seqNumber)
{
   if(length > 0) {
      ChunkNode* chunkNode = new ChunkNode(buffer, length, seqNumber);
      if(chunkNode == NULL) {
         return(false);
      }
      if(simpleRedBlackTreeInsert(&ChunkSet, &chunkNode->Node) != &chunkNode->Node) {
         // Chunk is a duplicate!
         puts("Duplicate!");
         delete chunkNode;
         return(false);
      }
   }
   return(true);
}


// ###### Read from chunk buffer ############################################
size_t StreamNode::readFromBuffer(char*        buffer,
                                  const size_t bufferSize,
                                  bool         peek)
{
   size_t   bytesRead = 0;
   uint32_t seqNumber = SeqNumber;

   ChunkNode* chunkNode = getFirstChunkNode();
   while(chunkNode != NULL) {
      if( (bytesRead >= bufferSize) ||
          (chunkNode->ChunkSeqNumber + (uint32_t)chunkNode->Position != seqNumber) ) {
         break;
      }

      size_t chunkBytes = chunkNode->Length - chunkNode->Position;
      if(chunkBytes > bufferSize - bytesRead) {
         chunkBytes = bufferSize - bytesRead;
      }
      ChunkNode* nextChunkNode = getNextChunkNode(chunkNode);

      if(buffer) {
         memcpy(&buffer[bytesRead], &chunkNode->Buffer[chunkNode->Position], chunkBytes);
         if(!peek) {
            SeqNumber           += (uint32_t)chunkBytes;
            chunkNode->Position += chunkBytes;
            if(chunkNode->Position >= chunkNode->Length) {
               CHECK(simpleRedBlackTreeRemove(&ChunkSet, &chunkNode->Node) == &chunkNode->Node);
               delete chunkNode;
            }
         }
      }

      bytesRead += chunkBytes;
      seqNumber += (uint32_t)chunkBytes;

      chunkNode = nextChunkNode;
   }

   return(bytesRead);
}


// ###### Read from chunk buffer ############################################
size_t StreamNode::readMessage(char*        buffer,
                               const size_t bufferSize,
                               bool         peek)
{
   const size_t headerLength = PacketDissector->getHeaderLength(Protocol, &SrcAddress, &DstAddress);
   const size_t available    = bytesAvailable();
   if(available) {
      if(bufferSize < headerLength) {
         printf("ERROR: StreamNode::readMessage() - Your buffer size is too small!\n");
         return(0);
      }
      size_t bytesRead = readFromBuffer(buffer, headerLength, true);
      CHECK(bytesRead == headerLength);

      const size_t totalLength = PacketDissector->getPacketLength(Protocol, &SrcAddress, &DstAddress,
                                                                  buffer, available);
      return(readFromBuffer(buffer, totalLength, peek));
   }
   return(0);
}




// ###### Constructor #######################################################
StreamTree::StreamTree()
{
   simpleRedBlackTreeNew(&FlowTree, &StreamNode::FlowNodePrintFunction, &StreamNode::FlowNodeComparisonFunction);
   simpleRedBlackTreeNew(&TimerTree, &StreamNode::TimerNodePrintFunction, &StreamNode::TimerNodeComparisonFunction);
}


// ###### Destructor ########################################################
StreamTree::~StreamTree()
{
   simpleRedBlackTreeDelete(&FlowTree);
   simpleRedBlackTreeDelete(&TimerTree);
}


// ###### Add stream ########################################################
void StreamTree::addStream(int                   protocol,
                           const sockaddr_union* srcAddress,
                           const sockaddr_union* dstAddress,
                           const uint32_t        initSeq,
                           Dissector*            dissector)
{
   StreamNode* streamNode = new StreamNode(protocol, srcAddress, dstAddress, initSeq, dissector);
   if(streamNode) {
      CHECK(simpleRedBlackTreeInsert(&FlowTree, &streamNode->FlowNode) == &streamNode->FlowNode);
      CHECK(simpleRedBlackTreeInsert(&TimerTree, &streamNode->TimerNode) == &streamNode->TimerNode);
   }
}


// ###### Find stream #######################################################
StreamNode* StreamTree::findStream(int                   protocol,
                                   const sockaddr_union* srcAddress,
                                   const sockaddr_union* dstAddress)
{
   StreamNode cmpNode;
   cmpNode.Protocol   = protocol;
   cmpNode.SrcAddress = *srcAddress;
   cmpNode.DstAddress = *dstAddress;
   SimpleRedBlackTreeNode* node = simpleRedBlackTreeFind(&FlowTree, &cmpNode.FlowNode);
   if(node) {
      return(StreamNode::getStreamNodeFromFlowNode(node));
   }
   return(NULL);
}


// ###### Remove stream #####################################################
void StreamTree::removeStream(StreamNode* streamNode)
{
   CHECK(simpleRedBlackTreeRemove(&FlowTree, &streamNode->FlowNode) == &streamNode->FlowNode);
   CHECK(simpleRedBlackTreeRemove(&TimerTree, &streamNode->TimerNode) == &streamNode->TimerNode);
   delete streamNode;
}


// ###### Remove stream #####################################################
void StreamTree::removeStream(int                   protocol,
                              const sockaddr_union* srcAddress,
                              const sockaddr_union* dstAddress)
{
   StreamNode* streamNode = findStream(protocol, srcAddress, dstAddress);
   if(streamNode) {
      removeStream(streamNode);
   }
}

// ###### Handle incoming data ##############################################
StreamNode* StreamTree::handleData(int                   protocol,
                                   const sockaddr_union* srcAddress,
                                   const sockaddr_union* dstAddress,
                                   const char*           buffer,
                                   const size_t          length,
                                   const uint32_t        seqNumber)
{
   StreamNode* streamNode = findStream(protocol, srcAddress, dstAddress);
   if(streamNode) {
      if(streamNode->handleData(buffer, length, seqNumber) > 0) {
         CHECK(simpleRedBlackTreeRemove(&TimerTree, &streamNode->TimerNode) == &streamNode->TimerNode);
         streamNode->LastUpdate = getMicroTime();
         CHECK(simpleRedBlackTreeInsert(&TimerTree, &streamNode->TimerNode) == &streamNode->TimerNode);
         return(streamNode);
      }
   }
   return(NULL);
}


// ###### Purge old-of-date entries #########################################
size_t StreamTree::purge(const unsigned long long oldestTimeStamp)
{
   size_t purged = 0;
   StreamNode* streamNode = getFirstStreamNodeFromTimerNode();
   while(streamNode != NULL) {
      if(streamNode->LastUpdate >= oldestTimeStamp) {
         break;
      }
      StreamNode* nextStreamNode = getNextStreamNodeFromTimerNode(streamNode);
      removeStream(streamNode);
      streamNode = nextStreamNode;
      purged++;
   }
   return(purged);
}


// ###### Print stream tree #################################################
void StreamTree::dump()
{
   puts("StreamTree:");
   simpleRedBlackTreePrint(&FlowTree, stdout);
}







void test1(StreamTree* tree,
           const sockaddr_union* srcAddress,
           const sockaddr_union* dstAddress,
           int seq)
{
   char q = 65;
   uint32_t s=seq;
   size_t b=0;
   for(size_t i = 0;i < 133;i++) {
      size_t len = random() % 1500;
      char data[len];
      for(size_t j = 0;j < len;j++) { data[j] = q++; }
      tree->handleData(IPPROTO_SCTP, srcAddress, dstAddress, data, len, s);
      s += len; b+=len;
   }

   StreamNode* sn = tree->findStream(IPPROTO_SCTP, srcAddress, dstAddress);
   CHECK(sn);

   printf("Reading...\n");
   q=65;
   size_t u = 0;
   for(size_t i = 0;i <= b;i++) {
      size_t n = 3;
      char arr[n];
      size_t r = sn->readFromBuffer((char*)&arr, sizeof(arr), false);
      if(r <= 0) {
         printf("STOP at %d\n",i);
         break;
      }
//       printf("%d %d\n",(unsigned char)q,(unsigned char)c);
      for(size_t i = 0;i < r;i++) {
         if(q!=arr[i]) {
            printf("DIFF at %d\n",i);
            exit(1);
         }
         q++; u++;
      }
   }

   printf("IN: %d  OUT: %d\n",b,u);
}


void test2(StreamTree* tree,
           const sockaddr_union* srcAddress,
           const sockaddr_union* dstAddress,
           int seq)
{
   struct Packet {
      uint8_t Type;
      uint8_t Flags;
      uint16_t Length;
      char Data[];
   }* newPkt;

   uint32_t s = seq;
   for(size_t i = 0; i < 135;i++) {
      size_t l = random() % 399;
      newPkt = (Packet*)malloc(sizeof(Packet) + l);
      memset(newPkt, sizeof(Packet) + l, 0);
      newPkt->Length = htons(sizeof(Packet) + l);
      printf("write: %d  (%d)\n", sizeof(Packet) + l,l);

      tree->handleData(IPPROTO_SCTP, srcAddress, dstAddress, (char*)newPkt, sizeof(Packet) + l, s);
      tree->handleData(IPPROTO_SCTP, srcAddress, dstAddress, (char*)newPkt, sizeof(Packet) + l, s);
      s += (uint32_t)(sizeof(Packet) + l);

      free(newPkt);
   }

   StreamNode* sn = tree->findStream(IPPROTO_SCTP, srcAddress, dstAddress);
   CHECK(sn);

   char data[65536];
   size_t l;
   while( (l = sn->readMessage((char*)&data, sizeof(data), false)) > 0) {
      printf("read: %d\n", l);
   }



   puts("-----------------");
   for(size_t i = 0; i < 73;i++) {
      size_t l = random() % 399;
      newPkt = (Packet*)malloc(sizeof(Packet) + l);
      memset(newPkt, sizeof(Packet) + l, 0);
      newPkt->Length = htons(sizeof(Packet) + l);
      printf("write: %d  (%d)\n", sizeof(Packet) + l,l);

      char* ptr = (char*)newPkt;
      size_t w = sizeof(Packet) + l;
      while(w > 0) {
         size_t q = random() % 67;
         if(q > w) { q = w; }
         tree->handleData(IPPROTO_SCTP, srcAddress, dstAddress, ptr, q, s);
         ptr = (char*)&ptr[q];
         s += (uint32_t)q;
         w -= q;
      }

      free(newPkt);
   }

   while( (l = sn->readMessage((char*)&data, sizeof(data), false)) > 0) {
      printf("read: %d\n", l);
   }

}




int main(int argc, char** argv)
{
   StreamTree st;
   puts("Start!");

   unsigned int seed = (unsigned int)(getMicroTime() % 517);
   printf("seed=%u\n", seed);
   srandom(seed);

   sockaddr_union a1;
   sockaddr_union a2;
   sockaddr_union a3;
   string2address("127.0.0.1:1234",&a1);
   string2address("132.252.152.123:5000",&a2);
   string2address("1.2.3.4:9999",&a3);

   TLVDissector tlvd;
   st.addStream(IPPROTO_SCTP, &a1, &a2, 0xffff0000);
   st.addStream(IPPROTO_SCTP, &a1, &a3, 0xffff0000);
   st.addStream(IPPROTO_SCTP, &a2, &a3, 0xfffff000, &tlvd);

//    st.removeStream(IPPROTO_SCTP, &a1, &a2);

   test1(&st, &a1, &a2, 0xffff0000);
   test2(&st, &a2, &a3, 0xfffff000);

   st.dump();
   printf("PURGED=%d\n",st.purge(~0ULL));
   st.dump();
}
