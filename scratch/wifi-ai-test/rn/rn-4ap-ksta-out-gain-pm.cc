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

#include "mygym.h"

#define UDP_IP_WIFI_HEADER_SIZE 64

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("wifi-test");

int dc = 0;

void cb(Ptr<const Packet> p){
  p->PrintPacketTags (std::cout);
  std::cout<< dc <<std::endl;
  dc ++;
}


void cb_asso(uint16_t /* AID */ a, Mac48Address b){
  std::cout<< "AssociatedSta:" << a << ":" << b <<std::endl;

}
void cb_deasso(uint16_t /* AID */ a, Mac48Address b){
  std::cout<< "DeAssociatedSta:" << a << ":" << b <<std::endl;
}

int main (int argc, char *argv[])
{
  LogComponentEnableAll (LOG_PREFIX_TIME);
  LogComponentEnableAll (LOG_PREFIX_NODE);
//
  LogComponentEnable ("wifi-test", LOG_LEVEL_INFO);
//
//  LogComponentEnable ("UdpServer", ns3::LOG_LEVEL_ALL);
//  LogComponentEnable ("UdpClient", ns3::LOG_LEVEL_ALL);
//  LogComponentEnable ("UdpSocketImpl", ns3::LOG_LEVEL_ALL);
//  LogComponentEnable ("Ipv4Interface", ns3::LOG_LEVEL_ALL);
//  LogComponentEnable ("Ipv4L3Protocol", ns3::LOG_LEVEL_ALL);
//  LogComponentEnable ("Ipv4", ns3::LOG_LEVEL_ALL);
//  LogComponentEnable ("NetDevice", ns3::LOG_LEVEL_ALL);
//  LogComponentEnable ("ArpL3Protocol", ns3::LOG_LEVEL_ALL);
//  LogComponentEnable ("ArpCache", ns3::LOG_LEVEL_ALL);
//  LogComponentEnable ("PointToPointChannel", ns3::LOG_LEVEL_ALL);
//  LogComponentEnable ("PointToPointNetDevice", ns3::LOG_LEVEL_ALL);
//  LogComponentEnable ("YansWifiChannel", ns3::LOG_LEVEL_WARN);
//  LogComponentEnable ("InterferenceHelper", ns3::LOG_LEVEL_WARN);
//  LogComponentEnable ("WifiPhyStateHelper", ns3::LOG_LEVEL_WARN);

  std::string phyMode ("S1gOfdmRate0_30MbpsBW1MHz");
  uint32_t packetSize = 20; // bytes
  uint32_t numPackets = 100000;
  uint32_t interval_in_us = 40000;

  int n_ap = 4;
  int n_sta = 10;

  uint32_t simSeed = 1000;
  uint32_t openGymPort = 5000;
  int simTime = 200;

  int time_for_arp_start = 1;
  int time_for_arp_end = 5;

  int time_for_test_start = 10;

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
  int time_for_test_end = time_for_test_start + simTime;
  Time interval = MicroSeconds(interval_in_us);

  std::cout << "interval_in_us:" <<interval_in_us << std::endl;

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

  MobilityHelper mobility_ap;
  Ptr<ListPositionAllocator> positionAlloc_ap = CreateObject<ListPositionAllocator> ();
  positionAlloc_ap->Add (Vector(500,500,0.0));
  positionAlloc_ap->Add (Vector(-500,500,0.0));
  positionAlloc_ap->Add (Vector(500,-500,0.0));
  positionAlloc_ap->Add (Vector(-500,-500,0.0));
  mobility_ap.SetPositionAllocator (positionAlloc_ap);
  mobility_ap.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility_ap.Install (ap_nodes);


  MobilityHelper mobility_sta;
  Ptr<RandomRectanglePositionAllocator> positionAlloc_sta = CreateObject<RandomRectanglePositionAllocator> ();
  Ptr<UniformRandomVariable> rnd = CreateObject<UniformRandomVariable> ();
  rnd->SetAttribute("Min", DoubleValue(-1000));
  rnd->SetAttribute("Max", DoubleValue(1000));
  positionAlloc_sta->SetX(rnd);
  positionAlloc_sta->SetY(rnd);
  mobility_sta.SetPositionAllocator (positionAlloc_sta);
  mobility_sta.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility_sta.Install (sta_nodes);


  // The below set of helpers will help us to put together the wifi NICs we want
  WifiHelper wifi;
  wifi.SetStandard (ns3::WIFI_STANDARD_80211ah);

  YansWifiPhyHelper wifiPhy;
  wifiPhy.Set ("ChannelSettings", StringValue ("{0, 1, BAND_S1GHZ, 0}"));
  wifiPhy.Set ("RxGain", DoubleValue (0) );
  wifiPhy.SetPcapDataLinkType (WifiPhyHelper::DLT_IEEE802_11_RADIO);


  Ptr<FriisPropagationLossModel> lossModel = CreateObject<FriisPropagationLossModel> ();
  lossModel->SetFrequency(1e9);

  Ptr<YansWifiChannel> yanswc = CreateObject <YansWifiChannel> ();
  yanswc->SetPropagationLossModel (lossModel);
//  Ptr<RandomPropagationDelayModel> propagationdelayModel = CreateObject <RandomPropagationDelayModel>();
//  propagationdelayModel->SetAttribute ("Variable",StringValue("ns3::UniformRandomVariable[Min=0.0|Max=6e-6]"));
//  yanswc->SetPropagationDelayModel (propagationdelayModel);
  yanswc->SetPropagationDelayModel (CreateObject <ConstantSpeedPropagationDelayModel> ());
  wifiPhy.SetChannel (yanswc);

  // Add a mac and disable rate control
  WifiMacHelper wifiMac;
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                "DataMode", StringValue (phyMode),
//                                "MaxSsrc", UintegerValue(2),
//                                "MaxSlrc", UintegerValue(2),
                                "ControlMode", StringValue (phyMode));

  // Setup the rest of the MAC
  Ssid ssid = Ssid ("wifi-default");
  // setup AP
  wifiMac.SetType ("ns3::ApWifiMac",
                   "Ssid", SsidValue (ssid),
                   "QosSupported", BooleanValue (false)
  );
  NetDeviceContainer apDevice = wifi.Install (wifiPhy, wifiMac, ap_nodes);
  for (uint32_t i = 0; i < n_ap; i++)
  {
    auto m = apDevice.Get(i);
    auto w = m->GetObject<WifiNetDevice>();
    auto v = DynamicCast<ApWifiMac>(w->GetMac());
    v->SetBeaconOffset(i* (v->GetBeaconInterval()/n_ap) );
    std::cout<< "AP: "<< i <<" BeaconOffset:" << v->GetBeaconOffset() << std::endl;
    v->TraceConnectWithoutContext("MacRxDrop",MakeCallback(&cb));
    v->TraceConnectWithoutContext("AssociatedSta",MakeCallback(&cb_asso));
    v->TraceConnectWithoutContext("DeAssociatedSta",MakeCallback(&cb_deasso));
    v->GetTxop()->GetWifiMacQueue()->SetMaxSize (QueueSize ("500p"));
  }

  // setup sta
  wifiMac.SetType ("ns3::StaWifiMac",
                   "Ssid", SsidValue (ssid),
                   "QosSupported", BooleanValue (false)
  );
  NetDeviceContainer staDevice = wifi.Install (wifiPhy, wifiMac, sta_nodes);

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

  for (int j = 0; j < n_sta; j++) {
      std::vector<double> gain_list;
      for (int i = 0; i < n_ap; i++){
             auto A = ap_nodes.Get(i)->GetObject<MobilityModel>();
             auto B = sta_nodes.Get(j)->GetObject<MobilityModel>();
             double g = lossModel->CalcRxPower(0,B,A);
             gain_list.push_back (g);
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

  Simulator::Stop (Seconds (time_for_test_end+0.1));
  Simulator::Run ();
  for (int i = 0; i < n_sta; ++i) {
    auto m = staDevice.Get(i);
    auto w = m->GetObject<WifiNetDevice>();
    auto v = w->GetMac();
    std::cout<< "STA: " << i << ", bssid: " << v->GetBssid() << std::endl;
    std::cout<< "STA: " << i << ", addr: " << v->GetAddress() << std::endl;
    std::cout<< "STA: " << i << ", queue size: " << (v->GetTxop())->GetWifiMacQueue()->GetMaxSize() << std::endl;
    std::cout<< "STA: " << i << ", queue drop: " << (v->GetTxop())->GetWifiMacQueue()->GetTotalDroppedPackets() << std::endl;
    std::cout<< "STA: " << i << ", queue totp: " << (v->GetTxop())->GetWifiMacQueue()->GetTotalReceivedPackets() << std::endl;
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
    std::cout<< "STA: " << i << ", bssid: " << v->GetBssid() << std::endl;
    std::cout<< "STA: " << i << ", queue size: " << (v->GetTxop())->GetWifiMacQueue()->GetMaxSize() << std::endl;
    std::cout<< "STA: " << i << ", queue drop: " << (v->GetTxop())->GetWifiMacQueue()->GetTotalDroppedPackets() << std::endl;
    std::cout<< "STA: " << i << ", queue totp: " << (v->GetTxop())->GetWifiMacQueue()->GetTotalReceivedPackets() << std::endl;
    auto r = ServerApps.Get(i);
    std::cout<< "STA: " << i << ", X-Y: " << sta_nodes.Get(i)->GetObject<MobilityModel>()->GetPosition().x << "\t," << sta_nodes.Get(i)->GetObject<MobilityModel>()->GetPosition().y << std::endl;
    std::cout<< "   packets received " << DynamicCast<UdpServer>(r)->GetReceived() << std::endl;
    std::cout<< "   bits received " << DynamicCast<UdpServer>(r)->GetReceived() * (packetSize + UDP_IP_WIFI_HEADER_SIZE) * 8 << std::endl;
    std::cout<< "   aoi received " << DynamicCast<UdpServer>(r)->GetLastAoI_us() << std::endl;
    std::cout<< "   delay received " << DynamicCast<UdpServer>(r)->GetAvgDelay_us() << std::endl;
    std::cout<< "   interval received " << DynamicCast<UdpServer>(r)->GetAvgInterval_us() << std::endl;
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
  Ptr<OpenGymInterface> openGymInterface = CreateObject<OpenGymInterface> (openGymPort);
  Ptr<MyGymEnv> myGymEnv = CreateObject<MyGymEnv> (n_ap, n_sta, lossModel);
  myGymEnv->m_staNodes.Add(sta_nodes);
  myGymEnv->m_apNodes.Add(ap_nodes);
  myGymEnv->m_serverApps.Add(ServerApps);
  myGymEnv->SetOpenGymInterface(openGymInterface);
  myGymEnv->Notify();
  myGymEnv->NotifySimulationEnd();
  Simulator::Destroy ();

  return 0;
}
