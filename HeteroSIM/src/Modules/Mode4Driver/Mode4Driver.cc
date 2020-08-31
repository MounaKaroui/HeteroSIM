//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

#include "Mode4Driver.h"
#include "common/LteControlInfo.h"
#include "stack/phy/packet/cbr_m.h"

Define_Module(Mode4Driver);

void Mode4Driver::initialize(int stage)
{
    Mode4BaseApp::initialize(stage);
    if (stage==inet::INITSTAGE_LOCAL){
        // Register the node with the binder
        // Issue primarily is how do we set the link layer address
        // Get the binder
        binder_ = getBinder();
        // Get our UE
        cModule *ue = getParentModule();
        //Register with the binder
        nodeId_ = binder_->registerNode(ue, UE, 0);
        // Register the nodeId_ with the binder.
        binder_->setMacNodeId(nodeId_, nodeId_);
    } else if (stage==inet::INITSTAGE_APPLICATION_LAYER) {
        cbr_ = registerSignal("cbr");
        fromDecider=findGate("fromDecider");
        toDecider=findGate("toDecider");
    }
}

//void Mode4Driver::handleLowerMessage(cMessage* msg)
//{
//    if (msg->isName("CBR")) {
//        Cbr* cbrPkt = check_and_cast<Cbr*>(msg);
//        double channel_load = cbrPkt->getCbr();
//        emit(cbr_, channel_load);
//        delete cbrPkt;
//    } else {
//        AlertPacket* pkt = check_and_cast<AlertPacket*>(msg);
//        delete msg;
//    }
//}

void Mode4Driver::handleMessage(cMessage* msg)
{
    //
    int arrivalGateId=msg->getArrivalGateId();
    if (arrivalGateId==fromDecider){
        // Replace method
        //AlertPacket* packet = new AlertPacket("Alert");
        //packet->setTimestamp(simTime());
        //packet->setByteLength(size_);
        //packet->setSno(nextSno_);
        //nextSno_++;
        auto lteControlInfo = new FlowControlInfoNonIp();
        lteControlInfo->setSrcAddr(nodeId_);
        lteControlInfo->setDirection(D2D_MULTI);
        //lteControlInfo->setPriority(priority_);
        //lteControlInfo->setDuration(duration_);
        //lteControlInfo->setCreationTime(simTime());
        //msg->setControlInfo(lteControlInfo);
        //Mode4BaseApp::sendLowerPackets(msg);
        //emit(sentMsg_, (long)1);
    }
    else
    {
        send(msg,toDecider);

    }
}
void Mode4Driver::handleSelfMessage(cMessage* msg)
{

}
void Mode4Driver::handleLowerMessage(cMessage* msg)
{

}

void Mode4Driver::finish()
{

}

Mode4Driver::~Mode4Driver()
{
    binder_->unregisterNode(nodeId_);
}
