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

#include <array>
#include <cmath>
#include "s1g-ofdm-phy.h"
#include "s1g-ofdm-ppdu.h"
#include "ns3/wifi-psdu.h"
#include "ns3/wifi-phy.h" //only used for static mode constructor
#include "ns3/log.h"
#include "ns3/assert.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("S1gOfdmPhy");

/*******************************************************
*       S1G PHY (IEEE 802.11-2016, clause 18)
*******************************************************/
/* *NS_CHECK_STYLE_OFF* */
const PhyEntity::PpduFormats S1gOfdmPhy::m_ofdmPpduFormats {
    { WIFI_PREAMBLE_S1G_LONG, { WIFI_PPDU_FIELD_PREAMBLE,      //STF + LTF
                                      WIFI_PPDU_FIELD_SIG_A, //SIG-A
                                      WIFI_PPDU_FIELD_SIG_B, //SIG-A
                            WIFI_PPDU_FIELD_DATA } },
    { WIFI_PREAMBLE_S1G_SHORT, { WIFI_PPDU_FIELD_PREAMBLE,      //STF + LTF
                            WIFI_PPDU_FIELD_NON_HT_HEADER, //SIG
                            WIFI_PPDU_FIELD_DATA } },
    { WIFI_PREAMBLE_S1G_1M, { WIFI_PPDU_FIELD_PREAMBLE,      //STF + LTF
                            WIFI_PPDU_FIELD_NON_HT_HEADER, //SIG
                            WIFI_PPDU_FIELD_DATA } }
};
/* *NS_CHECK_STYLE_OFF* */
const PhyEntity::ModulationLookupTable S1gOfdmPhy::m_s1gOfdmModulationLookupTable {
        // Unique name           Code rate           Constellation size
      {"S1gOfdmRate0_30MbpsBW1MHz",	{WIFI_CODE_RATE_1_2,	2}},
      {"S1gOfdmRate0_60MbpsBW1MHz",	{WIFI_CODE_RATE_1_2,	4}},
      {"S1gOfdmRate0_90MbpsBW1MHz",	{WIFI_CODE_RATE_3_4,	4}},
      {"S1gOfdmRate1_20MbpsBW1MHz",	{WIFI_CODE_RATE_1_2,	16}},
      {"S1gOfdmRate1_80MbpsBW1MHz",	{WIFI_CODE_RATE_3_4,	16}},
      {"S1gOfdmRate2_40MbpsBW1MHz",	{WIFI_CODE_RATE_3_4,	64}},
      {"S1gOfdmRate2_70MbpsBW1MHz",	{WIFI_CODE_RATE_3_4,	64}},
      {"S1gOfdmRate3_00MbpsBW1MHz",	{WIFI_CODE_RATE_5_6,	64}},
      {"S1gOfdmRate3_60MbpsBW1MHz",	{WIFI_CODE_RATE_3_4,	256}},
      {"S1gOfdmRate4_00MbpsBW1MHz",	{WIFI_CODE_RATE_5_6,	256}},
      {"S1gOfdmRate0_15MbpsBW1MHz",	{WIFI_CODE_RATE_1_2,	2}},
      {"S1gOfdmRate0_65MbpsBW2MHz",	{WIFI_CODE_RATE_1_2,	2}},
      {"S1gOfdmRate1_30MbpsBW2MHz",	{WIFI_CODE_RATE_1_2,	4}},
      {"S1gOfdmRate1_95MbpsBW2MHz",	{WIFI_CODE_RATE_3_4,	4}},
      {"S1gOfdmRate2_60MbpsBW2MHz",	{WIFI_CODE_RATE_1_2,	16}},
      {"S1gOfdmRate3_90MbpsBW2MHz",	{WIFI_CODE_RATE_3_4,	16}},
      {"S1gOfdmRate5_20MbpsBW2MHz",	{WIFI_CODE_RATE_3_4,	64}},
      {"S1gOfdmRate5_85MbpsBW2MHz",	{WIFI_CODE_RATE_3_4,	64}},
      {"S1gOfdmRate6_50MbpsBW2MHz",	{WIFI_CODE_RATE_5_6,	64}},
      {"S1gOfdmRate7_80MbpsBW2MHz",	{WIFI_CODE_RATE_3_4,	256}},
      {"S1gOfdmRate1_35MbpsBW4MHz",	{WIFI_CODE_RATE_1_2,	2}},
      {"S1gOfdmRate2_70MbpsBW4MHz",	{WIFI_CODE_RATE_1_2,	4}},
      {"S1gOfdmRate4_05MbpsBW4MHz",	{WIFI_CODE_RATE_3_4,	4}},
      {"S1gOfdmRate5_40MbpsBW4MHz",	{WIFI_CODE_RATE_1_2,	16}},
      {"S1gOfdmRate8_10MbpsBW4MHz",	{WIFI_CODE_RATE_3_4,	16}},
      {"S1gOfdmRate10_80MbpsBW4MHz",	{WIFI_CODE_RATE_3_4,	64}},
      {"S1gOfdmRate12_20MbpsBW4MHz",	{WIFI_CODE_RATE_3_4,	64}},
      {"S1gOfdmRate13_50MbpsBW4MHz",	{WIFI_CODE_RATE_5_6,	64}},
      {"S1gOfdmRate16_20MbpsBW4MHz",	{WIFI_CODE_RATE_3_4,	256}},
      {"S1gOfdmRate18_00MbpsBW4MHz",	{WIFI_CODE_RATE_5_6,	256}},
      {"S1gOfdmRate2_93MbpsBW8MHz",	{WIFI_CODE_RATE_1_2,	2}},
      {"S1gOfdmRate5_85MbpsBW8MHz",	{WIFI_CODE_RATE_1_2,	4}},
      {"S1gOfdmRate8_78MbpsBW8MHz",	{WIFI_CODE_RATE_3_4,	4}},
      {"S1gOfdmRate11_70MbpsBW8MHz",	{WIFI_CODE_RATE_1_2,	16}},
      {"S1gOfdmRate17_60MbpsBW8MHz",	{WIFI_CODE_RATE_3_4,	16}},
      {"S1gOfdmRate23_40MbpsBW8MHz",	{WIFI_CODE_RATE_3_4,	64}},
      {"S1gOfdmRate26_30MbpsBW8MHz",	{WIFI_CODE_RATE_3_4,	64}},
      {"S1gOfdmRate29_30MbpsBW8MHz",	{WIFI_CODE_RATE_5_6,	64}},
      {"S1gOfdmRate35_10MbpsBW8MHz",	{WIFI_CODE_RATE_3_4,	256}},
      {"S1gOfdmRate39_00MbpsBW8MHz",	{WIFI_CODE_RATE_5_6,	256}},
      {"S1gOfdmRate5_85MbpsBW16MHz",	{WIFI_CODE_RATE_1_2,	2}},
      {"S1gOfdmRate11_70MbpsBW16MHz",	{WIFI_CODE_RATE_1_2,	4}},
      {"S1gOfdmRate17_60MbpsBW16MHz",	{WIFI_CODE_RATE_3_4,	4}},
      {"S1gOfdmRate23_40MbpsBW16MHz",	{WIFI_CODE_RATE_1_2,	16}},
      {"S1gOfdmRate35_10MbpsBW16MHz",	{WIFI_CODE_RATE_3_4,	16}},
      {"S1gOfdmRate46_80MbpsBW16MHz",	{WIFI_CODE_RATE_3_4,	64}},
      {"S1gOfdmRate52_70MbpsBW16MHz",	{WIFI_CODE_RATE_3_4,	64}},
      {"S1gOfdmRate58_50MbpsBW16MHz",	{WIFI_CODE_RATE_5_6,	64}},
      {"S1gOfdmRate70_20MbpsBW16MHz",	{WIFI_CODE_RATE_3_4,	256}},
      {"S1gOfdmRate78_00MbpsBW16MHz",	{WIFI_CODE_RATE_5_6,	256}},

};

/// S1G OFDM rates in bits per second
//static const std::array<uint64_t, 8> s_s1gOfdmRatesBpsList =
//        {  6000000,  9000000, 12000000, 18000000,
//           24000000, 36000000, 48000000, 54000000};

const std::map<uint16_t, std::vector<uint64_t> > s_s1gOfdmRatesBpsList =
        {
                {1, {300000,600000,900000,1200000,1800000,2400000,2700000,3000000,3600000,4000000,150000,}},
                {2, {650000,1300000,1950000,2600000,3900000,5200000,5850000,6500000,7800000,}},
                {4, {1350000,2700000,4050000,5400000,8100000,10800000,12200000,13500000,16200000,18000000,}},
                {8, {2930000,5850000,8780000,11700000,17600000,23400000,26300000,29300000,35100000,39000000,}},
                {16, {5850000,11700000,17600000,23400000,35100000,46800000,52700000,58500000,70200000,78000000,}},
        };



/* *NS_CHECK_STYLE_ON* */

/**
* Get the array of possible S1G OFDM rates.
*
* \return the S1G OFDM rates in bits per second
*/
const std::map<uint16_t, std::vector<uint64_t> >& GetS1gOfdmRatesBpsList (void)
{
  return s_s1gOfdmRatesBpsList;
};

S1gOfdmPhy::S1gOfdmPhy ()
        : OfdmPhy (OFDM_PHY_DEFAULT, false) //don't add OFDM modes to list
{
  NS_LOG_FUNCTION (this);
  auto bwRatesMap = GetS1gOfdmRatesBpsList ();
  for (int i = 0; i < 5; i++){
    int bw = std::pow(2,i);
    for (const auto & rate : bwRatesMap.at (bw))
    {
      WifiMode mode = GetS1gOfdmRate (rate, bw);
      NS_LOG_LOGIC ("Add " << mode << " to list");
      m_modeList.emplace_back (mode);
    }
  }

}

S1gOfdmPhy::~S1gOfdmPhy ()
{
  NS_LOG_FUNCTION (this);
}

WifiMode
S1gOfdmPhy::GetSigMode (WifiPpduField field, const WifiTxVector& txVector) const
{
  switch (field)
  {
    case WIFI_PPDU_FIELD_PREAMBLE: //consider header mode for preamble (useful for InterferenceHelper)
    case WIFI_PPDU_FIELD_NON_HT_HEADER:
    case WIFI_PPDU_FIELD_SIG_A:
    case WIFI_PPDU_FIELD_SIG_B:
      return GetHeaderMode (txVector);
    default:
      return PhyEntity::GetSigMode (field, txVector);
  }
}

const PhyEntity::PpduFormats &
S1gOfdmPhy::GetPpduFormats (void) const
{
  return m_ofdmPpduFormats;
}

Time
S1gOfdmPhy::GetDuration (WifiPpduField field, const WifiTxVector& txVector) const
{
  switch (field)
  {
    case WIFI_PPDU_FIELD_SIG_A:
      return GetSigADuration (txVector.GetPreambleType ());
    case WIFI_PPDU_FIELD_SIG_B:
      return GetSigBDuration (txVector.GetPreambleType ());
    default:
      return OfdmPhy::GetDuration (field, txVector);
  }
}

Time
S1gOfdmPhy::GetSigADuration (WifiPreamble preamble)
{
  return (preamble == WIFI_PREAMBLE_S1G_LONG) ? MicroSeconds (40*2) : MicroSeconds (0);
}
Time
S1gOfdmPhy::GetSigBDuration (WifiPreamble preamble)
{
  return (preamble == WIFI_PREAMBLE_S1G_LONG) ? MicroSeconds (40*1) : MicroSeconds (0);
}

WifiMode
S1gOfdmPhy::GetHeaderMode (const WifiTxVector& txVector) const
{
  NS_ASSERT (txVector.GetMode ().GetModulationClass () == WIFI_MOD_CLASS_S1G);
  auto bwRatesMap = GetS1gOfdmRatesBpsList ();
  return GetS1gOfdmRate(bwRatesMap.at(txVector.GetChannelWidth()).at(0),txVector.GetChannelWidth());
}

PhyEntity::PhyFieldRxStatus
S1gOfdmPhy::DoEndReceiveField (WifiPpduField field, Ptr<Event> event)
{
  NS_LOG_FUNCTION (this << field << *event);
  if (field == WIFI_PPDU_FIELD_SIG_A || field == WIFI_PPDU_FIELD_SIG_B)
  {
    return PhyFieldRxStatus (true);
  }
  return OfdmPhy::DoEndReceiveField (field, event);
}

Time
S1gOfdmPhy::GetPreambleDuration (const WifiTxVector& txVector) const
{
  NS_ASSERT (txVector.GetMode ().GetModulationClass () == WIFI_MOD_CLASS_S1G);
  switch (txVector.GetPreambleType())
  {
    case WIFI_PREAMBLE_S1G_1M:
      return MicroSeconds (40 * 8);
    case WIFI_PREAMBLE_S1G_SHORT:
    case WIFI_PREAMBLE_S1G_LONG:
    default:
      return MicroSeconds (40 * 4);

  }
}

Time
S1gOfdmPhy::GetHeaderDuration (const WifiTxVector& txVector) const
{
  NS_ASSERT (txVector.GetMode ().GetModulationClass () == WIFI_MOD_CLASS_S1G);
  switch (txVector.GetPreambleType())
  {
    case WIFI_PREAMBLE_S1G_1M:
      return MicroSeconds (40 * 6);
    case WIFI_PREAMBLE_S1G_SHORT:
      return MicroSeconds (40 * 2);
    case WIFI_PREAMBLE_S1G_LONG:
    default:
      return MicroSeconds (0);
  }
}

Time
S1gOfdmPhy::GetPayloadDuration (uint32_t size, const WifiTxVector& txVector, WifiPhyBand band, MpduType /* mpdutype */,
                             bool /* incFlag */, uint32_t & /* totalAmpduSize */, double & /* totalAmpduNumSymbols */,
                             uint16_t /* staId */) const
{
  Time symbolDuration = MicroSeconds (40);

  double numDataBitsPerSymbol = txVector.GetMode ().GetDataRate (txVector) * symbolDuration.GetNanoSeconds () / 1e9;

  //The number of OFDM symbols in the data field when BCC encoding
  //is used is given in equation 19-32 of the IEEE 802.11-2016 standard.
  double numSymbols = lrint (ceil ((8.0 + size * 8.0 + 6.0) / (numDataBitsPerSymbol)));

  Time payloadDuration = FemtoSeconds (static_cast<uint64_t> (numSymbols * symbolDuration.GetFemtoSeconds ()));

  NS_LOG_FUNCTION(this << "payloadDuration (us): " << payloadDuration.GetMicroSeconds());
  NS_LOG_FUNCTION(this << "numSymbols : " << numSymbols);
  NS_LOG_FUNCTION(this << "numBits : " << size * 8);
  NS_LOG_FUNCTION(this << "data rate : " << txVector.GetMode ().GetDataRate (txVector));

  return payloadDuration;
}



Ptr<WifiPpdu>
S1gOfdmPhy::BuildPpdu (const WifiConstPsduMap & psdus, const WifiTxVector& txVector, Time /* ppduDuration */)
{
  NS_LOG_FUNCTION (this << psdus << txVector);
  return Create<S1gOfdmPpdu> (psdus.begin ()->second, txVector, m_wifiPhy->GetPhyBand (),
                              ObtainNextUid (txVector));
}

void
S1gOfdmPhy::InitializeModes (void)
{
  for (const auto & ratesPerBw : GetS1gOfdmRatesBpsList())
  {
    for (const auto & rate : ratesPerBw.second)
    {
      GetS1gOfdmRate (rate, ratesPerBw.first);
    }
  }
}

WifiMode
S1gOfdmPhy::GetS1gOfdmRate (uint64_t rate, uint16_t bw)
{
  switch (bw)
  {
    case 1:
      switch (rate)
      {
        case 300000:
          return GetS1gOfdmRate0_30MbpsBW1MHz();
        case 600000:
          return GetS1gOfdmRate0_60MbpsBW1MHz();
        case 900000:
          return GetS1gOfdmRate0_90MbpsBW1MHz();
        case 1200000:
          return GetS1gOfdmRate1_20MbpsBW1MHz();
        case 1800000:
          return GetS1gOfdmRate1_80MbpsBW1MHz();
        case 2400000:
          return GetS1gOfdmRate2_40MbpsBW1MHz();
        case 2700000:
          return GetS1gOfdmRate2_70MbpsBW1MHz();
        case 3000000:
          return GetS1gOfdmRate3_00MbpsBW1MHz();
        case 3600000:
          return GetS1gOfdmRate3_60MbpsBW1MHz();
        case 4000000:
          return GetS1gOfdmRate4_00MbpsBW1MHz();
        case 150000:
          return GetS1gOfdmRate0_15MbpsBW1MHz();
        default:
          NS_ABORT_MSG ("Inexistent rate (" << rate << " bps) requested for s1g OFDM (default)");
          return WifiMode ();
      }
    case 2:
      switch (rate)
      {
        case 650000:
          return GetS1gOfdmRate0_65MbpsBW2MHz();
        case 1300000:
          return GetS1gOfdmRate1_30MbpsBW2MHz();
        case 1950000:
          return GetS1gOfdmRate1_95MbpsBW2MHz();
        case 2600000:
          return GetS1gOfdmRate2_60MbpsBW2MHz();
        case 3900000:
          return GetS1gOfdmRate3_90MbpsBW2MHz();
        case 5200000:
          return GetS1gOfdmRate5_20MbpsBW2MHz();
        case 5850000:
          return GetS1gOfdmRate5_85MbpsBW2MHz();
        case 6500000:
          return GetS1gOfdmRate6_50MbpsBW2MHz();
        case 7800000:
          return GetS1gOfdmRate7_80MbpsBW2MHz();
        default:
          NS_ABORT_MSG ("Inexistent rate (" << rate << " bps) requested for s1g OFDM (default)");
          return WifiMode ();
      }
    case 4:
      switch (rate)
      {
        case 1350000:
          return GetS1gOfdmRate1_35MbpsBW4MHz();
        case 2700000:
          return GetS1gOfdmRate2_70MbpsBW4MHz();
        case 4050000:
          return GetS1gOfdmRate4_05MbpsBW4MHz();
        case 5400000:
          return GetS1gOfdmRate5_40MbpsBW4MHz();
        case 8100000:
          return GetS1gOfdmRate8_10MbpsBW4MHz();
        case 10800000:
          return GetS1gOfdmRate10_80MbpsBW4MHz();
        case 12200000:
          return GetS1gOfdmRate12_20MbpsBW4MHz();
        case 13500000:
          return GetS1gOfdmRate13_50MbpsBW4MHz();
        case 16200000:
          return GetS1gOfdmRate16_20MbpsBW4MHz();
        case 18000000:
          return GetS1gOfdmRate18_00MbpsBW4MHz();
        default:
          NS_ABORT_MSG ("Inexistent rate (" << rate << " bps) requested for s1g OFDM (default)");
          return WifiMode ();
      }
    case 8:
      switch (rate)
      {
        case 2930000:
          return GetS1gOfdmRate2_93MbpsBW8MHz();
        case 5850000:
          return GetS1gOfdmRate5_85MbpsBW8MHz();
        case 8780000:
          return GetS1gOfdmRate8_78MbpsBW8MHz();
        case 11700000:
          return GetS1gOfdmRate11_70MbpsBW8MHz();
        case 17600000:
          return GetS1gOfdmRate17_60MbpsBW8MHz();
        case 23400000:
          return GetS1gOfdmRate23_40MbpsBW8MHz();
        case 26300000:
          return GetS1gOfdmRate26_30MbpsBW8MHz();
        case 29300000:
          return GetS1gOfdmRate29_30MbpsBW8MHz();
        case 35100000:
          return GetS1gOfdmRate35_10MbpsBW8MHz();
        case 39000000:
          return GetS1gOfdmRate39_00MbpsBW8MHz();
        default:
          NS_ABORT_MSG ("Inexistent rate (" << rate << " bps) requested for s1g OFDM (default)");
          return WifiMode ();
      }
    case 16:
      switch (rate)
      {
        case 5850000:
          return GetS1gOfdmRate5_85MbpsBW16MHz();
        case 11700000:
          return GetS1gOfdmRate11_70MbpsBW16MHz();
        case 17600000:
          return GetS1gOfdmRate17_60MbpsBW16MHz();
        case 23400000:
          return GetS1gOfdmRate23_40MbpsBW16MHz();
        case 35100000:
          return GetS1gOfdmRate35_10MbpsBW16MHz();
        case 46800000:
          return GetS1gOfdmRate46_80MbpsBW16MHz();
        case 52700000:
          return GetS1gOfdmRate52_70MbpsBW16MHz();
        case 58500000:
          return GetS1gOfdmRate58_50MbpsBW16MHz();
        case 70200000:
          return GetS1gOfdmRate70_20MbpsBW16MHz();
        case 78000000:
          return GetS1gOfdmRate78_00MbpsBW16MHz();
        default:
          NS_ABORT_MSG ("Inexistent rate (" << rate << " bps) requested for s1g OFDM (default)");
          return WifiMode ();
      }
    default:
      NS_ABORT_MSG ("Inexistent bandwidth (" << +bw << " MHz) requested for s1g ofdm");
    return WifiMode ();
  }
}

#define GET_S1G_OFDM_MODE(x, f) \
WifiMode \
S1gOfdmPhy::Get ## x (void) \
{ \
static WifiMode mode = CreateS1gOfdmMode (#x, f); \
return mode; \
}; \

GET_S1G_OFDM_MODE (S1gOfdmRate0_30MbpsBW1MHz, true)
GET_S1G_OFDM_MODE (S1gOfdmRate0_60MbpsBW1MHz, true)
GET_S1G_OFDM_MODE (S1gOfdmRate0_90MbpsBW1MHz, true)
GET_S1G_OFDM_MODE (S1gOfdmRate1_20MbpsBW1MHz, true)
GET_S1G_OFDM_MODE (S1gOfdmRate1_80MbpsBW1MHz, true)
GET_S1G_OFDM_MODE (S1gOfdmRate2_40MbpsBW1MHz, true)
GET_S1G_OFDM_MODE (S1gOfdmRate2_70MbpsBW1MHz, true)
GET_S1G_OFDM_MODE (S1gOfdmRate3_00MbpsBW1MHz, true)
GET_S1G_OFDM_MODE (S1gOfdmRate3_60MbpsBW1MHz, true)
GET_S1G_OFDM_MODE (S1gOfdmRate4_00MbpsBW1MHz, true)
GET_S1G_OFDM_MODE (S1gOfdmRate0_15MbpsBW1MHz, true)
GET_S1G_OFDM_MODE (S1gOfdmRate0_65MbpsBW2MHz, true)
GET_S1G_OFDM_MODE (S1gOfdmRate1_30MbpsBW2MHz, true)
GET_S1G_OFDM_MODE (S1gOfdmRate1_95MbpsBW2MHz, true)
GET_S1G_OFDM_MODE (S1gOfdmRate2_60MbpsBW2MHz, true)
GET_S1G_OFDM_MODE (S1gOfdmRate3_90MbpsBW2MHz, true)
GET_S1G_OFDM_MODE (S1gOfdmRate5_20MbpsBW2MHz, true)
GET_S1G_OFDM_MODE (S1gOfdmRate5_85MbpsBW2MHz, true)
GET_S1G_OFDM_MODE (S1gOfdmRate6_50MbpsBW2MHz, true)
GET_S1G_OFDM_MODE (S1gOfdmRate7_80MbpsBW2MHz, true)
GET_S1G_OFDM_MODE (S1gOfdmRate1_35MbpsBW4MHz, true)
GET_S1G_OFDM_MODE (S1gOfdmRate2_70MbpsBW4MHz, true)
GET_S1G_OFDM_MODE (S1gOfdmRate4_05MbpsBW4MHz, true)
GET_S1G_OFDM_MODE (S1gOfdmRate5_40MbpsBW4MHz, true)
GET_S1G_OFDM_MODE (S1gOfdmRate8_10MbpsBW4MHz, true)
GET_S1G_OFDM_MODE (S1gOfdmRate10_80MbpsBW4MHz, true)
GET_S1G_OFDM_MODE (S1gOfdmRate12_20MbpsBW4MHz, true)
GET_S1G_OFDM_MODE (S1gOfdmRate13_50MbpsBW4MHz, true)
GET_S1G_OFDM_MODE (S1gOfdmRate16_20MbpsBW4MHz, true)
GET_S1G_OFDM_MODE (S1gOfdmRate18_00MbpsBW4MHz, true)
GET_S1G_OFDM_MODE (S1gOfdmRate2_93MbpsBW8MHz, true)
GET_S1G_OFDM_MODE (S1gOfdmRate5_85MbpsBW8MHz, true)
GET_S1G_OFDM_MODE (S1gOfdmRate8_78MbpsBW8MHz, true)
GET_S1G_OFDM_MODE (S1gOfdmRate11_70MbpsBW8MHz, true)
GET_S1G_OFDM_MODE (S1gOfdmRate17_60MbpsBW8MHz, true)
GET_S1G_OFDM_MODE (S1gOfdmRate23_40MbpsBW8MHz, true)
GET_S1G_OFDM_MODE (S1gOfdmRate26_30MbpsBW8MHz, true)
GET_S1G_OFDM_MODE (S1gOfdmRate29_30MbpsBW8MHz, true)
GET_S1G_OFDM_MODE (S1gOfdmRate35_10MbpsBW8MHz, true)
GET_S1G_OFDM_MODE (S1gOfdmRate39_00MbpsBW8MHz, true)
GET_S1G_OFDM_MODE (S1gOfdmRate5_85MbpsBW16MHz, true)
GET_S1G_OFDM_MODE (S1gOfdmRate11_70MbpsBW16MHz, true)
GET_S1G_OFDM_MODE (S1gOfdmRate17_60MbpsBW16MHz, true)
GET_S1G_OFDM_MODE (S1gOfdmRate23_40MbpsBW16MHz, true)
GET_S1G_OFDM_MODE (S1gOfdmRate35_10MbpsBW16MHz, true)
GET_S1G_OFDM_MODE (S1gOfdmRate46_80MbpsBW16MHz, true)
GET_S1G_OFDM_MODE (S1gOfdmRate52_70MbpsBW16MHz, true)
GET_S1G_OFDM_MODE (S1gOfdmRate58_50MbpsBW16MHz, true)
GET_S1G_OFDM_MODE (S1gOfdmRate70_20MbpsBW16MHz, true)
GET_S1G_OFDM_MODE (S1gOfdmRate78_00MbpsBW16MHz, true)
#undef GET_S1G_OFDM_MODE

WifiMode
S1gOfdmPhy::CreateS1gOfdmMode (std::string uniqueName, bool isMandatory)
{
  // Check whether uniqueName is in lookup table
  const auto it = m_s1gOfdmModulationLookupTable.find (uniqueName);
  NS_ASSERT_MSG (it != m_s1gOfdmModulationLookupTable.end (), "S1G-OFDM mode cannot be created because it is not in the lookup table!");

  return WifiModeFactory::CreateWifiMode (uniqueName,
                                          WIFI_MOD_CLASS_S1G,
                                          isMandatory,
                                          MakeBoundCallback (&GetCodeRate, uniqueName),
                                          MakeBoundCallback (&GetConstellationSize, uniqueName),
                                          MakeCallback (&GetPhyRateFromTxVector),
                                          MakeCallback (&GetDataRateFromTxVector),
                                          MakeCallback (&IsAllowed));
}

WifiCodeRate
S1gOfdmPhy::GetCodeRate (const std::string& name)
{
  return m_s1gOfdmModulationLookupTable.at (name).first;
}

uint16_t
S1gOfdmPhy::GetConstellationSize (const std::string& name)
{
  return m_s1gOfdmModulationLookupTable.at (name).second;
}

uint64_t
S1gOfdmPhy::GetPhyRate (const std::string& name, uint16_t channelWidth)
{
  WifiCodeRate codeRate = GetCodeRate (name);
  uint64_t dataRate = GetDataRate (name, channelWidth);
  return CalculatePhyRate (codeRate, dataRate);
}

uint64_t
S1gOfdmPhy::CalculatePhyRate (WifiCodeRate codeRate, uint64_t dataRate)
{
  return (uint64_t)((double)dataRate / GetCodeRatio (codeRate));
}

uint64_t
S1gOfdmPhy::GetPhyRateFromTxVector (const WifiTxVector &txVector, uint16_t  /* staId */)
{
  return GetPhyRate (txVector.GetMode ().GetUniqueName (),
                     txVector.GetChannelWidth ());
}

double
S1gOfdmPhy::GetCodeRatio (WifiCodeRate codeRate)
{
  switch (codeRate)
  {
    case WIFI_CODE_RATE_5_6:
      return (5.0 / 6.0);
    case WIFI_CODE_RATE_3_4:
      return (3.0 / 4.0);
    case WIFI_CODE_RATE_2_3:
      return (2.0 / 3.0);
    case WIFI_CODE_RATE_1_2:
      return (1.0 / 2.0);
    case WIFI_CODE_RATE_UNDEFINED:
    default:
      NS_FATAL_ERROR ("trying to get code ratio for undefined coding rate");
      return 0;
  }
}

uint64_t
S1gOfdmPhy::GetDataRateFromTxVector (const WifiTxVector& txVector, uint16_t /* staId */)
{
  return GetDataRate (txVector.GetMode ().GetUniqueName (),
                      txVector.GetChannelWidth ());
}

uint64_t
S1gOfdmPhy::GetDataRate (const std::string& name, uint16_t channelWidth)
{
  WifiCodeRate codeRate = GetCodeRate (name);
  uint16_t constellationSize = GetConstellationSize (name);
  return CalculateDataRate (codeRate, constellationSize, channelWidth);
}

uint64_t
S1gOfdmPhy::CalculateDataRate (WifiCodeRate codeRate, uint16_t constellationSize, uint16_t channelWidth)
{
  double symbolDuration = 32; //in us
  uint16_t guardInterval = 8000; //in ns
  uint16_t usableSubCarriers = 0;
  switch (channelWidth)
  {
    case 1:
      usableSubCarriers = 24;
      break;
    case 2:
      usableSubCarriers = 52;
      break;
    case 4:
      usableSubCarriers = 108;
      break;
    case 8:
      usableSubCarriers = 234;
      break;
    case 16:
      usableSubCarriers = 468;
      break;
    default:
      NS_FATAL_ERROR ("trying to get code ratio for undefined channelWidth at " << channelWidth);
  }

  return CalculateDataRate (symbolDuration, guardInterval,
                            usableSubCarriers, static_cast<uint16_t> (log2 (constellationSize)),
                            GetCodeRatio (codeRate));
}

uint64_t
S1gOfdmPhy::CalculateDataRate (double symbolDuration, uint16_t guardInterval,
                            uint16_t usableSubCarriers, uint16_t numberOfBitsPerSubcarrier,
                            double codingRate)
{
  double symbolRate = (1 / (symbolDuration + (static_cast<double> (guardInterval) / 1000))) * 1e6;
  return lrint (ceil (symbolRate * usableSubCarriers * numberOfBitsPerSubcarrier * codingRate));
}


bool
S1gOfdmPhy::IsAllowed (const WifiTxVector& /*txVector*/)
{
  return true;
}

uint32_t
S1gOfdmPhy::GetMaxPsduSize (void) const
{
  return 4095;
}

} //namespace ns3

namespace {

/**
* Constructor class for S1G-OFDM modes
*/
static class ConstructorS1gOfdm
{
public:
    ConstructorS1gOfdm ()
    {
      ns3::S1gOfdmPhy::InitializeModes ();
      ns3::WifiPhy::AddStaticPhyEntity (ns3::WIFI_MOD_CLASS_S1G, ns3::Create<ns3::S1gOfdmPhy> ());
    }
} g_constructor_s1g_ofdm; ///< the constructor for S1G-OFDM modes

}
