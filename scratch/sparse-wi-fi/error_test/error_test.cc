//
// Created by Zhouyou Gu on 6/07/22.
//

#include <ns3/propagation-loss-model.h>
#include <ns3/propagation-module.h>
#include <ns3/point-to-point-helper.h>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/internet-module.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/ssid.h"
#include "ns3/wifi-mac.h"
#include "ns3/wifi-net-device.h"
#include "ns3/txop.h"
#include "ns3/wifi-mac-queue.h"
#include "ns3/ap-wifi-mac.h"
#include "ns3/sta-wifi-mac.h"

#include "ns3/udp-client.h"
#include "ns3/udp-server.h"
#include "ns3/boolean.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/ofdm-phy.h"
#include "ns3/ppv-error-rate-model.h"

#include <chrono>

#define UDP_IP_WIFI_HEADER_SIZE 64

using namespace ns3;

// Map of OFDM rates in bits per second for different bandwidths
std::map<uint16_t, std::array<uint64_t, 8>> s_ofdmRatesBpsList = {
    {20, {6000000, 9000000, 12000000, 18000000, 24000000, 36000000, 48000000, 54000000}}, // 20 MHz
    {10, {3000000, 4500000, 6000000, 9000000, 12000000, 18000000, 24000000, 27000000}}, // 10 MHz
    {5, {1500000, 2250000, 3000000, 4500000, 6000000, 9000000, 12000000, 13500000}} // 5 MHz
};

NS_LOG_COMPONENT_DEFINE ("wifi-test");

// Callback function for tx packet drop events
void
cb_tx_drop (Ptr<const Packet> packet)
{
  LlcSnapHeader llc;
  auto copy = packet->Copy ();
  copy->RemoveHeader (llc);
  Ipv4Header head;
  copy->PeekHeader (head);
  NS_LOG_UNCOND ("cb_tx_drop:" << Simulator::Now ().GetMicroSeconds () << "," << head.GetSource ()
                               << "," << head.GetDestination () << "," << packet->GetSize ());
}
// Callback function for tx packet succ events
void
cb_tx_succ (Ptr<const Packet> packet)
{
  LlcSnapHeader llc;
  auto copy = packet->Copy ();
  copy->RemoveHeader (llc);
  Ipv4Header head;
  copy->PeekHeader (head);
  NS_LOG_UNCOND ("cb_tx_succ:" << Simulator::Now ().GetMicroSeconds () << "," << head.GetSource ()
                               << "," << head.GetDestination () << "," << packet->GetSize ());
}

// Callback function for packet drop events
void
cb_rx_drop (Ptr<const Packet> packet)
{
  LlcSnapHeader llc;
  auto copy = packet->Copy ();
  copy->RemoveHeader (llc);
  Ipv4Header head;
  copy->PeekHeader (head);
  NS_LOG_UNCOND ("cb_rx_drop:" << Simulator::Now ().GetMicroSeconds () << "," << head.GetSource ()
                               << "," << head.GetDestination () << "," << packet->GetSize ());
}

// Callback function for successful packet reception
void
cb_rx_succ (Ptr<const Packet> packet)
{
  LlcSnapHeader llc;
  auto copy = packet->Copy ();
  copy->RemoveHeader (llc);
  Ipv4Header head;
  copy->PeekHeader (head);
  NS_LOG_UNCOND ("cb_rx_succ:" << Simulator::Now ().GetMicroSeconds () << "," << head.GetSource ()
                               << "," << head.GetDestination () << "," << packet->GetSize ());
}

// Callback function for station association events
void
cb_asso (uint16_t /* AID */ a, Mac48Address b)
{
  NS_LOG_UNCOND ("AssociatedSta:" << a << " : " << b
                                  << " time:" << Simulator::Now ().GetMicroSeconds ());
  std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now ();
  NS_LOG_UNCOND ("sim start:" << std::chrono::time_point_cast<std::chrono::microseconds> (begin)
                                     .time_since_epoch ()
                                     .count ());
}

// Callback function for station de-association events
void
cb_deasso (uint16_t /* AID */ a, Mac48Address b)
{
  NS_LOG_UNCOND ("DeassociatedSta:" << a << ":" << b
                                    << " time:" << Simulator::Now ().GetMicroSeconds ());
}

// Callback function for packet transmission start events
void
cb_tx_start (Ptr<const Packet> packet, double power)
{
  WifiMacHeader head;
  packet->PeekHeader (head);
  Mac48Address src = head.GetAddr2 ();
  if (head.GetType () == WIFI_MAC_DATA)
    {
      NS_LOG_UNCOND ("cb_tx_start:" << Simulator::Now ().GetMicroSeconds () << ","
                                    << head.GetAddr2 () << "," << head.GetAddr1 () << ","
                                    << packet->GetSize () << "," << head.GetSequenceNumber ());
    }
}

// Callback function for packet transmission end events
void
cb_tx_ended (Ptr<const Packet> packet)
{
  WifiMacHeader head;
  packet->PeekHeader (head);
  if (head.GetType () == WIFI_MAC_DATA)
    {
      NS_LOG_UNCOND ("cb_tx_end:" << Simulator::Now ().GetMicroSeconds () << "," << head.GetAddr2 ()
                                  << "," << head.GetAddr1 () << "," << packet->GetSize () << ","
                                  << head.GetSequenceNumber ());
    }
}

int
main (int argc, char *argv[])
{
  // Record simulation start time
  std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now ();
  NS_LOG_UNCOND ("sim start:" << std::chrono::time_point_cast<std::chrono::microseconds> (begin)
                                     .time_since_epoch ()
                                     .count ());

  // Enable logging components
  LogComponentEnableAll (LOG_PREFIX_TIME);
  LogComponentEnableAll (LOG_PREFIX_NODE);
  LogComponentEnable ("wifi-test", LOG_LEVEL_INFO);
  //   LogComponentEnable ("StaWifiMac", LOG_LEVEL_ALL);
  //   LogComponentEnable ("ApWifiMac", LOG_LEVEL_ALL);
  //   LogComponentEnable ("InterferenceHelper", LOG_LEVEL_ALL);
  //   LogComponentEnable ("YansWifiPhy", LOG_LEVEL_ALL);

  // Simulation parameters
  uint32_t MaxNumRetx = 5;
  std::string phyMode ("OfdmRate6Mbps");
  uint32_t packetSize = 100 - UDP_IP_WIFI_HEADER_SIZE; // Packet size in bytes
  uint32_t numPackets = 5; // Number of packets to send
  uint32_t interval_in_us = 10000; // Interval between packets in microseconds

  int n_ap = 1; // Number of Access Points (APs)
  int n_sta = 1; // Number of Stations (STAs

  float TxPower = 5.;
  float RxNoiseFigure = 5.0;
  float CcaEdThreshold = -95.0;
  float RxSensitivity = -95.0;
  float PreambleDetectionThresholdMinimumRssi = -95.0;

  uint32_t simSeed = 1000; // Simulation seed
  int simTime = 2; // Simulation time in seconds

  bool verbose = true;

  CommandLine cmd (__FILE__);
  cmd.AddValue ("MaxNumRetx", "MaxNumRetx", MaxNumRetx);
  cmd.AddValue ("phyMode", "Wifi Phy mode", phyMode);
  cmd.AddValue ("packetSize", "size of application packet sent", packetSize);
  cmd.AddValue ("numPackets", "number of packets generated", numPackets);
  cmd.AddValue ("interval_in_us", "interval between packets", interval_in_us);
  cmd.AddValue ("n_ap", "number of aps", n_ap);
  cmd.AddValue ("n_sta", "number of stations", n_sta);
  cmd.AddValue ("simSeed", "Seed for random generator. Default: 1", simSeed);
  cmd.AddValue ("simTime", "simulation time", simTime);
  cmd.AddValue ("TxPower", "TxPower", TxPower);
  cmd.AddValue ("RxNoiseFigure", "RxNoiseFigure", RxNoiseFigure);
  cmd.AddValue ("CcaEdThreshold", "CcaEdThreshold", CcaEdThreshold);
  cmd.AddValue ("RxSensitivity", "RxSensitivity", RxSensitivity);
  cmd.AddValue ("PreambleDetectionThresholdMinimumRssi", "PreambleDetectionThresholdMinimumRssi",
                PreambleDetectionThresholdMinimumRssi);

  //   cmd.AddValue ("openGymPort", "Port number for OpenGym env. Default: 5555", openGymPort);

  cmd.Parse (argc, argv);
  // Set random seed and run number
  RngSeedManager::SetSeed (1);
  RngSeedManager::SetRun (simSeed);

  double time_for_test_start = 1.0; // Time to start the test
  double time_for_test_end = time_for_test_start + simTime; // Time to end the test
  Time interval = MicroSeconds (interval_in_us); // Packet interval

  // Output simulation parameters
  std::cout << "MaxNumRetx:" << MaxNumRetx << std::endl;
  std::cout << "phyMode:" << phyMode << std::endl;
  std::cout << "packetSize:" << packetSize << std::endl;
  std::cout << "numPackets:" << numPackets << std::endl;
  std::cout << "interval_in_us:" << interval_in_us << std::endl;
  std::cout << "n_ap:" << n_ap << std::endl;
  std::cout << "n_sta:" << n_sta << std::endl;
  std::cout << "simSeed:" << simSeed << std::endl;
  std::cout << "simTime:" << simTime << std::endl;
  std::cout << "TxPower:" << TxPower << std::endl;
  std::cout << "RxNoiseFigure:" << RxNoiseFigure << std::endl;
  std::cout << "CcaEdThreshold:" << CcaEdThreshold << std::endl;
  std::cout << "RxSensitivity:" << RxSensitivity << std::endl;
  std::cout << "PreambleDetectionThresholdMinimumRssi:" << PreambleDetectionThresholdMinimumRssi
            << std::endl;

  std::cout << "time_for_test_start:" << time_for_test_start << std::endl;

  // Configure non-unicast data rate to be the same as unicast
  Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode", StringValue (phyMode));

  // Create node containers
  NodeContainer ap_nodes; // Access Points
  ap_nodes.Create (n_ap);

  NodeContainer sta_nodes; // Stations
  sta_nodes.Create (n_sta);

  NodeContainer server_node; // Server node
  server_node.Create (1);

  NodeContainer gate_node; // Gateway node
  gate_node.Create (1);

  /* Install Internet stack */
  InternetStackHelper stack;
  stack.Install (server_node);
  stack.Install (gate_node);
  stack.Install (ap_nodes);
  stack.Install (sta_nodes);
  Ipv4AddressGenerator::TestMode ();

  // Set up mobility for APs (fixed positions)
  MobilityHelper mobility_ap;
  Ptr<ListPositionAllocator> positionAlloc_ap = CreateObject<ListPositionAllocator> ();
  positionAlloc_ap->Add (Vector (0., 0., 0.0));
  mobility_ap.SetPositionAllocator (positionAlloc_ap);
  mobility_ap.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility_ap.Install (ap_nodes);

  // Set up mobility for STAs (fixed positions)
  float distance_to_ap = 10;
  MobilityHelper mobility_sta;
  Ptr<ListPositionAllocator> positionAlloc_sta = CreateObject<ListPositionAllocator> ();
  positionAlloc_sta->Add (Vector (-distance_to_ap, 0., 0.0));
  positionAlloc_sta->Add (Vector (distance_to_ap, 0., 0.0));
  mobility_sta.SetPositionAllocator (positionAlloc_sta);
  mobility_sta.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility_sta.Install (sta_nodes);

  // Set up Wi-Fi devices
  WifiHelper wifi;
  wifi.SetStandard (ns3::WIFI_STANDARD_80211a); // Set Wi-Fi standard to 802.11ah
  //   wifi.EnableLogComponents (); // Turn on all Wifi logging

  YansWifiPhyHelper wifiPhy;
  wifiPhy.Set ("ChannelSettings", StringValue ("{161, 20, BAND_5GHZ, 0}"));
  wifiPhy.Set ("CcaEdThreshold", DoubleValue (CcaEdThreshold));
  wifiPhy.Set ("RxNoiseFigure", DoubleValue (RxNoiseFigure));
  wifiPhy.Set ("RxSensitivity", DoubleValue (RxSensitivity));
  wifiPhy.Set ("TxPowerEnd", DoubleValue (TxPower));
  wifiPhy.Set ("TxPowerStart", DoubleValue (TxPower));
  wifiPhy.SetPreambleDetectionModel ("ns3::ThresholdPreambleDetectionModel", "MinimumRssi",
                                     DoubleValue (PreambleDetectionThresholdMinimumRssi));
  wifiPhy.SetPcapDataLinkType (WifiPhyHelper::DLT_IEEE802_11_RADIO);
  wifiPhy.SetErrorRateModel ("ns3::PpvErrorRateModel");

  // Create propagation loss model (Friis model)
  Ptr<FriisPropagationLossModel> lossModel = CreateObject<FriisPropagationLossModel> ();
  lossModel->SetFrequency (5.8e9); // Set frequency to 5.8 GHz

  // Create Wi-Fi channel
  Ptr<YansWifiChannel> yanswc = CreateObject<YansWifiChannel> ();
  yanswc->SetPropagationLossModel (lossModel);
  yanswc->SetPropagationDelayModel (CreateObject<ConstantSpeedPropagationDelayModel> ());
  wifiPhy.SetChannel (yanswc);

  // Set up MAC and disable rate control
  WifiMacHelper wifiMac;
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager", "DataMode", StringValue (phyMode),
                                "ControlMode", StringValue (phyMode), "MaxSsrc",
                                UintegerValue (MaxNumRetx), "MaxSlrc", UintegerValue (MaxNumRetx));

  // Set up the rest of the MAC
  Ssid ssid = Ssid ("wifi-default");

  // Configure AP devices
  wifiMac.SetType ("ns3::ApWifiMac", "Ssid", SsidValue (ssid), "QosSupported",
                   BooleanValue (false));
  NetDeviceContainer apDevice = wifi.Install (wifiPhy, wifiMac, ap_nodes);

  // Configure AP devices' settings
  for (uint32_t i = 0; i < n_ap; i++)
    {
      auto m = apDevice.Get (i);
      auto w = m->GetObject<WifiNetDevice> ();
      auto v = DynamicCast<ApWifiMac> (w->GetMac ());
      v->GetTxop ()->GetWifiMacQueue ()->SetMaxSize (QueueSize ("500p"));
      // Disable beacons since we manually associate sta
      v->SetBeaconGeneration (false);
    }

  // Configure STA devices
  wifiMac.SetType ("ns3::StaWifiMac", "Ssid", SsidValue (ssid), "QosSupported",
                   BooleanValue (false), "WaitBeaconTimeout",
                   TimeValue (MilliSeconds (200)), // Time to collect beacon
                   "AssocRequestTimeout", TimeValue (MilliSeconds (200)), "MaxMissedBeacons",
                   UintegerValue (100000));
  NetDeviceContainer staDevice = wifi.Install (wifiPhy, wifiMac, sta_nodes);

  // Configure STA devices' settings
  Time twtperiod = MicroSeconds (10000);
  for (uint32_t j = 0; j < n_sta; j++)
    {
      auto m = staDevice.Get (j);
      auto w = m->GetObject<WifiNetDevice> ();
      auto z = DynamicCast<StaWifiMac> (w->GetMac ());
      z->GetTxop ()->GetWifiMacQueue ()->SetMaxSize (QueueSize ("1p"));
      z->GetTxop ()->GetWifiMacQueue ()->SetMaxDelay (twtperiod);
      z->GetTxop ()->SetMinCw (15);
      z->GetTxop ()->SetMaxCw (127);
      // No need to scan the channel as we are manually associating the sta
      // z->SetAttribute ("scanningstartoffset", TimeValue (MilliSeconds (100) * (j + 1)));
    }

  double loss_step = 96.31379955930004;
  // Set up STA Modulation and Coding Scheme (MCS)
  for (uint32_t j = 0; j < n_sta; j++)
    {
      //Compute the noise floor
      auto m = staDevice.Get (j);
      auto w = m->GetObject<WifiNetDevice> ();
      auto v = DynamicCast<WifiPhy> (w->GetPhy ());
      uint16_t bw = v->GetChannelWidth ();
      double nf = DbToRatio (RxNoiseFigure);
      double BOLTZMANN = 1.3803e-23;
      double Nt = BOLTZMANN * 290 * bw * 1e6;
      double noiseFloor = nf * Nt;

      WifiTxVector txVector;
      Ptr<PpvErrorRateModel> ppv = CreateObject<PpvErrorRateModel> ();
      double pw = v->GetTxPowerStart ();
      auto bps_array = s_ofdmRatesBpsList[bw];
      for (const auto &bps : bps_array)
        {
          auto mode = OfdmPhy::GetOfdmRate (bps, bw);
          txVector.SetMode (mode);
          txVector.SetChannelWidth (bw);
          double ps = ppv->GetChunkSuccessRate (mode, txVector,
                                                DbToRatio (pw - loss_step - WToDbm (noiseFloor)),
                                                (packetSize + UDP_IP_WIFI_HEADER_SIZE) * 8);
          uint64_t rate = OfdmPhy::GetDataRate (mode.GetUniqueName (), bw);
          auto a = DynamicCast<StaWifiMac> (w->GetMac ());
          a->GetWifiRemoteStationManager ()->SetAttribute ("DataMode",
                                                           StringValue (mode.GetUniqueName ()));
          auto preambleDuration_us =
              v->CalculatePhyPreambleAndHeaderDuration (txVector).GetMicroSeconds ();
          auto payloadsDuration_us = v->GetPayloadDuration ((packetSize + UDP_IP_WIFI_HEADER_SIZE),
                                                            txVector, v->GetPhyBand ())
                                         .GetMicroSeconds ();

          std::cout << "STA:" << j << ", channel_width:" << txVector.GetChannelWidth ()
                    << ", error:" << 1 - ps << ", Gain:" << loss_step
                    << ", noiseFloor:" << WToDbm (noiseFloor)
                    << ", snr:" << pw - loss_step - WToDbm (noiseFloor)
                    << ", phyMode:" << mode.GetUniqueName ()
                    << ", tx data duration:" << payloadsDuration_us
                    << ", tx preamble duration:" << preambleDuration_us << std::endl;
        }
      //164 bytes payload is approximately with 224 us with the lowest data rate 6mbps
      //164 bytes payload is approximately with 28 us with the highest data rate 54mbps
      //preamble and header duration is 20 us
    }

    for (uint32_t j = 0; j < n_sta; j++)
    {
      double max_gain = -loss_step;

      //Compute the noise floor
      auto m = staDevice.Get (j);
      auto w = m->GetObject<WifiNetDevice> ();
      auto v = DynamicCast<WifiPhy> (w->GetPhy ());
      uint16_t bw = v->GetChannelWidth ();
      double nf = DbToRatio (RxNoiseFigure);
      double BOLTZMANN = 1.3803e-23;
      double Nt = BOLTZMANN * 290 * bw * 1e6;
      double noiseFloor = nf * Nt;

      WifiTxVector txVector;
      Ptr<PpvErrorRateModel> ppv = CreateObject<PpvErrorRateModel> ();
      double pw = v->GetTxPowerStart ();
      std::string max_mode_name ("OfdmRate6Mbps");
      auto max_mode = WifiMode (max_mode_name);
      uint64_t max_rate = OfdmPhy::GetDataRate (max_mode.GetUniqueName (), bw);
      auto bps_array = s_ofdmRatesBpsList[bw];
      for (const auto &bps : bps_array)
        {
          auto mode = OfdmPhy::GetOfdmRate (bps, bw);
          txVector.SetMode (mode);
          txVector.SetChannelWidth (bw);
          double ps = ppv->GetChunkSuccessRate (mode, txVector,
                                                DbToRatio (pw + max_gain - WToDbm (noiseFloor)),
                                                (packetSize + UDP_IP_WIFI_HEADER_SIZE) * 8);
          uint64_t rate = OfdmPhy::GetDataRate (mode.GetUniqueName (), bw);
          if (rate >= max_rate && ps > (1.0 - 1e-5))
            {
              max_rate = rate;
              max_mode_name = mode.GetUniqueName ();
              max_mode = mode;
            }
        }
      //164 bytes payload is approximately with 224 us with the lowest data rate 6mbps
      //164 bytes payload is approximately with 28 us with the highest data rate 54mbps
      //preamble and header duration is 20 us

      auto a = DynamicCast<StaWifiMac> (w->GetMac ());
      a->GetWifiRemoteStationManager ()->SetAttribute ("DataMode", StringValue (max_mode_name));
      auto preambleDuration_us =
          v->CalculatePhyPreambleAndHeaderDuration (txVector).GetMicroSeconds ();
      auto payloadsDuration_us =
          v->GetPayloadDuration ((packetSize + UDP_IP_WIFI_HEADER_SIZE), txVector, v->GetPhyBand ())
              .GetMicroSeconds ();

      std::cout << "STA:" << j << "-" << ", Gain: " << max_gain
                << "\n\t\t noiseFloor:" << WToDbm (noiseFloor)
                << ", snr:" << pw + max_gain - WToDbm (noiseFloor)
                << "\n\t\t phyMode:" << max_mode_name
                << "\n\t\t tx data duration:" << payloadsDuration_us
                << "\n\t\t tx preamble duration:" << preambleDuration_us << std::endl;

      a->SetAttribute ("twtguardtime",
                       TimeValue (MicroSeconds (preambleDuration_us + payloadsDuration_us)));
    }
  return 0;
}
