/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2020 Orange Labs
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
 * Author: Zhouyou <zhouyou.gu@sydney.edu.au>
 */

#include "ns3/wifi-psdu.h"
#include "s1g-ofdm-phy.h"
#include "s1g-ofdm-ppdu.h"
#include "ns3/log.h"

namespace ns3 {

    NS_LOG_COMPONENT_DEFINE ("S1gOfdmPpdu");

    S1gOfdmPpdu::S1gOfdmPpdu (Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector,
                              WifiPhyBand band, uint64_t uid)
            : OfdmPpdu (psdu, txVector, band, uid, false) //instantiate LSigHeader of OfdmPpdu
    {
      NS_LOG_FUNCTION (this << psdu << txVector << band << uid);
      m_mode = txVector.GetMode ();
    }

    S1gOfdmPpdu::~S1gOfdmPpdu ()
    {
    }

    WifiTxVector
    S1gOfdmPpdu::DoGetTxVector (void) const
    {
      WifiTxVector txVector;
      txVector.SetPreambleType (m_preamble);
      txVector.SetMode (m_mode);
      txVector.SetChannelWidth (m_channelWidth);
      return txVector;
    }

    Ptr<WifiPpdu>
    S1gOfdmPpdu::Copy (void) const
    {
      return Create<S1gOfdmPpdu> (GetPsdu (), GetTxVector (), m_band, m_uid);
    }

} //namespace ns3
