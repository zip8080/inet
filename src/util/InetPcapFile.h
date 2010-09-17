/**
 * pcap file reader/writer.
 *
 * @author Zoltan Bojthe
 */

#ifndef __INET_UTIL_IPPCAPFILE_H
#define __INET_UTIL_IPPCAPFILE_H

#include "PcapFile.h"

// Foreign declarations
class IPDatagram;


class InetPcapFileWriter : public PcapFileWriter
{
  public:
    ~InetPcapFileWriter() {};
    void write(cPacket *packet);
};

class InetPcapParserIf : public cPolymorphic
{
  public:
    virtual ~InetPcapParserIf() {};
    virtual cPacket* parse(const unsigned char *buf, uint32 caplen, uint32 totlen) = 0;
};

class InetPcapParser : public InetPcapParserIf
{
  public:
    virtual ~InetPcapParser() {};
    virtual cPacket* parse(const unsigned char *buf, uint32 caplen, uint32 totlen);
};

class IPDatagramPcapParser : public InetPcapParserIf
{
  public:
    virtual ~IPDatagramPcapParser() {};
    virtual cPacket* parse(const unsigned char *buf, uint32 caplen, uint32 totlen);
};

class InetPcapFileReader : public PcapFileReader
{
    uint32 sec0;
    uint32 usec0;
    InetPcapParserIf *parser;
  public:
    InetPcapFileReader() : parser(NULL) {};
    ~InetPcapFileReader() { delete parser; };
    void setParser(const char* parserName);
    void open(const char* filename);
    cPacket* read(simtime_t &stime);
};

#endif //__INET_UTIL_IPPCAPFILE_H
