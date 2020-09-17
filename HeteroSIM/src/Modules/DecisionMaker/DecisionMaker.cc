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
    pathToConfigFiles=par("pathToConfigFiles").stringValue();
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
        send(msg, gateId);
    }
    else
    {
        delete msg;
        EV_INFO<<"interface index doesn't exist" <<endl;
    }
}



void DecisionMaker::sendToUpper(cMessage*  msg)
{
    HeterogeneousMessage* hetNetsMsg=dynamic_cast<HeterogeneousMessage*>(msg);
    int id=hetNetsMsg->getApplId();
    int gateId=gate("toApplication",id)->getId(); // To get Id
    removePacketsFromQueue(id);
    send(msg, gateId);
}

void DecisionMaker::pushPacketstoQueue(int id, HeterogeneousMessage* msg)
{
    // enqueue packet from applications
    packetQueue.insert(std::pair<int, HeterogeneousMessage*>(id, msg));

    // test
    //int n=packetQueue.size();
    //HeterogeneousMessage* test=packetQueue.find(msg->getApplId())->second;
    //std::string trafficType=test->getTrafficType();
    //std::cout << "test => " <<trafficType<<'\n';
}

void DecisionMaker::removePacketsFromQueue(int id)
{
    // Delete msg given a key
    packetQueue.erase(id);
}





int DecisionMaker::takeDecision(cMessage* msg)
{
    int networkIndex=2;
    // MCDM_procedure
    HeterogeneousMessage* hetMsg=dynamic_cast<HeterogeneousMessage*>(msg);
    std::string trafficType=hetMsg->getTrafficType();
    cModule* mStats=getParentModule()->getSubmodule("statistics");
    CollectStats* stats=dynamic_cast<CollectStats*>(mStats);
    std::cout<< stats->allPathsCriteriaValues<< endl;

//    if(stats->allPathsCriteriaValues!="")
//    {
//    networkIndex=McdaAlg::decisionProcess(stats->allPathsCriteriaValues, pathToConfigFiles,critNumb, trafficType, "VIKOR");
//    }
//    else
//    {
//    networkIndex=0; // random
//    }
    return networkIndex;
}


void DecisionMaker::handleMessage(cMessage *msg)
{

    HeterogeneousMessage* hetMsg=dynamic_cast<HeterogeneousMessage*>(msg);
    int id;


    if(hetMsg!=nullptr)
    {
    id=hetMsg->getApplId();
    pushPacketstoQueue(id, hetMsg);  // store Upper packets

    int arrivalGate = msg->getArrivalGateId();
    int gateId=gate("fromApplication",id)->getId();

    if (arrivalGate == gateId)
    {
        int networkIndex=takeDecision(msg);
        sendToLower(msg, networkIndex);

    }else
    {
        // from  radio
        sendToUpper(msg);

    }
    }

    handleLteLowerMsg(msg);

}


DecisionMaker::~DecisionMaker()
{
    if(mode4)
    {
        binder_->unregisterNode(nodeId_);
    }

}

void DecisionMaker::handleLteLowerMsg(cMessage* msg)
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


