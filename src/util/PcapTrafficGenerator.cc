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

#include "InterfaceTableAccess.h"
#include "IPControlInfo.h"
#include "IPDatagram.h"
#include "IPRoute.h"
#include "RoutingTableAccess.h"

Define_Module(PcapTrafficGenerator);

void PcapTrafficGenerator::initialize()
{
    const char* filename = this->par("pcapFile");
    const char* parserName = this->par("pcapParser");
    enabled = filename && *filename && parserName && *parserName;
    if (enabled)
    {
        timeShift = this->par("timeShift");
        endTime = this->par("endTime");
        repeatGap = this->par("repeatGap");

        ev << getFullPath() << ".PcapTrafficGenerator::initialize(): "
                <<"file:" << filename
                << ", timeShift:" << timeShift
                << ", endTime:" << endTime
                << ", repeatGap:" << repeatGap
                << endl;

        pcapFile.open(filename);
        pcapFile.setParser(parserName);
        scheduleNextPacket();
    }
}

void PcapTrafficGenerator::handleMessage(cMessage *msg)
{
    if (!enabled)
    {
        error("disbled PcapTrafficGenerator received a message (module=%s)", getFullPath().c_str());
    }
    else if (msg->isSelfMessage())
    {
        if (msg->isPacket())
        {
            IPDatagram* ipd = dynamic_cast<IPDatagram*>(msg);
            if (ipd)
            {
                IPAddress destAddr = ipd->getDestAddress();
                IPRoutingDecision *controlInfo = new IPRoutingDecision();

                ipd->setControlInfo(controlInfo);
                ipd->setTransportProtocol(IP_PROT_NONE);
                send(msg, "out");
                msg = NULL;
            }
        }
        delete msg;
        scheduleNextPacket();
    }
    else
        error("PcapTrafficGenerator received a non-self message (module=%s)", getFullPath().c_str());
}

void PcapTrafficGenerator::scheduleNextPacket()
{
    if (!enabled)
        return;

    simtime_t curtime = simTime();
    simtime_t pcaptime = 0;
    cMessage* msg = NULL;

    if (!pcapFile.isOpen())
        return;

    do
    {
        delete msg;
        msg = (cMessage*)pcapFile.read(pcaptime);
        if (!msg && pcapFile.eof() && repeatGap > SIMTIME_ZERO)
        {
            pcapFile.restart();
            timeShift = curtime + repeatGap;
            msg = (cMessage*)pcapFile.read(pcaptime);
        }
        pcaptime += timeShift;
    } while (msg && (curtime > (pcaptime)));

    if (msg)
        scheduleAt(pcaptime, msg);
}
