/*
 * An Efficient RSerPool Pool Namespace Management Implementation
 * Copyright (C) 2004 by Thomas Dreibholz
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Contact: dreibh@exp-math.uni-essen.de
 *
 */

#ifndef POOLMANAGEMENT_H
#define POOLMANAGEMENT_H


#include "poolnamespacemanagement-basics.h"
#include "poolpolicysettings.h"
#include "transportaddressblock.h"
#include "stringutilities.h"

#include "linearlist.h"
#include "binarytree.h"
#include "leaflinkedbinarytree.h"
#include "leaflinkedtreap.h"
#include "leaflinkedredblacktree.h"


#define INTERNAL_POOLTEMPLATE


#ifdef INCLUDE_LINEARLIST
#define STN_CLASSNAME LinearListNode
#define STN_METHOD(x) linearListNode##x
#define ST_CLASSNAME LinearList
#define ST_CLASS(x) x##_LinearList
#define ST_METHOD(x) linearList##x

#include "poolpolicy-template.h"
#include "poolelementnode-template.h"
#include "poolnode-template.h"
#include "poolnamespacenode-template.h"
#include "poolnamespacemanagement-template.h"
#include "peerlistnode-template.h"
#include "peerlist-template.h"
#include "peerlistmanagement-template.h"

#ifdef INTERNAL_POOLTEMPLATE_IMPLEMENT_IT
#include "poolpolicy-template.c"
#include "poolelementnode-template.c"
#include "poolnode-template.c"
#include "poolnamespacenode-template.c"
#include "poolnamespacemanagement-template.c"
#include "peerlistnode-template.c"
#include "peerlist-template.c"
#include "peerlistmanagement-template.c"
#endif

#undef STN_CLASSNAME
#undef STN_METHOD
#undef ST_CLASSNAME
#undef ST_CLASS
#undef ST_METHOD
#endif



#ifdef INCLUDE_BINARYTREE
#define STN_CLASSNAME BinaryTreeNode
#define STN_METHOD(x) binaryTreeNode##x
#define ST_CLASSNAME BinaryTree
#define ST_CLASS(x) x##_BinaryTree
#define ST_METHOD(x) binaryTree##x

#include "poolpolicy-template.h"
#include "poolelementnode-template.h"
#include "poolnode-template.h"
#include "poolnamespacenode-template.h"
#include "poolnamespacemanagement-template.h"
#include "peerlistnode-template.h"
#include "peerlist-template.h"
#include "peerlistmanagement-template.h"

#ifdef INTERNAL_POOLTEMPLATE_IMPLEMENT_IT
#include "poolpolicy-template.c"
#include "poolelementnode-template.c"
#include "poolnode-template.c"
#include "poolnamespacenode-template.c"
#include "poolnamespacemanagement-template.c"
#include "peerlistnode-template.c"
#include "peerlist-template.c"
#include "peerlistmanagement-template.c"
#endif

#undef STN_CLASSNAME
#undef STN_METHOD
#undef ST_CLASSNAME
#undef ST_CLASS
#undef ST_METHOD
#endif




#ifdef INCLUDE_LEAFLINKEDBINARYTREE
#define STN_CLASSNAME LeafLinkedBinaryTreeNode
#define STN_METHOD(x) leafLinkedBinaryTreeNode##x
#define ST_CLASSNAME LeafLinkedBinaryTree
#define ST_CLASS(x) x##_LeafLinkedBinaryTree
#define ST_METHOD(x) leafLinkedBinaryTree##x

#include "poolpolicy-template.h"
#include "poolelementnode-template.h"
#include "poolnode-template.h"
#include "poolnamespacenode-template.h"
#include "poolnamespacemanagement-template.h"
#include "peerlistnode-template.h"
#include "peerlist-template.h"
#include "peerlistmanagement-template.h"

#ifdef INTERNAL_POOLTEMPLATE_IMPLEMENT_IT
#include "poolpolicy-template.c"
#include "poolelementnode-template.c"
#include "poolnode-template.c"
#include "poolnamespacenode-template.c"
#include "poolnamespacemanagement-template.c"
#include "peerlistnode-template.c"
#include "peerlist-template.c"
#include "peerlistmanagement-template.c"
#endif

#undef STN_CLASSNAME
#undef STN_METHOD
#undef ST_CLASSNAME
#undef ST_CLASS
#undef ST_METHOD
#endif




#ifdef INCLUDE_LEAFLINKEDTREAP
#define STN_CLASSNAME LeafLinkedTreapNode
#define STN_METHOD(x) leafLinkedTreapNode##x
#define ST_CLASSNAME LeafLinkedTreap
#define ST_CLASS(x) x##_LeafLinkedTreap
#define ST_METHOD(x) leafLinkedTreap##x

#include "poolpolicy-template.h"
#include "poolelementnode-template.h"
#include "poolnode-template.h"
#include "poolnamespacenode-template.h"
#include "poolnamespacemanagement-template.h"
#include "peerlistnode-template.h"
#include "peerlist-template.h"
#include "peerlistmanagement-template.h"

#ifdef INTERNAL_POOLTEMPLATE_IMPLEMENT_IT
#include "poolpolicy-template.c"
#include "poolelementnode-template.c"
#include "poolnode-template.c"
#include "poolnamespacenode-template.c"
#include "poolnamespacemanagement-template.c"
#include "peerlistnode-template.c"
#include "peerlist-template.c"
#include "peerlistmanagement-template.c"
#endif

#undef STN_CLASSNAME
#undef STN_METHOD
#undef ST_CLASSNAME
#undef ST_CLASS
#undef ST_METHOD
#endif




#ifdef INCLUDE_LEAFLINKEDREDBLACKTREE
#define STN_CLASSNAME LeafLinkedRedBlackTreeNode
#define STN_METHOD(x) leafLinkedRedBlackTreeNode##x
#define ST_CLASSNAME LeafLinkedRedBlackTree
#define ST_CLASS(x) x##_LeafLinkedRedBlackTree
#define ST_METHOD(x) leafLinkedRedBlackTree##x

#include "poolpolicy-template.h"
#include "poolelementnode-template.h"
#include "poolnode-template.h"
#include "poolnamespacenode-template.h"
#include "poolnamespacemanagement-template.h"
#include "peerlistnode-template.h"
#include "peerlist-template.h"
#include "peerlistmanagement-template.h"

#ifdef INTERNAL_POOLTEMPLATE_IMPLEMENT_IT
#include "poolpolicy-template.c"
#include "poolelementnode-template.c"
#include "poolnode-template.c"
#include "poolnamespacenode-template.c"
#include "poolnamespacemanagement-template.c"
#include "peerlistnode-template.c"
#include "peerlist-template.c"
#include "peerlistmanagement-template.c"
#endif

#undef STN_CLASSNAME
#undef STN_METHOD
#undef ST_CLASSNAME
#undef ST_CLASS
#undef ST_METHOD
#endif




#define TMPL_CLASS(x, c) x##_##c
#define TMPL_METHOD(x, c) c##x


#ifdef USE_TREAP
#define STN_CLASSNAME LeafLinkedTreapNode
#define STN_METHOD(x) leafLinkedTreapNode##x
#define ST_CLASSNAME LeafLinkedTreap
#define ST_CLASS(x) x##_LeafLinkedTreap
#define ST_METHOD(x) leafLinkedTreap##x
#endif
#ifdef USE_LEAFLINKEDREDBLACKTREE
#define STN_CLASSNAME LeafLinkedRedBlackTreeNode
#define STN_METHOD(x) leafLinkedRedBlackTreeNode##x
#define ST_CLASSNAME LeafLinkedRedBlackTree
#define ST_CLASS(x) x##_LeafLinkedRedBlackTree
#define ST_METHOD(x) leafLinkedRedBlackTree##x
#endif
#ifdef USE_LEAFLINKEDBINARYTREE
#define STN_CLASSNAME LeafLinkedBinaryTreeNode
#define STN_METHOD(x) leafLinkedBinaryTreeNode##x
#define ST_CLASSNAME LeafLinkedBinaryTree
#define ST_CLASS(x) x##_LeafLinkedBinaryTree
#define ST_METHOD(x) leafLinkedBinaryTree##x
#endif
#ifdef USE_BINARYTREE
#define STN_CLASSNAME BinaryTreeNode
#define STN_METHOD(x) binaryTreeNode##x
#define ST_CLASSNAME BinaryTree
#define ST_CLASS(x) x##_BinaryTree
#define ST_METHOD(x) binaryTree##x
#endif
#ifdef USE_LIST
#define STN_CLASSNAME LinearListNode
#define STN_METHOD(x) linearListNode##x
#define ST_CLASSNAME LinearList
#define ST_CLASS(x) x##_LinearList
#define ST_METHOD(x) linearList##x
#endif


#ifdef __cplusplus
extern "C" {
#endif


unsigned int poolPolicyGetPoolPolicyTypeByName(const char* policyName);
const char* poolPolicyGetPoolPolicyNameByType(const unsigned int policyType);


#ifdef __cplusplus
}
#endif


#undef INTERNAL_POOLTEMPLATE

#endif
