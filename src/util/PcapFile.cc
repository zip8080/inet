/**
 * pcap file reader/writer.
 *
 * @author Zoltan Bojthe
 */

#include <omnetpp.h>

#include "PcapFile.h"

#include "IPDatagram.h"
#include "IPSerializer.h"


#define PCAP_MAGIC          0xa1b2c3d4
#define MAXBUFLENGTH        65536

/* "libpcap" file header (minus magic number). */
struct pcap_hdr {
     uint32 magic;      /* magic */
     uint16 version_major;   /* major version number */
     uint16 version_minor;   /* minor version number */
     uint32 thiszone;   /* GMT to local correction */
     uint32 sigfigs;        /* accuracy of timestamps */
     uint32 snaplen;        /* max length of captured packets, in octets */
     uint32 network;        /* data link type */
};

/* "libpcap" record header. */
struct pcaprec_hdr {
     int32  ts_sec;     /* timestamp seconds */
     uint32 ts_usec;        /* timestamp microseconds */
     uint32 incl_len;   /* number of octets of packet saved in file */
     uint32 orig_len;   /* actual length of packet */
};

void PcapOutFile::writeHeader(int snaplen)
{
    if (!f.fail())
    {
        // Write PCAP header:
        struct pcap_hdr fh;
        fh.magic = PCAP_MAGIC;
        fh.version_major = 2;
        fh.version_minor = 4;
        fh.thiszone = 0;
        fh.sigfigs = 0;
        fh.snaplen = snaplen;
        fh.network = 0;
        f.write((char *)&fh, sizeof(fh));
    }
}

bool PcapInFile::readHeader()
{
    if (f.fail())
        return false;

    struct pcap_hdr fh;
    f.read((char *)&fh, sizeof(fh));
    if (f.fail())
        return false;

    return (
            fh.magic == PCAP_MAGIC &&
            fh.version_major == 2 &&
            fh.version_minor == 4
        );
}

void PcapOutFile::open(const char* filename, int snaplen)
{
    f.open(filename, std::ios::out | std::ios::trunc | std::ios::binary);
    writeHeader(snaplen);
}

void PcapOutFile::write(IPDatagram *ipPacket)
{
    uint8 buf[MAXBUFLENGTH];
    memset((void*)&buf, 0, sizeof(buf));

    const simtime_t stime = simulation.getSimTime();
    // Write PCap header

    struct pcaprec_hdr ph;
    ph.ts_sec = (int32)stime.dbl();
    ph.ts_usec = (uint32)((stime.dbl() - ph.ts_sec)*1000000);
     // Write Ethernet header
    uint32 hdr = 2; //AF_INET
                 //We do not want this to end in an error if EtherAutoconf messages
    // IP header:
    //struct sockaddr_in *to = (struct sockaddr_in*) malloc(sizeof(struct sockaddr_in));
    //int32 tosize = sizeof(struct sockaddr_in);
    int32 serialized_ip = IPSerializer().serialize(ipPacket, buf, sizeof(buf));
    ph.incl_len = serialized_ip + sizeof(uint32);
    ph.orig_len = ph.incl_len;
    f.write((char *)&ph, sizeof(ph));
    f.write((char *)&hdr, sizeof(uint32));
    f.write((char *)buf, serialized_ip);
}

void PcapInFile::open(const char* filename)
{
    f.open(filename, std::ios::in | std::ios::binary);
    if(!f.is_open())
        return;
    if (!readHeader())
        f.close();
}

cMessage* PcapInFile::read(simtime_t &stime)
{
    cMessage* ret = NULL;
    uint8 buf[MAXBUFLENGTH];
    memset((void*)&buf, 0, sizeof(buf));

    struct pcaprec_hdr ph;
    uint32 hdr;
    f.read((char *)&ph, sizeof(ph));
    if (f.fail())
        return ret;

    stime = simtime_t((double)ph.ts_sec + (0.0000001 * ph.ts_usec));

    ASSERT(ph.orig_len == ph.incl_len);

    // Read packet
    uint32 len = ph.incl_len;
    f.read((char *)buf, len);
    if (f.fail())
        return ret;
    switch(*(uint32*)buf) // Read packet type header
    {
      case 2:
        {
            IPDatagram *ipPacket = new IPDatagram();
            IPSerializer().parse(buf+sizeof(hdr), len, ipPacket, false);
            ret = ipPacket;
        }
        break;

      default:
        ret = new cMessage();
    }
    return ret;
}

void PcapInFile::restart()
{
    f.seekg(0, std::ios::beg);
    if (!readHeader())
        f.close();
}
