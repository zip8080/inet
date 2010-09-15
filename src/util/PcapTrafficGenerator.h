//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

#ifndef __INET_PCAPTRAFFICGENERATOR_H_
#define __INET_PCAPTRAFFICGENERATOR_H_

#include <omnetpp.h>

#include "PcapFile.h"


/**
 * Generate traffic from a pcap file
 */
class PcapTrafficGenerator : public cSimpleModule
{
  public:
    PcapTrafficGenerator() : enabled(false) {}
  protected:
    bool enabled;
    PcapInFile pcapFile;
    // parameters:
    simtime_t timeShift;    // simtime = pcap_time + timeshift
    simtime_t endTime;      // simtime of last sent packet, 0 is infinity
    simtime_t repeatGap;   // simtime = pcap_time + timeshift + [0..n] * repeattime

  protected:
    virtual void initialize();
    virtual void handleMessage(cMessage *msg);
    virtual void scheduleNextPacket();
};

#endif
