#include "populatearpcache.h"
namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("PopulateARPcache");

void PopulateARPcache () {
  Ptr<ArpCache> arp = CreateObject<ArpCache>();
  arp->SetAliveTimeout(Seconds(3600 * 24 * 365));

  for (NodeList::Iterator i = NodeList::Begin(); i != NodeList::End(); ++i) {
    Ptr<Ipv4L3Protocol> ip = (*i)->GetObject<Ipv4L3Protocol>();
    NS_ASSERT (ip != 0);
    ObjectVectorValue interfaces;
    ip->GetAttribute("InterfaceList", interfaces);

    for (ObjectVectorValue::Iterator j = interfaces.Begin(); j != interfaces.End(); j++) {
      Ptr<Ipv4Interface> ipIface = (*j).second->GetObject<Ipv4Interface>();
      NS_ASSERT (ipIface != 0);
      Ptr<NetDevice> device = ipIface->GetDevice();
      NS_ASSERT (device != 0);
      Mac48Address addr = Mac48Address::ConvertFrom(device->GetAddress());

      for (uint32_t k = 0; k < ipIface->GetNAddresses(); k++) {
        Ipv4Address ipAddr = ipIface->GetAddress(k).GetLocal();
        if (ipAddr == Ipv4Address::GetLoopback())
          continue;
        NS_LOG_FUNCTION("PopulateARPcacheForWiFiInterface" << "Node id: " << device->GetNode()->GetId());
        NS_LOG_FUNCTION("PopulateARPcacheForWiFiInterface" << "device name id: " << device->GetInstanceTypeId());
        NS_LOG_FUNCTION("PopulateARPcacheForWiFiInterface" << "if id: " << ipIface->GetMetric() << "/" << j->first);
        NS_LOG_FUNCTION("PopulateARPcacheForWiFiInterface" << "address id: " << ipAddr);

        ArpCache::Entry *entry = arp->Add(ipAddr);
        Ipv4Header ipv4Hdr;
        ipv4Hdr.SetDestination(ipAddr);
        Ptr<Packet> p = Create<Packet>(100);
        entry->MarkWaitReply(ArpCache::Ipv4PayloadHeaderPair(p, ipv4Hdr));
        entry->MarkAlive(addr);
      }
    }
  }
}

void PopulateARPcacheForWiFiInterface(NetDeviceContainer Dc) {
//  Ptr<ArpCache> arp = CreateObject<ArpCache> ();
//  arp->SetAliveTimeout (Seconds (3600 * 24 * 365) );
//
//  for (uint32_t i = 0; i != Dc.GetN(); ++i)
//  {
//    auto device = Dc.Get(i);
//    NS_LOG_FUNCTION("PopulateARPcacheForWiFiInterface"  << "Node id: " << device->GetNode()->GetId()
//                                                        << "Interface id : " << device->GetIfIndex()
//                                                        << "device name : " << device->GetInstanceTypeId()
//    );
//    Mac48Address addr = Mac48Address::ConvertFrom (device->GetAddress ());
//
//    auto ipv4 = device->GetNode()->GetObject<Ipv4>();
//    auto ipAddr = ipv4->GetAddress(device->GetIfIndex(),0);
//
//
//
//
//    device->GetIfIndex();
//    auto ipAddr = Ic.GetAddress(i);
//    if (ipAddr == Ipv4Address::GetLoopback ())
//      continue;
//    ArpCache::Entry *entry = arp->Add (ipAddr);
//    Ipv4Header ipv4Hdr;
//    ipv4Hdr.SetDestination (ipAddr);
//    Ptr<Packet> p = Create<Packet> (100);
//    entry->MarkWaitReply (ArpCache::Ipv4PayloadHeaderPair (p, ipv4Hdr));
//    entry->MarkAlive (addr);
//  }
//  for (uint32_t i = 0; i != Ic.GetN(); ++i)
//  {
//    Ptr<Ipv4> a = Ic.Get(i).first;
//    auto n = a->GetNetDevice(Ic.Get(i).second)->GetNode();
//    Ptr<Ipv4L3Protocol> ip = n->GetObject<Ipv4L3Protocol> ();
//    NS_ASSERT (ip !=0);
//    ObjectVectorValue interfaces;
//    ip->GetAttribute ("InterfaceList", interfaces);
//    for (auto j = interfaces.Begin (); j != interfaces.End (); j ++)
//    {
//      Ptr<Ipv4Interface> ipIface = (*j).second->GetObject<Ipv4Interface> ();
//      ipIface->SetAttribute ("ArpCache", PointerValue (arp) );
//    }
//  }
}


}