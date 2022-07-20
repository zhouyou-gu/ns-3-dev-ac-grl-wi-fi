#include <ns3/propagation-loss-model.h>
#include <ns3/propagation-module.h>
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
#include "populatearp/populatearpcache.h"


using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("wifi-single-ap-rate-test");

int main (int argc, char *argv[])
{
  LogComponentEnable ("wifi-single-ap-rate-test", LOG_LEVEL_INFO);
  LogComponentEnable ("S1gOfdmPhy", ns3::LOG_LEVEL_ALL);
  LogComponentEnable ("S1gOfdmPpdu", ns3::LOG_LEVEL_ALL);
  LogComponentEnable ("WifiNetDevice", ns3::LOG_LEVEL_ALL);
  LogComponentEnable ("ApWifiMac", ns3::LOG_LEVEL_ALL);
  LogComponentEnable ("TableBasedErrorRateModel", ns3::LOG_LEVEL_ALL);

//  LogComponentEnable ("UdpServer", ns3::LOG_LEVEL_ALL);
  LogComponentEnable ("UdpClient", ns3::LOG_LEVEL_ALL);
//  LogComponentEnable ("UdpSocketImpl", ns3::LOG_LEVEL_ALL);
//  LogComponentEnable ("Ipv4Interface", ns3::LOG_LEVEL_ALL);
//  LogComponentEnable ("Ipv4L3Protocol", ns3::LOG_LEVEL_ALL);
//  LogComponentEnable ("ArpL3Protocol", ns3::LOG_LEVEL_ALL);

  std::string phyMode ("S1gOfdmRate0_30MbpsBW1MHz");
  double rss = -70;  // -dBm
  uint32_t packetSize = 100; // bytes
  uint32_t numPackets = 10000;
  Time interval = MicroSeconds(1000);
  bool verbose = true;

  CommandLine cmd (__FILE__);
  cmd.AddValue ("phyMode", "Wifi Phy mode", phyMode);
  cmd.AddValue ("rss", "received signal strength", rss);
  cmd.AddValue ("packetSize", "size of application packet sent", packetSize);
  cmd.AddValue ("numPackets", "number of packets generated", numPackets);
  cmd.AddValue ("interval", "interval between packets", interval);
  cmd.AddValue ("verbose", "turn on all WifiNetDevice log components", verbose);
  cmd.Parse (argc, argv);

  // Fix non-unicast data rate to be the same as that of unicast
  Config::SetDefault ("ns3::WifiRemoteStationManager::NonUnicastMode",
                      StringValue (phyMode));

  NodeContainer ap_nodes;
  ap_nodes.Create (1);

  int n_sta = 2;
  NodeContainer sta_nodes;
  sta_nodes.Create (n_sta);

  MobilityHelper mobility_ap;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector (0.0, 0.0, 0.0));
  mobility_ap.SetPositionAllocator (positionAlloc);
  mobility_ap.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility_ap.Install (ap_nodes);


  MobilityHelper mobility_sta;
  Ptr<ListPositionAllocator> positionAlloc_sta = CreateObject<ListPositionAllocator> ();
  for (int i = 0; i < n_sta; ++i) {
//    positionAlloc_sta->Add (Vector (200*(i+1) * (i % 2 == 0 ? -1 : 1), 0.0, 0.0));
    positionAlloc_sta->Add (Vector (200* (i % 2 == 0 ? -1 : 1), 0.0, 0.0));
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


//  YansWifiChannelHelper wifiChannel;
//  wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
//  wifiChannel.AddPropagationLoss ("ns3::FriisPropagationLossModel",
//                                  "Frequency", DoubleValue (1e9));
  Ptr<MatrixPropagationLossModel> lossModel = CreateObject<MatrixPropagationLossModel> ();
  lossModel->SetDefaultLoss (200); // set default loss to 200 dB (no link)
  lossModel->SetLoss (ap_nodes.Get (0)->GetObject<MobilityModel> (), sta_nodes.Get (0)->GetObject<MobilityModel> (), 50); // set symmetric loss 0 <-> 1 to 50 dB
  lossModel->SetLoss (ap_nodes.Get (0)->GetObject<MobilityModel> (), sta_nodes.Get (1)->GetObject<MobilityModel> (), 50); // set symmetric loss 2 <-> 1 to 50 dB

  Ptr<YansWifiChannel> wifiChannel = CreateObject <YansWifiChannel> ();
  wifiChannel->SetPropagationLossModel (lossModel);
  wifiChannel->SetPropagationDelayModel (CreateObject <ConstantSpeedPropagationDelayModel> ());

  wifiPhy.SetChannel (wifiChannel);

  // Add a mac and disable rate control
  WifiMacHelper wifiMac;
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                "DataMode", StringValue (phyMode),
                                "MaxSsrc", UintegerValue(10),
                                "MaxSlrc", UintegerValue(10),
                                "ControlMode", StringValue (phyMode));

  // Setup the rest of the MAC
  Ssid ssid = Ssid ("wifi-default");
  // setup AP
  wifiMac.SetType ("ns3::ApWifiMac",
                   "Ssid", SsidValue (ssid),
                   "QosSupported", BooleanValue (false)
  );
  NetDeviceContainer apDevice = wifi.Install (wifiPhy, wifiMac, ap_nodes);
  wifiMac.SetType ("ns3::StaWifiMac",
                   "Ssid", SsidValue (ssid),
                   "QosSupported", BooleanValue (false)
  );
  NetDeviceContainer staDevice = wifi.Install (wifiPhy, wifiMac, sta_nodes);
  // Note that with FixedRssLossModel, the positions below are not
  // used for received signal strength.



  /* Internet stack */
  InternetStackHelper stack;
  stack.Install (ap_nodes);
  stack.Install (sta_nodes);

  Ipv4AddressHelper address;
  address.SetBase ("10.0.0.0", "255.255.255.0");
  Ipv4InterfaceContainer apInterface;
  apInterface = address.Assign (apDevice);
  Ipv4InterfaceContainer staInterface;
  staInterface = address.Assign (staDevice);


  uint16_t port_off_set = 1000;
  ApplicationContainer ServerApps;
  for (int i = 0; i < n_sta; ++i) {
    uint16_t port = i + port_off_set;
    Ptr<UdpServer> r = CreateObject<UdpServer> ();
    r->SetAttribute("Port",UintegerValue (port));
    r->SetStartTime(Seconds (1));
    ap_nodes.Get(0)->AddApplication(r);
    ServerApps.Add(r);
  }

  for (int i = 0; i < n_sta; ++i) {
    uint16_t port = i + port_off_set;
    Ptr<UdpClient> s = CreateObject<UdpClient> ();
    s->SetRemote(apInterface.GetAddress(0),port);
    s->SetAttribute("MaxPackets",UintegerValue (numPackets));
    s->SetAttribute("Interval",TimeValue(interval));
    s->SetAttribute("PacketSize",UintegerValue (packetSize));
    s->SetStartTime(Seconds (1+DoubleValue(interval.GetSeconds()*i).Get()/DoubleValue(n_sta).Get()));
    sta_nodes.Get(i)->AddApplication(s);
  }

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
  PopulateARPcache();

  Simulator::Stop (Seconds (5));
  Simulator::Run ();
  for (int i = 0; i < n_sta; ++i) {
    auto r = ServerApps.Get(i);
    std::cout<< "from " <<" packets received " << DynamicCast<UdpServer>(r)->GetReceived() << std::endl;
    std::cout<< "bits received " << DynamicCast<UdpServer>(r)->GetReceived() * (packetSize + 64) * 8 << std::endl;
  }

  Simulator::Destroy ();

  return 0;
}
