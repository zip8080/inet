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

#ifndef __INET_MACADDRESSTABLE_H_
#define __INET_MACADDRESSTABLE_H_

#include "MACAddress.h"

class MACAddressTable : public cSimpleModule
{
    protected:

        struct AddressEntry
        {
                unsigned int vid;           // VLAN ID
                int portno;                 // Input port
                simtime_t insertionTime;    // Arrival time of Lookup Address Table entry
                AddressEntry() :
                        vid(0)
                {
                }
                AddressEntry(unsigned int vid, int portno, simtime_t insertionTime) :
                        vid(vid), portno(portno), insertionTime(insertionTime)
                {
                }
        };

        typedef std::map<MACAddress, AddressEntry> AddressTable;
        typedef std::map<unsigned int, AddressTable*> VlanAddressTable;

        simtime_t agingTime;                // Max idle time for address table entries
        AddressTable * addressTable;        // VLAN-unaware address lookup (vid = 0)
        VlanAddressTable vlanAddressTable;  // VLAN-aware address lookup

    protected:

        virtual void initialize();
        virtual void handleMessage(cMessage *msg);

        /**
         * @brief Returns a MAC Address Table for a specified VLAN ID
         */
        AddressTable * getTableForVid(unsigned int vid);

    public:

        MACAddressTable();
        ~MACAddressTable();

    public:
        // Table management

        /**
         * @brief For a known arriving port, V-TAG and destination MAC. It finds out the port where relay component should deliver the message
         * @param address MAC destination
         * @param vid VLAN ID
         * @return Output port for address, or -1 if unknown.
         */
        virtual int getPortForAddress(MACAddress& address, unsigned int vid = 0);

        /**
         * @brief Register a new MAC address at AddressTable.
         * @return True if refreshed. False if it is new.
         */
        virtual bool updateTableWithAddress(int portno, MACAddress& address, unsigned int vid = 0);

        /**
         *  @brief Clears portno cache
         */
        virtual void flush(int portno);

        /**
         *  @brief Prints cached data
         */
        virtual void printState();

        /**
         * @brief Copy cache from portA to portB port
         */
        virtual void copyTable(int portA, int portB);

        /**
         * @brief Remove aged entries from a specified VLAN
         */
        virtual void removeAgedEntriesFromVlan(unsigned int vid = 0);
        /**
         * @brief Remove aged entries from all VLANs
         */
        virtual void removeAgedEntriesFromAllVlans();
};

#endif
