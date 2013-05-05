//
// Copyright (C) 2012 Andras Varga
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

#include "INETDefs.h"

#if OMNETPP_VERSION >= 0x0500 && defined HAVE_CEVENTLOGLISTENER  /* cEventlogListener is only supported from 5.0 */

#include "NotifierConsts.h"
#include "NotificationBoard.h"
#include "IInterfaceTable.h"
#include "IRoutingTable.h"
#include "IPv4RoutingTable.h"
#include "IPv6RoutingTable.h"
#include "GenericRoutingTable.h"
#include "IPv4InterfaceData.h" // TODO: remove?
#include "RoutingTableRecorder.h"


Define_Module(RoutingTableRecorder);

#define LL INT64_PRINTF_FORMAT  // for eventnumber_t


// We need this because we want to know which NotificationBoard the notification comes from
// (INotifiable::receiveChangeNotification() doesn't have NotificationBoard* as arg).
class RoutingTableNotificationBoardListener : public INotifiable
{
  private:
    NotificationBoard *nb;
    RoutingTableRecorder *recorder;

  public:
    RoutingTableNotificationBoardListener(RoutingTableRecorder *recorder, NotificationBoard *nb) {this->recorder = recorder; this->nb = nb;}
    virtual void receiveChangeNotification(int category, const cObject *details) {
        recorder->receiveChangeNotification(nb, category, details);
    }
};

RoutingTableRecorder::RoutingTableRecorder()
{
    this->interfaceKey = 0;
    this->routeKey = 0;
}

RoutingTableRecorder::~RoutingTableRecorder()
{
}

void RoutingTableRecorder::initialize(int stage)
{
    if (par("enabled").boolValue())
        hookListeners();
}

void RoutingTableRecorder::handleMessage(cMessage *)
{
    throw cRuntimeError(this, "This module doesn't process messages");
}

void RoutingTableRecorder::hookListeners()
{
    // hook existing notification boards (we won't cover dynamically created hosts/routers, but oh well)
    for (int id = 0; id < simulation.getLastModuleId(); id++) {
        NotificationBoard *nb = dynamic_cast<NotificationBoard *>(simulation.getModule(id));
        if (nb) {
            INotifiable *listener = new RoutingTableNotificationBoardListener(this, nb);
            nb->subscribe(listener, NF_INTERFACE_CREATED);
            nb->subscribe(listener, NF_INTERFACE_DELETED);
            nb->subscribe(listener, NF_INTERFACE_CONFIG_CHANGED);
            nb->subscribe(listener, NF_INTERFACE_IPv4CONFIG_CHANGED);
            //nb->subscribe(listener, NF_INTERFACE_IPv6CONFIG_CHANGED);
            //nb->subscribe(listener, NF_INTERFACE_STATE_CHANGED);
            nb->subscribe(listener, NF_ROUTE_ADDED);
            nb->subscribe(listener, NF_ROUTE_DELETED);
            nb->subscribe(listener, NF_ROUTE_CHANGED);
        }
    }
    // hook on eventlog manager
    cEnvir* envir = simulation.getEnvir();
    cIndexedEventlogManager *eventlogManager = dynamic_cast<cIndexedEventlogManager *>(envir->getEventlogManager());
    if (eventlogManager)
        eventlogManager->addEventlogListener(this);
}

void RoutingTableRecorder::receiveChangeNotification(NotificationBoard *nb, int category, const cObject *details)
{
    cModule *host = nb->getParentModule();
    if (category==NF_ROUTE_ADDED || category==NF_ROUTE_DELETED || category==NF_ROUTE_CHANGED)
        recordRoute(host, check_and_cast<IRoute *>(details), category);
    else if (category==NF_INTERFACE_CREATED || category==NF_INTERFACE_DELETED || category==NF_INTERFACE_CONFIG_CHANGED || category==NF_INTERFACE_IPv4CONFIG_CHANGED)
        recordInterface(host, check_and_cast<InterfaceEntry *>(details), category);
}

void RoutingTableRecorder::recordSnapshot()
{
    for (int id = 0; id < simulation.getLastModuleId(); id++) {
        cModule* module = simulation.getModule(id);
        IInterfaceTable *ift = dynamic_cast<IInterfaceTable *>(module);
        if (ift) {
            cModule *host = module->getParentModule();
            for (int i = 0; i < ift->getNumInterfaces(); i++)
                recordInterface(host, ift->getInterface(i), -1);
        }
    }
    for (int id = 0; id < simulation.getLastModuleId(); id++) {
        cModule* module = simulation.getModule(id);
        IPv4RoutingTable *rt = dynamic_cast<IPv4RoutingTable *>(module);
        if (rt) {
            // TODO: find out host correctly
            cModule *host = module->getParentModule();
            for (int i = 0; i < rt->getNumRoutes(); i++)
                recordRoute(host, rt->getRoute(i), -1);
        }
        IPv6RoutingTable *rt6 = dynamic_cast<IPv6RoutingTable *>(module);
        if (rt6) {
            // TODO: find out host correctly
            cModule *host = module->getParentModule();
            for (int i = 0; i < rt6->getNumRoutes(); i++)
                recordRoute(host, rt6->getRoute(i), -1);
        }
        GenericRoutingTable *generic = dynamic_cast<GenericRoutingTable *>(module);
        if (generic) {
            // TODO: find out host correctly
            cModule *host = module->getParentModule();
            for (int i = 0; i < generic->getNumRoutes(); i++)
                recordRoute(host, generic->getRoute(i), -1);
        }
    }
}

void RoutingTableRecorder::recordInterface(cModule *host, InterfaceEntry *interface, int category)
{
    cEnvir* envir = simulation.getEnvir();
    // moduleId, ifname, address
    std::stringstream content;
    content << host->getId() << " " << interface->getName() << " ";
    content << (interface->ipv4Data()!=NULL ? interface->ipv4Data()->getIPAddress().str() : IPv4Address().str());
    switch (category) {
        case NF_INTERFACE_CREATED:
            envir->customCreatedEntry("IT", interfaceKey, content.str().c_str());
            interfaceEntryToKey[interface] = interfaceKey;
            interfaceKey++;
            break;
        case NF_INTERFACE_DELETED:
            envir->customDeletedEntry("IT", interfaceEntryToKey[interface]);
            interfaceEntryToKey.erase(interface);
            break;
        case NF_INTERFACE_CONFIG_CHANGED:
        case NF_INTERFACE_IPv4CONFIG_CHANGED:
            envir->customChangedEntry("IT", interfaceEntryToKey[interface], content.str().c_str());
            break;
        case -1:
            envir->customFoundEntry("IT", interfaceEntryToKey[interface], content.str().c_str());
            break;
        default:
            throw cRuntimeError("Unexpected notification category %d", category);
    }
}

void RoutingTableRecorder::recordRoute(cModule *host,  IRoute *route, int category)
{
    cEnvir* envir = simulation.getEnvir();
    // moduleId, dest, dest netmask, nexthop
    std::stringstream content;
    content << host->getId() << " " << route->getDestinationAsGeneric().str() << " " << route->getPrefixLength() << " " << route->getNextHopAsGeneric().str();
    switch (category) {
        case NF_ROUTE_ADDED:
            envir->customCreatedEntry("RT", routeKey, content.str().c_str());
            routeToKey[route] = routeKey;
            routeKey++;
            break;
        case NF_ROUTE_DELETED:
            envir->customDeletedEntry("RT", routeToKey[route]);
            routeToKey.erase(route);
            break;
        case NF_ROUTE_CHANGED:
            envir->customChangedEntry("RT", routeToKey[route], content.str().c_str());
            break;
        case -1:
            envir->customFoundEntry("RT", routeToKey[route], content.str().c_str());
            break;
        default:
            throw cRuntimeError("Unexpected notification category %d", category);
    }
}

#endif /*OMNETPP_VERSION*/
