This directory contains files that provides abstractions for different link layers.

TODO TODO TODO

Network Layer to Link Layer (MAC)
---------------------------------

Connections (in node):
 - networkLayer.lowerLayerOut --> linkLayer.upperLayerIn

Messages:
 - none

Packets:
 - INetworkDatagram

Control Infos:
 - ILinkLayerControlInfo

C++ Interfacces:
 - none

Callbacks:
 - none

Signals:
 - none

Link Layer (MAC) to Network Layer
---------------------------------

Connections:
 - linkLayer.upperLayerOut --> networkLayer.lowerLayerIn

Messages:
 - none

Packets:
 - INetworkDatagram

Control Infos:
 - ILinkLayerControlInfo

C++ Interfaces:
 - none

Callbacks:
 - none

Signals:
 - none
