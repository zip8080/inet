This directory contains files that provides abstractions for different physical layers.

TODO TODO TODO

COMMUNICATION IN DOWNWARD DIRECTION
===================================

Link Layer (MAC) to Physical Layer
----------------------------------

Connections (in nic):
 - linkLayer.lowerLayerOut --> physicalLayer.upperLayerIn

Messages: // TODO
 - cMessage(PHY_C_CONFIGURERADIO)

Packets:
 - ILinkFrame

Control Infos:
 - IPhysicalControlInfo

C++ Interfaces:
 - IPhysicalLayer

Callbacks:
 - none

Signals:
 - none

COMMUNICATION IN UPWARD DIRECTION
=================================

Physical Layer to Link Layer (MAC)
----------------------------------

Connections (in nic):
 - physicalLayer.upperLayerOut --> linkLayer.lowerLayerIn

Messages: // TODO
 - cMessage(COLLISION) (mixim)
 - cMessage(TX_OVER) (mixim)
 - cMessage(RADIO_SWITCHING_OVER) (mixim)

Packets:
 - ILinkFrame

Control Infos:
 - IPhysicalControlInfo

C++ Interfaces:
 - ILinkLayer

Callbacks: // TODO
 - INotifiable(NF_RADIOSTATE_CHANGED) (inet)
 - INotifiable(NF_RADIO_CHANNEL_CHANGED) (inet)
 - INotifiable(NF_L2_BEACON_LOST) (inet)
 - INotifiable(NF_LINK_FULL_PROMISCUOUS) (inet)

Signals: // TODO
 - radioState

COMMUNICATION BETWEEN PHYSICAL LAYERS
=====================================

Connections (in network):
 - physicalLayer.phys <--> physicalLayer.phys // TODO: name
 - physicalLayer --> physicalLayer.radioIn

Messages:
 - none

Packets:
 - IPhysicalFrame

Control Infos:
 - none

C++ Interfaces:
 - none

Callbacks:
 - none

Signals:
 - none
