/**
 * Records a pcap trace. See NED file for more information.
 *
 * @author Andras Varga, Zoltan Bojthe
 */

#include <fstream>
#include <omnetpp.h>

#include "IPDatagram.h"
#include "IPSerializer.h"
#include "InetPcapFile.h"

class PcapTrace : public cSimpleModule, protected cListener
{
  protected:
    static simsignal_t messageSentSignal;
    InetPcapFileWriter f;
  protected:
    virtual void initialize();
    virtual void handleMessage(cMessage *msg);
    virtual void finish();
    virtual void dump();
    virtual void dumpPacket(cChannel::MessageSentSignalValue *msgSentSignal);
    virtual void receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj);
    virtual bool isRelevantModule(cModule *mod);
    virtual void addNode(cModule *mod);
    virtual void addLink(cGate *gate);
};

simsignal_t PcapTrace::messageSentSignal;

Define_Module(PcapTrace);

void PcapTrace::initialize()
{
    if (!par("enabled").boolValue())
        return;

    const char *filename = par("filename");
    int snaplen = par("snaplen");
    f.open(filename, snaplen);
    if (!f.isOpen())
        throw cRuntimeError("Cannot open file \"%s\" for writing", filename);

    dump();

    const char *moduleName = par("moduleName");
    cModule *mod = simulation.getModuleByPath(moduleName);
    if(!mod)
        throw cRuntimeError("Cannot find module \"%s\" for capturing", moduleName);

    const char* gateName = par("gateName");
    int gateIndex = par("gateIndex").longValue();
    cGate* gate = mod->gate(gateName, gateIndex);
    cChannel* chan = gate->getChannel();
    if(!chan)
        throw cRuntimeError("Cannot find channel \"%s:%d\" on module \"%s\" for capturing", gateName, gateIndex, moduleName);

    messageSentSignal = registerSignal("messageSent");
    chan->subscribe(POST_MODEL_CHANGE, this);
    chan->subscribe(messageSentSignal, this);
}

void PcapTrace::handleMessage(cMessage *msg)
{
    throw cRuntimeError("This module does not handle messages");
}

void PcapTrace::finish()
{
    f.close();
}

void PcapTrace::dump()
{
/*
    cModule *parent = simulation.getSystemModule();
    for (cModule::SubmoduleIterator it(parent); !it.end(); it++)
        if (it() != this)
            addNode(it());
    for (cModule::SubmoduleIterator it(parent); !it.end(); it++)
        if (it() != this)
            for (cModule::GateIterator ig(it()); !ig.end(); ig++)
                if (ig()->getType()==cGate::OUTPUT && ig()->getNextGate())
                    addLink(ig());
*/
}

void PcapTrace::dumpPacket(cChannel::MessageSentSignalValue *msgSentSignal)
{
    cObject* msg = msgSentSignal->getMessage();
    cPacket *packet = dynamic_cast<cPacket *>(msg);
    while(packet)
    {
        IPDatagram *ipPacket = dynamic_cast<IPDatagram *>(packet);
        if (ipPacket)
        {
            f.write(ipPacket);
            return;
        }
        packet = packet->getEncapsulatedPacket();
    }
}

void PcapTrace::receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj)
{
    if (signalID == messageSentSignal && source->isChannel())
    {
        // record a "packet sent" line
        cChannel *channel = (cChannel *)source;
        cModule *srcModule = channel->getSourceGate()->getOwnerModule();
        if (isRelevantModule(srcModule))
        {
            cModule *destModule = channel->getSourceGate()->getNextGate()->getOwnerModule();
            cChannel::MessageSentSignalValue *v = check_and_cast<cChannel::MessageSentSignalValue *>(obj);
            dumpPacket(v);
/*
            cChannel::result_t *result = v->getChannelResult();
            simtime_t fbTx = v->getTimestamp(signalID);
            simtime_t lbTx = fbTx + result->duration;
            simtime_t fbRx = fbTx + result->delay;
            simtime_t lbRx = lbTx + result->delay;
            f << fbTx << " P " << srcModule->getId() << " " << destModule->getId() << " " << lbTx << " " << fbRx << " " << lbRx << "\n";
*/
        }
    }
    else if (signalID == POST_MODEL_CHANGE)
    {
        // record dynamic "node created" and "link created" lines.
        // note: at the time of writing, PcapTrace did not support "link removed" and "node removed" lines
        if (dynamic_cast<cPostModuleAddNotification *>(obj))
        {
            cPostModuleAddNotification *notification = (cPostModuleAddNotification *)obj;
            if (isRelevantModule(notification->module))
                addNode(notification->module);
        }
        else if (dynamic_cast<cPostGateConnectNotification *>(obj))
        {
            cPostGateConnectNotification *notification = (cPostGateConnectNotification *)obj;
            if (isRelevantModule(notification->gate->getOwnerModule()))
                addLink(notification->gate);
        }
    }
}

bool PcapTrace::isRelevantModule(cModule *mod)
{
    return true;
//    return mod->getParentModule() == simulation.getSystemModule();
}

void PcapTrace::addNode(cModule *mod)
{
//    double x, y;
//    resolveNodeCoordinates(mod, x, y);
//    f << simTime() << " N " << mod->getId() << " " << x << " " << y << "\n";
}

void PcapTrace::addLink(cGate *gate)
{
//    f << simTime() << " L " << gate->getOwnerModule()->getId() << " " << gate->getNextGate()->getOwnerModule()->getId() << "\n";
}
