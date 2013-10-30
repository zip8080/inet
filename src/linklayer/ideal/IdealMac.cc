//
// Copyright (C) 2013 OpenSim Ltd
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
// author: Zoltan Bojthe
//

#include <stdio.h>
#include <string.h>

#include "IdealMac.h"

#include "IdealRadio.h"
#include "Ieee802Ctrl_m.h"
#include "IInterfaceTable.h"
#include "InterfaceTableAccess.h"
#include "IPassiveQueue.h"
#include "opp_utils.h"


Define_Module(IdealMac);

simsignal_t IdealMac::radioStateSignal = SIMSIGNAL_NULL;
simsignal_t IdealMac::dropPkNotForUsSignal = SIMSIGNAL_NULL;

IdealMac::IdealMac()
{
    queueModule = NULL;
    radioModule = NULL;
}

IdealMac::~IdealMac()
{
}

void IdealMac::flushQueue()
{
    ASSERT(queueModule);
    while (!queueModule->isEmpty())
    {
        cMessage *msg = queueModule->pop();
        //TODO emit(dropPkIfaceDownSignal, msg); -- 'pkDropped' signals are missing in this module!
        delete msg;
    }
    queueModule->clear(); // clear request count
}

void IdealMac::clearQueue()
{
    ASSERT(queueModule);
    queueModule->clear();
}

void IdealMac::initialize(int stage)
{
    WirelessMacBase::initialize(stage);

    if (stage == INITSTAGE_LOCAL)
    {
        outStandingRequests = 0;
        lastTransmitStartTime = -1.0;

        bitrate = par("bitrate").doubleValue();
        headerLength = par("headerLength").longValue();
        promiscuous = par("promiscuous");

        radioStateSignal = registerSignal("radioState");
        dropPkNotForUsSignal = registerSignal("dropPkNotForUs");

        radioModule = gate("lowerLayerOut")->getPathEndGate()->getOwnerModule();
        IdealRadio *irm = check_and_cast<IdealRadio *>(radioModule);
        irm->subscribe(radioStateSignal, this);

        // find queueModule
        cGate *queueOut = gate("upperLayerIn")->getPathStartGate();
        queueModule = dynamic_cast<IPassiveQueue *>(queueOut->getOwnerModule());
        if (!queueModule)
            error("Missing queueModule");

        initializeMACAddress();
    }
    else if (stage == INITSTAGE_LINK_LAYER)
    {
        // register our interface entry in IInterfaceTable
        registerInterface();
    }
}

void IdealMac::initializeMACAddress()
{
    const char *addrstr = par("address");

    if (!strcmp(addrstr, "auto"))
    {
        // assign automatic address
        address = MACAddress::generateAutoAddress();

        // change module parameter from "auto" to concrete address
        par("address").setStringValue(address.str().c_str());
    }
    else
    {
        address.setAddress(addrstr);
    }
}

InterfaceEntry *IdealMac::createInterfaceEntry()
{
    InterfaceEntry *e = new InterfaceEntry(this);

    // interface name: NIC module's name without special characters ([])
    e->setName(OPP_Global::stripnonalnum(getParentModule()->getFullName()).c_str());

    // data rate
    e->setDatarate(bitrate);

    // generate a link-layer address to be used as interface token for IPv6
    e->setMACAddress(address);
    e->setInterfaceToken(address.formInterfaceIdentifier());

    // MTU: typical values are 576 (Internet de facto), 1500 (Ethernet-friendly),
    // 4000 (on some point-to-point links), 4470 (Cisco routers default, FDDI compatible)
    e->setMtu(par("mtu").longValue());

    // capabilities
    e->setMulticast(true);
    e->setBroadcast(true);

    return e;
}

void IdealMac::receiveSignal(cComponent *src, simsignal_t id, long x)
{
    if (id == radioStateSignal && src == radioModule)
    {
        radioState = (RadioState::State)x;
        if (radioState != RadioState::TRANSMIT)
            getNextMsgFromHL();
    }
}

void IdealMac::startTransmitting(cPacket *msg)
{
    // if there's any control info, remove it; then encapsulate the packet
    IdealFrame *frame = encapsulate(msg);

    // send
    EV << "Starting transmission of " << frame << endl;
    sendDown(frame);
}

void IdealMac::handleSelfMsg(cMessage *msg)
{
    error("Unexpected self-message");
}

void IdealMac::getNextMsgFromHL()
{
    ASSERT(outStandingRequests >= queueModule->getNumPendingRequests());
    if (outStandingRequests == 0 && lastTransmitStartTime < simTime())
    {
        queueModule->requestPacket();
        outStandingRequests++;
    }
    ASSERT(outStandingRequests <= 1);
}

void IdealMac::handleUpperMsg(cPacket *msg)
{
    outStandingRequests--;
    if (radioState == RadioState::TRANSMIT)
    {
        // Logic error: we do not request packet from the external queue when radio is transmitting
        error("Received msg for transmission but transmitter is busy");
    }
    else if (radioState == RadioState::SLEEP)
    {
        EV << "Dropped upper layer message " << msg << " because radio is SLEEPing.\n";
    }
    else
    {
        // We are idle, so we can start transmitting right away.
        EV << "Received " << msg << " for transmission\n";
        startTransmitting(msg);
    }
}

void IdealMac::handleCommand(cMessage *msg)
{
    error("Unexpected command received from higher layer");
}

void IdealMac::handleLowerMsg(cPacket *msg)
{
    IdealFrame *frame = check_and_cast<IdealFrame *>(msg);
    if (frame->hasBitError())
    {
        EV << "Bit error in " << frame << ", discarding" << endl;
        delete frame;
        return;
    }

    if (!dropFrameNotForUs(frame))
    {
        // decapsulate and attach control info
        cPacket *higherlayerMsg = decapsulate(frame);
        EV << "Passing up contained packet `" << higherlayerMsg->getName() << "' to higher layer\n";
        sendUp(higherlayerMsg);
    }
}

IdealFrame *IdealMac::encapsulate(cPacket *msg)
{
    Ieee802Ctrl *ctrl = check_and_cast<Ieee802Ctrl*>(msg->removeControlInfo());
    IdealFrame *frame = new IdealFrame(msg->getName());
    frame->setByteLength(headerLength);
    frame->setSrc(ctrl->getSrc());
    frame->setDest(ctrl->getDest());
    frame->encapsulate(msg);
    delete ctrl;
    return frame;
}

bool IdealMac::dropFrameNotForUs(IdealFrame *frame)
{
    // Current implementation does not support the configuration of multicast
    // MAC address groups. We rather accept all multicast frames (just like they were
    // broadcasts) and pass it up to the higher layer where they will be dropped
    // if not needed.
    // All frames must be passed to the upper layer if the interface is
    // in promiscuous mode.

    if (frame->getDest().equals(address))
        return false;

    if (frame->getDest().isBroadcast())
        return false;

    if (promiscuous || frame->getDest().isMulticast())
        return false;

    EV << "Frame `" << frame->getName() <<"' not destined to us, discarding\n";
    emit(dropPkNotForUsSignal, frame);
    delete frame;
    return true;
}

cPacket *IdealMac::decapsulate(IdealFrame *frame)
{
    // decapsulate and attach control info
    cPacket *packet = frame->decapsulate();
    Ieee802Ctrl *etherctrl = new Ieee802Ctrl();
    etherctrl->setSrc(frame->getSrc());
    etherctrl->setDest(frame->getDest());
    packet->setControlInfo(etherctrl);

    delete frame;
    return packet;
}

