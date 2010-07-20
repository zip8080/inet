/**
 * pcap file writer.
 *
 * @author Zoltan Bojthe
 */

#ifndef __INET_UTIL_PCAPFILE_H
#define __INET_UTIL_PCAPFILE_H

#include <fstream>

class IPDatagram;

class PcapFile
{
    std::fstream f;
  public:
    void open(const char* filename, int snaplen);
    bool fail() { return f.fail(); }
    void close() { f.close(); }
    bool isOpen() { return f.is_open(); }
    void write(IPDatagram *ipPacket);
};

#endif //__INET_UTIL_PCAPFILE_H
