#include "populatearpcache.h"
using namespace ns3;

void ns3::PopulateARPcache ()
{
  Ptr<ArpCache> arp = CreateObject<ArpCache> ();
  arp->SetAliveTimeout (Seconds (3600 * 24 * 365) );

  for (NodeList::Iterator i = NodeList::Begin (); i != NodeList::End (); ++i)
  {
    Ptr<Ipv4L3Protocol> ip = (*i)->GetObject<Ipv4L3Protocol> ();
    NS_ASSERT (ip !=0);
    ObjectVectorValue interfaces;
    ip->GetAttribute ("InterfaceList", interfaces);

    for (ObjectVectorValue::Iterator j = interfaces.Begin (); j != interfaces.End (); j++)
    {
      Ptr<Ipv4Interface> ipIface = (*j).second->GetObject<Ipv4Interface> ();
      NS_ASSERT (ipIface != 0);
      Ptr<NetDevice> device = ipIface->GetDevice ();
      NS_ASSERT (device != 0);
      Mac48Address addr = Mac48Address::ConvertFrom (device->GetAddress () );

      for (uint32_t k = 0; k < ipIface->GetNAddresses (); k++)
      {
        Ipv4Address ipAddr = ipIface->GetAddress (k).GetLocal();
        if (ipAddr == Ipv4Address::GetLoopback ())
          continue;

        ArpCache::Entry *entry = arp->Add (ipAddr);
        Ipv4Header ipv4Hdr;
        ipv4Hdr.SetDestination (ipAddr);
        Ptr<Packet> p = Create<Packet> (100);
        entry->MarkWaitReply (ArpCache::Ipv4PayloadHeaderPair (p, ipv4Hdr));
        entry->MarkAlive (addr);
      }
    }
  }

  for (NodeList::Iterator i = NodeList::Begin (); i != NodeList::End (); ++i)
  {
    Ptr<Ipv4L3Protocol> ip = (*i)->GetObject<Ipv4L3Protocol> ();
    NS_ASSERT (ip !=0);
    ObjectVectorValue interfaces;
    ip->GetAttribute ("InterfaceList", interfaces);

    for (ObjectVectorValue::Iterator j = interfaces.Begin (); j != interfaces.End (); j ++)
    {
      Ptr<Ipv4Interface> ipIface = (*j).second->GetObject<Ipv4Interface> ();
      ipIface->SetAttribute ("ArpCache", PointerValue (arp) );
    }
  }
}