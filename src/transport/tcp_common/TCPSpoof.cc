//
// Copyright 2006 Andras Varga
//
// This library is free software, you can redistribute it and/or modify
// it under  the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation;
// either version 2 of the License, or any later version.
// The library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU Lesser General Public License for more details.
//


#include "TCPSpoof.h"

#include "IAddressType.h"
#include "INetworkProtocolControlInfo.h"
#include "IPProtocolId_m.h"

Define_Module(TCPSpoof);

simsignal_t TCPSpoof::sentPkSignal = SIMSIGNAL_NULL;

void TCPSpoof::initialize()
{
    sentPkSignal = registerSignal("sentPk");
    simtime_t t = par("t").doubleValue();
    scheduleAt(t, new cMessage("timer"));
}

void TCPSpoof::handleMessage(cMessage *msg)
{
    if (!msg->isSelfMessage())
        error("Does not process incoming messages");

    sendSpoofPacket();
    delete msg;
}

void TCPSpoof::sendSpoofPacket()
{
    TCPSegment *tcpseg = new TCPSegment("spoof");

    Address srcAddr = AddressResolver().resolve(par("srcAddress"));
    Address destAddr = AddressResolver().resolve(par("destAddress"));
    int srcPort = par("srcPort");
    int destPort = par("destPort");
    bool isSYN = par("isSYN");
    unsigned long seq = par("seqNo").longValue()==-1 ? chooseInitialSeqNum() : par("seqNo").longValue();

    // one can customize the following according to concrete needs
    tcpseg->setSrcPort(srcPort);
    tcpseg->setDestPort(destPort);
    tcpseg->setByteLength(TCP_HEADER_OCTETS);
    tcpseg->setSequenceNo(seq);
    //tcpseg->setAckNo(...);
    tcpseg->setSynBit(isSYN);
    tcpseg->setWindow(16384);

    sendToIP(tcpseg, srcAddr, destAddr);
}

void TCPSpoof::sendToIP(TCPSegment *tcpseg, Address src, Address dest)
{
    EV << "Sending: ";
    //printSegmentBrief(tcpseg);

    IAddressType *addressType = dest.getAddressType();
    INetworkProtocolControlInfo *controlInfo = addressType->createNetworkProtocolControlInfo();
    controlInfo->setProtocol(IP_PROT_TCP);
    controlInfo->setSourceAddress(src);
    controlInfo->setDestinationAddress(dest);
    tcpseg->setControlInfo(check_and_cast<cObject *>(controlInfo));

    emit(sentPkSignal, tcpseg);
    send(tcpseg, "ipOut");
}

unsigned long TCPSpoof::chooseInitialSeqNum()
{
    // choose an initial send sequence number in the same way as TCP does
    return (unsigned long)SIMTIME_DBL(fmod(simTime()*250000.0, 1.0+(double)(unsigned)0xffffffffUL)) & 0xffffffffUL;
}


