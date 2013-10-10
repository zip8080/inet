//
// Copyright (C) 2004 Andras Varga
// Copyright (C) 2000 Institut fuer Telematik, Universitaet Karlsruhe
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//


//  Author: Andras Varga, 2004


#include <algorithm>
#include <sstream>

#include "IPv4InterfaceData.h"

std::string IPv4InterfaceData::HostMulticastData::info()
{
    std::stringstream out;
    if (!joinedMulticastGroups.empty())
    {
        out << " mcastgrps:";
        bool addComma = false;
        for (int i = 0; i < (int)joinedMulticastGroups.size(); ++i)
        {
                out << (addComma?",":"") << joinedMulticastGroups[i].multicastGroup;
                addComma = true;
        }
    }
    return out.str();
}

std::string IPv4InterfaceData::HostMulticastData::detailedInfo()
{

    std::stringstream out;
    out << "Joined Groups:";
    for (int i = 0; i < (int)joinedMulticastGroups.size(); ++i)
        out << " " << joinedMulticastGroups[i].multicastGroup; // << "(" << refCounts[i] << ")";
    out << "\n";
    return out.str();
}

std::string IPv4InterfaceData::RouterMulticastData::info()
{
    std::stringstream out;
    if (reportedMulticastGroups.size() > 0)
    {
        out << " mcast_listeners:";
        for (int i = 0; i < (int)reportedMulticastGroups.size(); ++i)
            out << (i>0?",":"") << reportedMulticastGroups[i];
    }
    if (multicastTtlThreshold > 0)
        out << " ttl_threshold: " << multicastTtlThreshold;
    return out.str();
}

std::string IPv4InterfaceData::RouterMulticastData::detailedInfo()
{
    std::stringstream out;
    out << "TTL Threshold: " << multicastTtlThreshold << "\n";
    out << "Multicast Listeners:";
    for (int i = 0; i < (int)reportedMulticastGroups.size(); ++i)
        out << " " << reportedMulticastGroups[i];
    out << "\n";
    return out.str();
}

IPv4InterfaceData::IPv4InterfaceData()
{
    netmask = IPv4Address::ALLONES_ADDRESS;
    metric = 0;
    hostData = NULL;
    routerData = NULL;
    nb = NULL;
}

IPv4InterfaceData::~IPv4InterfaceData()
{
    delete hostData;
    delete routerData;
}

std::string IPv4InterfaceData::info() const
{
    std::stringstream out;
    out << "IPv4:{inet_addr:" << getIPAddress() << "/" << getNetmask().getNetmaskLength();
    if (hostData)
        out << hostData->info();
    if (routerData)
        out << routerData->info();
    out << "}";
    return out.str();
}

std::string IPv4InterfaceData::detailedInfo() const
{
    std::stringstream out;
    out << "inet addr:" << getIPAddress() << "\tMask: " << getNetmask() << "\n";
    out << "Metric: " << getMetric() << "\n";
    if (hostData)
        out << hostData->detailedInfo();
    if (routerData)
        out << routerData->detailedInfo();
    return out.str();
}

bool IPv4InterfaceData::isMemberOfMulticastGroup(const IPv4Address &multicastAddress) const
{
    HostMulticastGroupVector groups = getHostData()->joinedMulticastGroups;
    for (HostMulticastGroupVector::const_iterator it = groups.begin(); it != groups.end(); ++it)
        if (it->multicastGroup == multicastAddress)
            return true;
    return false;
}

// XXX deprecated
void IPv4InterfaceData::joinMulticastGroup(const IPv4Address& multicastAddress)
{
    IPv4AddressVector empty;
    changeMulticastGroupMembership(multicastAddress, MCAST_INCLUDE_SOURCES, empty, MCAST_EXCLUDE_SOURCES, empty);
}

// XXX deprecated
void IPv4InterfaceData::leaveMulticastGroup(const IPv4Address& multicastAddress)
{
    IPv4AddressVector empty;
    changeMulticastGroupMembership(multicastAddress, MCAST_EXCLUDE_SOURCES, empty, MCAST_INCLUDE_SOURCES, empty);
}

/**
 * This method is called by sockets to register their multicast group membership changes in the interface.
 */
void IPv4InterfaceData::changeMulticastGroupMembership(IPv4Address multicastAddress,
        McastSourceFilterMode oldFilterMode, const IPv4AddressVector &oldSourceList,
        McastSourceFilterMode newFilterMode, const IPv4AddressVector &newSourceList)
{
    if(!multicastAddress.isMulticast())
        throw cRuntimeError("IPv4InterfaceData::changeMulticastGroupMembership(): multicast address expected, received %s.", multicastAddress.str().c_str());

    HostMulticastGroupData *entry = findHostGroupData(multicastAddress);
    if (!entry)
    {
        ASSERT(oldFilterMode == MCAST_INCLUDE_SOURCES && oldSourceList.empty());
        HostMulticastData *data = getHostData();
        data->joinedMulticastGroups.push_back(HostMulticastGroupData(multicastAddress));
        entry = &data->joinedMulticastGroups.back();
    }

    std::map<IPv4Address,int> &counts = oldFilterMode == MCAST_INCLUDE_SOURCES ? entry->includeCounts : entry->excludeCounts;
    for (IPv4AddressVector::const_iterator source = oldSourceList.begin(); source != oldSourceList.end(); ++source)
    {
        std::map<IPv4Address,int>::iterator count = counts.find(*source);
        if (count == counts.end())
            throw cRuntimeError("");
        else if (count->second == 1)
            counts.erase(count);
        else
            count->second--;
    }

    counts = newFilterMode == MCAST_INCLUDE_SOURCES ? entry->includeCounts : entry->excludeCounts;
    for (IPv4AddressVector::const_iterator source = newSourceList.begin(); source != newSourceList.end(); ++source)
    {
        std::map<IPv4Address,int>::iterator count = counts.find(*source);
        if (count == counts.end())
            counts[*source] = 1;
        else
            count->second++;
    }

    // update number of EXCLUDE mode sockets
    if (oldFilterMode == MCAST_INCLUDE_SOURCES && newFilterMode == MCAST_EXCLUDE_SOURCES)
        entry->numOfExcludeModeSockets++;
    else if (oldFilterMode == MCAST_EXCLUDE_SOURCES && newFilterMode == MCAST_INCLUDE_SOURCES)
        entry->numOfExcludeModeSockets--;

    // compute filterMode and sourceList
    bool changed = entry->updateSourceList();

    if (changed)
    {
        changed1(F_MULTICAST_ADDRESSES);

        if (!nb)
            nb = NotificationBoardAccess().get();
        IPv4MulticastGroupSourceInfo info(ownerp, multicastAddress, entry->filterMode, entry->sourceList);
        nb->fireChangeNotification(NF_IPv4_MCAST_CHANGE, &info);

        // Legacy notifications
        if (oldFilterMode != newFilterMode && oldSourceList.empty() && newSourceList.empty())
        {
            IPv4MulticastGroupInfo info2(ownerp, multicastAddress);
            nb->fireChangeNotification(newFilterMode == MCAST_EXCLUDE_SOURCES ? NF_IPv4_MCAST_JOIN : NF_IPv4_MCAST_LEAVE, &info2);
        }

        // remove group data if it is INCLUDE(empty)
        if (entry->filterMode == MCAST_INCLUDE_SOURCES && entry->sourceList.empty())
        {
            removeHostGroupData(multicastAddress);
        }
    }
}

/**
 * Computes the filterMode and sourceList of the interface from the socket reference counts
 * according to RFC3376 3.2.
 * Returns true if filterMode or sourceList has been changed.
 */
bool IPv4InterfaceData::HostMulticastGroupData::updateSourceList()
{
    // Filter mode is EXCLUDE if any of the sockets are in EXCLUDE mode, otherwise INCLUDE
    McastSourceFilterMode filterMode = numOfExcludeModeSockets == 0 ? MCAST_INCLUDE_SOURCES : MCAST_EXCLUDE_SOURCES;

    IPv4AddressVector sourceList;
    if (numOfExcludeModeSockets == 0)
    {
        // If all socket is in INCLUDE mode, then the sourceList is the union of included sources
        for (std::map<IPv4Address,int>::iterator it = includeCounts.begin(); it != includeCounts.end(); ++it)
            sourceList.push_back(it->first);

    }
    else
    {
        // If some socket is in EXCLUDE mode, then the sourceList contains the sources that are
        // excluded by all EXCLUDE mode sockets except if there is a socket including the source.
        for (std::map<IPv4Address,int>::iterator it = excludeCounts.begin(); it != excludeCounts.end(); ++it)
            if (it->second == numOfExcludeModeSockets && includeCounts.find(it->first) == includeCounts.end())
                sourceList.push_back(it->first);
    }

    if (this->filterMode != filterMode || this->sourceList != sourceList)
    {
        this->filterMode = filterMode;
        this->sourceList = sourceList;
        return true;
    }
    else
        return false;
}


bool IPv4InterfaceData::hasMulticastListener(const IPv4Address &multicastAddress) const
{
    const IPv4AddressVector &multicastGroups = getRouterData()->reportedMulticastGroups;
    return find(multicastGroups.begin(),  multicastGroups.end(), multicastAddress) != multicastGroups.end();
}

void IPv4InterfaceData::addMulticastListener(const IPv4Address &multicastAddress)
{
    if(!multicastAddress.isMulticast())
        throw cRuntimeError("IPv4InterfaceData::addMulticastListener(): multicast address expected, received %s.", multicastAddress.str().c_str());

    if (!hasMulticastListener(multicastAddress))
    {
        getRouterData()->reportedMulticastGroups.push_back(multicastAddress);
        changed1(F_MULTICAST_LISTENERS);
    }
}

void IPv4InterfaceData::removeMulticastListener(const IPv4Address &multicastAddress)
{
    IPv4AddressVector &multicastGroups = getRouterData()->reportedMulticastGroups;

    int n = multicastGroups.size();
    int i;
    for (i = 0; i < n; i++)
        if (multicastGroups[i] == multicastAddress)
            break;
    if (i != n)
    {
        multicastGroups.erase(multicastGroups.begin() + i);
        changed1(F_MULTICAST_LISTENERS);
    }
}

IPv4InterfaceData::HostMulticastGroupData *IPv4InterfaceData::findHostGroupData(IPv4Address multicastAddress)
{
    ASSERT(multicastAddress.isMulticast());
    HostMulticastGroupVector &entries = getHostData()->joinedMulticastGroups;
    for (HostMulticastGroupVector::iterator it = entries.begin(); it != entries.end(); ++it)
        if (it->multicastGroup == multicastAddress)
            return &(*it);
    return NULL;
}

bool IPv4InterfaceData::removeHostGroupData(IPv4Address multicastAddress)
{
    ASSERT(multicastAddress.isMulticast());
    HostMulticastGroupVector &entries = getHostData()->joinedMulticastGroups;
    for (HostMulticastGroupVector::iterator it = entries.begin(); it != entries.end(); ++it)
        if (it->multicastGroup == multicastAddress)
        {
            entries.erase(it);
            return true;
        }
    return false;
}
