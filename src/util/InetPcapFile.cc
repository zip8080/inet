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

Register_Class(InetPcapParser);
Register_Class(IPDatagramPcapParser);

void InetPcapFileReader::open(const char* filename)
{
    PcapFileReader::open(filename);
    uint32 caplen, origlen;
    // read first packet, for get time of first packet
    PcapFileReader::read(sec0, usec0, caplen, origlen);
    restart();
}

cPacket* InetPcapParser::parse(const unsigned char *buf, uint32 caplen, uint32 totlen)
{
    cPacket* ret = new cPacket();
    ret->setByteLength(totlen);
    char str[20];
    sprintf(str,"PCAP:%u/%ubytes", caplen, totlen);
    ret->setName(str);
    return ret;
}

namespace {
bool isIpHeader(const unsigned char *buf, uint32 caplen, uint32 totlen)
{
    switch (buf[0]&0xF0)
    {
    case 0x40:  // IPv4
        if ((totlen > 20) && ((((unsigned int)(buf[2])<<8) + buf[3]) == totlen))   // 20=IPv4 minimal header length
            return true;
        break;

    case 0x60:  // IPv6
        if ((totlen > 40) && ((((unsigned int)(buf[4])<<8) + buf[5])+40 == totlen))   // 40=IPv6 header length
            return true;
        break;

    default:
        break;
    }
    return false;
}

bool isEthHeader(const unsigned char *buf, uint32 caplen, uint32 totlen)
{
    return buf[12] == 8 && buf[13] == 0;
}

uint32 isNullHeader(const unsigned char *buf, uint32 caplen, uint32 totlen)
{
    if (buf[1] | buf[2])
        return false;
    if (buf[0] && buf[3])
        return false;
    switch(buf[0]|buf[3])
    {
    case 2:
    case 24:
    case 28:
    case 30:
        return 4;

    default:
        break;
    }
    return 0;
}

} // namespace

cPacket* IPDatagramPcapParser::parse(const unsigned char *buf, uint32 caplen, uint32 totlen)
{
    cPacket* ret = NULL;

    uint32 hdrlen = 0;
    uint32 x;

    x = isNullHeader(buf, caplen, totlen);
    if (x && isIpHeader(buf+x, caplen-x, totlen-x))
    {
        hdrlen = sizeof(uint32);
    }
    if (!hdrlen)
    {
        x = isEthHeader(buf, caplen, totlen);
        if (x && isIpHeader(buf+x, caplen-x, totlen-x))
        {
            hdrlen = x; // skip the {srcMAC, dstMAC, type=8} ethernet frame
        }
    }

    if (hdrlen)
    {
        IPDatagram *ipPacket = new IPDatagram();
        IPSerializer().parse(buf+hdrlen, caplen-hdrlen, ipPacket, false);
        ret = ipPacket;
    }
    return ret;
}

cPacket* InetPcapFileReader::read(simtime_t &stime)
{
    if (!pcap)
        return NULL;

    if (!parser)
        throw cRuntimeError("InetPcapFileReader haven't a parser!");

    uint32 sec, usec, caplen, len;

    const unsigned char* buf = (unsigned char *)PcapFileReader::read(sec, usec, caplen, len);
    if (!buf)
        return NULL;

    // calculate relative time:
    stime = simtime_t((double)(sec-sec0) + (0.000001 * (int32)(usec-usec0)));

    cPacket* ret = parser->parse(buf, caplen, len);
    return ret;
}

uint32 IPDatagramPcapSerializer::serialize(const cPacket* packet, unsigned char *buf, uint32 buflen)
{
    const IPDatagram *ipPacket = dynamic_cast<const IPDatagram *>(packet);

    if (ipPacket)
    {
        // IP header:
        *(uint32*)buf = 2; // IP header
        int32 serialized_ip = IPSerializer().serialize(ipPacket, buf+sizeof(uint32), buflen-sizeof(uint32));
        uint32 len = serialized_ip+sizeof(uint32);
        return len;
    }
    return 0;
}

void InetPcapFileWriter::write(cPacket *packet)
{
    if (!pcapDumper)
        return;

    if (!serializer)
        throw cRuntimeError("InetPcapFileWriter haven't a serializer!");

    uint32 bufLen = snapLen;
    uint8 buf[bufLen];

    memset((void*)buf, 0, snapLen);

    uint32 len = serializer->serialize(packet, buf, bufLen);
    if (len)
    {
        const simtime_t stime = simulation.getSimTime();
        uint32 sec = (uint32)stime.dbl();
        uint32 usec = (uint32)((stime.dbl() - sec)*1000000);
        PcapFileWriter::write(sec, usec, buf, std::min(bufLen, len), len);
    }
}

void InetPcapFileReader::setParser(const char* parserName)
{
    delete parser;
    parser = check_and_cast<InetPcapParserIf*>(createOne(parserName));
    if (!parser)
        throw cRuntimeError("InetPcapFileReader: Invalid pcap parser name: %s", parserName);
}

void InetPcapFileWriter::setSerializer(const char* serializerName)
{
    delete serializer;
    serializer = check_and_cast<InetPcapSerializerIf*>(createOne(serializerName));
    if (!serializer)
        throw cRuntimeError("InetPcapFileWriter: Invalid pcap serializer name: %s", serializerName);
}
