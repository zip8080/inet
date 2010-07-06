//
// Copyright (C) 2010 Zoltan Bojthe
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
// Provides tcpdump-like functionality
//
simple PcapTrace
{
    parameters:
        bool enabled = default(true);
        string filename = default("pcap-trace.pcap");
        string moduleName = default("");
        string gateName = default("");
        int gateIndex = default(-1);
        int snaplen = default(65535);
}
