/**
 * pcap file reader/writer.
 *
 * @author Zoltan Bojthe
 */

#include <stdio.h>

#include <omnetpp.h>

#include "InetPcapFile.h"

#include "IPDatagram.h"
#include "IPSerializer.h"

void InetPcapFileReader::open(const char* filename)
{
    PcapFileReader::open(filename);
    uint32 caplen, origlen;
    // read first packet, for get time of first packet
    PcapFileReader::read(sec0, usec0, caplen, origlen);
    restart();
}

cPacket* parse(const unsigned char *buf, uint32 caplen, uint32 totlen);

cPacket* InetPcapFileReader::read(simtime_t &stime)
{
    if (!pcap)
        return NULL;

    uint32 sec, usec, caplen, len;

    const unsigned char* buf = (unsigned char *)PcapFileReader::read(sec, usec, caplen, len);
    if (!buf)
        return NULL;

    // calculate relative time:
    stime = simtime_t((double)(sec-sec0) + (0.000001 * (int32)(usec-usec0)));

    cPacket* ret = NULL;

    if (len == caplen)
    {
        /* FIXME :
         * ph.len >= ph.caplen
         * Should modify all parse() functions, for use two length parameters: total length of packet and stored length.
         * Should modify also all serialize functions, for return larger length than buffer length, but not write
         * after end of buffer.
         */

        IPDatagram *ipPacket = new IPDatagram();
        IPSerializer().parse(buf, caplen, ipPacket, false);
        ret = ipPacket;
    }
    else
    {
        ret = new cPacket();
        ret->setByteLength(len);
        char str[20];
        sprintf(str,"PCAP:%d", *(const uint32*)buf);
        ret->setName(str);
    }
    return ret;
}

void InetPcapFileWriter::write(cPacket *packet)
{
    if (!pcapDumper)
        return;

    IPDatagram *ipPacket = dynamic_cast<IPDatagram *>(packet);

    if (ipPacket)
    {
        uint8 buf[snapLen];

        memset((void*)buf, 0, snapLen);

        const simtime_t stime = simulation.getSimTime();
        uint32 sec = (uint32)stime.dbl();
        uint32 usec = (uint32)((stime.dbl() - sec)*1000000);

        // IP header:
        *(uint32*)buf = 2; // IP header
        int32 serialized_ip = IPSerializer().serialize(ipPacket, buf+sizeof(uint32), snapLen-sizeof(uint32));
        uint32 len = serialized_ip+sizeof(uint32);
        PcapFileWriter::write(sec, usec, buf, snapLen, len);
    }
}
