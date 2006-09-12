#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "netutilities.h"
#include "timeutilities.h"
#include "simpleredblacktree.h"


class StreamTree;


class StreamNode
{
   public:
   StreamNode();
   ~StreamNode();

   inline void initialize(int                   protocol,
                          const sockaddr_union* srcAddress,
                          const sockaddr_union* dstAddress) {
      Protocol   = protocol,
      SrcAddress = *srcAddress;
      DstAddress = *dstAddress;
      LastUpdate = getMicroTime();
   }


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
   static int FlowNodeComparisonFunction(const void* node1, const void* node2);
   static void FlowNodePrintFunction(const void* node, FILE* fh);
   static int TimerNodeComparisonFunction(const void* node1, const void* node2);
   static void TimerNodePrintFunction(const void* node, FILE* fh);

   SimpleRedBlackTreeNode FlowNode;
   SimpleRedBlackTreeNode TimerNode;
   StreamTree*            Parent;

   int                    Protocol;
   sockaddr_union         SrcAddress;
   sockaddr_union         DstAddress;
   unsigned long long     LastUpdate;
};


class StreamTree
{
   public:
   StreamTree();
   ~StreamTree();

   private:
   SimpleRedBlackTree FlowTree;
   SimpleRedBlackTree TimerTree;
};



StreamNode::StreamNode()
{
   simpleRedBlackTreeNodeNew(&FlowNode);
   simpleRedBlackTreeNodeNew(&TimerNode);
}


StreamNode::~StreamNode()
{
   simpleRedBlackTreeNodeDelete(&FlowNode);
   simpleRedBlackTreeNodeDelete(&TimerNode);
}


int StreamNode::FlowNodeComparisonFunction(const void* nodePtr1, const void* nodePtr2)
{
   const StreamNode* node1 = getStreamNodeFromFlowNode((void*)nodePtr1);
   const StreamNode* node2 = getStreamNodeFromFlowNode((void*)nodePtr2);
   return(addresscmp(&node1->SrcAddress.sa, &node2->SrcAddress.sa, true));
}

void StreamNode::FlowNodePrintFunction(const void* nodePtr, FILE* fh)
{
   const StreamNode* node = getStreamNodeFromFlowNode((void*)nodePtr);
   switch(node->Protocol) {
      case IPPROTO_TCP:
        printf("TCP ");
       break;
      case IPPROTO_UDP:
        printf("UDP ");
       break;
      case IPPROTO_SCTP:
        printf("SCTP ");
       break;
      default:
        printf("Protocol #%d ", node->Protocol);
       break;
   }
   fputaddress(&node->SrcAddress.sa, true, fh);
   printf(" <-> ");
   fputaddress(&node->DstAddress.sa, true, fh);
}


int StreamNode::TimerNodeComparisonFunction(const void* node1, const void* node2)
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


void StreamNode::TimerNodePrintFunction(const void* node, FILE* fh)
{
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



int main(int argc, char** argv)
{
   StreamTree st;

   puts("Start!");
}
