This directory contains files that provides abstractions for different network layers.

TODO TODO TODO

Transport Layer to Network Layer
--------------------------------

Connections (in node):
 - transportLayer.lowerLayerOut --> networkLayer.transportIn
 - pingIn

Messages:
 - RegisterTransportProtocol

Packets:
 - ITransportPacket

Control Infos:
 - INetworkProtocolControlInfo

C++ Interfaces:
 - INetworkLayer

Callbacks:
 - none

Signals:
 - none

Network Layer to Transport Layer
--------------------------------

Connections:
 - transportOut
 - pingOut

Messages:
 - none

Packets:
 - ITransportPacket

Control Infos:
 - INetworkProtocolControlInfo

C++ Interfaces:
 - none

Callbacks:
 - none

Signals:
 - none
