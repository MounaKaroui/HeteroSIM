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
// @author Mouna KAROUI

#include "../../Modules/DecisionMaker/DecisionMaker.h"

#include <inet/common/InitStages.h>
#include <inet/common/ModuleAccess.h>
#include <inet/linklayer/ieee80211/mac/Ieee80211Mac.h>
#include <inet/physicallayer/ieee80211/packetlevel/Ieee80211Radio.h>
#include "stack/phy/packet/cbr_m.h"

Define_Module(DecisionMaker);


void DecisionMaker::initialize()
{

    // G5 stats
    G5MessagesSent=registerSignal("G5MessagesSent");
    G5MessagesReceived=registerSignal("G5MessagesReceived");

    // stats mode4
    mode4MsgSent=registerSignal("mode4MsgSent");
    mode4MsgReceived=registerSignal("mode4MsgReceived");

    toMode4=findGate("toMode4");
    fromMode4=findGate("fromMode4");

    mode4=par("mode4IsActive").boolValue();


}

void DecisionMaker::registerNodeToBinder()
{
    binder_ = getBinder();
    // Get our UE
    cModule *ue = getContainingNode(this);
    //Register with the binder
    nodeId_ = binder_->registerNode(ue, UE, 0);
    // Register the nodeId_ with the binder.
    binder_->setMacNodeId(nodeId_, nodeId_);
}

Ieee802Ctrl* DecisionMaker::buildCtrlInfo()
{

    cModule* host = inet::getContainingNode(this);
    std::string name=host->getFullName();
    auto controlInfo = new Ieee802Ctrl();
    inet::MACAddress srcAdr;
    char* c = const_cast<char*>(name.c_str());
    srcAdr.setAddressBytes(c);
    controlInfo->setSourceAddress(srcAdr);

    inet::MACAddress destAdress;
    destAdress.setBroadcast();
    controlInfo->setDest(destAdress);
    return controlInfo;
}



void DecisionMaker::sendToMode4(cPacket* packet)
{


     packet->setByteLength(10); // TODO to override
     auto lteControlInfo = new FlowControlInfoNonIp();
     lteControlInfo->setSrcAddr(nodeId_);
     lteControlInfo->setDirection(D2D_MULTI);
     lteControlInfo->setCreationTime(simTime());
     //lteControlInfo->setPriority(priority_); // optional
     //lteControlInfo->setDuration(duration_);
     packet->setControlInfo(lteControlInfo);

}

void DecisionMaker::sendToApp(cMessage*  msg)
{

    for(int i=0; i<gateSize("toApplication");i++)
    {
    int gateId=gate("toApplication",i)->getId();
    send(msg,gateId);
    }
}

void DecisionMaker::filterMsg(HeterogeneousMessage *msg)
{
    if (!msg) {
               std::cout<<"Message " << msg->getFullName() << " is not a HeterogeneousMessage, but a " << msg->getClassName();
               delete msg;
               return;
           }
}

void DecisionMaker:: sendToWlanRadio(cMessage*  msg, int networkIndex)
{

    msg->setControlInfo(buildCtrlInfo());

    if(networkIndex<gateSize("toRadio"))
    {
        int gateId=gate("toRadio",networkIndex)->getId();
        send(msg,gateId);
    }
    else
    {
        delete msg;
        EV_INFO<<"interface index doesn't exist" <<endl;
    }
}

void DecisionMaker::sendMsg(int networkType, cMessage* msg)
{

    switch (networkType)
            {
            case VANET:
                // TODO emit
                emit(G5MessagesSent, 1);
                sendToWlanRadio(msg,VANET); /// wlan[0] ---> Vanet radio
                break;
            case WSN:
                emit(WSNMessagesSent, 1);
                sendToWlanRadio(msg,WSN); /// wlan[1] ---> WSN radio
                std::cout<<"Message sent with sucess" ;
                break;
            case MODE4:
                // TODO LTE
                sendToMode4(check_and_cast<cPacket *>(msg));
                emit(mode4MsgSent, 1);
                break;

            }
}





void DecisionMaker::handleMessage(cMessage *msg)
{


          int arrivalGate = msg->getArrivalGateId();
          for(int i=0; i<gateSize("fromApplication");i++)
          {
          int gateId=gate("fromApplication",i)->getId();
          if (arrivalGate == gateId)
          {
              HeterogeneousMessage *heterogeneousMessage =dynamic_cast<HeterogeneousMessage *>(msg);
              filterMsg(heterogeneousMessage);
              int networkType=heterogeneousMessage->getNetworkType();
              sendMsg(networkType,heterogeneousMessage);
          }else
          {
                  handleLowerMessages(msg);
          }
          }



}




void  DecisionMaker::handleLowerMessages(cMessage* msg)
{

    int arrivalGateId=msg->getArrivalGateId();
    int gateId;
    int network;
    if(msg->isName("hetNets"))
    {
    HeterogeneousMessage* hetMsg=check_and_cast<HeterogeneousMessage*>(msg);
    network=hetMsg->getNetworkType();
    gateId=gate("fromRadio",network)->getId(); // TODO --> Find the index of WLAN
    }
    else
    {
        EV_INFO<< "This is not my message";
    }

    if(arrivalGateId==gateId) // test for Wlans
    {
            sendToApp(msg);
            if(network==VANET)
            {
                emit(G5MessagesReceived,1);
            }else if(network==WSN)
            {
                emit(WSNMessagesReceived, 1);
            }
            else
            {
                EV_INFO<< "Unknown interface";
            }
     }
     else if(arrivalGateId==fromMode4)
            {
            emit(mode4MsgReceived,1);
//            if (msg->isName("CBR")) {
//                 Cbr* cbrPkt = check_and_cast<Cbr*>(msg);
//                 //double channel_load = cbrPkt->getCbr();
//                 //emit(cbr_, channel_load);
//                 delete cbrPkt;
//            } else {
                sendToApp(msg);
           // }


    }else
    {
        cRuntimeError("Unknown gate");
    }
}



DecisionMaker::~DecisionMaker()
{
    if(mode4)
    {
        binder_->unregisterNode(nodeId_);
    }

}
void DecisionMaker::finish()
{


}
