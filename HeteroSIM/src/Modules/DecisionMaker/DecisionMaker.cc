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
#include "Modules/Stats/CollectStats.h"
Define_Module(DecisionMaker);


void DecisionMaker::initialize()
{

    critNumb=par("criteriaNum").intValue();
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


void DecisionMaker::ctrlInfoWithRespectToNetType(cMessage* msg, int networkType)
{


    if(networkType==MODE4)
    {
        //cPacket* packet=check_and_cast<cPacket *>(msg);
        msg->setControlInfo(Builder::LteCtrlInfo(nodeId_));

    }else
    {
        // WLAN
        cModule* host = inet::getContainingNode(this);
        std::string name=host->getFullName();
        // Control info is mandatory to pass message
        //to Mgmt layer and then to MAC
        msg->setControlInfo(Builder::Ieee802CtrlInfo(name));
    }

}
void DecisionMaker:: sendToLower(cMessage*  msg, int networkIndex)
{

    ctrlInfoWithRespectToNetType(msg, networkIndex);
    if(networkIndex<gateSize("toRadio"))
    {
        int gateId=gate("toRadio",networkIndex)->getId();
//        if(mode4)
//        {
//            cPacket* packet=check_and_cast<cPacket*>(msg);
//            send(packet, gateId);
//        }
//        else
//        {
            send(msg, gateId);
        //}


    }
    else
    {
        delete msg;
        EV_INFO<<"interface index doesn't exist" <<endl;
    }
}



void DecisionMaker::sendToUpper(cMessage*  msg)
{
    int n=gateSize("toApplication");
    for(int i=0; i<n;i++)
    {
        //int gateId=gate("toApplication",i)->getId(); // To get Id
        //cMessage *copy = msg->dup(); //  The problem is that this is a copy not the real msg
        send(msg, "toApplication", i);
    }

}

void DecisionMaker::storeUpperPackets()
{
    // enqueue packet from applications
    //HeterogeneousMessage* hetNetsMsg=dynamic_cast<HeterogeneousMessage*>(msg);
    //std::string appName=hetNetsMsg->getAppName();
    ////// Tests
    //packetQueue.insert(std::pair<std::string, cMessage*>(appName, msg));
    //int n=packetQueue.size();
    //std::cout << "test => " <<  packetQueue.find("Interactive")->second << '\n';
    //cMessage* test=packetQueue.find("Interactive")->second;

    //HeterogeneousMessage* testMsg=dynamic_cast<HeterogeneousMessage*>(test);
    //std::string appNam=testMsg->getAppName();
    //std::cout << "test => " <<appNam<<'\n';

}





int DecisionMaker::takeDecision(cMessage* msg)
{

    int networkIndex=2; // Just to test LTE

    // MCDM_procedure
    HeterogeneousMessage* hetMsg=dynamic_cast<HeterogeneousMessage*>(msg);
    std::string trafficType=hetMsg->getTrafficType();
    cModule* mStats=getParentModule()->getSubmodule("statistics");
    CollectStats* stats=dynamic_cast<CollectStats*>(mStats);
    //networkIndex=McdaAlg::decisionProcess(stats->allPathsCriteriaValues, critNumb, trafficType, "GRA");


    return networkIndex;
}


void DecisionMaker::handleMessage(cMessage *msg)
{

    std::map<std::string,cMessage*>::iterator it;
    int arrivalGate = msg->getArrivalGateId();

    for(int i=0; i<gateSize("fromApplication");i++)
    {
        int gateId=gate("fromApplication",i)->getId();

        if (arrivalGate == gateId)
        {
            int networkIndex=takeDecision(msg);
            sendToLower(msg, networkIndex);

        }else
        {
            sendToUpper(msg);
        }

    }
}


DecisionMaker::~DecisionMaker()
{
    if(mode4)
    {
        binder_->unregisterNode(nodeId_);
    }

}

void DecisionMaker::handleLowerMsg(cMessage* msg)
{
    if(mode4)
    {
        if (msg->isName("CBR")) {
            Cbr* cbrPkt = check_and_cast<Cbr*>(msg);
            //double channel_load = cbrPkt->getCbr();
            //emit(cbr_, channel_load);
            delete cbrPkt;
        }
    }

}


