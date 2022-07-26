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

#include "ns3/udp-client.h"
#include "ns3/udp-server.h"
#include "ns3/boolean.h"
#include "ns3/ipv4-global-routing-helper.h"


#define UDP_IP_WIFI_HEADER_SIZE 64

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("wifi-test");

int main (int argc, char *argv[])
{
  LogComponentEnableAll (LOG_PREFIX_TIME);
  LogComponentEnableAll (LOG_PREFIX_NODE);
//
  LogComponentEnable ("wifi-test", LOG_LEVEL_INFO);
//
  LogComponentEnable ("UdpServer", ns3::LOG_LEVEL_ALL);
  LogComponentEnable ("UdpClient", ns3::LOG_LEVEL_ALL);
  LogComponentEnable ("UdpSocketImpl", ns3::LOG_LEVEL_ALL);
  LogComponentEnable ("Ipv4Interface", ns3::LOG_LEVEL_ALL);
  LogComponentEnable ("Ipv4L3Protocol", ns3::LOG_LEVEL_ALL);
  LogComponentEnable ("Ipv4", ns3::LOG_LEVEL_ALL);
  LogComponentEnable ("NetDevice", ns3::LOG_LEVEL_ALL);
  LogComponentEnable ("ArpL3Protocol", ns3::LOG_LEVEL_ALL);
  LogComponentEnable ("PointToPointChannel", ns3::LOG_LEVEL_ALL);
  LogComponentEnable ("PointToPointNetDevice", ns3::LOG_LEVEL_ALL);

  std::string phyMode ("S1gOfdmRate0_30MbpsBW1MHz");
  uint32_t packetSize = 100; // bytes
  uint32_t numPackets = 10000;
  Time interval = MicroSeconds(1000);

  int n_ap = 2;
  int n_sta = 2;
  bool verbose = true;

  CommandLine cmd (__FILE__);
  cmd.AddValue ("phyMode", "Wifi Phy mode", phyMode);
  cmd.AddValue ("packetSize", "size of application packet sent", packetSize);
  cmd.AddValue ("numPackets", "number of packets generated", numPackets);
  cmd.AddValue ("interval", "interval between packets", interval);
  cmd.AddValue ("n_ap", "number of aps", n_ap);
  cmd.AddValue ("n_sta", "number of stations", n_sta);
  cmd.Parse (argc, argv);

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
  for (int i = 0; i < n_ap; ++i) {
    positionAlloc_ap->Add (Vector (500 * (i % 2 == 0 ? -1 : 1), 0.0, 0.0));
  }
  mobility_ap.SetPositionAllocator (positionAlloc_ap);
  mobility_ap.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility_ap.Install (ap_nodes);


  MobilityHelper mobility_sta;
  Ptr<ListPositionAllocator> positionAlloc_sta = CreateObject<ListPositionAllocator> ();
  for (int i = 0; i < n_sta; ++i) {
    positionAlloc_sta->Add (Vector ((500 +500* sqrt(500)) * (i % 2 == 0 ? -1 : 1), 0.0, 0.0));
  }
  mobility_sta.SetPositionAllocator (positionAlloc_sta);
  mobility_sta.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility_sta.Install (sta_nodes);


  // The below set of helpers will help us to put together the wifi NICs we want
  WifiHelper wifi;
  if (verbose)
  {
    wifi.EnableLogComponents ();  // Turn on all Wifi logging
  }
  wifi.SetStandard (ns3::WIFI_STANDARD_80211ah);

  YansWifiPhyHelper wifiPhy;
  wifiPhy.Set ("ChannelSettings", StringValue ("{0, 1, BAND_S1GHZ, 0}"));
  wifiPhy.Set ("RxGain", DoubleValue (0) );
  wifiPhy.SetPcapDataLinkType (WifiPhyHelper::DLT_IEEE802_11_RADIO);


  YansWifiChannelHelper wifiChannel;
  wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  wifiChannel.AddPropagationLoss ("ns3::FriisPropagationLossModel",
                                  "Frequency", DoubleValue (1e9));

  wifiPhy.SetChannel (wifiChannel.Create());

  // Add a mac and disable rate control
  WifiMacHelper wifiMac;
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                "DataMode", StringValue (phyMode),
                                "MaxSsrc", UintegerValue(1),
                                "MaxSlrc", UintegerValue(1),
                                "ControlMode", StringValue (phyMode));

  // Setup the rest of the MAC
  Ssid ssid = Ssid ("wifi-default");
  // setup AP
  wifiMac.SetType ("ns3::ApWifiMac",
                   "Ssid", SsidValue (ssid),
                   "QosSupported", BooleanValue (false)
  );
  NetDeviceContainer apDevice = wifi.Install (wifiPhy, wifiMac, ap_nodes);
  // setup sta
  wifiMac.SetType ("ns3::StaWifiMac",
                   "Ssid", SsidValue (ssid),
                   "QosSupported", BooleanValue (false)
  );
  NetDeviceContainer staDevice = wifi.Install (wifiPhy, wifiMac, sta_nodes);

  PointToPointHelper p2p;
  p2p.SetDeviceAttribute ("DataRate", StringValue ("1Gbps"));
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
    r->SetStartTime(Seconds (1));
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
    s->SetStartTime(Seconds (1+DoubleValue(interval.GetSeconds()*i).Get()/DoubleValue(n_sta).Get()));
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
    sta_r->AddHostRouteTo (server_to_gate_intf.GetAddress(0),"10.2.0.1",staInterface.Get(i).second);
    sta_r->AddHostRouteTo (server_to_gate_intf.GetAddress(1),"10.2.0.1",staInterface.Get(i).second);
  }

  Simulator::Stop (Seconds (2));

  Simulator::Run ();
  for (int i = 0; i < n_sta; ++i) {
    auto r = ServerApps.Get(i);
    std::cout<< " packets received " << DynamicCast<UdpServer>(r)->GetReceived() << std::endl;
    std::cout<< "bits received " << DynamicCast<UdpServer>(r)->GetReceived() * (packetSize + UDP_IP_WIFI_HEADER_SIZE) * 8 << std::endl;
  }
  std::cout<< "server ip: " << server_to_gate_intf.GetAddress(0) << std::endl;
  std::cout<< "server ip: " << server_to_gate_intf.GetAddress(1) << std::endl;

  Simulator::Destroy ();

  return 0;
}
