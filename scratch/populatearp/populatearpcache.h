#ifndef POPULATEARPCACHE_H
#define POPULATEARPCACHE_H
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
namespace ns3 {
    void PopulateARPcache();
    void PopulateARPcacheForWiFiInterface (NetDeviceContainer Dc);
}
#endif /* POPULATEARPCACHE_H */
