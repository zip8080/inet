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

#include "PcapTrafficGenerator.h"

Define_Module(PcapTrafficGenerator);

void PcapTrafficGenerator::initialize()
{
    const char* file = this->par("pcapfile");
    pcapFile.open(file);
    timeshift = this->par("timeshift");    // simtime = pcap_time + timeshift
    endtime = this->par("endtime");      // simtime of last sent packet, 0 is infinity
    repeattime = this->par("repeattime");   // simtime = pcap_time + timeshift + [0..n] * repeattime

    minPcapTime = 0;
    minPcapTimeUndef = true;
    scheduleNextPacket();
}

void PcapTrafficGenerator::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage())
    {
        send(msg, "out");
        scheduleNextPacket();
    }
    else
        error("PcapTrafficGenerator received a non-self message (module=%s)", getFullPath().c_str());
}

void PcapTrafficGenerator::scheduleNextPacket()
{
    simtime_t curtime = simTime();
    simtime_t pcaptime = 0;
    cMessage* msg = NULL;

    if (!pcapFile.isOpen())
        return;

    do
    {
        delete msg;
        msg = (cMessage*)pcapFile.read(pcaptime);
        if (msg)
        {
            if (minPcapTimeUndef)
            {
                minPcapTime = pcaptime;
                minPcapTimeUndef = false;
            }
            else
                if (repeattime > SIMTIME_ZERO && minPcapTime + repeattime < pcaptime)
                    error("The 'repeattime' parameter smaller than timelength of pcap file at module %s", getFullPath().c_str());
        }
        if (!msg && pcapFile.eof() && repeattime > SIMTIME_ZERO)
        {
            pcapFile.restart();
            msg = (cMessage*)pcapFile.read(pcaptime);
        }
    } while (msg && (curtime > (pcaptime + timeshift)));

    if (msg)
        scheduleAt(pcaptime + timeshift, msg);
}
