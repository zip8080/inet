/* -*- mode:c++ -*- ********************************************************
 * file:        BaseModule.cc
 *
 * author:      Steffen Sroka
 *              Andreas Koepke
 *
 * copyright:   (C) 2004 Telecommunication Networks Group (TKN) at
 *              Technische Universitaet Berlin, Germany.
 *
 *              This program is free software; you can redistribute it
 *              and/or modify it under the terms of the GNU General Public
 *              License as published by the Free Software Foundation; either
 *              version 2 of the License, or (at your option) any later
 *              version.
 *              For further information see file COPYING
 *              in the top level directory
 ***************************************************************************
 * part of:     framework implementation developed by tkn
 **************************************************************************/

#include "BaseModule.h"

#include <cassert>

#include "FindModule.h"

// Could not initialize simsignal_t it here!? I got the POST_MODEL_CHANGE id!?
const simsignal_t BaseModule::catHostStateSignal = cComponent::registerSignal(MIXIM_SIGNAL_HOSTSTATE_NAME);

BaseModule::BaseModule()
	: cSimpleModule()
	, cListener()
	, notAffectedByHostState(true)
{}

BaseModule::BaseModule(unsigned stacksize)
	: cSimpleModule(stacksize)
	, cListener()
	, notAffectedByHostState(true)
{}

/**
 * Subscription should be in stage==0, and firing
 * notifications in stage==1 or later.
 *
 * NOTE: You have to call this in the initialize() function of the
 * inherited class!
 */
void BaseModule::initialize(int stage) {
    if (stage == 0) {
    	notAffectedByHostState = 	hasPar("notAffectedByHostState")
								 && par("notAffectedByHostState").boolValue();
        findHost()->subscribe(catHostStateSignal, this);
    }
}

void BaseModule::receiveSignal(cComponent */*source*/, simsignal_t signalID, cObject *obj) {
	Enter_Method_Silent();
	if (signalID == catHostStateSignal) {
		const HostState *const pHostState = dynamic_cast<const HostState *const>(obj);
		if (pHostState) {
			handleHostState(*pHostState);
		}
		else {
			opp_warning("Got catHostStateSignal but obj was not a HostState pointer?");
		}
	}
}

void BaseModule::handleHostState(const HostState& state)
{
	if(notAffectedByHostState)
		return;

	if(state.get() != HostState::ACTIVE) {
		error("Hosts state changed to something else than active which"
			  " is not handled by this module. Either handle this state"
			  " correctly or if this module really isn't affected by the"
			  " hosts state set the parameter \"notAffectedByHostState\""
			  " of this module to true.");
	}
}

void BaseModule::switchHostState(HostState::States state)
{
	HostState hostState(state);
	emit(catHostStateSignal, &hostState);
}

cModule* BaseModule::findHost(void)
{
	return FindModule<>::findHost(this);
}

const cModule* BaseModule::findHost(void) const
{
	return FindModule<>::findHost(this);
}
