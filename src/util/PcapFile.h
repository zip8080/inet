/**
 * pcap file writer.
 *
 * @author Zoltan Bojthe
 */

#ifndef __INET_UTIL_PCAPFILE_H
#define __INET_UTIL_PCAPFILE_H

#include <fstream>

class IPDatagram;

class PcapOutFile
{
  protected:
    std::fstream f;
  protected:
    void writeHeader(int snaplen);
  public:
    bool fail() { return f.fail(); }
    void close() { f.close(); }
    bool isOpen() { return f.is_open(); }
    void open(const char* filename, int snaplen);
    void write(IPDatagram *ipPacket);
};

class PcapInFile
{
  protected:
    std::fstream f;
  protected:
    bool readHeader();
  public:
    bool fail() { return f.fail(); }
    void close() { f.close(); }
    bool eof() { return f.eof(); }
    bool isOpen() { return f.is_open(); }
    void open(const char* filename);
    cMessage* read(simtime_t &stime);
    void restart();
};

#endif //__INET_UTIL_PCAPFILE_H
