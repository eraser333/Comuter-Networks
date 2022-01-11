/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2017 Alexander Afanasyev
 *
 * This program is free software: you can redistribute it and/or modify it under the terms of
 * the GNU General Public License as published by the Free Software Foundation, either version
 * 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with this program.
 * If not, see <http://www.gnu.org/licenses/>.
 */

#include "simple-router.hpp"
#include "core/utils.hpp"

#include <fstream>

namespace simple_router {

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
// IMPLEMENT THIS METHOD
void
SimpleRouter::handlePacket(const Buffer& packet, const std::string& inIface)
{
  std::cerr << "Got packet of size " << packet.size() << " on interface " << inIface << std::endl;

  const Interface* iface = findIfaceByName(inIface);
  if (iface == nullptr) {
    std::cerr << "Received packet, but interface is unknown, ignoring" << std::endl;
    return;
  }

  // debug , it is strange everytime cerr out this
  // std::cerr << getRoutingTable() << std::endl;

  // FILL THIS IN

  // step1, check EthHeader
  ethernet_hdr* ethHeaderPtr = nullptr;
  if (!checkEthHeader(ethHeaderPtr, packet, inIface)){
    return;
  }

  // step2, arp or IPV4
  if (ntohs(ethHeaderPtr->ether_type) == ethertype_ip){
    ip_hdr* ipv4HeaderPtr = nullptr;
    if (!checkIPV4Header(ipv4HeaderPtr, packet, inIface)){
      return;
    }

    // get send Interface, if nullptr means packet's destination is this router
    // otherwise, packet need to be forwarding
    const Interface* sendIface = findIfaceByIp(ipv4HeaderPtr->ip_dst);
    if (sendIface){
      // debug
      // std::cerr << "sendIface != nullptr" << std::endl; 
      if(ipv4HeaderPtr->ip_p == ip_protocol_icmp) {
        // check icmp header: length, checksum
        icmp_hdr* icmpHeaderPtr = nullptr;
        if(!checkIcmpHeader(packet, icmpHeaderPtr)){
          return;
        }
        // ICMP echo reply
        std::cerr << "send ICMP echo reply" << std::endl;
        sendIcmpEchoReply(packet, inIface);
      }
      // tcp or udp 
      else if(ipv4HeaderPtr->ip_p == 0x0006 || ipv4HeaderPtr->ip_p == 0x0011) {
        // ICMP port unreachable
        std::cerr << "ICMP port unreachable" << std::endl;
        sendIcmpPacket(packet, inIface, 3, 3);
      }
    }
    else{
      // headle time Exceed
      if (ipv4HeaderPtr->ip_ttl <= 1){
        // send ICMP time Exceed
        sendIcmpPacket(packet, inIface, 11, 0);
      }
      else{
        // keep packet forwarding
        keepPacketForwarding(packet, inIface, ipv4HeaderPtr->ip_dst);
      }
    }
  }
  else if (ntohs(ethHeaderPtr->ether_type) == ethertype_arp) {
    arp_hdr* arpHeaderPtr = nullptr;
    if (!checkArpHeader(arpHeaderPtr, packet, inIface)) {
      return;
    }

    if (ntohs(arpHeaderPtr->arp_op) == arp_op_request){
      // handle arprequest == send arp reply , tricky
      handleArpRequest(arpHeaderPtr);
    }
    else if (ntohs(arpHeaderPtr->arp_op) == arp_op_reply){
      handleArpReply(arpHeaderPtr);
    }
  }
}
/*
  The following is the function implemented by myself
*/
bool 
SimpleRouter:: checkEthHeader(ethernet_hdr*& ethHeaderPtr, const Buffer& packet, const std::string& inIface){
  //debug
  //std::cerr <<  "checkEthHeader" << std::endl;
  if (packet.size() < sizeof(ethernet_hdr)) {
    return false;
  }

  ethHeaderPtr = (ethernet_hdr*) packet.data();
  if (ntohs(ethHeaderPtr->ether_type) != ethertype_ip && ntohs(ethHeaderPtr->ether_type) != ethertype_arp) {
    return false;
  }

  // is boardcast? or dest is this router
  bool isBoardcast = false;
  int i = 0;
  for (; i < ETHER_ADDR_LEN; i++){
    if (ethHeaderPtr->ether_dhost[i] != 0xff) break;
  }
  if (i == ETHER_ADDR_LEN) {
    isBoardcast = true;
  }

  const Interface* inInterface = findIfaceByName(inIface);
  bool isDestRouter = (memcmp(ethHeaderPtr->ether_dhost, inInterface->addr.data(), ETHER_ADDR_LEN) == 0);
  if (!isDestRouter && !isBoardcast){
    return false;
  }

  // std::cerr <<  "checkEthHeader true" << std::endl;
  return true;
}

bool
SimpleRouter::checkIPV4Header(ip_hdr*& ipv4HeaderPtr,const Buffer& packet, const std::string& inIface){
  //std::cerr <<  "checkIPV4Header" << std::endl;
  if (packet.size() < sizeof(ethernet_hdr) + sizeof(ip_hdr)) { 
    return false;
  }

  ipv4HeaderPtr = (ip_hdr*)((ethernet_hdr*) packet.data()+1);
  if (packet.size() < sizeof(ethernet_hdr) + ipv4HeaderPtr->ip_hl * 4){
    return false;
  }

  if (cksum(ipv4HeaderPtr, sizeof(ip_hdr)) != 0xffff) {
    return false;
  }

  // std::cerr <<  "checkIPV4Header true" << std::endl;
  return true;
}

void 
SimpleRouter::sendIcmpPacket(const Buffer& packet, const std::string& inIface, uint8_t icmp_type, uint8_t icmp_code){
  //debug
  std::cerr << "sendIcmpPacket" << std::endl;

  Buffer icmpPacket(sizeof(icmp_t3_hdr) + sizeof(ip_hdr) + sizeof(ethernet_hdr));

  //debug
  std::cerr << "icmpPacket size:" << icmpPacket.size() << std::endl; 

  uint32_t source_ip = findIfaceByName(inIface)->ip;
  ethernet_hdr* ethHdr = (ethernet_hdr*) icmpPacket.data();
  ip_hdr* ipHdr = (ip_hdr*)((ethernet_hdr*) icmpPacket.data()+1);
  icmp_t3_hdr* icmpHdr = (icmp_t3_hdr*)((ip_hdr*)((ethernet_hdr*) icmpPacket.data()+1)+1);

  ip_hdr* ipv4HeaderPtr = nullptr;
  ethernet_hdr* ethHeaderPtr = nullptr;
  
  checkEthHeader(ethHeaderPtr, packet, inIface);
  checkIPV4Header(ipv4HeaderPtr, packet, inIface);

  memcpy(ethHdr->ether_shost, ethHeaderPtr->ether_dhost, ETHER_ADDR_LEN); 
  memcpy(ethHdr->ether_dhost, ethHeaderPtr->ether_shost, ETHER_ADDR_LEN); 
  ethHdr->ether_type = htons(ethertype_ip);
  
  ipHdr->ip_len = htons(sizeof(ip_hdr) + sizeof(icmp_t3_hdr));
  ipHdr->ip_id = ipv4HeaderPtr->ip_id; // Ask TA in class, TA's words opaque
  ipHdr->ip_ttl = 64;
  ipHdr->ip_p = ip_protocol_icmp;
  ipHdr->ip_src = source_ip;
  ipHdr->ip_dst = ipv4HeaderPtr->ip_src;
  ipHdr->ip_hl = ipv4HeaderPtr->ip_hl;
  ipHdr->ip_off = ipv4HeaderPtr->ip_off; ///////////////////////////////////// Ask TA in class, TA's words opaque
  ipHdr->ip_v = ipv4HeaderPtr->ip_v;
  ipHdr->ip_tos = ipv4HeaderPtr->ip_tos;
  ipHdr->ip_sum = 0;
  uint16_t checksum_ipv4 = cksum(ipHdr, sizeof(ip_hdr));
  ipHdr->ip_sum = checksum_ipv4;
    
  // according to document , data = Internet Header + 64 bits of Original Data Datagram ???
  icmpHdr->icmp_type = icmp_type;
  icmpHdr->icmp_code = icmp_code; 
  icmpHdr->unused = 0; 
  icmpHdr->next_mtu = 0; 
  icmpHdr->icmp_sum = 0;
  memcpy(icmpHdr->data, ipv4HeaderPtr, ICMP_DATA_SIZE); // not sure
  uint16_t checksum_icmp = cksum(icmpHdr, sizeof(icmp_t3_hdr));
  icmpHdr->icmp_sum = checksum_icmp;


  //debug
  print_hdr_eth((uint8_t*)ethHdr);
  print_hdr_ip((uint8_t*)ipHdr);

  sendPacket(icmpPacket, inIface);
}

void 
SimpleRouter::keepPacketForwarding(const Buffer& packet, const std::string& inIface, uint32_t dest_ip){
  // debug
  std:: cerr << "keepPacketForwarding" << std::endl;

  std::shared_ptr<ArpEntry> arp_entry = m_arp.lookup(dest_ip);
  // std::cerr << arp_entry << std::endl;
  // if there is no ip-mac 
  if (!arp_entry) {
    auto useless = m_arp.queueRequest(dest_ip, packet, inIface);
    std::cerr << "add arpRequest"  << std::endl;
    return;
  }

  Buffer packet_forward = packet;
  ethernet_hdr* ethHeaderPtr = (ethernet_hdr*) packet_forward.data();
  RoutingTableEntry route_entry = m_routingTable.lookup(dest_ip);
  const Interface* outInface = findIfaceByName(route_entry.ifName);

  memcpy(ethHeaderPtr->ether_dhost, arp_entry->mac.data(), ETHER_ADDR_LEN); 
  memcpy(ethHeaderPtr->ether_shost, outInface->addr.data(), ETHER_ADDR_LEN); 

  ip_hdr* ipv4HeaderPtr = (ip_hdr*)((ethernet_hdr*) packet_forward.data()+1);
  ipv4HeaderPtr->ip_ttl--;
  ipv4HeaderPtr->ip_sum = 0;
  ipv4HeaderPtr->ip_sum = cksum(ipv4HeaderPtr, sizeof(ip_hdr));

  sendPacket(packet_forward, route_entry.ifName);
}

bool 
SimpleRouter::checkIcmpHeader(const Buffer& packet, icmp_hdr*& icmpHeaderPtr){
  if (packet.size() < sizeof(icmp_hdr) + sizeof(ip_hdr) + sizeof(ethernet_hdr)){
    return false;
  }

  icmpHeaderPtr = (icmp_hdr*)((ip_hdr*)((ethernet_hdr*) packet.data()+1)+1);
  if (cksum(icmpHeaderPtr, packet.size() - sizeof(ethernet_hdr) - sizeof(ip_hdr)) != 0xffff){
    return false;
  }

  return true;
}

void
SimpleRouter::sendIcmpEchoReply(const Buffer& packet, const std::string& inIface){
  Buffer icmpPacket(packet.size());

  //debug
  std::cerr << packet.size() << " " << icmpPacket.size() << std::endl;

  ethernet_hdr* ethHdr = (ethernet_hdr*) icmpPacket.data();
  ip_hdr* ipHdr = (ip_hdr*)((ethernet_hdr*) icmpPacket.data()+1);
  icmp_hdr* icmpHdr = (icmp_hdr*)((ip_hdr*)((ethernet_hdr*) icmpPacket.data()+1)+1);

  ip_hdr* ipv4HeaderPtr = nullptr;
  ethernet_hdr* ethHeaderPtr = nullptr;
  uint8_t* icmpHeaderPtr =  (uint8_t*)((ip_hdr*)((ethernet_hdr*) packet.data()+1)+1);
  
  checkEthHeader(ethHeaderPtr, packet, inIface);
  checkIPV4Header(ipv4HeaderPtr, packet, inIface);

  memcpy(ethHdr->ether_shost, ethHeaderPtr->ether_dhost, ETHER_ADDR_LEN); 
  memcpy(ethHdr->ether_dhost, ethHeaderPtr->ether_shost, ETHER_ADDR_LEN); 
  ethHdr->ether_type = htons(ethertype_ip);

  ipHdr->ip_len = ipv4HeaderPtr->ip_len;
  ipHdr->ip_id = ipv4HeaderPtr->ip_id; // not sure
  ipHdr->ip_ttl = 64;
  ipHdr->ip_p = ip_protocol_icmp;
  ipHdr->ip_src = ipv4HeaderPtr->ip_dst;
  ipHdr->ip_dst = ipv4HeaderPtr->ip_src;
  ipHdr->ip_hl = ipv4HeaderPtr->ip_hl;
  ipHdr->ip_off = ipv4HeaderPtr->ip_off; 
  //0x4000 ? it is strange that if I use 0x4000, it is wrong, but it works correctly in function 'sendIcmpPacket'
  ipHdr->ip_v = ipv4HeaderPtr->ip_v;
  ipHdr->ip_tos = ipv4HeaderPtr->ip_tos;
  ipHdr->ip_sum = 0;
  uint16_t checksum_ipv4 = cksum(ipHdr, sizeof(ip_hdr));
  ipHdr->ip_sum = checksum_ipv4;

  memcpy(icmpHdr, icmpHeaderPtr, packet.size()-sizeof(ethernet_hdr)-sizeof(ip_hdr));
  icmpHdr->icmp_type = 0; //echo reply
  icmpHdr->icmp_code = 0; //echo reply
  icmpHdr->icmp_sum = 0;
  uint16_t checksum_icmp = cksum(icmpHdr, packet.size()-sizeof(ethernet_hdr)-sizeof(ip_hdr));
  icmpHdr->icmp_sum = checksum_icmp;
  
  //debug
  print_hdr_icmp((uint8_t*)icmpHdr);
  print_hdr_ip((uint8_t*)ipHdr);
  print_hdr_ip((uint8_t*)ipv4HeaderPtr);
  print_hdr_eth((uint8_t*)ethHdr);
  
  //debug
  std::cerr << inIface << std::endl;
  sendPacket(icmpPacket, inIface);
}

bool 
SimpleRouter::checkArpHeader(arp_hdr*& arpHeaderPtr,const Buffer& packet, const std::string& inIface){
  if (packet.size() == sizeof(ethernet_hdr) + sizeof(arp_hdr)){
    arpHeaderPtr = (arp_hdr*)((ethernet_hdr*)packet.data()+1);
    return true;
  }
  else {
    return false;
  }
}

void 
SimpleRouter::handleArpRequest(arp_hdr*& arpHeaderPtr){
  const Interface* iface = findIfaceByIp(arpHeaderPtr->arp_tip);

  // wrong! eth frame dest to router but arp not
  if (iface == nullptr){
    return;
  }

  // send arp reply
  // debug
  std::cerr << "send arp reply" << std::endl;
  Buffer packet(sizeof(ethernet_hdr) + sizeof(arp_hdr));
  ethernet_hdr* packetEthHeaderPtr = (ethernet_hdr*)packet.data();
  arp_hdr* packetArpHeaderPtr = (arp_hdr*)((ethernet_hdr*)packet.data()+1);

  packetArpHeaderPtr->arp_hrd = arpHeaderPtr->arp_hrd;
  packetArpHeaderPtr->arp_pro = arpHeaderPtr->arp_pro;
  packetArpHeaderPtr->arp_hln = arpHeaderPtr->arp_hln;
  packetArpHeaderPtr->arp_pln = arpHeaderPtr->arp_pln;
  packetArpHeaderPtr->arp_op = htons(arp_op_reply);
  packetArpHeaderPtr->arp_sip = iface->ip;
  packetArpHeaderPtr->arp_tip = arpHeaderPtr->arp_sip;
  memcpy(packetArpHeaderPtr->arp_tha, arpHeaderPtr->arp_sha, ETHER_ADDR_LEN);
  memcpy(packetArpHeaderPtr->arp_sha, iface->addr.data(), ETHER_ADDR_LEN);

  memcpy(packetEthHeaderPtr->ether_dhost, arpHeaderPtr->arp_sha, ETHER_ADDR_LEN);
  memcpy(packetEthHeaderPtr->ether_shost, iface->addr.data(), ETHER_ADDR_LEN);
  packetEthHeaderPtr->ether_type = htons(ethertype_arp);

  sendPacket(packet, iface->name);
}

void 
SimpleRouter::sendArpRequest(uint32_t ip){
  //debug
  std::cerr << "sendArpRequest" << std::endl;
  RoutingTableEntry  outEntry = m_routingTable.lookup(ip);
  const Interface* outIface = findIfaceByName(outEntry.ifName);

  Buffer packet(sizeof(ethernet_hdr) + sizeof(arp_hdr));
  ethernet_hdr* packetEthHeaderPtr = (ethernet_hdr*)packet.data();
  arp_hdr* packetArpHeaderPtr = (arp_hdr*)((ethernet_hdr*)packet.data()+1);

  packetArpHeaderPtr->arp_hrd = htons(arp_hrd_ethernet);
  packetArpHeaderPtr->arp_pro = htons(0x0800);
  packetArpHeaderPtr->arp_hln = 0x06;
  packetArpHeaderPtr->arp_pln = 0x04;
  packetArpHeaderPtr->arp_op = htons(arp_op_request);
  packetArpHeaderPtr->arp_sip = outIface->ip;
  packetArpHeaderPtr->arp_tip = ip;
  uint8_t broadcastAddr[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
  memcpy(packetArpHeaderPtr->arp_tha, broadcastAddr, ETHER_ADDR_LEN);
  memcpy(packetArpHeaderPtr->arp_sha, outIface->addr.data(), ETHER_ADDR_LEN);

  memcpy(packetEthHeaderPtr->ether_dhost, broadcastAddr, ETHER_ADDR_LEN);
  memcpy(packetEthHeaderPtr->ether_shost, outIface->addr.data(), ETHER_ADDR_LEN);
  packetEthHeaderPtr->ether_type = htons(ethertype_arp);

  sendPacket(packet, outIface->name);
}

void 
SimpleRouter::handleArpReply(arp_hdr*& arpHeaderPtr){
  uint32_t ip = arpHeaderPtr->arp_sip;
  // debug
  std::cerr << "receive ArpReply from " << ipToString(ip) << std::endl;
  if (!m_arp.lookup(ip)){
    Buffer macAddr(ETHER_ADDR_LEN);
    memcpy(macAddr.data(), arpHeaderPtr->arp_sha, ETHER_ADDR_LEN);
    // insert  ip-mac
    auto request = m_arp.insertArpEntry(macAddr, ip);
    if (request) {
      for (auto packet: request->packets){
        //std::cerr << "fsjykc" << std::endl;
        handlePacket(packet.packet, packet.iface);
      }
      m_arp.removeRequest(request);
    }
  }
}
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// You should not need to touch the rest of this code.
SimpleRouter::SimpleRouter()
  : m_arp(*this)
{
}

void
SimpleRouter::sendPacket(const Buffer& packet, const std::string& outIface)
{
  m_pox->begin_sendPacket(packet, outIface);
}

bool
SimpleRouter::loadRoutingTable(const std::string& rtConfig)
{
  return m_routingTable.load(rtConfig);
}

void
SimpleRouter::loadIfconfig(const std::string& ifconfig)
{
  std::ifstream iff(ifconfig.c_str());
  std::string line;
  while (std::getline(iff, line)) {
    std::istringstream ifLine(line);
    std::string iface, ip;
    ifLine >> iface >> ip;

    in_addr ip_addr;
    if (inet_aton(ip.c_str(), &ip_addr) == 0) {
      throw std::runtime_error("Invalid IP address `" + ip + "` for interface `" + iface + "`");
    }

    m_ifNameToIpMap[iface] = ip_addr.s_addr;
  }
}

void
SimpleRouter::printIfaces(std::ostream& os)
{
  if (m_ifaces.empty()) {
    os << " Interface list empty " << std::endl;
    return;
  }

  for (const auto& iface : m_ifaces) {
    os << iface << "\n";
  }
  os.flush();
}

const Interface*
SimpleRouter::findIfaceByIp(uint32_t ip) const
{
  auto iface = std::find_if(m_ifaces.begin(), m_ifaces.end(), [ip] (const Interface& iface) {
      return iface.ip == ip;
    });

  if (iface == m_ifaces.end()) {
    return nullptr;
  }

  return &*iface;
}

const Interface*
SimpleRouter::findIfaceByMac(const Buffer& mac) const
{
  auto iface = std::find_if(m_ifaces.begin(), m_ifaces.end(), [mac] (const Interface& iface) {
      return iface.addr == mac;
    });

  if (iface == m_ifaces.end()) {
    return nullptr;
  }

  return &*iface;
}

const Interface*
SimpleRouter::findIfaceByName(const std::string& name) const
{
  auto iface = std::find_if(m_ifaces.begin(), m_ifaces.end(), [name] (const Interface& iface) {
      return iface.name == name;
    });

  if (iface == m_ifaces.end()) {
    return nullptr;
  }

  return &*iface;
}

void
SimpleRouter::reset(const pox::Ifaces& ports)
{
  std::cerr << "Resetting SimpleRouter with " << ports.size() << " ports" << std::endl;

  m_arp.clear();
  m_ifaces.clear();

  for (const auto& iface : ports) {
    auto ip = m_ifNameToIpMap.find(iface.name);
    if (ip == m_ifNameToIpMap.end()) {
      std::cerr << "IP_CONFIG missing information about interface `" + iface.name + "`. Skipping it" << std::endl;
      continue;
    }

    m_ifaces.insert(Interface(iface.name, iface.mac, ip->second));
  }

  printIfaces(std::cerr);
}


} // namespace simple_router {
