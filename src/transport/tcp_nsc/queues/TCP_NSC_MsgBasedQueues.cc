//
// Copyright (C) 2004 Andras Varga
//               2009 Zoltan Bojthe
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

#ifdef WITH_TCP_NSC

#include <omnetpp.h>

#include "TCP_NSC_MsgBasedQueues.h"

#include "sim_interface.h" // NSC header
#include "TCPCommand_m.h"
#include "TCP_NSC_Connection.h"
#include "TCPSerializer.h"


Register_Class(TCP_NSC_MsgBasedSendQueue);

Register_Class(TCP_NSC_MsgBasedReceiveQueue);


/**
 * Ctor.
 */
TCP_NSC_MsgBasedSendQueue::TCP_NSC_MsgBasedSendQueue()
    :
    unsentNscBytesM(0),
    lastByteOffsetM(0),
    isInitialisedSeqNoM(false),
    startSeqNoM(0)
{
}

/**
 * Virtual dtor.
 */
TCP_NSC_MsgBasedSendQueue::~TCP_NSC_MsgBasedSendQueue()
{
}

/**
 * set connection queue.
 */
void TCP_NSC_MsgBasedSendQueue::setConnection(TCP_NSC_Connection *connP)
{
    unsentNscBytesM = 0;
    lastByteOffsetM = 0;
    isInitialisedSeqNoM = false;
    startSeqNoM = 0;
    payloadQueueM.clear();
    TCP_NSC_SendQueue::setConnection(connP);
}

/**
 * Called on SEND app command, it inserts in the queue the data the user
 * wants to send. Implementations of this abstract class will decide
 * what this means: copying actual bytes, just increasing the
 * "last byte queued" variable, or storing cMessage object(s).
 * The msg object should not be referenced after this point (sendQueue may
 * delete it.)
 */
void TCP_NSC_MsgBasedSendQueue::enqueueAppData(cPacket *msgP)
{
    ASSERT(msgP);

    int bytes = msgP->getByteLength();

    unsentNscBytesM += bytes;
    lastByteOffsetM += bytes;
    payloadQueueM.push_back(PayLoad(lastByteOffsetM, msgP));
}

/**
 * Copy data to the buffer for send to NSC.
 * returns lengh of copied data.
 * create msg for socket->send_data()
 *
 * called before called socket->send_data()
 */
int TCP_NSC_MsgBasedSendQueue::getNscMsg(void* bufferP, int bufferLengthP)
{
    ASSERT(bufferP);

    return (unsentNscBytesM > bufferLengthP) ? bufferLengthP : unsentNscBytesM;
}

/**
 * Remove msgLengthP bytes from NSCqueue
 * But the NSC sometimes reread from this datapart (when data destroyed in IP Layer)
 *
 *
 * called with return value of socket->send_data() if larger than 0
 */
void TCP_NSC_MsgBasedSendQueue::dequeueNscMsg(int msgLengthP)
{
    ASSERT(msgLengthP <= unsentNscBytesM);
    unsentNscBytesM -= msgLengthP;
}

/**
 * Utility function: returns how many bytes are available in the queue.
 */
ulong TCP_NSC_MsgBasedSendQueue::getBytesAvailable()
{
    return unsentNscBytesM; // TODO
}

// returns true if a smaller than b in circle mode of uint32 (0xFFFFFFFF < 0x00000001)
static inline bool smaller(uint32 a, uint32 b)
{
    return ((int32)(a-b)) < 0;
}

/*
 * checks is x between a and b
 *
 * should a<=b is true
 *
 * if x < a then returns -1
 * if x > b then returns 1
 * if x between a and b then returns 0
 */
static inline short int between(uint32 x, uint32 a, uint32 b)
{
    ASSERT(smaller(b,a));

    if(smaller(x,a))
        return -1;
    if(smaller(b,x))
        return 1;
    return 0;
}

/**
 * Called when the TCP wants to send or retransmit data, it constructs
 * a TCP segment which contains the data from the requested sequence
 * number range. The actually returned segment may contain less then
 * maxNumBytes bytes if the subclass wants to reproduce the original
 * segment boundaries when retransmitting.
 *
 * The method called from inside of send_callback()
 * The method called before called the send() to IP layer
 */
TCPSegment* TCP_NSC_MsgBasedSendQueue::createSegmentWithBytes(
        const void* tcpDataP, int tcpLengthP)
{
    ASSERT(tcpDataP);

    TCPSegment *tcpseg = new TCPSegment("tcp-segment");

    TCPSerializer().parse((const unsigned char *)tcpDataP, tcpLengthP, tcpseg);
    uint32 seqBgn = tcpseg->getSequenceNo();
    uint32 winSize = 65536; // TODO
    if(tcpseg->getSynBit())
    {
        ASSERT(!isInitialisedSeqNoM);
        isInitialisedSeqNoM = true;
        startSeqNoM = ++seqBgn;
    }
    uint32 seqEnd = seqBgn + tcpseg->getPayloadLength();

    seqEnd -= startSeqNoM;
    seqBgn -= startSeqNoM;

    PayloadQueue::iterator i = payloadQueueM.begin();
    // drop old messages:
    while(i != payloadQueueM.end())
    {
        short int bw = between(i->endOffsetM, seqEnd - 2*winSize, seqEnd - winSize - 1);
        if(bw)
            break;
        i = payloadQueueM.erase(i);
    }
    // get PayLoad data: find [seqBgn+1,seqEnd] and add to tcpseg
    while(i != payloadQueueM.end())
    {
        short int bw = between(i->endOffsetM, seqBgn+1, seqEnd);
        if(bw > 0)
            break;
        if(0 == bw)
        {
            tcpseg->addPayloadMessage(i->msgM, i->endOffsetM + startSeqNoM);
        }
        ++i;
    }
    return tcpseg;
}

/**
 * Tells the queue that bytes up to (but NOT including) seqNum have been
 * transmitted and ACKed, so they can be removed from the queue.
 */
void TCP_NSC_MsgBasedSendQueue::discardUpTo(uint32 seqNumP)
{
    // nothing to do here
}

/**
 * Ctor.
 */
TCP_NSC_MsgBasedReceiveQueue::TCP_NSC_MsgBasedReceiveQueue()
    :
    bytesInQueueM(0),
    isInitialisedSeqNoM(false),
    startSeqNoM(0)
{
}

/**
 * Virtual dtor.
 */
TCP_NSC_MsgBasedReceiveQueue::~TCP_NSC_MsgBasedReceiveQueue()
{
    // nothing to do here
}

/**
 * Add a connection queue.
 */
void TCP_NSC_MsgBasedReceiveQueue::setConnection(TCP_NSC_Connection *connP)
{
    ASSERT(connP);

    bytesInQueueM = 0;
    isInitialisedSeqNoM = false;
    startSeqNoM = 0;
    TCP_NSC_ReceiveQueue::setConnection(connP);
}

/**
 * Called when a TCP segment arrives, it should extract the payload
 * from the segment and store it in the receive queue. The segment
 * object should *not* be deleted.
 *
 * The method should return the number of bytes to copied to buffer.
 *
 * The method should fill the bufferP for data sending to NSC stack
 *
 * The method called before nsc_stack->if_receive_packet() called
 */
uint32 TCP_NSC_MsgBasedReceiveQueue::insertBytesFromSegment(
        const TCPSegment *tcpsegP, void* bufferP, size_t bufferLengthP)
{
    ASSERT(tcpsegP);
    ASSERT(bufferP);

    uint32 ret = TCPSerializer().serialize(tcpsegP, (unsigned char *)bufferP, bufferLengthP);
    // TODO store PayLoad data to queue
    return ret;
}

/**
 * The method called when data received from NSC
 * The method should set status of the data in queue to received.
 *
 * The method called after socket->read_data() successfull
 */
void TCP_NSC_MsgBasedReceiveQueue::enqueueNscData(void* dataP, int dataLengthP)
{
    ASSERT(connM);

    bytesInQueueM += dataLengthP;
}

/**
 * Should create a packet to be passed up to the app.
 * It should return NULL if there's no more data to be passed up --
 * this method is called several times until it returns NULL.
 *
 * the method called after socket->read_data() successful
 */
cPacket* TCP_NSC_MsgBasedReceiveQueue::extractBytesUpTo()
{
    ASSERT(connM);

    cPacket *dataMsg = NULL;
    if(bytesInQueueM)
    {
        dataMsg = new cPacket("DATA");
        dataMsg->setKind(TCP_I_DATA);
        dataMsg->setByteLength(bytesInQueueM);
        TCPConnectInfo *tcpConnectInfo = new TCPConnectInfo();
        tcpConnectInfo->setConnId(connM->connIdM);
        tcpConnectInfo->setLocalAddr(connM->inetSockPairM.localM.ipAddrM);
        tcpConnectInfo->setRemoteAddr(connM->inetSockPairM.remoteM.ipAddrM);
        tcpConnectInfo->setLocalPort(connM->inetSockPairM.localM.portM);
        tcpConnectInfo->setRemotePort(connM->inetSockPairM.remoteM.portM);
        dataMsg->setControlInfo(tcpConnectInfo);
        queueSeqNoM += dataMsg->getByteLength();
        bytesInQueueM -= dataMsg->getByteLength();
    }
    return dataMsg;
}

/**
 * Returns the number of bytes currently buffered in queue.
 */
uint32 TCP_NSC_MsgBasedReceiveQueue::getAmountOfBufferedBytes()
{
    return bytesInQueueM;
}

/**
 * Returns the number of blocks currently buffered in queue.
 */
uint32 TCP_NSC_MsgBasedReceiveQueue::getQueueLength()
{
    return bytesInQueueM ? 1 : 0;
}

/**
 * Shows current queue status.
 */
void TCP_NSC_MsgBasedReceiveQueue::getQueueStatus()
{
    // TODO
}

/**
 * notify the queue about output messages
 *
 * called when connM send out a packet.
 * for read AckNo, if have
 */
void TCP_NSC_MsgBasedReceiveQueue::notifyAboutSending(const TCPSegment *tcpsegP)
{
    if(!isInitialisedSeqNoM)
    {
        if(tcpsegP->getAckBit())
        {
            isInitialisedSeqNoM = true;
            queueSeqNoM = startSeqNoM = tcpsegP->getAckNo();
        }
    }
}
#endif // WITH_TCP_NSC

