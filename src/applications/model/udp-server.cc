/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 *  Copyright (c) 2007,2008,2009 INRIA, UDCAST
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Amine Ismail <amine.ismail@sophia.inria.fr>
 *                      <amine.ismail@udcast.com>
 */

#include "ns3/log.h"
#include "ns3/ipv4-address.h"
#include "ns3/nstime.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "packet-loss-counter.h"

#include "seq-ts-header.h"
#include "udp-server.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("UdpServer");

NS_OBJECT_ENSURE_REGISTERED (UdpServer);


TypeId
UdpServer::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::UdpServer")
    .SetParent<Application> ()
    .SetGroupName("Applications")
    .AddConstructor<UdpServer> ()
    .AddAttribute ("Port",
                   "Port on which we listen for incoming packets.",
                   UintegerValue (100),
                   MakeUintegerAccessor (&UdpServer::m_port),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("PacketWindowSize",
                   "The size of the window used to compute the packet loss. This value should be a multiple of 8.",
                   UintegerValue (32),
                   MakeUintegerAccessor (&UdpServer::GetPacketWindowSize,
                                         &UdpServer::SetPacketWindowSize),
                   MakeUintegerChecker<uint16_t> (8,256))
    .AddTraceSource ("Rx", "A packet has been received",
                     MakeTraceSourceAccessor (&UdpServer::m_rxTrace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("RxWithAddresses", "A packet has been received",
                     MakeTraceSourceAccessor (&UdpServer::m_rxTraceWithAddresses),
                     "ns3::Packet::TwoAddressTracedCallback")
  ;
  return tid;
}

UdpServer::UdpServer ()
  : m_lossCounter (0)
{
  NS_LOG_FUNCTION (this);
  m_received=0;
}

UdpServer::~UdpServer ()
{
  NS_LOG_FUNCTION (this);
}

uint16_t
UdpServer::GetPacketWindowSize () const
{
  NS_LOG_FUNCTION (this);
  return m_lossCounter.GetBitMapSize ();
}

void
UdpServer::SetPacketWindowSize (uint16_t size)
{
  NS_LOG_FUNCTION (this << size);
  m_lossCounter.SetBitMapSize (size);
}

uint32_t
UdpServer::GetLost (void) const
{
  NS_LOG_FUNCTION (this);
  return m_lossCounter.GetLost ();
}

uint64_t
UdpServer::GetReceived (void) const
{
  NS_LOG_FUNCTION (this);
  return m_received;
}

void
UdpServer::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  Application::DoDispose ();
}

void
UdpServer::StartApplication (void)
{
  NS_LOG_FUNCTION (this);
  m_last_received_packet_sending_time = Simulator::Now();
  if (m_socket == 0)
    {
      TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
      m_socket = Socket::CreateSocket (GetNode (), tid);
      InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (),
                                                   m_port);
      if (m_socket->Bind (local) == -1)
        {
          NS_FATAL_ERROR ("Failed to bind socket");
        }
    }

  m_socket->SetRecvCallback (MakeCallback (&UdpServer::HandleRead, this));

  if (m_socket6 == 0)
    {
      TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
      m_socket6 = Socket::CreateSocket (GetNode (), tid);
      Inet6SocketAddress local = Inet6SocketAddress (Ipv6Address::GetAny (),
                                                   m_port);
      if (m_socket6->Bind (local) == -1)
        {
          NS_FATAL_ERROR ("Failed to bind socket");
        }
    }

  m_socket6->SetRecvCallback (MakeCallback (&UdpServer::HandleRead, this));

}

void
UdpServer::StopApplication ()
{
  NS_LOG_FUNCTION (this);
  Time r = Simulator::Now() - m_last_received_packet_sending_time;
  m_aoi_area += (double) (r.GetMicroSeconds() * r.GetMicroSeconds()) / 2;
  m_mean_aoi = m_aoi_area / (double) (m_stopTime.GetMicroSeconds() - m_startTime.GetMicroSeconds());
  m_delay_avg = m_delay_sum / (double) m_received;
  m_interval_avg = m_interval_sum / (double) m_received;

  Time t = Simulator::Now() - m_startTime;
  m_throughput = ((double) m_received)/t.GetSeconds();

  if (m_socket != 0)
    {
      m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
    }
  m_is_stopped = true;
}

void
UdpServer::HandleRead (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  Ptr<Packet> packet;
  Address from;
  Address localAddress;
  while ((packet = socket->RecvFrom (from)))
    {
      socket->GetSockName (localAddress);
      m_rxTrace (packet);
      m_rxTraceWithAddresses (packet, from, localAddress);
      if (packet->GetSize () > 0)
        {
          uint32_t receivedSize = packet->GetSize ();
          SeqTsHeader seqTs;
          packet->RemoveHeader (seqTs);
          uint32_t currentSequenceNumber = seqTs.GetSeq ();
          if (InetSocketAddress::IsMatchingType (from))
            {
              NS_LOG_INFO ("TraceDelay: RX " << receivedSize <<
                           " bytes from "<< InetSocketAddress::ConvertFrom (from).GetIpv4 () <<
                           " Sequence Number: " << currentSequenceNumber <<
                           " Uid: " << packet->GetUid () <<
                           " TXtime: " << seqTs.GetTs () <<
                           " RXtime: " << Simulator::Now () <<
                           " Delay: " << Simulator::Now () - seqTs.GetTs ());
            }
          else if (Inet6SocketAddress::IsMatchingType (from))
            {
              NS_LOG_INFO ("TraceDelay: RX " << receivedSize <<
                           " bytes from "<< Inet6SocketAddress::ConvertFrom (from).GetIpv6 () <<
                           " Sequence Number: " << currentSequenceNumber <<
                           " Uid: " << packet->GetUid () <<
                           " TXtime: " << seqTs.GetTs () <<
                           " RXtime: " << Simulator::Now () <<
                           " Delay: " << Simulator::Now () - seqTs.GetTs ());
            }

          m_lossCounter.NotifyReceived (currentSequenceNumber);
          m_received++;
          Time interval = seqTs.GetTs () - m_last_received_packet_sending_time;
          Time p_delay = Simulator::Now () - seqTs.GetTs ();
          double aoi_a_delta = 1./2. *  (double) ( interval.GetMicroSeconds() * interval.GetMicroSeconds()) +  (double)  (interval.GetMicroSeconds() * p_delay.GetMicroSeconds());
          m_aoi_area += aoi_a_delta;
          m_last_received_packet_sending_time = seqTs.GetTs ();
          m_delay_sum += p_delay.GetMicroSeconds();
          m_interval_sum += interval.GetMicroSeconds();
          NS_LOG_INFO ("AoI UDP: Port " << m_port);
          NS_LOG_INFO ("AoI UDP: Packet sending interval (us) " << interval.GetMicroSeconds());
          NS_LOG_INFO ("AoI UDP: Packet delay (us) " << p_delay.GetMicroSeconds());
          NS_LOG_INFO ("AoI UDP: AoI area (us^2) " << m_aoi_area);
          NS_LOG_INFO ("AoI UDP: AoI area delta (us^2) " << aoi_a_delta);
          NS_LOG_INFO ("AoI UDP: Delay sum (us) " << m_delay_sum);
        }
    }
}

double
UdpServer::GetLastAoI_us()
{
  NS_LOG_FUNCTION (this);
  if (! m_is_stopped)
  {
    NS_LOG_UNCOND("aoi should be calculated after the app is stopped");
  }
  return m_mean_aoi;
}

double
UdpServer::GetAvgDelay_us()
{
  NS_LOG_FUNCTION (this);
  if (! m_is_stopped)
  {
    NS_LOG_UNCOND("Delay should be calculated after the app is stopped");
  }
  return m_delay_avg;
}


double
UdpServer::GetAvgInterval_us()
{
  NS_LOG_FUNCTION (this);
  if (! m_is_stopped)
  {
    NS_LOG_UNCOND("Interval should be calculated after the app is stopped");
  }
  return m_interval_avg;
}

double
UdpServer::GetAvgThroughput_pkt ()
{
  NS_LOG_FUNCTION (this);
  if (! m_is_stopped)
  {
    NS_LOG_UNCOND("Interval should be calculated after the app is stopped");
  }
  return m_throughput;
}

} // Namespace ns3
