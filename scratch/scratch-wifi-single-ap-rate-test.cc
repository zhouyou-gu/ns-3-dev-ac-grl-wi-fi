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
#include "ns3/populatearpcache.h"


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
//  LogComponentEnable ("UdpClient", ns3::LOG_LEVEL_ALL);

//  LogComponentEnable ("UdpSocketImpl", ns3::LOG_LEVEL_ALL);
//  LogComponentEnable ("Ipv4Interface", ns3::LOG_LEVEL_ALL);
//  LogComponentEnable ("Ipv4L3Protocol", ns3::LOG_LEVEL_ALL);
//  LogComponentEnable ("ArpL3Protocol", ns3::LOG_LEVEL_ALL);

  std::string phyMode ("S1gOfdmRate0_30MbpsBW1MHz");
  double rss = -70;  // -dBm
  uint32_t packetSize = 100; // bytes
  uint32_t numPackets = 10000;
  Time interval = MicroSeconds(2000);
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

  NodeContainer c;
  c.Create (2);
  auto ap = c.Get(0);
  auto sta = c.Get(1);

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
  // The below FixedRssLossModel will cause the rss to be fixed regardless
  // of the distance between the two stations, and the transmit power
  wifiChannel.AddPropagationLoss ("ns3::FriisPropagationLossModel",
                                  "Frequency", DoubleValue (1e9));
  wifiPhy.SetChannel (wifiChannel.Create ());

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
  NetDeviceContainer apDevice = wifi.Install (wifiPhy, wifiMac, ap);
  wifiMac.SetType ("ns3::StaWifiMac",
                   "Ssid", SsidValue (ssid),
                   "QosSupported", BooleanValue (false)
  );
  NetDeviceContainer staDevice = wifi.Install (wifiPhy, wifiMac, sta);
  // Note that with FixedRssLossModel, the positions below are not
  // used for received signal strength.
  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector (0.0, 0.0, 0.0));
  positionAlloc->Add (Vector (100.0, 0.0, 0.0));
  mobility.SetPositionAllocator (positionAlloc);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (c);

  /* Internet stack */
  InternetStackHelper stack;
  stack.Install (c);

  Ipv4AddressHelper address;
  address.SetBase ("10.0.0.0", "255.255.255.0");
  Ipv4InterfaceContainer apInterface;
  apInterface = address.Assign (apDevice);
  Ipv4InterfaceContainer staInterface;
  staInterface = address.Assign (staDevice);

  std::cout<< "apInterface.GetAddress (0)" << apInterface.GetAddress(0)<< std::endl;
  std::cout<< "staInterface.GetAddress (0)" << staInterface.GetAddress(0)<< std::endl;

  uint16_t port = 10;
  Ptr<UdpClient> s = CreateObject<UdpClient> ();
  s->SetRemote(staInterface.GetAddress(0),port);
  s->SetAttribute("MaxPackets",UintegerValue (numPackets));
  s->SetAttribute("Interval",TimeValue(interval));
  s->SetAttribute("PacketSize",UintegerValue (packetSize));
  s->SetStartTime(Seconds (1));
  ap->AddApplication(s);


  Ptr<UdpServer> r = CreateObject<UdpServer> ();
  r->SetAttribute("Port",UintegerValue (port));
  r->SetStartTime(Seconds (1));
  sta->AddApplication(r);

  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
  PopulateARPcache();

  Simulator::Stop (Seconds (1.005));
  Simulator::Run ();


  std::cout<< "n packets received " << r->GetReceived() << std::endl;
  std::cout<< "n bits received " << r->GetReceived() * (packetSize + 64) * 8 << std::endl;

  Simulator::Destroy ();

  return 0;
}