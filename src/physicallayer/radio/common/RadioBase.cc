//
// Copyright (C) 2013 OpenSim Ltd.
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

#include "RadioBase.h"
#include "ModuleAccess.h"

RadioBase::RadioBase()
{
    radioMode = RADIO_MODE_OFF;
    radioChannelState = RADIO_CHANNEL_STATE_UNKNOWN;
    radioChannel = 0;
    mobility = NULL;
    upperLayerOut = NULL;
    upperLayerIn = NULL;
    radioIn = NULL;
}

void RadioBase::initialize(int stage)
{
    cSimpleModule::initialize(stage);
    EV << "Initializing RadioBase, stage = " << stage << endl;
    if (stage == INITSTAGE_LOCAL)
    {
        upperLayerIn = gate("upperLayerIn");
        upperLayerOut = gate("upperLayerOut");
        radioIn = gate("radioIn");
        radioIn->setDeliverOnReceptionStart(true);
        cModule *node = findContainingNode(this);
        mobility = check_and_cast<IMobility*>(node->getSubmodule("mobility"));
        WATCH(radioMode);
        WATCH(radioChannelState);
        WATCH(radioChannel);
    }
}
