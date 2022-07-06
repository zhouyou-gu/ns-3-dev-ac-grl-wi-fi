#include "ns3/command-line.h"
#include "ns3/config.h"
#include "ns3/double.h"
#include "ns3/string.h"
#include "ns3/log.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/ssid.h"
#include "ns3/mobility-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/yans-wifi-channel.h"
#include "ns3/mobility-model.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/uinteger.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("wifi-multi-ap");

void ReceivePacket (Ptr<Socket> socket)
{
  while (socket->Recv ())
    {
      std::cout << "Received one packet!" << std::endl;
    }
}

static void GenerateTraffic (Ptr<Socket> socket, uint32_t pktSize,
                 uint32_t pktCount, Time pktInterval )
{
  if (pktCount > 0)
    {
      NS_LOG_INFO ("Generating one packet of size " << pktSize);
      socket->Send (Create<Packet> (pktSize));
      Simulator::Schedule (pktInterval, &GenerateTraffic,
                           socket, pktSize, pktCount - 1, pktInterval);
    }
  else
    {
      socket->Close ();
    }
}

int main (int argc, char *argv[])
{
  LogComponentEnable ("wifi-multi-ap", LOG_LEVEL_INFO);

  std::string phyMode ("S1gOfdmRate0_30MbpsBW1MHz");
  double rss = -80;  // -dBm
  uint32_t packetSize = 1000; // bytes
  uint32_t numPackets = 10;
  Time interval = Seconds (1.0);
  bool verbose = false;

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

  // The below set of helpers will help us to put together the wifi NICs we want
  WifiHelper wifi;
  if (verbose)
    {
      wifi.EnableLogComponents ();  // Turn on all Wifi logging
    }
  wifi.SetStandard (ns3::WIFI_STANDARD_80211ah);

  YansWifiPhyHelper wifiPhy;
  wifiPhy.Set ("ChannelSettings", StringValue ("{0, 1, BAND_S1GHZ, 0}"));
//  std::cout << "Testing " << wifiPhy.Get  << " packets sent with receiver rss " << rss << std::endl;

  // This is one parameter that matters when using FixedRssLossModel
  // set it to zero; otherwise, gain will be added
  wifiPhy.Set ("RxGain", DoubleValue (0) );
  // ns-3 supports RadioTap and Prism tracing extensions for 802.11b
  wifiPhy.SetPcapDataLinkType (WifiPhyHelper::DLT_IEEE802_11_RADIO);

  YansWifiChannelHelper wifiChannel;
  wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  // The below FixedRssLossModel will cause the rss to be fixed regardless
  // of the distance between the two stations, and the transmit power
  wifiChannel.AddPropagationLoss ("ns3::FixedRssLossModel","Rss",DoubleValue (rss));
  wifiPhy.SetChannel (wifiChannel.Create ());

  // Add a mac and disable rate control
  WifiMacHelper wifiMac;
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                "DataMode", StringValue (phyMode),
                                "ControlMode", StringValue (phyMode));

  // Setup the rest of the MAC
  Ssid ssid = Ssid ("wifi-default");
  // setup STA
  wifiMac.SetType ("ns3::StaWifiMac",
                   "Ssid", SsidValue (ssid));
  NetDeviceContainer staDevice = wifi.Install (wifiPhy, wifiMac, c.Get (0));
  NetDeviceContainer devices = staDevice;
  // setup AP
  wifiMac.SetType ("ns3::ApWifiMac",
                   "Ssid", SsidValue (ssid));
  NetDeviceContainer apDevice = wifi.Install (wifiPhy, wifiMac, c.Get (1));
  devices.Add (apDevice);

  // Note that with FixedRssLossModel, the positions below are not
  // used for received signal strength.
  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector (0.0, 0.0, 0.0));
  positionAlloc->Add (Vector (5.0, 0.0, 0.0));
  mobility.SetPositionAllocator (positionAlloc);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (c);

  InternetStackHelper internet;
  internet.Install (c);

  Ipv4AddressHelper ipv4;
  ipv4.SetBase ("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer i = ipv4.Assign (devices);

  TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
  Ptr<Socket> recvSink = Socket::CreateSocket (c.Get (0), tid);
  InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), 80);
  recvSink->Bind (local);
  recvSink->SetRecvCallback (MakeCallback (&ReceivePacket));

  Ptr<Socket> source = Socket::CreateSocket (c.Get (1), tid);
  InetSocketAddress remote = InetSocketAddress (Ipv4Address ("255.255.255.255"), 80);
  source->SetAllowBroadcast (true);
  source->Connect (remote);

  // Tracing
  wifiPhy.EnablePcap ("wifi-simple-infra", devices);

  // Output what we are doing
  std::cout << "Testing " << numPackets  << " packets sent with receiver rss " << rss << std::endl;

  Simulator::ScheduleWithContext (source->GetNode ()->GetId (),
                                  Seconds (1.0), &GenerateTraffic,
                                  source, packetSize, numPackets, interval);

  Simulator::Stop (Seconds (1.1));
  Simulator::Run ();
  Simulator::Destroy ();

  return 0;
}
