/**
 * pcap file reader/writer.
 *
 * @author Zoltan Bojthe
 */

#include <stdio.h>

#include <omnetpp.h>

#ifdef HAVE_PCAP
// prevent pcap.h to redefine int8_t,... types on Windows
#include "bsdint.h"
#define HAVE_U_INT8_T
#define HAVE_U_INT16_T
#define HAVE_U_INT32_T
#define HAVE_U_INT64_T
#include <pcap.h>
#endif

#include "PcapFile.h"

#include "IPDatagram.h"
#include "IPSerializer.h"

PcapFileReader::PcapFileReader() :
    pcap(NULL)
{
}

PcapFileReader::~PcapFileReader()
{
    close();
}

void PcapFileReader::open(const char* filename)
{
    char errbuf[PCAP_ERRBUF_SIZE];

    pcap = pcap_open_offline(filename, errbuf);

    if(!pcap)
        throw cRuntimeError("Pcap open file '%s' error: %s\n", filename, errbuf);
    fgetpos(pcap_file(pcap), &pos0);
}

const void* PcapFileReader::read(uint32 &sec, uint32 &usec, uint32 &capLen, uint32& origLen)
{
    if (!pcap)
        return NULL;

    struct pcap_pkthdr ph;
    const u_char* buf = pcap_next(pcap, &ph);
    if (buf)
    {
        sec = ph.ts.tv_sec;
        usec = ph.ts.tv_usec;
        capLen = ph.caplen;
        origLen = ph.len;
    }
    return buf;
}

bool PcapFileReader::eof()
{
    return !pcap || feof(pcap_file(pcap));
}

void PcapFileReader::restart()
{
    if (pcap)
        fsetpos(pcap_file(pcap), &pos0);
}

void PcapFileReader::close()
{
    if (pcap)
    {
        pcap_close(pcap);
        pcap = NULL;
    }
}

PcapFileWriter::PcapFileWriter() :
    pcap(NULL), pcapDumper(NULL)
{
}

PcapFileWriter::~PcapFileWriter()
{
    close();
}

void PcapFileWriter::close()
{
    if (pcapDumper)
    {
        pcap_dump_close(pcapDumper);
        pcapDumper = NULL;
    }
    if (pcap)
    {
        pcap_close(pcap);
        pcap = NULL;
    }
}

void PcapFileWriter::open(const char* filename, unsigned int snaplen)
{
    char errbuf[PCAP_ERRBUF_SIZE];

    pcap = pcap_open_offline(filename, errbuf);

    if(!pcap)
        throw cRuntimeError("Pcap open dumpfile '%s' error: %s\n", filename, errbuf);
    pcapDumper = pcap_dump_open(pcap,filename);
    if(!pcapDumper)
    {
        throw cRuntimeError("Pcap dump open dumpfile '%s' error: %s\n", filename, pcap_geterr(pcap));
    }
    snapLen = snaplen;
    if (0 != pcap_set_snaplen(pcap, snapLen))
        throw cRuntimeError("Pcap open dumpfile '%s' error: Invalid snaplen=%d\n", filename, snaplen);
}

void PcapFileWriter::write(uint32 sec, uint32 usec, const void *buff, uint32 capLen, uint32 fullLen)
{
    struct pcap_pkthdr ph;
    ph.ts.tv_sec = sec;
    ph.ts.tv_usec = usec;
    ph.len = fullLen;
    ph.caplen = std::min(std::min(snapLen, capLen), fullLen);

    pcap_dump((unsigned char *)pcapDumper, &ph, (const unsigned char *)buff);
}
