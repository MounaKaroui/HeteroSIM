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
#include <inet/common/ModuleAccess.h>
#include "stack/phy/packet/cbr_m.h"

Define_Module(DecisionMaker);


void DecisionMaker::initialize()
{

    // TODO: createStat method
    // G5 stats
    G5MessagesSent=registerSignal("G5MessagesSent");
    G5MessagesReceived=registerSignal("G5MessagesReceived");
    // stats mode4
    mode4MsgSent=registerSignal("mode4MsgSent");
    mode4MsgReceived=registerSignal("mode4MsgReceived");

    toMode4=findGate("toMode4");
    fromMode4=findGate("fromMode4");

    mode4=par("mode4IsActive").boolValue();

    if(mode4)
    {
        registerNodeToBinder();
    }
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






void DecisionMaker::takeDecision()
{
        // TODO: call mcdm prodecures here

}

// TODO: merge sendToMode4 and sendToRadio
// --> sendToLower() if possible

void DecisionMaker::sendToMode4(cPacket* packet)
{

     packet->setControlInfo(Builder::LteCtrlInfo(nodeId_));

     //FixME bug about range of mode4 physical layer
     //send(packet, toMode4);
}


void DecisionMaker:: sendToWlanRadio(cMessage*  msg, int networkIndex)
{

    cModule* host = inet::getContainingNode(this);
    std::string name=host->getFullName();
    // Control info is mandatory to pass message
    //to Mgmt layer and then to MAC
    msg->setControlInfo(Builder::Ieee802CtrlInfo(name));
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



void DecisionMaker::sendToUpper(cMessage*  msg)
{

    for(int i=0; i<gateSize("toApplication");i++)
    {
    int gateId=gate("toApplication",i)->getId();
    send(msg,gateId);
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
                // TODO LTE check_and_cast<cPacket *>(msg):
                //cPacket* because lower layers can't cast  msg type
                //cPacket* test=new cPacket();
                sendToMode4(check_and_cast<cPacket *>(msg)); // FixME: lte physical layer crash
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
              HeterogeneousMessage *heterogeneousMessage =dynamic_cast<HeterogeneousMessage*>(msg);
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

            if(network==VANET)
            {
                emit(G5MessagesReceived,1);
            }else if(network==WSN)
            {
                emit(WSNMessagesReceived, 1);
            }
            else if(network==MODE4)
            {
                emit(mode4MsgReceived,1);
            }
     }else
     {
         EV_INFO<<"Unknown gate" ;
     }

}



DecisionMaker::~DecisionMaker()
{
    if(mode4)
    {
        binder_->unregisterNode(nodeId_);
    }

}

