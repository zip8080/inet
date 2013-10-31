#include "BasePhyLayer.h"

#include <cxmlelement.h>
#include <limits>

#include "IRadio.h"
#include "PhyControlInfo_m.h"
#include "PhyToMacControlInfo.h"
#include "FindModule.h"
#include "AnalogueModel.h"
#include "Decider.h"
#include "BaseConnectionManager.h"

short BasePhyLayer::airFramePriority = 10;

Coord NoMobiltyPos = Coord::ZERO;

template<typename _Tp>
static inline bool isFiniteNumber(register _Tp value) {
    return value < std::numeric_limits<_Tp>::infinity() && value > -std::numeric_limits<_Tp>::infinity() && value != std::numeric_limits<_Tp>::quiet_NaN();
}

//--Initialization----------------------------------

BasePhyLayer::BasePhyLayer()
	: DetailedRadioChannelAccess()
	, DeciderToPhyInterface()
	, thermalNoise(NULL)
    , radioMode(RADIO_MODE_OFF)
    , radioChannelState(RADIO_CHANNEL_STATE_UNKNOWN)
    , radioChannel(0)
	, maxTXPower(0)
	, sensitivity(0)
	, recordStats(false)
	, channelInfo()
	, radio(NULL)
	, decider(NULL)
	, analogueModels()
	, upperLayerIn(-1)
	, upperLayerOut(-1)
	, radioSwitchingOverTimer(NULL)
	, txOverTimer(NULL)
	, headerLength(-1)
{}

template<class T> T BasePhyLayer::readPar(const char* parName, const T defaultValue) const {
	if(hasPar(parName))
		return par(parName);
	return defaultValue;
}

// the following line is needed to allow linking when compiled in RELEASE mode.
// Add a declaration for each parameterization of the template used in
// code to be linked, e.g. in modules or in examples, if it is not already
// used in base (double and simtime_t). Needed with (at least): gcc 4.4.1.
template int BasePhyLayer::readPar<int>(const char* parName, const int) const;
template double BasePhyLayer::readPar<double>(const char* parName, const double) const;
template<> simtime_t BasePhyLayer::readPar<simtime_t>(const char* parName, const simtime_t defaultValue) const {
	if(hasPar(parName))
		return simtime_t( par(parName).doubleValue() );
	return defaultValue;
}
template bool BasePhyLayer::readPar<bool>(const char* parName, const bool) const;

void BasePhyLayer::initialize(int stage) {

	DetailedRadioChannelAccess::initialize(stage);

	if (stage == 0) {
		// if using sendDirect, make sure that messages arrive without delay
		gate("radioIn")->setDeliverOnReceptionStart(true);

		//get gate ids
		upperLayerIn = findGate("upperLayerIn");
		upperLayerOut = findGate("upperLayerOut");

		//read simple ned-parameters
		//	- initialize basic parameters
		if(par("useThermalNoise").boolValue()) {
			double thermalNoiseVal = FWMath::dBm2mW(par("thermalNoise").doubleValue());
			thermalNoise = new ConstantSimpleConstMapping(DimensionSet::timeDomain,
														  thermalNoiseVal);
		} else {
			thermalNoise = 0;
		}
		headerLength = par("headerLength").longValue();
		sensitivity = par("sensitivity").doubleValue();
		if (!isFiniteNumber(sensitivity) || sensitivity <= -999999)
		    sensitivity = 0; // disabled
		else
		    sensitivity = FWMath::dBm2mW(sensitivity);
		if (!isFiniteNumber(sensitivity))
		    sensitivity = 0; // disabled
		txPower = par("txPower").doubleValue();
		maxTXPower = par("maxTXPower").doubleValue();

		recordStats = par("recordStats").boolValue();

		//	- initialize radio
		radio = initializeRadio();
	    radioChannel = readPar("initialRadioChannel", 0);
	    emit(radioModeChangedSignal, radio->getCurrentState());

        if(cc->hasPar("sat")
		   && (sensitivity - FWMath::dBm2mW(cc->par("sat").doubleValue())) < -0.000001) {
            opp_error("Sensitivity can't be smaller than the "
					  "signal attenuation threshold (sat) in ConnectionManager. "
					  "Please adjust your omnetpp.ini file accordingly.");
		}

//	} else if (stage == 1){
		//read complex(xml) ned-parameters
		//	- analogue model parameters
		initializeAnalogueModels(par("analogueModels").xmlValue());
		//	- decider parameters
		initializeDecider(par("decider").xmlValue());

		//initialise timer messages
		radioSwitchingOverTimer = new cMessage("radio switching over", RADIO_SWITCHING_OVER);
		txOverTimer = new cMessage("transmission over", TX_OVER);

	}
}

MiximRadio* BasePhyLayer::initializeRadio() const {
	int    initialRadioState   = par("initialRadioState").longValue();
	double radioMinAtt         = par("radioMinAtt").doubleValue();
	double radioMaxAtt         = par("radioMaxAtt").doubleValue();
	int    nbRadioChannels     = readPar("nbRadioChannels",     1);

	MiximRadio* radio = MiximRadio::createNewRadio(recordStats, initialRadioState,
										 radioMinAtt, radioMaxAtt, nbRadioChannels);

	//	- switch times to TX
	//if no RX to TX defined asume same time as sleep to TX
	radio->setSwitchTime(RADIO_MODE_RECEIVER, RADIO_MODE_TRANSMITTER, (hasPar("timeRXToTX") ? par("timeRXToTX") : par("timeSleepToTX")).doubleValue());
	//if no sleep to TX defined asume same time as RX to TX
	radio->setSwitchTime(RADIO_MODE_SLEEP, RADIO_MODE_TRANSMITTER, (hasPar("timeSleepToTX") ? par("timeSleepToTX") : par("timeRXToTX")).doubleValue());

	//	- switch times to RX
	//if no TX to RX defined asume same time as sleep to RX
	radio->setSwitchTime(RADIO_MODE_TRANSMITTER, RADIO_MODE_RECEIVER, (hasPar("timeTXToRX") ? par("timeTXToRX") : par("timeSleepToRX")).doubleValue());
	//if no sleep to RX defined asume same time as TX to RX
	radio->setSwitchTime(RADIO_MODE_SLEEP, RADIO_MODE_RECEIVER, (hasPar("timeSleepToRX") ? par("timeSleepToRX") : par("timeTXToRX")).doubleValue());

	//	- switch times to sleep
	//if no TX to sleep defined asume same time as RX to sleep
	radio->setSwitchTime(RADIO_MODE_TRANSMITTER, RADIO_MODE_SLEEP, (hasPar("timeTXToSleep") ? par("timeTXToSleep") : par("timeRXToSleep")).doubleValue());
	//if no RX to sleep defined asume same time as TX to sleep
	radio->setSwitchTime(RADIO_MODE_RECEIVER, RADIO_MODE_SLEEP, (hasPar("timeRXToSleep") ? par("timeRXToSleep") : par("timeTXToSleep")).doubleValue());

	return radio;
}

void BasePhyLayer::getParametersFromXML(cXMLElement* xmlData, ParameterMap& outputMap) const {
	cXMLElementList parameters = xmlData->getElementsByTagName("Parameter");

	for(cXMLElementList::const_iterator it = parameters.begin();
		it != parameters.end(); it++) {

		const char* name = (*it)->getAttribute("name");
		const char* type = (*it)->getAttribute("type");
		const char* value = (*it)->getAttribute("value");
		if(name == 0 || type == 0 || value == 0) {
			EV << "Invalid parameter, could not find name, type or value." << endl;
			continue;
		}

		std::string sType = type; 	//needed for easier comparision
		std::string sValue = value;	//needed for easier comparision

		cMsgPar param(name);

		//parse type of parameter and set value
		if (sType == "bool") {
			param.setBoolValue(sValue == "true" || sValue == "1");

		} else if (sType == "double") {
			param.setDoubleValue(strtod(value, 0));

		} else if (sType == "string") {
			param.setStringValue(value);

		} else if (sType == "long") {
			param.setLongValue(strtol(value, 0, 0));

		} else {
			EV << "Unknown parameter type: \"" << sType << "\"" << endl;
			continue;
		}

		//add parameter to output map
		outputMap[name] = param;
	}
}

void BasePhyLayer::finish(){
	// give decider the chance to do something
	decider->finish();
}

//-----Decider initialization----------------------


void BasePhyLayer::initializeDecider(cXMLElement* xmlConfig) {

	decider = NULL;

	if(xmlConfig == NULL) {
		opp_error("No decider configuration file specified.");
		return;
	}

	cXMLElementList deciderList = xmlConfig->getElementsByTagName("Decider");

	if(deciderList.empty()) {
		opp_error("No decider configuration found in configuration file.");
		return;
	}

	if(deciderList.size() > 1) {
		opp_error("More than one decider configuration found in configuration file.");
		return;
	}

	cXMLElement* deciderData = deciderList.front();

	const char* name = deciderData->getAttribute("type");

	if(name == NULL) {
		opp_error("Could not read type of decider from configuration file.");
		return;
	}

	ParameterMap params;
	getParametersFromXML(deciderData, params);

	decider = getDeciderFromName(name, params);

	if(decider == NULL) {
		opp_error("Could not find a decider with the name \"%s\".", name);
		return;
	}

	EV_INFO << "Decider \"" << name << "\" loaded." << endl;
}

Decider* BasePhyLayer::getDeciderFromName(const std::string& /*name*/, ParameterMap& /*params*/)
{
	return NULL;
}


//-----AnalogueModels initialization----------------

void BasePhyLayer::initializeAnalogueModels(cXMLElement* xmlConfig) {

	/*
	* first of all, attach the AnalogueModel that represents the RadioState
	* to the AnalogueModelList as first element.
	*/

	std::string s("RadioStateAnalogueModel");
	ParameterMap p;

	AnalogueModel* newAnalogueModel = getAnalogueModelFromName(s, p);

	if(newAnalogueModel == 0)
	{
		opp_warning("Could not find an analogue model with the name \"%s\".", s.c_str());
	}
	else
	{
		analogueModels.push_back(newAnalogueModel);
	}


	if(xmlConfig == 0) {
		opp_warning("No analogue models configuration file specified.");
		return;
	}

	cXMLElementList analogueModelList = xmlConfig->getElementsByTagName("AnalogueModel");

	if(analogueModelList.empty()) {
		opp_warning("No analogue models configuration found in configuration file.");
		return;
	}

	// iterate over all AnalogueModel-entries, get a new AnalogueModel instance and add
	// it to analogueModels
	for(cXMLElementList::const_iterator it = analogueModelList.begin();
		it != analogueModelList.end(); it++) {


		cXMLElement* analogueModelData = *it;

		const char* name = analogueModelData->getAttribute("type");

		if(name == 0) {
			opp_warning("Could not read name of analogue model.");
			continue;
		}

		ParameterMap params;
		getParametersFromXML(analogueModelData, params);

		AnalogueModel* newAnalogueModel = getAnalogueModelFromName(name, params);

		if(newAnalogueModel == 0) {
			opp_warning("Could not find an analogue model with the name \"%s\".", name);
			continue;
		}

		// attach the new AnalogueModel to the AnalogueModelList
		analogueModels.push_back(newAnalogueModel);

		EV_INFO << "AnalogueModel \"" << name << "\" loaded." << endl;

	} // end iterator loop


}

AnalogueModel* BasePhyLayer::getAnalogueModelFromName(const std::string& name, ParameterMap& /*params*/) const {

	// add default analogue models here

	// case "RSAM", pointer is valid as long as the radio exists
	if (name == "RadioStateAnalogueModel") {
		return radio->getAnalogueModel();
	}

	return NULL;
}

//--Message handling--------------------------------------

void BasePhyLayer::handleMessage(cMessage* msg) {

	//self messages
	if(msg->isSelfMessage()) {
		handleSelfMessage(msg);

    //controlmessages
    } else if(msg->getArrivalGateId() == upperLayerIn && !dynamic_cast<cPacket *>(msg)) {
        handleUpperControlMessage(msg);

	//MacPkts <- MacToPhyControlInfo
	} else if(msg->getArrivalGateId() == upperLayerIn) {
		handleUpperMessage(msg);

	//AirFrames
	} else if(msg->getKind() == AIR_FRAME){
		handleAirFrame(static_cast<DetailedRadioFrame*>(msg));

	//unknown message
	} else {
		EV << "Unknown message received." << endl;
		delete msg;
	}
}

void BasePhyLayer::handleAirFrame(DetailedRadioFrame* frame) {
	//TODO: ask jerome to set air frame priority in his UWBIRPhy
	//assert(frame->getSchedulingPriority() == airFramePriority);

	switch(frame->getState()) {
	case START_RECEIVE:
		handleAirFrameStartReceive(frame);
		break;

	case RECEIVING: {
		handleAirFrameReceiving(frame);
		break;
	}
	case END_RECEIVE:
		handleAirFrameEndReceive(frame);
		break;

	default:
		opp_error( "Unknown AirFrame state: %s", frame->getState());
		break;
	}
	updateRadioChannelState();
}

void BasePhyLayer::handleAirFrameStartReceive(DetailedRadioFrame* frame) {
	EV_INFO << "Received new AirFrame " << frame << " from channel " << frame->getChannel() << "." << endl;

	if(channelInfo.isChannelEmpty()) {
		radio->setTrackingModeTo(true);
	}

	channelInfo.addAirFrame(frame, simTime());
	assert(!channelInfo.isChannelEmpty());

	if(usePropagationDelay) {
	    DetailedRadioSignal&   s     = frame->getSignal();
		simtime_t delay = simTime() - s.getSendingStart();
		s.setPropagationDelay(delay);
	}
	assert(frame->getSignal().getReceptionStart() == simTime());

	frame->getSignal().setReceptionSenderInfo(frame);
	filterSignal(frame);

	if(decider && isKnownProtocolId(frame->getProtocolId())) {
		frame->setState(RECEIVING);

		//pass the AirFrame the first time to the Decider
		handleAirFrameReceiving(frame);

	//if no decider is defined we will schedule the message directly to its end
	} else {
	    DetailedRadioSignal& signal = frame->getSignal();

		simtime_t signalEndTime = signal.getReceptionStart() + frame->getDuration();
		frame->setState(END_RECEIVE);

		sendSelfMessage(frame, signalEndTime);
	}
}

void BasePhyLayer::handleAirFrameReceiving(DetailedRadioFrame* frame) {

    DetailedRadioSignal&   FrameSignal    = frame->getSignal();
	simtime_t nextHandleTime = decider->processSignal(frame);
	simtime_t signalEndTime  = FrameSignal.getReceptionEnd();

	assert(FrameSignal.getDuration() == frame->getDuration());

	//check if this is the end of the receiving process
	if(simTime() >= signalEndTime) {
		frame->setState(END_RECEIVE);
		handleAirFrameEndReceive(frame);
		return;
	}

	//smaller zero means don't give it to me again
	if(nextHandleTime < SIMTIME_ZERO) {
		nextHandleTime = signalEndTime;
		frame->setState(END_RECEIVE);

	//invalid point in time
	} else if(nextHandleTime < simTime() || nextHandleTime > signalEndTime) {
		opp_error("Invalid next handle time returned by Decider. Expected a value between current simulation time (%s) and end of signal (%s) but got %s",
		          SIMTIME_STR(simTime()), SIMTIME_STR(signalEndTime), SIMTIME_STR(nextHandleTime));
	}

	EV_INFO << "Handed AirFrame with ID " << frame->getId() << " to Decider. Next handling in " << (nextHandleTime - simTime()) << "s." << endl;

	sendSelfMessage(frame, nextHandleTime);
}

void BasePhyLayer::handleAirFrameEndReceive(DetailedRadioFrame* frame)
{
	EV_INFO << "End of Airframe with ID " << frame->getId() << "." << endl;

	simtime_t earliestInfoPoint = channelInfo.removeAirFrame(frame);

	/* clean information in the radio until earliest time-point
	*  of information in the ChannelInfo,
	*  since this time-point might have changed due to removal of
	*  the AirFrame
	*/
	if(channelInfo.isChannelEmpty()) {
		earliestInfoPoint = simTime();
		radio->setTrackingModeTo(false);
	}

	radio->cleanAnalogueModelUntil(earliestInfoPoint);
}

void BasePhyLayer::handleUpperMessage(cMessage* msg){
	// check if Radio is in TX state
	if (radio->getCurrentState() != RADIO_MODE_TRANSMITTER)
	{
        delete msg;
        msg = 0;
		opp_error("Error: message for sending received, but radio not in state TX");
	}

	// check if not already sending
	if(txOverTimer->isScheduled())
	{
        delete msg;
        msg = 0;
		opp_error("Error: message for sending received, but radio already sending");
	}

	// build the AirFrame to send
	assert(dynamic_cast<cPacket*>(msg) != 0);

	DetailedRadioFrame* frame = encapsMsg(static_cast<cPacket*>(msg));

	// make sure there is no self message of kind TX_OVER scheduled
	// and schedule the actual one
	assert (!txOverTimer->isScheduled());
	sendSelfMessage(txOverTimer, simTime() + frame->getDuration());

	sendMessageDown(frame);
}

DetailedRadioFrame* BasePhyLayer::encapsMsg(cPacket *macPkt)
{
	// the cMessage passed must be a MacPacket... but no cast needed here
	// MacPkt* pkt = static_cast<MacPkt*>(msg);

	// create the new AirFrame
	DetailedRadioFrame* frame = new DetailedRadioFrame(macPkt->getName(), AIR_FRAME);

	// Retrieve the pointer to the Signal-instance from the ControlInfo-instance.
	// We are now the new owner of this instance.
	// TODO: KLUDGE: XXX:
    DetailedRadioSignal* s = createSignal(macPkt);
	// make sure we really obtained a pointer to an instance
	assert(s);

	// set the members
	assert(s->getDuration() > 0);
	frame->setDuration(s->getDuration());
	// copy the signal into the AirFrame
	frame->setSignal(*s);
	//set priority of AirFrames above the normal priority to ensure
	//channel consistency (before any thing else happens at a time
	//point t make sure that the channel has removed every AirFrame
	//ended at t and added every AirFrame started at t)
	frame->setSchedulingPriority(airFramePriority);
	frame->setProtocolId(myProtocolId());
	frame->setBitLength(headerLength);
	frame->setId(simulation.getUniqueNumber());
	frame->setChannel(radioChannel);

	// pointer and Signal not needed anymore
	delete s;
	s = 0;

	// delete the Control info

    // ...and must always have a ControlInfo attached (contains Signal)
	// TODO:
    cObject* ctrlInfo = macPkt->removeControlInfo();
//    assert(ctrlInfo);
	delete ctrlInfo;
	ctrlInfo = 0;

	frame->encapsulate(macPkt);

	// --- from here on, the AirFrame is the owner of the MacPacket ---
	macPkt = 0;
	EV_INFO <<"AirFrame encapsulated, length: " << frame->getBitLength() << "\n";

	return frame;
}

void BasePhyLayer::handleChannelSenseRequest(cMessage* msg) {
	ChannelSenseRequest* senseReq = static_cast<ChannelSenseRequest*>(msg);

	simtime_t nextHandleTime = decider->handleChannelSenseRequest(senseReq);

	if(nextHandleTime >= simTime()) { //schedule request for next handling
		sendSelfMessage(msg, nextHandleTime);

		//don't throw away any AirFrames while ChannelSenseRequest is active
		if(!channelInfo.isRecording()) {
			channelInfo.startRecording(simTime());
		}
	} else if(nextHandleTime >= SIMTIME_ZERO){
		opp_error("Next handle time of ChannelSenseRequest returned by the Decider is smaller then current simulation time: %.2f",
				SIMTIME_DBL(nextHandleTime));
	}

	// else, i.e. nextHandleTime < 0.0, the Decider doesn't want to handle
	// the request again
}

void BasePhyLayer::handleUpperControlMessage(cMessage* msg) {
	switch(msg->getKind()) {
// TODO: complete implementation
    case PHY_C_CONFIGURERADIO:
    {
        PhyControlInfo *phyCtrl = check_and_cast<PhyControlInfo *>(msg->getControlInfo());
        double newBitrate = phyCtrl->getBitrate();
        delete phyCtrl;
        if (newBitrate != -1)
        {
            // TODO:
        }
        break;
    }
	case CHANNEL_SENSE_REQUEST:
		handleChannelSenseRequest(msg);
		break;
	default:
		EV << "Received unknown control message from upper layer!" << endl;
		break;
	}
}

void BasePhyLayer::handleSelfMessage(cMessage* msg) {

	switch(msg->getKind()) {
	//transmission over
	case TX_OVER:
		assert(msg == txOverTimer);
		sendControlMsgToMac(new cMessage("Transmission over", TX_OVER));
		updateRadioChannelState();
		break;

	//radio switch over
	case RADIO_SWITCHING_OVER:
		assert(msg == radioSwitchingOverTimer);
		finishRadioSwitching();
		break;

	//AirFrame
	case AIR_FRAME:
		handleAirFrame(static_cast<DetailedRadioFrame*>(msg));
		break;

	//ChannelSenseRequest
	case CHANNEL_SENSE_REQUEST:
		handleChannelSenseRequest(msg);
		break;

	default:
		break;
	}
}

//--Send messages------------------------------

void BasePhyLayer::sendControlMessageUp(cMessage* msg) {
	// TODO: refactor send(msg, upperControlOut);
}

void BasePhyLayer::sendMacPktUp(cMessage* pkt) {
	send(pkt, upperLayerOut);
}

void BasePhyLayer::sendMessageDown(DetailedRadioFrame* msg) {
	sendToChannel(msg);
	updateRadioChannelState();
}

void BasePhyLayer::sendSelfMessage(cMessage* msg, simtime_t_cref time) {
	//TODO: maybe delete this method because it doesn't makes much sense,
	//		or change it to "scheduleIn(msg, timeDelta)" which schedules
	//		a message to +timeDelta from current time
	scheduleAt(time, msg);
}


void BasePhyLayer::filterSignal(DetailedRadioFrame* frame) {
	if (analogueModels.empty())
		return;

	DetailedRadioChannelAccess *const senderModule   = dynamic_cast<DetailedRadioChannelAccess *const>(frame->getSenderModule());
	DetailedRadioChannelAccess *const receiverModule = dynamic_cast<DetailedRadioChannelAccess *const>(frame->getArrivalModule());
	//const simtime_t      sStart         = frame->getSignal().getReceptionStart();

	assert(senderModule); assert(receiverModule);

	/** claim the Move pattern of the sender from the Signal */
	IMobility * sendersMobility  = senderModule   ? senderModule->getMobilityModule()   : NULL;
	IMobility * receiverMobility = receiverModule ? receiverModule->getMobilityModule() : NULL;

	const Coord sendersPos  = sendersMobility  ? sendersMobility->getCurrentPosition(/*sStart*/) : NoMobiltyPos;
	const Coord receiverPos = receiverMobility ? receiverMobility->getCurrentPosition(/*sStart*/): NoMobiltyPos;

	for(AnalogueModelList::const_iterator it = analogueModels.begin(); it != analogueModels.end(); ++it)
		(*it)->filterSignal(frame, sendersPos, receiverPos);
}

//--Destruction--------------------------------

BasePhyLayer::~BasePhyLayer() {
	//get AirFrames from ChannelInfo and delete
	//(although ChannelInfo normally owns the AirFrames it
	//is not able to cancel and delete them itself
	AirFrameVector channel;
	channelInfo.getAirFrames(0, simTime(), channel);

	for(AirFrameVector::iterator it = channel.begin();
		it != channel.end(); ++it)
	{
		cancelAndDelete(*it);
	}

	//free timer messages
	if(txOverTimer) {
		cancelAndDelete(txOverTimer);
	}
	if(radioSwitchingOverTimer) {
		cancelAndDelete(radioSwitchingOverTimer);
	}

	//free thermal noise mapping
	if(thermalNoise) {
		delete thermalNoise;
	}

	//free Decider
	if(decider != 0) {
		delete decider;
	}

	/*
	 * get a pointer to the radios RSAM again to avoid deleting it,
	 * it is not created by calling new (BasePhyLayer is not the owner)!
	 */
	AnalogueModel* rsamPointer = radio ? radio->getAnalogueModel() : NULL;

	//free AnalogueModels
	for(AnalogueModelList::const_iterator it = analogueModels.begin();
		it != analogueModels.end(); ++it) {

		AnalogueModelList::value_type tmp = *it;

		// do not delete the RSAM, it's not allocated by new!
		if (tmp == rsamPointer) {
			rsamPointer = 0;
			continue;
		}

		if(tmp != 0) {
			delete tmp;
		}
	}


	// free radio
	if(radio != 0) {
		delete radio;
	}
}

//--MacToPhyInterface implementation-----------------------

int BasePhyLayer::getRadioState() const {
	Enter_Method_Silent();
	assert(radio);
	return radio->getCurrentState();
}

/**
 * @brief Returns the true if the radio is in RX state.
 */
bool BasePhyLayer::isRadioInRX() const {
    return getRadioState() == RADIO_MODE_RECEIVER;
}

void BasePhyLayer::finishRadioSwitching(bool bSendCtrlMsg /*= true*/)
{
	radio->endSwitch(simTime());
	RadioMode newRadioMode = (RadioMode)radio->getCurrentState();
    EV << "Changing radio mode from " << getRadioModeName(radioMode) << " to " << getRadioModeName(newRadioMode) << ".\n";
	radioMode = newRadioMode;
    emit(radioModeChangedSignal, radio->getCurrentState());
    updateRadioChannelState();
	if (bSendCtrlMsg) {
	    sendControlMsgToMac(new cMessage("Radio switching over", RADIO_SWITCHING_OVER));
	}
}

void BasePhyLayer::setRadioMode(RadioMode radioMode) {
    setRadioState(radioMode);
}

simtime_t BasePhyLayer::setRadioState(int rs) {
	Enter_Method_Silent();
	assert(radio);

	if(txOverTimer && txOverTimer->isScheduled()) {
		opp_warning("Switched radio while sending an AirFrame. The effects this would have on the transmission are not simulated by the BasePhyLayer!");
	}

	simtime_t switchTime = radio->switchTo(rs, simTime());

	//invalid switch time, we are probably already switching
	if(switchTime < SIMTIME_ZERO)
		return switchTime;

	// if switching is done in exactly zero-time no extra self-message is scheduled
	if (switchTime == SIMTIME_ZERO) {
		// In case of zero-time-switch, send no control-message to MAC!
		finishRadioSwitching(false);
	}
	else {
		sendSelfMessage(radioSwitchingOverTimer, simTime() + switchTime);
	}

	return switchTime;
}

ChannelState BasePhyLayer::getChannelState() const {
	Enter_Method_Silent();
	assert(decider);
	return decider->getChannelState();
}

void BasePhyLayer::updateRadioChannelState() {
    RadioChannelState newRadioChannelState;
    if (radioMode == RADIO_MODE_OFF || radioMode == RADIO_MODE_SLEEP)
        newRadioChannelState = RADIO_CHANNEL_STATE_UNKNOWN;
    else if (txOverTimer->isScheduled())
        newRadioChannelState = RADIO_CHANNEL_STATE_TRANSMITTING;
    else if (!channelInfo.isChannelEmpty())
        newRadioChannelState = IRadio::RADIO_CHANNEL_STATE_RECEIVING;
    else
        newRadioChannelState = RADIO_CHANNEL_STATE_FREE;
    if (radioChannelState != newRadioChannelState)
    {
        EV << "Changing radio channel state from " << getRadioChannelStateName(radioChannelState) << " to " << getRadioChannelStateName(newRadioChannelState) << ".\n";
        radioChannelState = newRadioChannelState;
        emit(radioChannelStateChangedSignal, newRadioChannelState);
    }
}

long BasePhyLayer::getPhyHeaderLength() const {
	Enter_Method_Silent();
	if (headerLength < 0)
		return par("headerLength").longValue();
	return headerLength;
}

void BasePhyLayer::setRadioChannel(int newRadioChannel) {
	if (newRadioChannel != radioChannel)
	{
        if(txOverTimer && txOverTimer->isScheduled())
            opp_warning("Switched channel while sending an AirFrame. The effects this would have on the transmission are not simulated by the BasePhyLayer!");
        EV << "Changing radio channel from " << radioChannel << " to " << newRadioChannel << ".\n";
        radioChannel = newRadioChannel;
        decider->channelChanged(newRadioChannel);
        emit(radioChannelChangedSignal, newRadioChannel);
	}
}

int BasePhyLayer::getNbRadioChannels() const {
	return radio->getNbChannels();
}

//--DeciderToPhyInterface implementation------------

void BasePhyLayer::getChannelInfo(simtime_t_cref from, simtime_t_cref to, AirFrameVector& out) const {
	if (getNbRadioChannels() < 2) {
		channelInfo.getAirFrames(from, to, out);
	}
	else {
		// here we filter out all air frames which are not on current channel
		struct airframe_filter_channel_fctr : public ChannelInfo::airframe_filter_fctr {
			int iChNum;

			airframe_filter_channel_fctr(int chNum) : ChannelInfo::airframe_filter_fctr(), iChNum(chNum)
			{}

			virtual bool pass(const DetailedRadioFrame*& a) const {
				return a->getChannel() == iChNum;
			}
		};

		airframe_filter_channel_fctr fctrFilterChannel(getRadioChannel());
		channelInfo.getAirFrames(from, to, out, &fctrFilterChannel);
	}
}

ConstMapping* BasePhyLayer::getThermalNoise(simtime_t_cref from, simtime_t_cref /*to*/) {
	if(thermalNoise)
		thermalNoise->initializeArguments(Argument(from));

	return thermalNoise;
}

void BasePhyLayer::sendControlMsgToMac(cMessage* msg) {
	if(msg->getKind() == CHANNEL_SENSE_REQUEST) {
		if(channelInfo.isRecording()) {
			channelInfo.stopRecording();
		}
	}
	sendControlMessageUp(msg);
}

void BasePhyLayer::sendUp(DetailedRadioFrame* frame, DeciderResult* result) {

	EV_INFO << "Decapsulating MacPacket from Airframe with ID " << frame->getId() << " and sending it up to MAC." << endl;

	cMessage* packet = frame->decapsulate();

	assert(packet);

	setUpControlInfo(packet, result);

	sendMacPktUp(packet);
}

simtime_t BasePhyLayer::getSimTime() const {

	return simTime();
}

void BasePhyLayer::cancelScheduledMessage(cMessage* msg) {
	if(msg->isScheduled()){
		cancelEvent(msg);
	} else {
		EV << "Warning: Decider wanted to cancel a scheduled message but message"
		   << " wasn't actually scheduled. Message is: " << msg << endl;
	}
}

void BasePhyLayer::rescheduleMessage(cMessage* msg, simtime_t_cref t) {
	cancelScheduledMessage(msg);
	scheduleAt(t, msg);
}

void BasePhyLayer::drawCurrent(double amount, int activity) {
	MiximBatteryAccess::drawCurrent(amount, activity);
}

void BasePhyLayer::recordScalar(const char *name, double value, const char *unit) {
	DetailedRadioChannelAccess::recordScalar(name, value, unit);
}

/**
 * Attaches a "control info" (PhyToMac) structure (object) to the message pMsg.
 */
cObject* BasePhyLayer::setUpControlInfo(cMessage *const pMsg, DeciderResult *const pDeciderResult)
{
	return PhyToMacControlInfo::setControlInfo(pMsg, pDeciderResult);
}

DetailedRadioSignal* BasePhyLayer::createSignal(cPacket *macPkt)
{
    // TODO: KLUDGE:
    double bitrate = 2E+6;
    double duration = macPkt->getBitLength() / bitrate;
    return createSignal(simTime(), duration, txPower, bitrate);
}

DetailedRadioSignal* BasePhyLayer::createSignal(simtime_t_cref start, simtime_t_cref length, double power, double bitrate)
{
    simtime_t end = start + length;
    //create signal with start at current simtime and passed length
    DetailedRadioSignal* s = new DetailedRadioSignal(start, length);

    //create and set tx power mapping
    Mapping* txPowerMapping = createRectangleMapping(start, end, power);
    s->setTransmissionPower(txPowerMapping);

    //create and set bitrate mapping
    Mapping* bitrateMapping = createConstantMapping(start, end, bitrate);
    s->setBitrate(bitrateMapping);

    return s;
}

Mapping* BasePhyLayer::createConstantMapping(simtime_t_cref start, simtime_t_cref end, Argument::mapped_type_cref value)
{
    //create mapping over time
    Mapping* m = MappingUtils::createMapping(Argument::MappedZero, DimensionSet::timeDomain, Mapping::LINEAR);

    //set position Argument
    Argument startPos(start);

    //set mapping at position
    m->setValue(startPos, value);

    //set position Argument
    Argument endPos(end);

    //set mapping at position
    m->setValue(endPos, value);

    return m;
}

Mapping* BasePhyLayer::createRectangleMapping(simtime_t_cref start, simtime_t_cref end, Argument::mapped_type_cref value)
{
    //create mapping over time
    Mapping* m = MappingUtils::createMapping(DimensionSet::timeDomain, Mapping::LINEAR);

    //set position Argument
    Argument startPos(start);
    //set discontinuity at position
    MappingUtils::addDiscontinuity(m, startPos, Argument::MappedZero, MappingUtils::post(start), value);

    //set position Argument
    Argument endPos(end);
    //set discontinuity at position
    MappingUtils::addDiscontinuity(m, endPos, Argument::MappedZero, MappingUtils::pre(end), value);

    return m;
}

ConstMapping* BasePhyLayer::createSingleFrequencyMapping(simtime_t_cref             start,
                                                         simtime_t_cref             end,
                                                         Argument::mapped_type_cref centerFreq,
                                                         Argument::mapped_type_cref halfBandwidth,
                                                         Argument::mapped_type_cref value)
{
    Mapping* res = MappingUtils::createMapping(Argument::MappedZero, DimensionSet::timeFreqDomain, Mapping::LINEAR);

    Argument pos(DimensionSet::timeFreqDomain);

    pos.setArgValue(Dimension::frequency, centerFreq - halfBandwidth);
    pos.setTime(start);
    res->setValue(pos, value);

    pos.setTime(end);
    res->setValue(pos, value);

    pos.setArgValue(Dimension::frequency, centerFreq + halfBandwidth);
    res->setValue(pos, value);

    pos.setTime(start);
    res->setValue(pos, value);

    return res;
}
