/*
 * Builder.cpp
 *
 *  Created on: Aug 31, 2020
 *      Author: Mouna KAROUI
 */

#include "Builder.h"
using namespace inet;
namespace Builder {

Ieee802Ctrl* Ieee802CtrlInfo(std::string ModuleName)
{
    auto controlInfo = new Ieee802Ctrl();
    inet::MACAddress srcAdr;
    char* c = const_cast<char*>(ModuleName.c_str());
    srcAdr.setAddressBytes(c);
    controlInfo->setSourceAddress(srcAdr);

    inet::MACAddress destAdress;
    destAdress.setBroadcast();
    controlInfo->setDest(destAdress);
    return controlInfo;
}

FlowControlInfoNonIp* LteCtrlInfo(MacNodeId nodeId_)
{
    auto lteControlInfo = new FlowControlInfoNonIp();
    lteControlInfo->setSrcAddr(nodeId_);
    lteControlInfo->setDirection(D2D_MULTI);
    lteControlInfo->setCreationTime(simTime());
    return lteControlInfo;
}


} /* namespace Builder */
