//
// Copyright (C) 2013 Andras Varga
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

#ifndef __INET_IRADIOCHANNEL_H_
#define __INET_IRADIOCHANNEL_H_

#include "INETDefs.h"
#include "IRadio.h"
#include "IRadioFrame.h"

/**
 * This interface provides an abstraction for different radio channels.
 */
class INET_API IRadioChannel {
  public:
    virtual ~IRadioChannel() { }

    virtual void registerRadio(IRadio * radio) = 0;

    virtual void unregisterRadio(IRadio * radio) = 0;

    virtual void transmitRadioFrame(IRadio * radio, IRadioFrame * radioFrame) = 0;

//    virtual std::vector<IRadioFrame *>& getOngoingTransmissions(int radioChannel) = 0;
};

#endif
