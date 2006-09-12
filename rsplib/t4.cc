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
   public:
   ChunkNode(const char*    buffer,
             const size_t   length,
             const uint32_t seqNumber);
   ~ChunkNode();

   public:
   uint32_t ChunkSeqNumber;
   size_t   Length;
   size_t   Position;
   char*    Buffer;

   private:
   friend class StreamNode;

   inline static ChunkNode* getChunkNode(void* nodePtr) {
       const ChunkNode* dummy = (ChunkNode*)nodePtr;
       long n = (long)nodePtr - ((long)&dummy->Node - (long)dummy);
       return((ChunkNode*)n);
   }
   static int ComparisonFunction(const void* nodePtr1, const void* nodePtr2);
   static void PrintFunction(const void* nodePtr, FILE* fh);

   SimpleRedBlackTreeNode Node;
};


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

ChunkNode::~ChunkNode()
{
   delete Buffer;
   Buffer = NULL;
   simpleRedBlackTreeNodeDelete(&Node);
}

int ChunkNode::ComparisonFunction(const void* nodePtr1, const void* nodePtr2)
{
   const ChunkNode* node1 = getChunkNode((void*)nodePtr1);
   const ChunkNode* node2 = getChunkNode((void*)nodePtr2);

   if( (((int32_t)node1->ChunkSeqNumber < 0) &&
         ((int32_t)node2->ChunkSeqNumber >= 0)) ||
       (((int32_t)node1->ChunkSeqNumber >= 0) &&
         ((int32_t)node2->ChunkSeqNumber < 0)) ) {
     puts("couter wrap!");
exit(1);

   }

   uint64_t s1 = node1->ChunkSeqNumber;
   uint64_t s2 = node2->ChunkSeqNumber;


   if(s1 < s2) {
      return(-1);
   }
   else if(s1 > s2) {
      return(1);
   }
   return(0);
}

void ChunkNode::PrintFunction(const void* nodePtr, FILE* fh)
{
   const ChunkNode* chunkNode = getChunkNode((void*)nodePtr);
   fprintf(fh, "$%08x [%u bytes]\n",
           chunkNode->ChunkSeqNumber, chunkNode->Length);
}





class StreamTree;

class StreamNode
{
   public:
   StreamNode();
   StreamNode(int                   protocol,
              const sockaddr_union* srcAddress,
              const sockaddr_union* dstAddress,
              const uint32_t        initSeq);
   ~StreamNode();

   size_t handleData(const char*    buffer,
                     const size_t   length,
                     const uint32_t seqNumber);
   size_t bytesAvailable();

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


   SimpleRedBlackTreeNode FlowNode;
   SimpleRedBlackTreeNode TimerNode;
   SimpleRedBlackTree     ChunkSet;

   int                    Protocol;
   sockaddr_union         SrcAddress;
   sockaddr_union         DstAddress;
   unsigned long long     LastUpdate;
   uint32_t               SeqNumber;
};


class StreamTree
{
   public:
   StreamTree();
   ~StreamTree();

   void addStream(int                   protocol,
                  const sockaddr_union* srcAddress,
                  const sockaddr_union* dstAddress,
                  const uint32_t        initSeq);
   StreamNode* findStream(int                   protocol,
                          const sockaddr_union* srcAddress,
                          const sockaddr_union* dstAddress);
   void removeStream(int                   protocol,
                     const sockaddr_union* srcAddress,
                     const sockaddr_union* dstAddress);
   size_t handleData(int                   protocol,
                     const sockaddr_union* srcAddress,
                     const sockaddr_union* dstAddress,
                     const char*           buffer,
                     const size_t          length,
                     const uint32_t        seqNumber);
   void dump();

   private:
   SimpleRedBlackTree FlowTree;
   SimpleRedBlackTree TimerTree;
};


StreamNode::StreamNode()
{
}

StreamNode::StreamNode(int                   protocol,
                       const sockaddr_union* srcAddress,
                       const sockaddr_union* dstAddress,
                       const uint32_t        initSeq)
{
   Protocol   = protocol,
   SrcAddress = *srcAddress;
   DstAddress = *dstAddress;
   LastUpdate = getMicroTime();
   SeqNumber  = initSeq;

   simpleRedBlackTreeNodeNew(&FlowNode);
   simpleRedBlackTreeNodeNew(&TimerNode);
   simpleRedBlackTreeNew(&ChunkSet, &ChunkNode::PrintFunction, &ChunkNode::ComparisonFunction);
}


StreamNode::~StreamNode()
{
   simpleRedBlackTreeDelete(&ChunkSet);
   simpleRedBlackTreeNodeDelete(&FlowNode);
   simpleRedBlackTreeNodeDelete(&TimerNode);
}


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


void StreamNode::TimerNodePrintFunction(const void* nodePtr, FILE* fh)
{
   const StreamNode* node = getStreamNodeFromTimerNode((void*)nodePtr);
   StreamNode::FlowNodePrintFunction(&node->FlowNode, fh);
}


size_t StreamNode::handleData(const char*    buffer,
                              const size_t   length,
                              const uint32_t seqNumber)
{
   ChunkNode* chunkNode = new ChunkNode(buffer, length, seqNumber);
   if(chunkNode == NULL) {
      return(0);
   }

   if(simpleRedBlackTreeInsert(&ChunkSet, &chunkNode->Node) != &chunkNode->Node) {
      // Chunk is a duplicate!
      puts("Duplicate!");
      delete chunkNode;
   }

puts("OK!");
   return(bytesAvailable());
}


size_t StreamNode::bytesAvailable()
{
   size_t bytes = 0;

   ChunkNode* chunkNode = getFirstChunkNode();
   while(chunkNode != NULL) {

puts("xxx");
      chunkNode = getNextChunkNode(chunkNode);
   }

   return(bytes);
}




StreamTree::StreamTree()
{
   simpleRedBlackTreeNew(&FlowTree, &StreamNode::FlowNodePrintFunction, &StreamNode::FlowNodeComparisonFunction);
   simpleRedBlackTreeNew(&TimerTree, &StreamNode::TimerNodePrintFunction, &StreamNode::TimerNodeComparisonFunction);
}

StreamTree::~StreamTree()
{
   simpleRedBlackTreeDelete(&FlowTree);
   simpleRedBlackTreeDelete(&TimerTree);
}


void StreamTree::addStream(int                   protocol,
                           const sockaddr_union* srcAddress,
                           const sockaddr_union* dstAddress,
                           const uint32_t        initSeq)
{
   StreamNode* streamNode = new StreamNode(protocol, srcAddress, dstAddress, initSeq);
   if(streamNode) {
      CHECK(simpleRedBlackTreeInsert(&FlowTree, &streamNode->FlowNode) == &streamNode->FlowNode);
      CHECK(simpleRedBlackTreeInsert(&TimerTree, &streamNode->TimerNode) == &streamNode->TimerNode);
   }
}


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


void StreamTree::removeStream(int                   protocol,
                              const sockaddr_union* srcAddress,
                              const sockaddr_union* dstAddress)
{
   StreamNode* streamNode = findStream(protocol, srcAddress, dstAddress);
   if(streamNode) {
      CHECK(simpleRedBlackTreeRemove(&FlowTree, &streamNode->FlowNode) == &streamNode->FlowNode);
      CHECK(simpleRedBlackTreeRemove(&TimerTree, &streamNode->TimerNode) == &streamNode->TimerNode);
      delete streamNode;
   }
}


size_t StreamTree::handleData(int                   protocol,
                              const sockaddr_union* srcAddress,
                              const sockaddr_union* dstAddress,
                              const char*           buffer,
                              const size_t          length,
                              const uint32_t        seqNumber)
{
   StreamNode* streamNode = findStream(protocol, srcAddress, dstAddress);
   if(streamNode) {
      return(streamNode->handleData(buffer, length, seqNumber));
   }
   return(0);
}


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
   char q = 0;
   uint32_t s=seq;
   for(size_t i = 0;i < 3;i++) {
      size_t len = random() % 1500;
      char data[len];
      for(size_t j = 0;j < len;j++) data[j++] = q++;
      tree->handleData(IPPROTO_SCTP, srcAddress, dstAddress, data, len, s);
      s += len;
   }
}


int main(int argc, char** argv)
{
   StreamTree st;
   puts("Start!");

   sockaddr_union a1;
   sockaddr_union a2;
   sockaddr_union a3;
   string2address("127.0.0.1:1234",&a1);
   string2address("132.252.152.123:5000",&a2);
   string2address("1.2.3.4:9999",&a3);

   st.addStream(IPPROTO_SCTP, &a1, &a2, 0xffff0000);
   st.addStream(IPPROTO_SCTP, &a1, &a3, 1234);
   st.addStream(IPPROTO_SCTP, &a2, &a3, 1234);

//    st.removeStream(IPPROTO_SCTP, &a1, &a2);

   test1(&st, &a1, &a2, 0xffff0000);

   st.dump();


}
