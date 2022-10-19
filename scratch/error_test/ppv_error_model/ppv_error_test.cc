//
// Created by Zhouyou Gu on 6/07/22.
//
#include <fstream>
#include <cmath>
#include "ns3/gnuplot.h"
#include "ns3/command-line.h"
#include "ns3/ppv-error-rate-model.h"
#include "ns3/wifi-tx-vector.h"
#include "ns3/s1g-ofdm-phy.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("wifi-test");

int main (int argc, char *argv[]) {
    uint32_t size = 60*8;
    Ptr<PpvErrorRateModel> ppv = CreateObject<PpvErrorRateModel>();
    WifiTxVector txVector;
    std::vector<std::string> modes;
    auto table = S1gOfdmPhy::GetModulationLookupTable();
    auto bandtable = S1gOfdmPhy::GetModulationBandLookupTable();
    for (auto it = table.begin(); it != table.end(); it++) {
        std::cout << it->first << ":" << it->second.first << ":" << it->second.second << ":" << bandtable.at(it->first)<< std::endl;
        txVector.SetMode(it->first);
        txVector.SetChannelWidth(bandtable.at(it->first));
        for (double snr = -15.0; snr <= 20.; snr += 1) {
            double ps = ppv->GetChunkSuccessRate(WifiMode(it->first), txVector, std::pow(10.0, snr / 10.0), size);
            std::cout << it->first  << ":" << snr << ":" << ps << std::endl;
        }
    }
//    std::stringstream mode;
//    mode << "S1gOfdmRate" << "Mcs" << +beginMcs;
//    for (uint8_t mcs = (beginMcs + stepMcs); mcs < endMcs; mcs += stepMcs)
//      {
//        mode.str ("");
//        mode << format << "Mcs" << +mcs;
//        modes.push_back (mode.str ());
//      }
//    mode.str ("");
//    mode << format << "Mcs" << +endMcs;
//    modes.push_back (mode.str ());
//
//    for (uint32_t i = 0; i < modes.size (); i++)
//      {
//        std::cout << modes[i] << std::endl;
//        Gnuplot2dDataset yansdataset (modes[i]);
//        Gnuplot2dDataset nistdataset (modes[i]);
//        Gnuplot2dDataset tabledataset (modes[i]);
//        txVector.SetMode (modes[i]);
//
//        for (double snr = -5.0; snr <= (endMcs * 5); snr += 0.1)
//          {
//            double ps = yans->GetChunkSuccessRate (WifiMode (modes[i]), txVector, std::pow (10.0, snr / 10.0), size);
  return 0;
}
