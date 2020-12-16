#pragma once


#include <Label.hpp>


class PublishLabel : public Label
{
public:


    void Write(InterfaceID localInterfaceID, PortID localPortID = PEER_PORT_ID_DEFAULT)
    {
        reset();
        GetSrc().interfaceID_ = localInterfaceID;
        GetSrc().portID_ = localPortID;
    }

}