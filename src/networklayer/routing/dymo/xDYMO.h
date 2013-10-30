//
// Copyright (C) 2004 Andras Varga
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//

#ifndef __INET__DYMO_H_
#define __INET__DYMO_H_

#include <vector>
#include <map>
#include <omnetpp.h>
#include "ILifecycle.h"
#include "IAddressType.h"
#include "INetfilter.h"
#include "IRoutingTable.h"
#include "NodeStatus.h"
#include "UDPPacket.h"
#include "DYMOdefs.h"
#include "DYMORouteData.h"
#include "DYMO_m.h"

DYMO_NAMESPACE_BEGIN

/**
 * This class provides Dynamic MANET On-demand (DYMO also known as AODVv2) Routing
 * based on the IETF draft at http://tools.ietf.org/html/draft-ietf-manet-dymo-24.
 *
 * Optional features implemented:
 *  - 7.1. Route Discovery Retries and Buffering
 *    To reduce congestion in a network, repeated attempts at route
      discovery for a particular Target Node SHOULD utilize a binary
      exponential backoff.
 *  - 13.1. Expanding Rings Multicast
 *    Increase hop limit from min to max with each retry.
 *  - 13.2. Intermediate RREP
 *    Allow intermediate DYMO routers to reply with RREP.
 *  - 13.6. Message Aggregation
 *    RFC5148 add jitter to broadcasts
 */
class INET_API xDYMO : public cSimpleModule, public ILifecycle, public cListener, public INetfilter::IHook
{
  private:
    // DYMO parameters from RFC
    const char * clientAddresses;
    bool useMulticastRREP;
    const char * interfaces;
    double activeInterval;
    double maxIdleTime;
    double maxSequenceNumberLifetime;
    double routeRREQWaitTime;
    double rreqHolddownTime;
    int maxHopCount;
    int discoveryAttemptsMax;
    bool appendInformation;
    int bufferSizePackets;
    int bufferSizeBytes;

    // DYMO extension parameters
    simtime_t maxJitter;
    bool sendIntermediateRREP;
    int minHopLimit;
    int maxHopLimit;

    // context
    cModule * host;
    NodeStatus * nodeStatus;
    IAddressType * addressType;
    IInterfaceTable * interfaceTable;
    IRoutingTable * routingTable;
    INetfilter * networkProtocol;

    // internal
    cMessage * expungeTimer;
    DYMOSequenceNumber sequenceNumber;
    std::map<Address, DYMOSequenceNumber> targetAddressToSequenceNumber;
    std::map<Address, RREQTimer *> targetAddressToRREQTimer;
    std::multimap<Address, INetworkDatagram *> targetAddressToDelayedPackets;
    std::vector<std::pair<Address, int> > clientAddressAndPrefixLengthPairs; // 5.3.  Router Clients and Client Networks

  public:
    xDYMO();
    virtual ~xDYMO();

  protected:
    // module interface
    virtual int numInitStages() const { return NUM_INIT_STAGES; }
    void initialize(int stage);
    void handleMessage(cMessage * message);

  private:
    // handling messages
    void processSelfMessage(cMessage * message);
    void processMessage(cMessage * message);

    // route discovery
    void startRouteDiscovery(const Address & target);
    void retryRouteDiscovery(const Address & target, int retryCount);
    void completeRouteDiscovery(const Address & target);
    void cancelRouteDiscovery(const Address & target);
    bool hasOngoingRouteDiscovery(const Address & target);

    // handling IP datagrams
    void delayDatagram(INetworkDatagram * datagram);
    void reinjectDelayedDatagram(INetworkDatagram * datagram);
    void dropDelayedDatagram(INetworkDatagram * datagram);
    void eraseDelayedDatagrams(const Address & target);
    bool hasDelayedDatagrams(const Address & target);

    // handling RREQ timers
    void cancelRREQTimer(const Address & target);
    void deleteRREQTimer(const Address & target);
    void eraseRREQTimer(const Address & target);

    // handling RREQ wait RREP timer
    RREQWaitRREPTimer * createRREQWaitRREPTimer(const Address & target, int retryCount);
    void scheduleRREQWaitRREPTimer(RREQWaitRREPTimer * message);
    void processRREQWaitRREPTimer(RREQWaitRREPTimer * message);

    // handling RREQ backoff timer
    RREQBackoffTimer * createRREQBackoffTimer(const Address & target, int retryCount);
    void scheduleRREQBackoffTimer(RREQBackoffTimer * message);
    void processRREQBackoffTimer(RREQBackoffTimer * message);
    simtime_t computeRREQBackoffTime(int retryCount);

    // handling RREQ holddown timer
    RREQHolddownTimer * createRREQHolddownTimer(const Address & target);
    void scheduleRREQHolddownTimer(RREQHolddownTimer * message);
    void processRREQHolddownTimer(RREQHolddownTimer * message);

    // handling UDP packets
    void sendUDPPacket(UDPPacket * packet, double delay);
    void processUDPPacket(UDPPacket * packet);

    // handling DYMO packets
    void sendDYMOPacket(DYMOPacket * packet, const InterfaceEntry * interfaceEntry, const Address & nextHop, double delay);
    void processDYMOPacket(DYMOPacket * packet);

    // handling RteMsg packets
    bool permissibleRteMsg(RteMsg * rteMsg);
    void processRteMsg(RteMsg * rteMsg);
    int computeRteMsgBitLength(RteMsg * rteMsg);

    // handling RREQ packets
    RREQ * createRREQ(const Address & target, int retryCount);
    void sendRREQ(RREQ * rreq);
    void processRREQ(RREQ * rreq);
    int computeRREQBitLength(RREQ * rreq);

    // handling RREP packets
    RREP * createRREP(RteMsg * rteMsg);
    RREP * createRREP(RteMsg * rteMsg, IRoute * route);
    void sendRREP(RREP * rrep);
    void sendRREP(RREP * rrep, IRoute * route);
    void processRREP(RREP * rrep);
    int computeRREPBitLength(RREP * rrep);

    // handling RERR packets
    RERR * createRERR(std::vector<Address> & addresses);
    void sendRERR(RERR * rerr);
    void sendRERRForUndeliverablePacket(const Address & destination);
    void sendRERRForBrokenLink(const InterfaceEntry * interfaceEntry, const Address & nextHop);
    void processRERR(RERR * rerr);
    int computeRERRBitLength(RERR * rerr);

    // handling routes
    IRoute * createRoute(RteMsg * rteMsg, AddressBlock & addressBlock);
    void updateRoutes(RteMsg * rteMsg, AddressBlock & addressBlock);
    void updateRoute(RteMsg * rteMsg, AddressBlock & addressBlock, IRoute * route);
    int getLinkCost(const InterfaceEntry * interfaceEntry, DYMOMetricType metricType);
    bool isLoopFree(RteMsg * rteMsg, IRoute * route);

    // handling expunge timer
    void processExpungeTimer();
    void scheduleExpungeTimer();
    void expungeRoutes();
    simtime_t getNextExpungeTime();
    DYMORouteState getRouteState(DYMORouteData * routeData);

    // configuration
    bool isNodeUp();
    void configureInterfaces();

    // address
    std::string getHostName();
    Address getSelfAddress();
    bool isClientAddress(const Address & address);

    // added node
    void addSelfNode(RteMsg * rteMsg);
    void addNode(RteMsg * rteMsg, AddressBlock & addressBlock);

    // sequence number
    void incrementSequenceNumber();

    // routing
    Result ensureRouteForDatagram(INetworkDatagram * datagram);

    // netfilter
    virtual Result datagramPreRoutingHook(INetworkDatagram * datagram, const InterfaceEntry * inputInterfaceEntry, const InterfaceEntry *& outputInterfaceEntry, Address & nextHopAddress) { Enter_Method("datagramPreRoutingHook"); return ensureRouteForDatagram(datagram); }
    virtual Result datagramForwardHook(INetworkDatagram * datagram, const InterfaceEntry * inputInterfaceEntry, const InterfaceEntry *& outputInterfaceEntry, Address & nextHopAddress) { return ACCEPT; }
    virtual Result datagramPostRoutingHook(INetworkDatagram * datagram, const InterfaceEntry * inputInterfaceEntry, const InterfaceEntry *& outputInterfaceEntry, Address & nextHopAddress) { return ACCEPT; }
    virtual Result datagramLocalInHook(INetworkDatagram * datagram, const InterfaceEntry * inputInterfaceEntry) { return ACCEPT; }
    virtual Result datagramLocalOutHook(INetworkDatagram * datagram, const InterfaceEntry *& outputInterfaceEntry, Address & nextHopAddress) { Enter_Method("datagramLocalOutHook"); return ensureRouteForDatagram(datagram); }

    // lifecycle
    virtual bool handleOperationStage(LifecycleOperation * operation, int stage, IDoneCallback * doneCallback);

    // notification
    virtual void receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj);
};

DYMO_NAMESPACE_END

#endif
