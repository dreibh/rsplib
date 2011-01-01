/* $Id$
 * --------------------------------------------------------------------------
 *
 *              //===//   //=====   //===//   //=====  //   //      //
 *             //    //  //        //    //  //       //   //=/  /=//
 *            //===//   //=====   //===//   //====   //   //  //  //
 *           //   \\         //  //             //  //   //  //  //
 *          //     \\  =====//  //        =====//  //   //      //  Version V
 *
 * ------------- An Open Source RSerPool Simulation for OMNeT++ -------------
 *
 * Copyright (C) 2003-2011 by Thomas Dreibholz
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Contact: dreibh@iem.uni-due.de
 */

#ifndef POOLNAMESPACEMANAGEMENT_H
#define POOLNAMESPACEMANAGEMENT_H


#include "config.h"
#include "debug.h"
#include "rserpoolerror.h"
#include "poolhandlespacechecksum.h"
#include "poolhandlespacemanagement-basics.h"
#include "poolhandle.h"
#include "poolpolicysettings.h"
#include "transportaddressblock.h"
#include "stringutilities.h"

#include <math.h>


#ifdef INCLUDE_LINEARLIST
#include "linearlist.h"
#endif
#ifdef INCLUDE_SIMPLEBINARYTREE
#include "simplebinarytree.h"
#endif
#ifdef INCLUDE_LEAFLINKEDBINARYTREE
#include "leaflinkedbinarytree.h"
#endif
#ifdef INCLUDE_SIMPLETREAP
#include "simpletreap.h"
#endif
#ifdef INCLUDE_LEAFLINKEDTREAP
#include "leaflinkedtreap.h"
#endif
#ifdef INCLUDE_SIMPLEREDBLACKTREE
#include "simpleredblacktree.h"
#endif
#ifdef INCLUDE_LEAFLINKEDREDBLACKTREE
#include "leaflinkedredblacktree.h"
#endif


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
#include "poolhandlespacenode-template.h"
#include "poolhandlespacemanagement-template.h"
#include "peerlistnode-template.h"
#include "peerlist-template.h"
#include "peerlistmanagement-template.h"
#include "poolusernode-template.h"
#include "pooluserlist-template.h"

#ifdef INTERNAL_POOLTEMPLATE_IMPLEMENT_IT
#include "poolpolicy-template_impl.h"
#include "poolelementnode-template_impl.h"
#include "poolnode-template_impl.h"
#include "poolhandlespacenode-template_impl.h"
#include "poolhandlespacemanagement-template_impl.h"
#include "peerlistnode-template_impl.h"
#include "peerlist-template_impl.h"
#include "peerlistmanagement-template_impl.h"
#include "poolusernode-template_impl.h"
#include "pooluserlist-template_impl.h"
#endif

#undef STN_CLASSNAME
#undef STN_METHOD
#undef ST_CLASSNAME
#undef ST_CLASS
#undef ST_METHOD
#endif



#ifdef INCLUDE_SIMPLEBINARYTREE
#define STN_CLASSNAME SimpleBinaryTreeNode
#define STN_METHOD(x) simpleBinaryTreeNode##x
#define ST_CLASSNAME SimpleBinaryTree
#define ST_CLASS(x) x##_SimpleBinaryTree
#define ST_METHOD(x) simpleBinaryTree##x

#include "poolpolicy-template.h"
#include "poolelementnode-template.h"
#include "poolnode-template.h"
#include "poolhandlespacenode-template.h"
#include "poolhandlespacemanagement-template.h"
#include "peerlistnode-template.h"
#include "peerlist-template.h"
#include "peerlistmanagement-template.h"
#include "poolusernode-template.h"
#include "pooluserlist-template.h"

#ifdef INTERNAL_POOLTEMPLATE_IMPLEMENT_IT
#include "poolpolicy-template_impl.h"
#include "poolelementnode-template_impl.h"
#include "poolnode-template_impl.h"
#include "poolhandlespacenode-template_impl.h"
#include "poolhandlespacemanagement-template_impl.h"
#include "peerlistnode-template_impl.h"
#include "peerlist-template_impl.h"
#include "peerlistmanagement-template_impl.h"
#include "poolusernode-template_impl.h"
#include "pooluserlist-template_impl.h"
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
#include "poolhandlespacenode-template.h"
#include "poolhandlespacemanagement-template.h"
#include "peerlistnode-template.h"
#include "peerlist-template.h"
#include "peerlistmanagement-template.h"
#include "poolusernode-template.h"
#include "pooluserlist-template.h"

#ifdef INTERNAL_POOLTEMPLATE_IMPLEMENT_IT
#include "poolpolicy-template_impl.h"
#include "poolelementnode-template_impl.h"
#include "poolnode-template_impl.h"
#include "poolhandlespacenode-template_impl.h"
#include "poolhandlespacemanagement-template_impl.h"
#include "peerlistnode-template_impl.h"
#include "peerlist-template_impl.h"
#include "peerlistmanagement-template_impl.h"
#include "poolusernode-template_impl.h"
#include "pooluserlist-template_impl.h"
#endif

#undef STN_CLASSNAME
#undef STN_METHOD
#undef ST_CLASSNAME
#undef ST_CLASS
#undef ST_METHOD
#endif




#ifdef INCLUDE_SIMPLETREAP
#define STN_CLASSNAME SimpleTreapNode
#define STN_METHOD(x) simpleTreapNode##x
#define ST_CLASSNAME SimpleTreap
#define ST_CLASS(x) x##_SimpleTreap
#define ST_METHOD(x) simpleTreap##x

#include "poolpolicy-template.h"
#include "poolelementnode-template.h"
#include "poolnode-template.h"
#include "poolhandlespacenode-template.h"
#include "poolhandlespacemanagement-template.h"
#include "peerlistnode-template.h"
#include "peerlist-template.h"
#include "peerlistmanagement-template.h"
#include "poolusernode-template.h"
#include "pooluserlist-template.h"

#ifdef INTERNAL_POOLTEMPLATE_IMPLEMENT_IT
#include "poolpolicy-template_impl.h"
#include "poolelementnode-template_impl.h"
#include "poolnode-template_impl.h"
#include "poolhandlespacenode-template_impl.h"
#include "poolhandlespacemanagement-template_impl.h"
#include "peerlistnode-template_impl.h"
#include "peerlist-template_impl.h"
#include "peerlistmanagement-template_impl.h"
#include "poolusernode-template_impl.h"
#include "pooluserlist-template_impl.h"
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
#include "poolhandlespacenode-template.h"
#include "poolhandlespacemanagement-template.h"
#include "peerlistnode-template.h"
#include "peerlist-template.h"
#include "peerlistmanagement-template.h"
#include "poolusernode-template.h"
#include "pooluserlist-template.h"

#ifdef INTERNAL_POOLTEMPLATE_IMPLEMENT_IT
#include "poolpolicy-template_impl.h"
#include "poolelementnode-template_impl.h"
#include "poolnode-template_impl.h"
#include "poolhandlespacenode-template_impl.h"
#include "poolhandlespacemanagement-template_impl.h"
#include "peerlistnode-template_impl.h"
#include "peerlist-template_impl.h"
#include "peerlistmanagement-template_impl.h"
#include "poolusernode-template_impl.h"
#include "pooluserlist-template_impl.h"
#endif

#undef STN_CLASSNAME
#undef STN_METHOD
#undef ST_CLASSNAME
#undef ST_CLASS
#undef ST_METHOD
#endif




#ifdef INCLUDE_SIMPLEREDBLACKTREE
#define STN_CLASSNAME SimpleRedBlackTreeNode
#define STN_METHOD(x) simpleRedBlackTreeNode##x
#define ST_CLASSNAME SimpleRedBlackTree
#define ST_CLASS(x) x##_SimpleRedBlackTree
#define ST_METHOD(x) simpleRedBlackTree##x

#include "poolpolicy-template.h"
#include "poolelementnode-template.h"
#include "poolnode-template.h"
#include "poolhandlespacenode-template.h"
#include "poolhandlespacemanagement-template.h"
#include "peerlistnode-template.h"
#include "peerlist-template.h"
#include "peerlistmanagement-template.h"
#include "poolusernode-template.h"
#include "pooluserlist-template.h"

#ifdef INTERNAL_POOLTEMPLATE_IMPLEMENT_IT
#include "poolpolicy-template_impl.h"
#include "poolelementnode-template_impl.h"
#include "poolnode-template_impl.h"
#include "poolhandlespacenode-template_impl.h"
#include "poolhandlespacemanagement-template_impl.h"
#include "peerlistnode-template_impl.h"
#include "peerlist-template_impl.h"
#include "peerlistmanagement-template_impl.h"
#include "poolusernode-template_impl.h"
#include "pooluserlist-template_impl.h"
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
#include "poolhandlespacenode-template.h"
#include "poolhandlespacemanagement-template.h"
#include "peerlistnode-template.h"
#include "peerlist-template.h"
#include "peerlistmanagement-template.h"
#include "poolusernode-template.h"
#include "pooluserlist-template.h"

#ifdef INTERNAL_POOLTEMPLATE_IMPLEMENT_IT
#include "poolpolicy-template_impl.h"
#include "poolelementnode-template_impl.h"
#include "poolnode-template_impl.h"
#include "poolhandlespacenode-template_impl.h"
#include "poolhandlespacemanagement-template_impl.h"
#include "peerlistnode-template_impl.h"
#include "peerlist-template_impl.h"
#include "peerlistmanagement-template_impl.h"
#include "poolusernode-template_impl.h"
#include "pooluserlist-template_impl.h"
#endif

#undef STN_CLASSNAME
#undef STN_METHOD
#undef ST_CLASSNAME
#undef ST_CLASS
#undef ST_METHOD
#endif




#define TMPL_CLASS(x, c) x##_##c
#define TMPL_METHOD(x, c) c##x


#ifdef USE_LIST
#define STN_CLASSNAME LinearListNode
#define STN_METHOD(x) linearListNode##x
#define ST_CLASSNAME LinearList
#define ST_CLASS(x) x##_LinearList
#define ST_METHOD(x) linearList##x
#endif
#ifdef USE_SIMPLEBINARYTREE
#define STN_CLASSNAME SimpleBinaryTreeNode
#define STN_METHOD(x) simpleBinaryTreeNode##x
#define ST_CLASSNAME SimpleBinaryTree
#define ST_CLASS(x) x##_SimpleBinaryTree
#define ST_METHOD(x) simpleBinaryTree##x
#endif
#ifdef USE_LEAFLINKEDBINARYTREE
#define STN_CLASSNAME LeafLinkedBinaryTreeNode
#define STN_METHOD(x) leafLinkedBinaryTreeNode##x
#define ST_CLASSNAME LeafLinkedBinaryTree
#define ST_CLASS(x) x##_LeafLinkedBinaryTree
#define ST_METHOD(x) leafLinkedBinaryTree##x
#endif
#ifdef USE_SIMPLETREAP
#define STN_CLASSNAME SimpleTreapNode
#define STN_METHOD(x) simpleTreapNode##x
#define ST_CLASSNAME SimpleTreap
#define ST_CLASS(x) x##_SimpleTreap
#define ST_METHOD(x) simpleTreap##x
#endif
#ifdef USE_LEAFLINKEDTREAP
#define STN_CLASSNAME LeafLinkedTreapNode
#define STN_METHOD(x) leafLinkedTreapNode##x
#define ST_CLASSNAME LeafLinkedTreap
#define ST_CLASS(x) x##_LeafLinkedTreap
#define ST_METHOD(x) leafLinkedTreap##x
#endif
#ifdef USE_SIMPLEREDBLACKTREE
#define STN_CLASSNAME SimpleRedBlackTreeNode
#define STN_METHOD(x) simpleRedBlackTreeNode##x
#define ST_CLASSNAME SimpleRedBlackTree
#define ST_CLASS(x) x##_SimpleRedBlackTree
#define ST_METHOD(x) simpleRedBlackTree##x
#endif
#ifdef USE_LEAFLINKEDREDBLACKTREE
#define STN_CLASSNAME LeafLinkedRedBlackTreeNode
#define STN_METHOD(x) leafLinkedRedBlackTreeNode##x
#define ST_CLASSNAME LeafLinkedRedBlackTree
#define ST_CLASS(x) x##_LeafLinkedRedBlackTree
#define ST_METHOD(x) leafLinkedRedBlackTree##x
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
