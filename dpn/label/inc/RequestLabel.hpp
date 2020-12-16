#pragma once


#include <Label.hpp>


class RequestLabel : public Label
{
public:

    void WriteRequest(PeerID remotePeerID, InterfaceID remoteInterfaceID, PortID remotePortID = PEER_PORT_ID_DEFAULT, PortID localPortID = PEER_PORT_ID_DEFAULT)
    {
        reset();
        contents_.dest_.interfaceID_ = remoteInterfaceID;
        contents_.dest_.peerID_ = remotePeerID;
        contents_.dest_.portID_ = remotePortID;
        contents_.src_.portID_ = localPortID;
    }


}