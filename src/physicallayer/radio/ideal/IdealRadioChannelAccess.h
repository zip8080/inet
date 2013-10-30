//
// Copyright (C) 2013 OpenSim Ltd
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

#ifndef __INET_IDEALRADIOCHANNELACCESS_H
#define __INET_IDEALRADIOCHANNELACCESS_H

#include "INETDefs.h"
#include "IMobility.h"
#include "IdealRadioChannel.h"

/**
 * This class contains functions that cooperate with IdealRadioChannel.
 *
 * author: Zoltan Bojthe, Levente Meszaros
 */
class INET_API IdealRadioChannelAccess : public cSimpleModule
{
  protected:
    cModule *node;
    IMobility *mobility;
    IdealRadioChannel *idealRadioChannel;
    IdealRadioChannel::RadioEntry *radioChannelEntry;

  public:
    IdealRadioChannelAccess() : node(NULL), mobility(NULL), idealRadioChannel(NULL), radioChannelEntry(NULL) { }
    virtual ~IdealRadioChannelAccess();

  protected:
    virtual void initialize(int stage);
    virtual void sendToChannel(IdealRadioFrame *msg);
};

#endif
