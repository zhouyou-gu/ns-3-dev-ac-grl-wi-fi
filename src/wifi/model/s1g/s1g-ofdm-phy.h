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

#ifndef S1G_OFDM_PHY_H
#define S1G_OFDM_PHY_H

#include "ns3/ofdm-phy.h"

/**
 * \file
 * \ingroup wifi
 * Declaration of ns3::S1gOfdmPhy class.
 */

namespace ns3 {

/**
 * \brief PHY entity for S1G-OFDM (11g)
 * \ingroup wifi
 *
 * S1G-OFDM PHY is based on OFDM PHY.
 * S1G-DSSS/CCK mode is not supported.
 *
 * Refer to IEEE 802.11-2016, clause 18.
 */
    class S1gOfdmPhy : public OfdmPhy
    {
    public:
        /**
         * Constructor for S1G-OFDM PHY
         */
        S1gOfdmPhy ();
        /**
         * Destructor for S1G-OFDM PHY
         */
        virtual ~S1gOfdmPhy ();
        WifiMode GetSigMode (WifiPpduField field, const WifiTxVector& txVector) const override;
        const PpduFormats & GetPpduFormats (void) const override;
        Time GetDuration (WifiPpduField field, const WifiTxVector& txVector) const override;
        Time GetPayloadDuration (uint32_t size, const WifiTxVector& txVector, WifiPhyBand band, MpduType mpdutype,
                                 bool incFlag, uint32_t &totalAmpduSize, double &totalAmpduNumSymbols,
                                 uint16_t staId) const override;

        Ptr<WifiPpdu> BuildPpdu (const WifiConstPsduMap & psdus, const WifiTxVector& txVector, Time ppduDuration) override;
        uint32_t GetMaxPsduSize (void) const override;

        PhyFieldRxStatus DoEndReceiveField (WifiPpduField field, Ptr<Event> event) override;

        static Time GetSigADuration (WifiPreamble preamble) ;
        static Time GetSigBDuration (WifiPreamble preamble) ;

        /**
         * Initialize all S1G-OFDM modes.
         */
        static void InitializeModes (void);
        /**
         * Return a WifiMode for S1G-OFDM
         * corresponding to the provided rate.
         *
         * \param rate the rate in bps
         * \return a WifiMode for S1G-OFDM
         */
        static WifiMode GetS1gOfdmRate (uint64_t rate, uint16_t bw = 1);


        static WifiMode GetS1gOfdmRate0_30MbpsBW1MHz ();
        static WifiMode GetS1gOfdmRate0_60MbpsBW1MHz ();
        static WifiMode GetS1gOfdmRate0_90MbpsBW1MHz ();
        static WifiMode GetS1gOfdmRate1_20MbpsBW1MHz ();
        static WifiMode GetS1gOfdmRate1_80MbpsBW1MHz ();
        static WifiMode GetS1gOfdmRate2_40MbpsBW1MHz ();
        static WifiMode GetS1gOfdmRate2_70MbpsBW1MHz ();
        static WifiMode GetS1gOfdmRate3_00MbpsBW1MHz ();
        static WifiMode GetS1gOfdmRate3_60MbpsBW1MHz ();
        static WifiMode GetS1gOfdmRate4_00MbpsBW1MHz ();
        static WifiMode GetS1gOfdmRate0_15MbpsBW1MHz ();
        static WifiMode GetS1gOfdmRate0_65MbpsBW2MHz ();
        static WifiMode GetS1gOfdmRate1_30MbpsBW2MHz ();
        static WifiMode GetS1gOfdmRate1_95MbpsBW2MHz ();
        static WifiMode GetS1gOfdmRate2_60MbpsBW2MHz ();
        static WifiMode GetS1gOfdmRate3_90MbpsBW2MHz ();
        static WifiMode GetS1gOfdmRate5_20MbpsBW2MHz ();
        static WifiMode GetS1gOfdmRate5_85MbpsBW2MHz ();
        static WifiMode GetS1gOfdmRate6_50MbpsBW2MHz ();
        static WifiMode GetS1gOfdmRate7_80MbpsBW2MHz ();
        static WifiMode GetS1gOfdmRate1_35MbpsBW4MHz ();
        static WifiMode GetS1gOfdmRate2_70MbpsBW4MHz ();
        static WifiMode GetS1gOfdmRate4_05MbpsBW4MHz ();
        static WifiMode GetS1gOfdmRate5_40MbpsBW4MHz ();
        static WifiMode GetS1gOfdmRate8_10MbpsBW4MHz ();
        static WifiMode GetS1gOfdmRate10_80MbpsBW4MHz ();
        static WifiMode GetS1gOfdmRate12_20MbpsBW4MHz ();
        static WifiMode GetS1gOfdmRate13_50MbpsBW4MHz ();
        static WifiMode GetS1gOfdmRate16_20MbpsBW4MHz ();
        static WifiMode GetS1gOfdmRate18_00MbpsBW4MHz ();
        static WifiMode GetS1gOfdmRate2_93MbpsBW8MHz ();
        static WifiMode GetS1gOfdmRate5_85MbpsBW8MHz ();
        static WifiMode GetS1gOfdmRate8_78MbpsBW8MHz ();
        static WifiMode GetS1gOfdmRate11_70MbpsBW8MHz ();
        static WifiMode GetS1gOfdmRate17_60MbpsBW8MHz ();
        static WifiMode GetS1gOfdmRate23_40MbpsBW8MHz ();
        static WifiMode GetS1gOfdmRate26_30MbpsBW8MHz ();
        static WifiMode GetS1gOfdmRate29_30MbpsBW8MHz ();
        static WifiMode GetS1gOfdmRate35_10MbpsBW8MHz ();
        static WifiMode GetS1gOfdmRate39_00MbpsBW8MHz ();
        static WifiMode GetS1gOfdmRate5_85MbpsBW16MHz ();
        static WifiMode GetS1gOfdmRate11_70MbpsBW16MHz ();
        static WifiMode GetS1gOfdmRate17_60MbpsBW16MHz ();
        static WifiMode GetS1gOfdmRate23_40MbpsBW16MHz ();
        static WifiMode GetS1gOfdmRate35_10MbpsBW16MHz ();
        static WifiMode GetS1gOfdmRate46_80MbpsBW16MHz ();
        static WifiMode GetS1gOfdmRate52_70MbpsBW16MHz ();
        static WifiMode GetS1gOfdmRate58_50MbpsBW16MHz ();
        static WifiMode GetS1gOfdmRate70_20MbpsBW16MHz ();
        static WifiMode GetS1gOfdmRate78_00MbpsBW16MHz ();

        /**
         * Return the WifiCodeRate from the S1G-OFDM mode's unique name using
         * ModulationLookupTable. This is mainly used as a callback for
         * WifiMode operation.
         *
         * \param name the unique name of the S1G-OFDM mode
         * \return WifiCodeRate corresponding to the unique name
         */
        static WifiCodeRate GetCodeRate (const std::string& name);
        /**
         * Return the constellation size from the S1G-OFDM mode's unique name using
         * ModulationLookupTable. This is mainly used as a callback for
         * WifiMode operation.
         *
         * \param name the unique name of the S1G-OFDM mode
         * \return constellation size corresponding to the unique name
         */
        static uint16_t GetConstellationSize (const std::string& name);
        /**
         * Return the PHY rate from the S1G-OFDM mode's unique name and
         * the supplied parameters. This function calls OfdmPhy::CalculatePhyRate
         * and is mainly used as a callback for WifiMode operation.
         *
         * \param name the unique name of the S1G-OFDM mode
         * \param channelWidth the considered channel width in MHz
         *
         * \return the physical bit rate of this signal in bps.
         */
        static uint64_t GetPhyRate (const std::string& name, uint16_t channelWidth);
        static uint64_t CalculatePhyRate (WifiCodeRate codeRate, uint64_t dataRate);
        static double   GetCodeRatio (WifiCodeRate codeRate);
        static uint64_t CalculateDataRate (WifiCodeRate codeRate, uint16_t constellationSize, uint16_t channelWidth);
        static uint64_t CalculateDataRate (double symbolDuration, uint16_t guardInterval, uint16_t usableSubCarriers, uint16_t numberOfBitsPerSubcarrier,double codingRate);

        /**
         * Return the PHY rate corresponding to
         * the supplied TXVECTOR.
         * This function is mainly used as a callback
         * for WifiMode operation.
         *
         * \param txVector the TXVECTOR used for the transmission
         * \param staId the station ID (only here to have a common signature for all callbacks)
         * \return the physical bit rate of this signal in bps.
         */
        static uint64_t GetPhyRateFromTxVector (const WifiTxVector& txVector, uint16_t staId);
        /**
         * Return the data rate corresponding to
         * the supplied TXVECTOR.
         * This function is mainly used as a callback
         * for WifiMode operation.
         *
         * \param txVector the TXVECTOR used for the transmission
         * \param staId the station ID (only here to have a common signature for all callbacks)
         * \return the data bit rate in bps.
         */
        static uint64_t GetDataRateFromTxVector (const WifiTxVector& txVector, uint16_t staId);
        /**
         * Return the data rate from the S1G-OFDM mode's unique name and
         * the supplied parameters. This function calls OfdmPhy::CalculateDataRate
         * and is mainly used as a callback for WifiMode operation.
         *
         * \param name the unique name of the S1G-OFDM mode
         * \param channelWidth the considered channel width in MHz
         *
         * \return the data bit rate of this signal in bps.
         */
        static uint64_t GetDataRate (const std::string& name, uint16_t channelWidth);
        /**
         * Check whether the combination in TXVECTOR is allowed.
         * This function is used as a callback for WifiMode operation.
         *
         * \param txVector the TXVECTOR
         * \returns true if this combination is allowed, false otherwise.
         */
        static bool IsAllowed (const WifiTxVector& txVector);

    private:
        WifiMode GetHeaderMode (const WifiTxVector& txVector) const override;
        Time GetPreambleDuration (const WifiTxVector& txVector) const override;
        Time GetHeaderDuration (const WifiTxVector& txVector) const override;

        /**
         * Create an S1G-OFDM mode from a unique name, the unique name
         * must already be contained inside ModulationLookupTable.
         * This method binds all the callbacks used by WifiMode.
         *
         * \param uniqueName the unique name of the WifiMode
         * \param isMandatory whether the WifiMode is mandatory
         * \return the S1G-OFDM WifiMode
         */
        static WifiMode CreateS1gOfdmMode (std::string uniqueName, bool isMandatory);
        static const PpduFormats m_ofdmPpduFormats; //!< OFDM PPDU formats

        static const ModulationLookupTable m_s1gOfdmModulationLookupTable; //!< lookup table to retrieve code rate and constellation size corresponding to a unique name of modulation
    }; //class S1gOfdmPhy

} //namespace ns3

#endif /* S1G_OFDM_PHY_H */
