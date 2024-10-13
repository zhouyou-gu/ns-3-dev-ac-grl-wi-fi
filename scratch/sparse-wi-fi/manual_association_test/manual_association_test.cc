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
  LogComponentEnable ("StaWifiMac", LOG_LEVEL_ALL);
  LogComponentEnable ("ApWifiMac", LOG_LEVEL_ALL);

  // Simulation parameters
  std::string phyMode ("OfdmRate6Mbps");
  uint32_t packetSize = 100; // Packet size in bytes
  uint32_t numPackets = 100000; // Number of packets to send
  uint32_t interval_in_us = 20000; // Interval between packets in microseconds

  int n_ap = 1; // Number of Access Points (APs)
  int n_sta = 2; // Number of Stations (STAs

  float CcaEdThreshold = -95.0;
  float RxNoiseFigure = 10.0;
  float RxSensitivity = -95.0;
  float PreambleDetectionThresholdSnr = 0.;
  float PreambleDetectionThresholdMinimumRssi = -95.0;

  uint32_t simSeed = 1000; // Simulation seed
  int simTime = 2; // Simulation time in seconds

  bool verbose = true;

  // Set random seed and run number
  RngSeedManager::SetSeed (6);
  RngSeedManager::SetRun (simSeed);

  double time_for_test_start = 1.0; // Time to start the test
  double time_for_test_end = time_for_test_start + simTime; // Time to end the test
  Time interval = MicroSeconds (interval_in_us); // Packet interval

  // Output simulation parameters
  std::cout << "packetSize:" << packetSize << std::endl;
  std::cout << "numPackets:" << numPackets << std::endl;
  std::cout << "interval_in_us:" << interval_in_us << std::endl;
  std::cout << "n_ap:" << n_ap << std::endl;
  std::cout << "n_sta:" << n_sta << std::endl;
  std::cout << "simSeed:" << simSeed << std::endl;
  std::cout << "simTime:" << simTime << std::endl;
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
  wifiPhy.SetPreambleDetectionModel ("ns3::ThresholdPreambleDetectionModel", "Threshold",
                                     DoubleValue (PreambleDetectionThresholdSnr), "MinimumRssi",
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
                                "ControlMode", StringValue (phyMode));

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
  for (uint32_t j = 0; j < n_sta; j++)
    {
      auto m = staDevice.Get (j);
      auto w = m->GetObject<WifiNetDevice> ();
      auto v = DynamicCast<WifiPhy> (w->GetPhy ());
      v->SetTxPowerStart (0.0);
      v->SetTxPowerEnd (0.0);
      auto z = DynamicCast<StaWifiMac> (w->GetMac ());
      z->GetTxop ()->GetWifiMacQueue ()->SetMaxSize (QueueSize ("1p"));
      z->SetAttribute ("scanningstartoffset", TimeValue (MilliSeconds (100) * (j + 1)));
    }

  // Note: The variable 'twt_start_time' is used but not defined in the original code.
  // Ensure it is defined if you intend to use it.
  /*
    for (int i = 0; i < n_sta; i++) {
        auto m = staDevice.Get(i);
        auto w = m->GetObject<WifiNetDevice>();
        auto v = DynamicCast<StaWifiMac>(w->GetMac());
        v->SetAttribute("twtstarttime", TimeValue(MicroSeconds(twt_start_time * 1000000)));
        v->SetAttribute("twtoffset", TimeValue(MicroSeconds((i % 4) * 10000)));
        v->SetAttribute("twtduration", TimeValue(MicroSeconds(10000)));
        v->SetAttribute("twtperiodicity", TimeValue(MicroSeconds(40000)));
    }
    */

  // Set up tracing if verbose
  if (verbose)
    {
      for (uint32_t i = 0; i < n_sta; i++)
        {
          auto m = staDevice.Get (i);
          auto w = m->GetObject<WifiNetDevice> ();
          auto v = DynamicCast<WifiPhy> (w->GetPhy ());
          v->TraceConnectWithoutContext ("PhyTxBegin", MakeCallback (&cb_tx_start));
          v->TraceConnectWithoutContext ("PhyTxEnd", MakeCallback (&cb_tx_ended));
        }
      for (uint32_t i = 0; i < n_sta; i++)
        {
          auto m = staDevice.Get (i);
          auto w = m->GetObject<WifiNetDevice> ();
          auto v = w->GetMac ();
          v->TraceConnectWithoutContext ("MacTx", MakeCallback (&cb_tx_succ));
          v->TraceConnectWithoutContext ("MacTxDrop", MakeCallback (&cb_tx_drop));
          v->TraceConnectWithoutContext ("MacRx", MakeCallback (&cb_rx_succ));
          v->TraceConnectWithoutContext ("MacRxDrop", MakeCallback (&cb_rx_drop));
        }
      for (int i = 0; i < n_ap; ++i)
        {
          auto m = apDevice.Get (i);
          auto w = m->GetObject<WifiNetDevice> ();
          auto v = w->GetMac ();
          v->TraceConnectWithoutContext ("MacTx", MakeCallback (&cb_tx_succ));
          v->TraceConnectWithoutContext ("MacTxDrop", MakeCallback (&cb_tx_drop));
          v->TraceConnectWithoutContext ("MacRx", MakeCallback (&cb_rx_succ));
          v->TraceConnectWithoutContext ("MacRxDrop", MakeCallback (&cb_rx_drop));
        }
    }

  // Set up Point-to-Point (P2P) links between server and gateway
  PointToPointHelper p2p;
  p2p.SetDeviceAttribute ("DataRate", StringValue ("100Gbps"));
  p2p.SetChannelAttribute ("Delay", StringValue ("0ms"));
  Ipv4AddressHelper server_address;
  server_address.SetBase ("10.1.0.0", "255.255.0.0");
  auto server_to_gate_netd = p2p.Install (server_node.Get (0), gate_node.Get (0));
  auto server_to_gate_intf = server_address.Assign (server_to_gate_netd);

  // Set up P2P links between gateway and APs
  NetDeviceContainer p2p_ap_to_gate;
  Ipv4InterfaceContainer if_ap_to_gate;
  for (int i = 0; i < n_ap; ++i)
    {
      std::stringstream a;
      a << "" << 11 << "." << i + 1 << ".0.0";
      std::cout << "ap base: " << i << " " << a.str () << " " << Ipv4Address (a.str ().c_str ())
                << std::endl;
      Ipv4AddressHelper p2p_address;
      p2p_address.SetBase (a.str ().c_str (), "255.255.0.0");
      auto d = p2p.Install (gate_node.Get (0), ap_nodes.Get (i));
      auto interface = p2p_address.Assign (d);
      p2p_ap_to_gate.Add (d.Get (1));
      if_ap_to_gate.Add (interface.Get (1));
    }

  // Assign IP addresses to AP interfaces
  Ipv4InterfaceContainer apInterface;
  for (int i = 0; i < n_ap; ++i)
    {
      Ipv4AddressHelper ap_address;
      ap_address.SetBase ("10.2.0.0", "255.255.0.0");
      auto a = ap_address.Assign (apDevice.Get (i));
      apInterface.Add (a);
    }

  // Assign IP addresses to STA interfaces
  Ipv4AddressHelper sta_address;
  sta_address.SetBase ("10.3.0.0", "255.255.0.0");
  Ipv4InterfaceContainer staInterface = sta_address.Assign (staDevice);

  uint16_t port_off_set = 1000;
  ApplicationContainer ServerApps;

  // Install UDP Server applications on the server node
  for (int i = 0; i < n_sta; ++i)
    {
      uint16_t port = i + port_off_set;
      Ptr<UdpServer> r = CreateObject<UdpServer> ();
      r->SetAttribute ("Port", UintegerValue (port));
      r->SetStartTime (Seconds (time_for_test_start));
      r->SetStopTime (Seconds (time_for_test_end));
      server_node.Get (0)->AddApplication (r);
      ServerApps.Add (r);
    }

  // Install UDP Client applications on the STA nodes
  for (int i = 0; i < n_sta; ++i)
    {
      uint16_t port = i + port_off_set;
      Ptr<UdpClient> s = CreateObject<UdpClient> ();
      s->SetRemote (server_to_gate_intf.GetAddress (0), port);
      s->SetAttribute ("MaxPackets", UintegerValue (numPackets));
      s->SetAttribute ("Interval", TimeValue (interval));
      s->SetAttribute ("PacketSize", UintegerValue (packetSize));
      s->SetStartTime (Seconds (time_for_test_start));
      s->SetStopTime (Seconds (time_for_test_end));
      sta_nodes.Get (i)->AddApplication (s);
    }

  // Set up static routing
  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  Ptr<Ipv4StaticRouting> gate_routing =
      ipv4RoutingHelper.GetStaticRouting (gate_node.Get (0)->GetObject<Ipv4> ());
  gate_routing->AddHostRouteTo (server_to_gate_intf.GetAddress (0),
                                server_to_gate_intf.Get (0).second);

  for (int i = 0; i < n_ap; ++i)
    {
      Ptr<Ipv4StaticRouting> ap_r =
          ipv4RoutingHelper.GetStaticRouting (ap_nodes.Get (i)->GetObject<Ipv4> ());
      ap_r->AddHostRouteTo (server_to_gate_intf.GetAddress (0), if_ap_to_gate.Get (i).second);
      ap_r->AddHostRouteTo (server_to_gate_intf.GetAddress (1), if_ap_to_gate.Get (i).second);
    }

  for (int i = 0; i < n_sta; ++i)
    {
      Ptr<Ipv4StaticRouting> sta_r =
          ipv4RoutingHelper.GetStaticRouting (sta_nodes.Get (i)->GetObject<Ipv4> ());
      sta_r->AddHostRouteTo (server_to_gate_intf.GetAddress (0), staInterface.Get (i).second);
      sta_r->AddHostRouteTo (server_to_gate_intf.GetAddress (1), staInterface.Get (i).second);
    }

  // Initialize ARP caches for STAs
  for (int j = 0; j < n_sta; j++)
    {
      std::vector<double> gain_list;
      for (int i = 0; i < n_ap; i++)
        {
          auto A = ap_nodes.Get (i)->GetObject<MobilityModel> ();
          auto B = sta_nodes.Get (j)->GetObject<MobilityModel> ();
          double g = lossModel->CalcRxPower (0, B, A);
          gain_list.push_back (g);
          std::cout << "sta: " << j << " ap:" << i << " " << g << std::endl;
        }
      auto result = std::max_element (gain_list.begin (), gain_list.end ());
      int ind = std::distance (gain_list.begin (), result);
      Mac48Address addr = Mac48Address::ConvertFrom (apDevice.Get (ind)->GetAddress ());
      Ptr<ArpCache> arp = CreateObject<ArpCache> ();
      arp->SetAliveTimeout (Seconds (3600 * 24 * 365)); // Set ARP cache timeout
      ArpCache::Entry *entry = arp->Add (server_to_gate_intf.GetAddress (0));
      Ipv4Header ipv4Hdr;
      ipv4Hdr.SetDestination (server_to_gate_intf.GetAddress (0));
      Ptr<Packet> p = Create<Packet> (100);
      entry->MarkWaitReply (ArpCache::Ipv4PayloadHeaderPair (p, ipv4Hdr));
      entry->MarkAlive (addr);

      Ptr<Ipv4L3Protocol> ip = sta_nodes.Get (j)->GetObject<Ipv4L3Protocol> ();
      NS_ASSERT (ip != 0);
      ObjectVectorValue interfaces;
      ip->GetAttribute ("InterfaceList", interfaces);
      for (ObjectVectorValue::Iterator p = interfaces.Begin (); p != interfaces.End (); p++)
        {
          Ptr<Ipv4Interface> ipIface = (*p).second->GetObject<Ipv4Interface> ();
          ipIface->SetAttribute ("ArpCache", PointerValue (arp));
        }
      std::cout << "STA: " << j << ", arp: " << addr << std::endl;
    }

  // Set up STA Modulation and Coding Scheme (MCS)
  for (uint32_t j = 0; j < n_sta; j++)
    {
      double max_gain = -std::numeric_limits<double>::infinity ();
      uint32_t max_i = 0;
      for (uint32_t i = 0; i < n_ap; i++)
        {
          double gain = lossModel->CalcRxPower (0, ap_nodes.Get (i)->GetObject<MobilityModel> (),
                                                sta_nodes.Get (j)->GetObject<MobilityModel> ());
          if (gain >= max_gain)
            {
              max_gain = gain;
              max_i = i;
            }
        }
      WifiTxVector txVector;
      Ptr<PpvErrorRateModel> ppv = CreateObject<PpvErrorRateModel> ();
      auto m = staDevice.Get (j);
      auto w = m->GetObject<WifiNetDevice> ();
      auto v = DynamicCast<WifiPhy> (w->GetPhy ());
      uint16_t bw = v->GetChannelWidth ();
      double pw = v->GetTxPowerStart ();
      double nf = DbToRatio (10.0);
      std::string max_mode ("OfdmRate6Mbps");
      uint64_t max_rate = 0;
      double BOLTZMANN = 1.3803e-23;
      double Nt = BOLTZMANN * 290 * bw * 1e6;
      double noiseFloor = nf * Nt;
      auto bps_array = s_ofdmRatesBpsList[bw];
      for (const auto &bps : bps_array)
        {
          auto mode = OfdmPhy::GetOfdmRate (bps, bw);
          txVector.SetMode (mode);
          txVector.SetChannelWidth (bw);
          double ps = ppv->GetChunkSuccessRate (
              mode, txVector, std::pow (10.0, (pw + max_gain - WToDbm (noiseFloor)) / 10.0),
              (packetSize + UDP_IP_WIFI_HEADER_SIZE) * 8);
          uint64_t rate = OfdmPhy::GetDataRate (mode.GetUniqueName (), bw);
          if (rate >= max_rate && ps > (1.0 - 1e-5))
            {
              max_rate = rate;
              max_mode = mode.GetUniqueName ();
            }
        }
      auto a = DynamicCast<StaWifiMac> (w->GetMac ());
      a->GetWifiRemoteStationManager ()->SetAttribute ("DataMode", StringValue (max_mode));
      std::cout << "STA:" << j << "-" << "AP:" << max_i << ", Gain: " << max_gain
                << "\n\t\t noiseFloor:" << WToDbm (noiseFloor) << " phyMode:" << max_mode
                << std::endl;
    }

  // Manually associate STA-AP
  for (uint32_t j = 0; j < n_sta; j++)
    {
      double max_gain = -std::numeric_limits<double>::infinity ();
      uint32_t max_i = 0;
      for (uint32_t i = 0; i < n_ap; i++)
        {
          double gain = lossModel->CalcRxPower (0, ap_nodes.Get (i)->GetObject<MobilityModel> (),
                                                sta_nodes.Get (j)->GetObject<MobilityModel> ());
          if (gain >= max_gain)
            {
              max_gain = gain;
              max_i = i;
            }
        }
      auto sd = staDevice.Get (j);
      auto sd_w = sd->GetObject<WifiNetDevice> ();
      auto s = DynamicCast<StaWifiMac> (sd_w->GetMac ());
      auto s_addr = s->GetAddress ();

      auto ad = apDevice.Get (max_i);
      auto ad_w = ad->GetObject<WifiNetDevice> ();
      auto a = DynamicCast<ApWifiMac> (ad_w->GetMac ());
      auto a_addr = a->GetAddress ();

      s->ManualAssoUpdateApInfo (a->ManualAssoGetProbRsp (), a->GetAddress (), a->GetBssid ());
      auto assoReq = s->ManualAssoGetAssoReq ();
      a->ManualAssoSetAssoReq (assoReq, s_addr);
      auto assoRsp = a->ManualAssoGetAssoRsp (s_addr);
      s->ManualAssoSetAssoRsp (assoRsp, a_addr);
      a->ManualAssoGetAssoRspTxOk (s_addr);
    }

  // Set simulation stop time
  Simulator::Stop (Seconds (time_for_test_end + 0.1));
  std::cout << "Sim Start" << std::endl;

  // Run the simulation
  Simulator::Run ();

  // Output simulation results for STAs
  for (int i = 0; i < n_sta; ++i)
    {
      auto m = staDevice.Get (i);
      auto w = m->GetObject<WifiNetDevice> ();
      auto v = w->GetMac ();
      std::cout << "STA: " << i << ", bssid: " << v->GetBssid () << std::endl;
      std::cout << "STA: " << i << ", addr: " << v->GetAddress () << std::endl;
      std::cout << "STA: " << i
                << ", queue size: " << (v->GetTxop ())->GetWifiMacQueue ()->GetMaxSize ()
                << std::endl;
      std::cout << "STA: " << i << ", queue drop: "
                << (v->GetTxop ())->GetWifiMacQueue ()->GetTotalDroppedPackets () << std::endl;
      std::cout << "STA: " << i << ", queue srxp: "
                << (v->GetTxop ())->GetWifiMacQueue ()->GetTotalReceivedPackets () << std::endl;
      auto r = ServerApps.Get (i);
      std::cout << "STA: " << i
                << ", X-Y: " << sta_nodes.Get (i)->GetObject<MobilityModel> ()->GetPosition ().x
                << "\t," << sta_nodes.Get (i)->GetObject<MobilityModel> ()->GetPosition ().y
                << std::endl;
      std::cout << "   packets received " << DynamicCast<UdpServer> (r)->GetReceived ()
                << std::endl;
      std::cout << "   bits received "
                << DynamicCast<UdpServer> (r)->GetReceived () *
                       (packetSize + UDP_IP_WIFI_HEADER_SIZE) * 8
                << std::endl;
      std::cout << "   aoi received " << DynamicCast<UdpServer> (r)->GetLastAoI_us () << std::endl;
      std::cout << "   delay received " << DynamicCast<UdpServer> (r)->GetAvgDelay_us ()
                << std::endl;
      std::cout << "   interval received " << DynamicCast<UdpServer> (r)->GetAvgInterval_us ()
                << std::endl;
    }

  // Output simulation results for APs
  for (int i = 0; i < n_ap; ++i)
    {
      auto m = apDevice.Get (i);
      auto w = m->GetObject<WifiNetDevice> ();
      auto v = w->GetMac ();
      std::cout << "AP: " << i << ", bssid: " << v->GetBssid () << std::endl;
      std::cout << "AP: " << i
                << ", queue size: " << (v->GetTxop ())->GetWifiMacQueue ()->GetMaxSize ()
                << std::endl;
      std::cout << "AP: " << i << ", queue drop: "
                << (v->GetTxop ())->GetWifiMacQueue ()->GetTotalDroppedPackets () << std::endl;
      std::cout << "AP: " << i << ", queue totp: "
                << (v->GetTxop ())->GetWifiMacQueue ()->GetTotalReceivedPackets () << std::endl;
    }

  std::cout << "server ip: " << server_to_gate_intf.GetAddress (0) << std::endl;
  std::cout << "server ip: " << server_to_gate_intf.GetAddress (1) << std::endl;

  // Output propagation loss information if verbose
  if (verbose)
    {
      for (uint32_t i = 0; i < n_ap; i++)
        {
          for (uint32_t j = 0; j < n_sta; j++)
            {
              auto A = ap_nodes.Get (i)->GetObject<MobilityModel> ();
              auto B = sta_nodes.Get (j)->GetObject<MobilityModel> ();
              std::cout << "loss_sta_ap: " << lossModel->CalcRxPower (0, B, A) << " "
                        << lossModel->CalcRxPower (0, A, B) << std::endl;
            }
        }
      for (uint32_t i = 0; i < n_sta; i++)
        {
          for (uint32_t j = 0; j < n_sta; j++)
            {
              auto A = sta_nodes.Get (i)->GetObject<MobilityModel> ();
              auto B = sta_nodes.Get (j)->GetObject<MobilityModel> ();
              std::cout << "loss_sta_sta: " << lossModel->CalcRxPower (0, B, A) << " "
                        << lossModel->CalcRxPower (0, A, B) << std::endl;
            }
        }
      for (uint32_t i = 0; i < n_ap; i++)
        {
          for (uint32_t j = 0; j < n_ap; j++)
            {
              auto A = ap_nodes.Get (i)->GetObject<MobilityModel> ();
              auto B = ap_nodes.Get (j)->GetObject<MobilityModel> ();
              std::cout << "loss_ap_ap: " << lossModel->CalcRxPower (0, B, A) << " "
                        << lossModel->CalcRxPower (0, A, B) << std::endl;
            }
        }
    }

  // Clean up and end simulation
  Simulator::Destroy ();

  // Record simulation end time
  std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now ();
  NS_LOG_UNCOND ("sim end:" << std::chrono::time_point_cast<std::chrono::microseconds> (end)
                                   .time_since_epoch ()
                                   .count ());
  return 0;
}
