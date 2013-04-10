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

/**
 * Represents a route in the route table. Routes with src=FROM_RA represent
 * on-link prefixes advertised by routers.
 */
class INET_API IPv6Route : public cObject, public IRoute
{
  public:
    /** Specifies where the route comes from */
    enum RouteSrc
    {
        FROM_RA,        ///< on-link prefix, from Router Advertisement
        OWN_ADV_PREFIX, ///< on routers: on-link prefix that the router **itself** advertises on the link
        STATIC,         ///< static route
        ROUTING_PROT,   ///< route is managed by a routing protocol (OSPF,BGP,etc)
    };

  protected:
    IPv6Address _destPrefix;
    short _length;
    RouteSrc _src;
    int _interfaceID;      //XXX IPv4 IIPv4RoutingTable uses interface pointer
    IPv6Address _nextHop;  // unspecified means "direct"
    simtime_t _expiryTime; // if route is an advertised prefix: prefix lifetime
    int _metric;

  public:
    /**
     * Constructor. The destination prefix and the route source is passed
     * to the constructor and cannot be changed afterwards.
     */
    IPv6Route(IPv6Address destPrefix, int length, RouteSrc src) {
        _destPrefix = destPrefix;
        _length = length;
        _src = src;
        _interfaceID = -1;
        _expiryTime = 0;
        _metric = 0;
    }

    virtual ~IPv6Route() { }

    virtual std::string info() const;
    virtual std::string detailedInfo() const;
    static const char *routeSrcName(RouteSrc src);

    void setInterfaceId(int interfaceId)  {_interfaceID = interfaceId;}
    void setNextHop(const IPv6Address& nextHop)  {_nextHop = nextHop;}
    void setExpiryTime(simtime_t expiryTime)  {_expiryTime = expiryTime;}
    void setMetric(int metric)  {_metric = metric;}

    const IPv6Address& getDestPrefix() const {return _destPrefix;}
    int getPrefixLength() const  {return _length;}
    RouteSrc getSrc() const  {return _src;}
    int getInterfaceId() const  {return _interfaceID;}
    const IPv6Address& getNextHop() const  {return _nextHop;}
    simtime_t getExpiryTime() const  {return _expiryTime;}
    int getMetric() const  {return _metric;}
    virtual IRoutingTable *getRoutingTableAsGeneric() const {return NULL; /*TODO: getRoutingTable();*/}

    virtual void setDestination(const Address& dest) {/*TODO: setDestination(dest.toIPv6());*/}
    virtual void setPrefixLength(int len) {/*TODO: setPrefixLength(len));*/}
    virtual void setNextHop(const Address& nextHop) {setNextHop(nextHop.toIPv6());}
    virtual void setSource(cObject *source) {/*TODO: setSource(source);*/}

    virtual Address getDestinationAsGeneric() const {return getDestPrefix();} //TODO rename IPv6 method
    virtual Address getNextHopAsGeneric() const {return getNextHop();}
    virtual InterfaceEntry *getInterface() const {return NULL; /*TODO getInterface();*/} //TODO change IPv6Route API from interfaceID to interface ptr
    virtual void setInterface(InterfaceEntry *ie) {_interfaceID = ie->getInterfaceId();}
    virtual cObject *getSource() const {return NULL; /*TODO: getSource();*/}
    virtual cObject *getProtocolData() const { return NULL; } // TODO:
    virtual void setProtocolData(cObject *protocolData) { } // TODO:
};

#endif
