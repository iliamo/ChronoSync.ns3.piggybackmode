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
 *	       Alexander Afanasyev <alexander.afanasyev@ucla.edu>
 *         Ilya Moiseenko <iliamo@ucla.edu>
 */

#ifndef SYNC_CCNX_WRAPPER_H
#define SYNC_CCNX_WRAPPER_H

#include <boost/exception/all.hpp>
#include <boost/function.hpp>
#include <string>

#include <ns3/ptr.h>
#include <ns3/node.h>
#include <ns3/random-variable.h>
#include <ns3/ndn-app.h>
#include <ns3/ndn-name.h>
#include <ns3/ndn-data.h>
#include <ns3/ndn-interest.h>

#include <ns3/ndnSIM/utils/trie/trie-with-policy.h>
#include <ns3/ndnSIM/utils/trie/counting-policy.h>

/**
 * \defgroup sync SYNC protocol
 *
 * Implementation of SYNC protocol
 */
namespace Sync {

template<class Callback>
struct CcnxFilterEntry : public ns3::SimpleRefCount< CcnxFilterEntry<Callback> >
{
public:
  CcnxFilterEntry (ns3::Ptr<const ns3::ndn::Name> prefix)
    : m_prefix (prefix)
  { }
  
  const ns3::ndn::Name &
  GetPrefix () const
  { return *m_prefix; }

  void
  AddCallback (Callback callback)
  { 
    m_callback = callback;
  }

  void
  ClearCallback ()
  {
    m_callback = 0;
  }
  
public:
  ns3::Ptr<const ns3::ndn::Name> m_prefix; ///< \brief Prefix of the PIT entry
  Callback m_callback;
};


template<class Callback>
struct CcnxFilterEntryContainer :
    public ns3::ndn::ndnSIM::trie_with_policy<ns3::ndn::Name,
                                              ns3::ndn::ndnSIM::smart_pointer_payload_traits< CcnxFilterEntry<Callback> >,
                                              ns3::ndn::ndnSIM::counting_policy_traits>
{
};


struct CcnxOperationException : virtual boost::exception, virtual std::exception { };
/**
 * \ingroup sync
 * @brief A wrapper for ccnx library; clients of this code do not need to deal
 * with ccnx library
 */
class CcnxWrapper
  : public ns3::ndn::App
{
public:
  typedef boost::function<void (std::string, std::string)> StringDataCallback;
  typedef boost::function<void (std::string, const char *buf, size_t len)> RawDataCallback;
  typedef boost::function<void (std::string)> InterestCallback;

  
  /**
   * @brief initialize the wrapper; a lot of things needs to be done. 1) init
   * keystore 2) init keylocator 3) start a thread to hold a loop of ccn_run
   *
   */
  CcnxWrapper();
  ~CcnxWrapper();

  /**
   * @brief send Interest; need to grab lock m_mutex first
   *
   * @param strInterest the Interest name
   * @param dataCallback the callback function to deal with the returned data
   * @return the return code of ccn_express_interest
   */
  int
  sendInterestForString (const std::string &strInterest, const StringDataCallback &strDataCallback/*, int retry = 0*/);

  int
  sendInterest (const std::string &strInterest, const RawDataCallback &rawDataCallback/*, int retry = 0*/);
  
  /**
   * @brief set Interest filter (specify what interest you want to receive)
   *
   * @param prefix the prefix of Interest
   * @param interestCallback the callback function to deal with the returned data
   * @return the return code of ccn_set_interest_filter
   */
  int
  setInterestFilter (const std::string &prefix, const InterestCallback &interestCallback);

  /**
   * @brief clear Interest filter
   * @param prefix the prefix of Interest
   */
  void
  clearInterestFilter (const std::string &prefix);

  /**
   * @brief publish data and put it to local ccn content store; need to grab
   * lock m_mutex first
   *
   * @param name the name for the data object
   * @param dataBuffer the data to be published
   * @param freshness the freshness time for the data object
   * @return code generated by ccnx library calls, >0 if success
   */
  int
  publishStringData (const std::string &name, const std::string &dataBuffer, int freshness)
  {
    return publishRawData (name, dataBuffer.c_str(), dataBuffer.length(), freshness);
  }

  int
  publishRawData (const std::string &name, const char *buf, size_t len, int freshness);
  
  // from ndn::App
  
  /**
   * @brief Method that will be called every time new Interest arrives
   * @param interest - NDN Interest
   */
  virtual void
  OnInterest (ns3::Ptr<const ns3::ndn::Interest> interest);
 
  /**
   * @brief Method that will be called every time new Data arrives
   * @param contentObject - NDN Data
   */
  virtual void
  OnData (ns3::Ptr<const ns3::ndn::Data> contentObject);

public:
  // inherited from Application base class.
  virtual void
  StartApplication ();    // Called at time specified by Start

  virtual void
  StopApplication ();     // Called at time specified by Stop

private:
  ns3::UniformVariable m_rand; // nonce generator

  CcnxFilterEntryContainer<RawDataCallback> m_dataCallbacks;
  CcnxFilterEntryContainer<InterestCallback> m_interestCallbacks;
};

typedef boost::shared_ptr<CcnxWrapper> CcnxWrapperPtr;

} // Sync

#endif // SYNC_CCNX_WRAPPER_H