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
#include "ns3/s1g-ofdm-phy.h"
#include "ns3/ppv-error-rate-model.h"
#include "mygym.h"

#define UDP_IP_WIFI_HEADER_SIZE 64

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("wifi-test");

void cb_rx_drop(Ptr<const Packet> packet){
  LlcSnapHeader llc;
  auto copy = packet->Copy();
  copy->RemoveHeader (llc);
  Ipv4Header head;
  copy->PeekHeader (head);

  NS_LOG_UNCOND("cb_rx_drop:" << (Simulator::Now()).GetMicroSeconds() << "," << head.GetSource() << "," << head.GetDestination()<<"," << packet->GetSize());

}
void cb_rx_succ(Ptr<const Packet> packet){
  LlcSnapHeader llc;
  auto copy = packet->Copy();
  copy->RemoveHeader (llc);
  Ipv4Header head;
  copy->PeekHeader (head);

  NS_LOG_UNCOND("cb_rx_succ:" << (Simulator::Now()).GetMicroSeconds() << "," << head.GetSource() << "," << head.GetDestination()<<"," << packet->GetSize());

}

void cb_asso(uint16_t /* AID */ a, Mac48Address b){
  NS_LOG_UNCOND("AssociatedSta:" << a << ":" << b  <<  " time:" << (Simulator::Now()).GetMicroSeconds() );
}
void cb_deasso(uint16_t /* AID */ a, Mac48Address b){
  NS_LOG_UNCOND("DeassociatedSta:" << a << ":" << b  <<  " time:" << (Simulator::Now()).GetMicroSeconds() );
}
void cb_tx_start(Ptr<const Packet> packet, double power){
    WifiMacHeader head;
    packet->PeekHeader (head);
    Mac48Address src = head.GetAddr2 ();
    if (head.GetType () == WIFI_MAC_DATA)
      {
          NS_LOG_UNCOND("cb_tx_start:" << (Simulator::Now()).GetMicroSeconds() << "," << head.GetAddr2 () << "," << head.GetAddr1 () <<"," << packet->GetSize() << "," << head.GetSequenceNumber());
      }
}
void cb_tx_ended(Ptr<const Packet> packet){
    WifiMacHeader head;
    packet->PeekHeader (head);
    if (head.GetType () == WIFI_MAC_DATA)
      {
          NS_LOG_UNCOND("cb_tx_end:" << (Simulator::Now()).GetMicroSeconds() << "," << head.GetAddr2 () << "," << head.GetAddr1 ()<<"," << packet->GetSize() << "," << head.GetSequenceNumber());
      }
}

int main (int argc, char *argv[])
{
  LogComponentEnableAll (LOG_PREFIX_TIME);
  LogComponentEnableAll (LOG_PREFIX_NODE);
  LogComponentEnable ("wifi-test", LOG_LEVEL_INFO);
//  LogComponentEnable ("InterferenceHelper", ns3::LOG_LEVEL_WARN);
//  LogComponentEnable ("ThresholdPreambleDetectionModel", ns3::LOG_LEVEL_ALL);

  std::string phyMode ("S1gOfdmRate0_30MbpsBW1MHz");
  uint32_t packetSize = 20; // bytes
  uint32_t numPackets = 100000;
  uint32_t interval_in_us = 20000;

  int n_ap = 4;
  int n_sta = 20;

  uint32_t simSeed = 1000;
  uint32_t openGymPort = 5000;
  int simTime = 5;

  int time_for_arp_start = 1;
  int time_for_arp_end = 2;


  bool verbose = false;

  CommandLine cmd (__FILE__);
  cmd.AddValue ("phyMode", "Wifi Phy mode", phyMode);
  cmd.AddValue ("packetSize", "size of application packet sent", packetSize);
  cmd.AddValue ("numPackets", "number of packets generated", numPackets);
  cmd.AddValue ("interval_in_us", "interval between packets", interval_in_us);
  cmd.AddValue ("n_ap", "number of aps", n_ap);
  cmd.AddValue ("n_sta", "number of stations", n_sta);
  cmd.AddValue ("openGymPort", "Port number for OpenGym env. Default: 5555", openGymPort);
  cmd.AddValue ("simSeed", "Seed for random generator. Default: 1", simSeed);
  cmd.AddValue ("simTime", "simulation time", simTime);
  cmd.Parse (argc, argv);
  RngSeedManager::SetSeed (1);
  RngSeedManager::SetRun(simSeed);

  double time_for_test_start = (double) n_sta * 0.5 + 10 - 0.01;
  double time_for_test_end = time_for_test_start + simTime + 0.01;
  Time interval = MicroSeconds(interval_in_us);

  std::cout << "packetSize:" << packetSize << std::endl;
  std::cout << "numPackets:" << numPackets << std::endl;
  std::cout << "interval_in_us:" << interval_in_us << std::endl;
  std::cout << "n_ap:" << n_ap << std::endl;
  std::cout << "n_sta:" << n_sta << std::endl;
  std::cout << "simSeed:" << simSeed << std::endl;
  std::cout << "simTime:" << simTime << std::endl;
  std::cout << "time_for_test_start:" << time_for_test_start << std::endl;

  // Fix non-unicast data rate to be the same as that of unicast
  Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode",
                      StringValue (phyMode));

  NodeContainer ap_nodes;
  ap_nodes.Create (n_ap);

  NodeContainer sta_nodes;
  sta_nodes.Create (n_sta);

  NodeContainer server_node;
  server_node.Create(1);

  NodeContainer gate_node;
  gate_node.Create(1);

  /* Internet stack */
  InternetStackHelper stack;
  stack.Install (server_node);
  stack.Install (gate_node);
  stack.Install (ap_nodes);
  stack.Install (sta_nodes);
  Ipv4AddressGenerator::TestMode();

  MobilityHelper mobility;
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (ap_nodes);
  mobility.Install (sta_nodes);

  // The below set of helpers will help us to put together the wifi NICs we want
  WifiHelper wifi;
  wifi.SetStandard (ns3::WIFI_STANDARD_80211ah);
//  wifi.EnableLogComponents ();  // Turn on all Wifi logging

  YansWifiPhyHelper wifiPhy;
  wifiPhy.Set ("ChannelSettings", StringValue ("{0, 1, BAND_S1GHZ, 0}"));
  wifiPhy.Set ("CcaEdThreshold", DoubleValue (-75.) );
  wifiPhy.SetPcapDataLinkType (WifiPhyHelper::DLT_IEEE802_11_RADIO);
  wifiPhy.SetErrorRateModel("ns3::PpvErrorRateModel");
  wifiPhy.SetPreambleDetectionModel ("ns3::ThresholdPreambleDetectionModel",
                                     "MinimumRssi", DoubleValue (-95.));

  Ptr<MatrixPropagationLossModel> lossModel = CreateObject<MatrixPropagationLossModel> ();
  lossModel->SetDefaultLoss (200); // set default loss to 200 dB (no link)
  Ptr<OpenGymInterface> openGymInterface = CreateObject<OpenGymInterface> (openGymPort);
  Ptr<MyGymEnv> myGymEnv = CreateObject<MyGymEnv> (n_ap,n_sta, lossModel);
  myGymEnv->m_staNodes.Add(sta_nodes);
  myGymEnv->m_apNodes.Add(ap_nodes);

  Ptr<YansWifiChannel> yanswc = CreateObject <YansWifiChannel> ();
  yanswc->SetPropagationLossModel (lossModel);
  yanswc->SetPropagationDelayModel (CreateObject <ConstantSpeedPropagationDelayModel> ());
  wifiPhy.SetChannel (yanswc);

  // Add a mac and disable rate control
  WifiMacHelper wifiMac;
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                "DataMode", StringValue (phyMode),
                                "ControlMode", StringValue (phyMode));

  // Setup the rest of the MAC
  Ssid ssid = Ssid ("wifi-default");
  // setup AP
  wifiMac.SetType ("ns3::ApWifiMac",
                   "Ssid", SsidValue (ssid),
                   "QosSupported", BooleanValue (false)
  );
  NetDeviceContainer apDevice = wifi.Install (wifiPhy, wifiMac, ap_nodes);
  for (uint32_t i = 0; i < n_ap; i++) {
        auto m = apDevice.Get(i);
        auto w = m->GetObject<WifiNetDevice>();
        auto v = DynamicCast<ApWifiMac>(w->GetMac());
        v->SetBeaconOffset(i * (v->GetBeaconInterval() / n_ap));
        std::cout << "AP: " << i << " BeaconOffset:" << v->GetBeaconOffset() << std::endl;
        v->GetTxop()->GetWifiMacQueue()->SetMaxSize(QueueSize("500p"));
        Simulator::Schedule (Seconds (time_for_test_start), &ApWifiMac::SetBeaconGeneration, v , false);
  }

  // setup sta
  wifiMac.SetType ("ns3::StaWifiMac",
                   "Ssid", SsidValue (ssid),
                   "QosSupported", BooleanValue (false),
                   "WaitBeaconTimeout", TimeValue (MilliSeconds (200)),
                   "AssocRequestTimeout", TimeValue (MilliSeconds (200)),
                   "MaxMissedBeacons", UintegerValue (100000)
  );
  NetDeviceContainer staDevice = wifi.Install (wifiPhy, wifiMac, sta_nodes);
  // setup sta tx power
    for (uint32_t j = 0; j < n_sta; j++) {
        auto m = staDevice.Get(j);
        auto w = m->GetObject<WifiNetDevice>();
        auto v = DynamicCast<WifiPhy>(w->GetPhy());
        v->SetTxPowerStart(0.);
        v->SetTxPowerEnd(0.);
        auto z = DynamicCast<StaWifiMac>(w->GetMac());
        z->GetTxop()->GetWifiMacQueue()->SetMaxSize(QueueSize("5p"));
        z->SetAttribute ("scanningstartoffset", TimeValue (MilliSeconds(500)* (j+1)));
    }

    if (verbose) {
        for (uint32_t i = 0; i < n_sta; i++) {
            auto m = staDevice.Get(i);
            auto w = m->GetObject<WifiNetDevice>();
            auto v = DynamicCast<WifiPhy>(w->GetPhy());
            v->TraceConnectWithoutContext("PhyTxBegin", MakeCallback(&cb_tx_start));
            v->TraceConnectWithoutContext("PhyTxEnd", MakeCallback(&cb_tx_ended));
        }
        for (int i = 0; i < n_ap; ++i) {
            auto m = apDevice.Get(i);
            auto w = m->GetObject<WifiNetDevice>();
            auto v = w->GetMac();
            v->TraceConnectWithoutContext("MacRx", MakeCallback(&cb_rx_succ));
            v->TraceConnectWithoutContext("MacRxDrop", MakeCallback(&cb_rx_drop));
            v->TraceConnectWithoutContext("AssociatedSta", MakeCallback(&cb_asso));
            v->TraceConnectWithoutContext("DeAssociatedSta", MakeCallback(&cb_deasso));
        }
    }

  myGymEnv->m_staDevices.Add(staDevice);

  PointToPointHelper p2p;
  p2p.SetDeviceAttribute ("DataRate", StringValue ("100Gbps"));
  p2p.SetChannelAttribute ("Delay", StringValue ("0ms"));
  Ipv4AddressHelper server_address;
  server_address.SetBase ("10.1.0.0", "255.255.0.0");
  auto server_to_gate_netd = p2p.Install(server_node.Get(0),gate_node.Get(0));
  auto server_to_gate_intf = server_address.Assign(server_to_gate_netd);

  NetDeviceContainer p2p_ap_to_gate;
  Ipv4InterfaceContainer if_ap_to_gate;
  for (int i = 0; i < n_ap; ++i) {
    std::stringstream a;
    a << "" << 11 << "." << i+1 << ".0.0";
    std::cout<< "ap base: " << i << " " << a.str() << " " << a.str().c_str() << Ipv4Address(a.str().c_str()) << std::endl;
    Ipv4AddressHelper p2p_address;
    p2p_address.SetBase (a.str().c_str(), "255.255.0.0");
    auto d = p2p.Install(gate_node.Get(0),ap_nodes.Get(i));
    auto interface = p2p_address.Assign(d);
    p2p_ap_to_gate.Add(d.Get(1));
    if_ap_to_gate.Add(interface.Get(1));
  }

  Ipv4InterfaceContainer apInterface;
  for (int i = 0; i < n_ap; ++i) {
    Ipv4AddressHelper ap_address;
    ap_address.SetBase ("10.2.0.0", "255.255.0.0");
    auto a = ap_address.Assign (apDevice.Get(i));
    apInterface.Add(a);
  }

  Ipv4AddressHelper sta_address;
  sta_address.SetBase ("10.3.0.0", "255.255.0.0");
  Ipv4InterfaceContainer staInterface;
  staInterface = sta_address.Assign (staDevice);

  uint16_t port_off_set = 1000;
  ApplicationContainer ServerApps;
  for (int i = 0; i < n_sta; ++i) {
    uint16_t port = i + port_off_set;
    Ptr<UdpServer> r = CreateObject<UdpServer> ();
    r->SetAttribute("Port",UintegerValue (port));
    r->SetStartTime(Seconds (time_for_test_start));
    r->SetStopTime(Seconds (time_for_test_end));
    server_node.Get(0)->AddApplication(r);
    ServerApps.Add(r);
  }

  for (int i = 0; i < n_sta; ++i) {
    uint16_t port = i + port_off_set;
    Ptr<UdpClient> s = CreateObject<UdpClient> ();
    s->SetRemote(server_to_gate_intf.GetAddress(0),port);
    s->SetAttribute("MaxPackets",UintegerValue (numPackets));
    s->SetAttribute("Interval",TimeValue(interval));
    s->SetAttribute("PacketSize",UintegerValue (packetSize));
    s->SetStartTime(Seconds (time_for_test_start));
    s->SetStopTime(Seconds (time_for_test_end));
    sta_nodes.Get(i)->AddApplication(s);
  }

  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  Ptr<Ipv4StaticRouting> gate_routing = ipv4RoutingHelper.GetStaticRouting (gate_node.Get(0)->GetObject<Ipv4>());
  gate_routing->AddHostRouteTo (server_to_gate_intf.GetAddress(0), server_to_gate_intf.Get(0).second);

  for (int i = 0; i < n_ap; ++i) {
    Ptr<Ipv4StaticRouting> ap_r = ipv4RoutingHelper.GetStaticRouting (ap_nodes.Get(i)->GetObject<Ipv4>());
    ap_r->AddHostRouteTo (server_to_gate_intf.GetAddress(0), if_ap_to_gate.Get(i).second);
    ap_r->AddHostRouteTo (server_to_gate_intf.GetAddress(1), if_ap_to_gate.Get(i).second);
  }

  for (int i = 0; i < n_sta; ++i) {
    Ptr<Ipv4StaticRouting> sta_r = ipv4RoutingHelper.GetStaticRouting (sta_nodes.Get(i)->GetObject<Ipv4>());
    sta_r->AddHostRouteTo (server_to_gate_intf.GetAddress(0),staInterface.Get(i).second);
    sta_r->AddHostRouteTo (server_to_gate_intf.GetAddress(1),staInterface.Get(i).second);
  }
    // Notify before arp, mcs, association start, because these processes need path loss
    myGymEnv->SetOpenGymInterface(openGymInterface);
    myGymEnv->Notify();
    // Set up ARP
  for (int j = 0; j < n_sta; j++) {
      std::vector<double> gain_list;
      for (int i = 0; i < n_ap; i++){
             auto A = ap_nodes.Get(i)->GetObject<MobilityModel>();
             auto B = sta_nodes.Get(j)->GetObject<MobilityModel>();
             double g = lossModel->CalcRxPower(0,B,A);
             gain_list.push_back (g);
             std::cout<< "sta: " << j << " ap:" << i << " " << g << std::endl;
      }
      auto result = std::max_element(gain_list.begin(), gain_list.end());
      int ind = std::distance(gain_list.begin(), result);
      Mac48Address addr = Mac48Address::ConvertFrom(apDevice.Get (ind)->GetAddress());
      Ptr<ArpCache> arp = CreateObject<ArpCache>();
      arp->SetAliveTimeout(Seconds(3600 * 24 * 365));
      ArpCache::Entry *entry = arp->Add(server_to_gate_intf.GetAddress(0));
      Ipv4Header ipv4Hdr;
      ipv4Hdr.SetDestination(server_to_gate_intf.GetAddress(0));
      Ptr<Packet> p = Create<Packet>(100);
      entry->MarkWaitReply(ArpCache::Ipv4PayloadHeaderPair(p, ipv4Hdr));
      entry->MarkAlive(addr);

      Ptr<Ipv4L3Protocol> ip = sta_nodes.Get (j)->GetObject<Ipv4L3Protocol> ();
      NS_ASSERT (ip !=0);
      ObjectVectorValue interfaces;
      ip->GetAttribute ("InterfaceList", interfaces);
      for (ObjectVectorValue::Iterator p = interfaces.Begin (); p != interfaces.End (); p ++)
      {
          Ptr<Ipv4Interface> ipIface = (*p).second->GetObject<Ipv4Interface> ();
          ipIface->SetAttribute ("ArpCache", PointerValue (arp) );
      }
      std::cout<< "STA: " << j << ", arp: " << addr << std::endl;
  }

    // setup sta mcs
    for (uint32_t j = 0; j < n_sta; j++) {
          double max_gain = -std::numeric_limits<double>::infinity();
          uint32_t max_i = 0;
          for (uint32_t i = 0; i < n_ap; i++) {
              double gain = lossModel->CalcRxPower(0, ap_nodes.Get(i)->GetObject<MobilityModel>(),
                                                   sta_nodes.Get(j)->GetObject<MobilityModel>());
              if (gain >= max_gain) {
                  max_gain = gain;
                  max_i = i;
              }
          }
          WifiTxVector txVector;
          Ptr<PpvErrorRateModel> ppv = CreateObject<PpvErrorRateModel>();
          auto table = S1gOfdmPhy::GetModulationLookupTable();
          auto bandtable = S1gOfdmPhy::GetModulationBandLookupTable();
          auto m = staDevice.Get(j);
          auto w = m->GetObject<WifiNetDevice>();
          auto v = DynamicCast<WifiPhy>(w->GetPhy());
          uint16_t bw = v->GetChannelWidth();
          double pw = v->GetTxPowerStart();
          double nf = DbToRatio(20.);
          std::string max_mode ("S1gOfdmRate0_30MbpsBW1MHz");
          uint64_t max_rate = 0;
          double BOLTZMANN = 1.3803e-23;
          double Nt = BOLTZMANN * 290 * bw * 1e6;
          double noiseFloor = nf * Nt;

          for (auto it = bandtable.begin(); it != bandtable.end(); it++) {
              if (it->second == bw) {
                  txVector.SetMode(it->first);
                  txVector.SetChannelWidth(bw);
                  double ps = ppv->GetChunkSuccessRate(WifiMode(it->first), txVector,
                                                       std::pow(10.0, (pw + max_gain - WToDbm(noiseFloor)) / 10.0),
                                                       (packetSize + UDP_IP_WIFI_HEADER_SIZE) * 8);
                  uint64_t rate = S1gOfdmPhy::GetDataRate(it->first, bw);
                  if (rate >= max_rate and ps > (1.-1e-5)) {
                      max_rate = rate;
                      max_mode = it->first;
                  }
              }
          }
          auto a = DynamicCast<StaWifiMac>(w->GetMac());
          a->GetWifiRemoteStationManager()->SetAttribute("DataMode", StringValue (max_mode));
          std::cout << "STA:" << j << "-" << "AP:" << max_i << ", Gain: " << max_gain << "\n\t\t noiseFloor:" << WToDbm(noiseFloor) << " phyMode:" << max_mode << std::endl;
      }
  Simulator::Stop (Seconds (time_for_test_end+0.1));
  std::cout<< "Sim Start" << std::endl;

  Simulator::Run ();


  for (int i = 0; i < n_sta; ++i) {
    auto m = staDevice.Get(i);
    auto w = m->GetObject<WifiNetDevice>();
    auto v = w->GetMac();
    std::cout<< "STA: " << i << ", bssid: " << v->GetBssid() << std::endl;
    std::cout<< "STA: " << i << ", addr: " << v->GetAddress() << std::endl;
    std::cout<< "STA: " << i << ", queue size: " << (v->GetTxop())->GetWifiMacQueue()->GetMaxSize() << std::endl;
    std::cout<< "STA: " << i << ", queue drop: " << (v->GetTxop())->GetWifiMacQueue()->GetTotalDroppedPackets() << std::endl;
    std::cout<< "STA: " << i << ", queue srxp: " << (v->GetTxop())->GetWifiMacQueue()->GetTotalReceivedPackets() << std::endl;
    auto r = ServerApps.Get(i);
    std::cout<< "STA: " << i << ", X-Y: " << sta_nodes.Get(i)->GetObject<MobilityModel>()->GetPosition().x << "\t," << sta_nodes.Get(i)->GetObject<MobilityModel>()->GetPosition().y << std::endl;
    std::cout<< "   packets received " << DynamicCast<UdpServer>(r)->GetReceived() << std::endl;
    std::cout<< "   bits received " << DynamicCast<UdpServer>(r)->GetReceived() * (packetSize + UDP_IP_WIFI_HEADER_SIZE) * 8 << std::endl;
    std::cout<< "   aoi received " << DynamicCast<UdpServer>(r)->GetLastAoI_us() << std::endl;
    std::cout<< "   delay received " << DynamicCast<UdpServer>(r)->GetAvgDelay_us() << std::endl;
    std::cout<< "   interval received " << DynamicCast<UdpServer>(r)->GetAvgInterval_us() << std::endl;
  }
  for (int i = 0; i < n_ap; ++i) {
    auto m = apDevice.Get(i);
    auto w = m->GetObject<WifiNetDevice>();
    auto v = w->GetMac();
    std::cout<< "AP: " << i << ", bssid: " << v->GetBssid() << std::endl;
    std::cout<< "AP: " << i << ", queue size: " << (v->GetTxop())->GetWifiMacQueue()->GetMaxSize() << std::endl;
    std::cout<< "AP: " << i << ", queue drop: " << (v->GetTxop())->GetWifiMacQueue()->GetTotalDroppedPackets() << std::endl;
    std::cout<< "AP: " << i << ", queue totp: " << (v->GetTxop())->GetWifiMacQueue()->GetTotalReceivedPackets() << std::endl;
//    auto r = ServerApps.Get(i);
//    std::cout<< "AP: " << i << ", X-Y: " << sta_nodes.Get(i)->GetObject<MobilityModel>()->GetPosition().x << "\t," << sta_nodes.Get(i)->GetObject<MobilityModel>()->GetPosition().y << std::endl;
//    std::cout<< "   packets received " << DynamicCast<UdpServer>(r)->GetReceived() << std::endl;
//    std::cout<< "   bits received " << DynamicCast<UdpServer>(r)->GetReceived() * (packetSize + UDP_IP_WIFI_HEADER_SIZE) * 8 << std::endl;
//    std::cout<< "   aoi received " << DynamicCast<UdpServer>(r)->GetLastAoI_us() << std::endl;
//    std::cout<< "   delay received " << DynamicCast<UdpServer>(r)->GetAvgDelay_us() << std::endl;
//    std::cout<< "   interval received " << DynamicCast<UdpServer>(r)->GetAvgInterval_us() << std::endl;
  }

  std::cout<< "server ip: " << server_to_gate_intf.GetAddress(0) << std::endl;
  std::cout<< "server ip: " << server_to_gate_intf.GetAddress(1) << std::endl;

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

  myGymEnv->m_serverApps.Add(ServerApps);
  myGymEnv->is_simulation_end = true;
  myGymEnv->Notify();
  myGymEnv->NotifySimulationEnd();
  Simulator::Destroy ();

  return 0;
}
