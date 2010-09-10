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
  protected:
    std::fstream f;

  public:
    bool fail() { return f.fail(); }
    void close() { f.close(); }
    bool eof() { return f.eof(); }
    bool isOpen() { return f.is_open(); }
    bool readHeader();
    void writeHeader(int snaplen);
};


class PcapOutFile : public PcapFile
{
  public:
    void open(const char* filename, int snaplen);
    void write(IPDatagram *ipPacket);
};

class PcapInFile : public PcapFile
{
    std::fstream f;
  public:
    void open(const char* filename);
    IPDatagram* read(simtime_t &stime);
    void restart();
};

#endif //__INET_UTIL_PCAPFILE_H
