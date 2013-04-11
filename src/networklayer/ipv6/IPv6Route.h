//
// Copyright (C) 2005 Andras Varga
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//

#ifndef __INET_IPV6ROUTE_H
#define __INET_IPV6ROUTE_H

#include <vector>

#include "INETDefs.h"

#include "IRoute.h"
#include "IPv6Address.h"

class InterfaceEntry;
class IPv6RoutingTable;

/**
 * Represents a route in the route table. Routes with sourceType=ROUTER_ADVERTISEMENT represent
 * on-link prefixes advertised by routers.
 */
// TODO change notifications
class INET_API IPv6Route : public cObject, public IRoute
{
  protected:
    IPv6RoutingTable *_rt; // TODO introduce IIPv6RoutingTable
    IPv6Address _destPrefix;
    short _prefixLength;
    SourceType _sourceType;
    InterfaceEntry *_interfacePtr;
    IPv6Address _nextHop;  // unspecified means "direct"
    simtime_t _expiryTime; // if route is an advertised prefix: prefix lifetime
    int _metric;
    cObject *_source;        /// Object identifying the source
    cObject *_protocolData; /// Routing Protocol specific data

  public:
    /**
     * Constructor. The destination prefix and the route source is passed
     * to the constructor and cannot be changed afterwards.
     */
    IPv6Route(IPv6Address destPrefix, int prefixLength, SourceType sourceType) {
        _rt = NULL;
        _destPrefix = destPrefix;
        _prefixLength = prefixLength;
        _sourceType = sourceType;
        _interfacePtr = NULL;
        _expiryTime = 0;
        _metric = 0;
        _source = NULL;
        _protocolData = NULL;
    }

    virtual ~IPv6Route() { }

    virtual std::string info() const;
    virtual std::string detailedInfo() const;

    /** To be called by the routing table when this route is added or removed from it */
    virtual void setRoutingTable(IPv6RoutingTable *rt) {_rt = rt;}
    IPv6RoutingTable *getRoutingTable() const {return _rt;}

    void setNextHop(const IPv6Address& nextHop)  {_nextHop = nextHop;}
    void setExpiryTime(simtime_t expiryTime)  {_expiryTime = expiryTime;}
    void setMetric(int metric)  {_metric = metric;}

    const IPv6Address& getDestPrefix() const {return _destPrefix;}
    virtual int getPrefixLength() const  {return _prefixLength;}
    virtual SourceType getSourceType() const { return _sourceType; }
    const IPv6Address& getNextHop() const  {return _nextHop;}
    simtime_t getExpiryTime() const  {return _expiryTime;}
    virtual int getMetric() const  {return _metric;}
    virtual IRoutingTable *getRoutingTableAsGeneric() const;

    virtual void setDestination(const Address& dest) { _destPrefix = dest.toIPv6();}
    virtual void setPrefixLength(int prefixLength) {_prefixLength = prefixLength;}
    virtual void setNextHop(const Address& nextHop) { _nextHop = nextHop.toIPv6();}
    virtual void setSource(cObject *source) {_source = source;}
    virtual void setSourceType(SourceType type) { _sourceType = type; }
    virtual Address getDestinationAsGeneric() const {return getDestPrefix();} //TODO rename IPv6 method
    virtual Address getNextHopAsGeneric() const {return getNextHop();}
    virtual InterfaceEntry *getInterface() const {return _interfacePtr; }
    virtual void setInterface(InterfaceEntry *ie) {_interfacePtr = ie;}
    virtual cObject *getSource() const {return _source;}
    virtual cObject *getProtocolData() const { return _protocolData; }
    virtual void setProtocolData(cObject *protocolData) { _protocolData = protocolData; }
};

#endif
