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

#ifndef S1G_OFDM_PPDU_H
#define S1G_OFDM_PPDU_H

#include "ns3/ofdm-ppdu.h"

/**
 * \file
 * \ingroup wifi
 * Declaration of ns3::S1gOfdmPpdu class.
 */

namespace ns3 {

class WifiPsdu;

/**
* \brief S1G-OFDM PPDU (11g)
* \ingroup wifi
*
* S1gOfdmPpdu stores a preamble, PHY headers and a PSDU of a PPDU with non-HT header,
* i.e., PPDU that uses S1G-OFDM modulation.
*/
class S1gOfdmPpdu : public OfdmPpdu
{
public:
    /**
     * Create an S1G-OFDM PPDU.
     *
     * \param psdu the PHY payload (PSDU)
     * \param txVector the TXVECTOR that was used for this PPDU
     * \param band the WifiPhyBand used for the transmission of this PPDU
     * \param uid the unique ID of this PPDU
     */
    S1gOfdmPpdu (Ptr<const WifiPsdu> psdu, const WifiTxVector& txVector,
                 WifiPhyBand band, uint64_t uid);
    /**
     * Destructor for S1gOfdmPpdu.
     */
    virtual ~S1gOfdmPpdu ();
    Time GetTxDuration (void) const override;
    Ptr<WifiPpdu> Copy (void) const override;

private:
    WifiTxVector DoGetTxVector (void) const override;
    WifiMode m_mode;
    uint16_t m_length; ///< LENGTH field


}; //class S1gOfdmPpdu

} //namespace ns3

#endif /* S1G_OFDM_PPDU_H */
