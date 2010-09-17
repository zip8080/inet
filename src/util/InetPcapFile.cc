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

cPacket* IPDatagramPcapParser::parse(const unsigned char *buf, uint32 caplen, uint32 totlen)
{
    cPacket* ret = NULL;

    uint32 hdrlen = 0;

    if ( ((*(uint32*)buf == 2) && (buf[4]&0xF0 == 0x40)))
    {
        hdrlen = sizeof(uint32);
    }
    else if (buf[12] == 8 && buf[13] == 0 && (buf[14]&0xF0 == 0x40))
    {
        hdrlen = 14; // skip the {srcMAC, dstMAC, type=8} ethernet frame
    }
    if (hdrlen)
    {
        /* FIXME :
         * totlen >= caplen
         * Should modify all parse() functions, for use two length parameters: total length of packet and stored length.
         * Should modify also all serialize functions, for return larger length than buffer length, but not write
         * out of buffer.
         */

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

    uint32 sec, usec, caplen, len;

    const unsigned char* buf = (unsigned char *)PcapFileReader::read(sec, usec, caplen, len);
    if (!buf)
        return NULL;

    // calculate relative time:
    stime = simtime_t((double)(sec-sec0) + (0.000001 * (int32)(usec-usec0)));

    cPacket* ret = parser->parse(buf, caplen, len);
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

void InetPcapFileReader::setParser(const char* parserName)
{
    delete parser;
    parser = check_and_cast<InetPcapParserIf*>(createOne(parserName));
    if (!parser)
        throw cRuntimeError("Invalid pcap parser name: %s", parserName);
}
