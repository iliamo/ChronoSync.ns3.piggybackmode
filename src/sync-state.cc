/* -*- Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2012 University of California, Los Angeles
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Zhenkai Zhu <zhenkai@cs.ucla.edu>
 *         Chaoyi Bian <bcy@pku.edu.cn>
 *	   Alexander Afanasyev <alexander.afanasyev@ucla.edu>
 */

#include "sync-state.h"
#include "sync-diff-leaf.h"
#include "sync-std-name-info.h"

#include <boost/assert.hpp>
#include <boost/foreach.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/throw_exception.hpp>
#include <boost/lexical_cast.hpp>

using namespace std;
using namespace boost;

typedef error_info<struct tag_errmsg, string> info_str; 

using namespace Sync::Error;

namespace Sync {

SyncStateMsg &
operator << (SyncStateMsg &ossm, const State &state)
{
  BOOST_FOREACH (boost::shared_ptr<const Leaf> leaf, state.getLeaves ().get<ordered> ())
  {
    SyncState *oss = ossm.add_ss();
    boost::shared_ptr<const DiffLeaf> diffLeaf = dynamic_pointer_cast<const DiffLeaf> (leaf);
    if (diffLeaf != 0 && diffLeaf->getOperation() != UPDATE)
    {
      oss->set_type(SyncState::DELETE);
    }
    else
    {
      oss->set_type(SyncState::UPDATE);
    }

    std::ostringstream os;
    os << *leaf->getInfo();
    oss->set_name(os.str());
    
    //std::ostringstream os2;
    //os2 << &(*leaf->getWireData());
    oss->set_wiredata(leaf->getWireData());

    if (diffLeaf == 0 || (diffLeaf != 0 && diffLeaf->getOperation () == UPDATE))
    {
      SyncState::SeqNo *seqNo = oss->mutable_seqno();
      seqNo->set_session(leaf->getSeq().getSession());
      seqNo->set_seq(leaf->getSeq().getSeq());
    }
  }
  return ossm;
}


SyncStateMsg &
operator >> (SyncStateMsg &issm, State &state)
{
  int n = issm.ss_size();
  for (int i = 0; i < n; i++)
  {
    const SyncState &ss = issm.ss(i);
    NameInfoConstPtr info = StdNameInfo::FindOrCreate (ss.name());
    if (ss.type() == SyncState::UPDATE)
    {
      uint32_t session = lexical_cast<uint32_t>(ss.seqno().session());
      uint32_t seq = lexical_cast<uint32_t>(ss.seqno().seq());
      std::string wiredata = lexical_cast<std::string>(ss.wiredata());
      SeqNo seqNo(session, seq);
      state.update(info, seqNo, wiredata);
      
    }
    else
    {
      state.remove(info);
    }
  }
  return issm;
}

}
