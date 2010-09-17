//
// Copyright (C) 2010  Zoltan Bojthe
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//
package inet.util;

//
// Provides traffic generator - like functionality
//
simple PcapTrafficGenerator like TrafficGenerator
{
    parameters:
        string pcapFile = default("");
        string pcapParser = default("IPDatagramPcapParser");
        double timeShift @unit("s") = default(0);   // simtime = pcap_time + timeshift
        double endTime @unit("s") = default(0);     // simtime of last sent packet, 0 is infinity
        double repeatGap @unit("s") = default(0);   // wait time after last packet, before resend first packet of pcap file, when smaller or equal than 0 then no repeating 
    gates:
        output out;
}
