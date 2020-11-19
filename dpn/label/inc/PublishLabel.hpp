#pragma once


#include <Label.hpp>


class PublishLabel : public Label
{
public:

    PublishLabel(InterfaceID topic):Label(){}
    PublishLabel(InterfaceID topic, PortID portID):Label()
    {
        SetPortID(portID, Endpoint::Self);
    }


}