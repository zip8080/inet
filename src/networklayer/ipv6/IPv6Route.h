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
// TODO change notifications
class INET_API IPv6Route : public cObject, public IRoute
{
  protected:
    IPv6Address _destPrefix;
    short _length;
    SourceType _sourceType;
    int _interfaceID;      //XXX IPv4 IIPv4RoutingTable uses interface pointer
    IPv6Address _nextHop;  // unspecified means "direct"
    simtime_t _expiryTime; // if route is an advertised prefix: prefix lifetime
    int _metric;

  public:
    /**
     * Constructor. The destination prefix and the route source is passed
     * to the constructor and cannot be changed afterwards.
     */
    IPv6Route(IPv6Address destPrefix, int length, SourceType sourceType) {
        _destPrefix = destPrefix;
        _length = length;
        _sourceType = sourceType;
        _interfaceID = -1;
        _expiryTime = 0;
        _metric = 0;
    }

    virtual ~IPv6Route() { }

    virtual std::string info() const;
    virtual std::string detailedInfo() const;

    void setInterfaceId(int interfaceId)  {_interfaceID = interfaceId;}
    void setNextHop(const IPv6Address& nextHop)  {_nextHop = nextHop;}
    void setExpiryTime(simtime_t expiryTime)  {_expiryTime = expiryTime;}
    void setMetric(int metric)  {_metric = metric;}

    const IPv6Address& getDestPrefix() const {return _destPrefix;}
    virtual int getPrefixLength() const  {return _length;}
    virtual SourceType getSourceType() const { return _sourceType; }
    int getInterfaceId() const  {return _interfaceID;}
    const IPv6Address& getNextHop() const  {return _nextHop;}
    simtime_t getExpiryTime() const  {return _expiryTime;}
    virtual int getMetric() const  {return _metric;}
    virtual IRoutingTable *getRoutingTableAsGeneric() const {return NULL; /*TODO: getRoutingTable();*/}

    virtual void setDestination(const Address& dest) {/*TODO: setDestination(dest.toIPv6());*/}
    virtual void setPrefixLength(int len) {/*TODO: setPrefixLength(len));*/}
    virtual void setNextHop(const Address& nextHop) {setNextHop(nextHop.toIPv6());}
    virtual void setSource(cObject *source) {/*TODO: setSource(source);*/}
    virtual void setSourceType(SourceType type) { _sourceType = type; }
    virtual Address getDestinationAsGeneric() const {return getDestPrefix();} //TODO rename IPv6 method
    virtual Address getNextHopAsGeneric() const {return getNextHop();}
    virtual InterfaceEntry *getInterface() const {return NULL; /*TODO getInterface();*/} //TODO change IPv6Route API from interfaceID to interface ptr
    virtual void setInterface(InterfaceEntry *ie) {_interfaceID = ie->getInterfaceId();}
    virtual cObject *getSource() const {return NULL; /*TODO: getSource();*/}
    virtual cObject *getProtocolData() const { return NULL; } // TODO:
    virtual void setProtocolData(cObject *protocolData) { } // TODO:
};

#endif
