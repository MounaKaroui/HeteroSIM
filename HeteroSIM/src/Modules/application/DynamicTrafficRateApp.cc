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

#include "DynamicTrafficRateApp.h"

Define_Module(DynamicTrafficRateApp);

void DynamicTrafficRateApp::initialize()
{
    BaseAppl::initialize();
    interfaceId=par("interfaceId").intValue();
    msgOfferedLoadRateSentTrigger=new cMessage("Offered Load trigger");
    pcktToSendSignal=registerSignal("pcktToSendSignal");
    setOfferedLoadRefreshInterval();
}

DynamicTrafficRateApp::~DynamicTrafficRateApp() {
    //cancelAndDelete(msgSentTrigger);
    cancelAndDelete(msgOfferedLoadRateSentTrigger);
}

void DynamicTrafficRateApp::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage()) {

        if(msg==msgSentTrigger){

            offeredLoadRateSentInterval= SimTime(par("offeredLoadRate").doubleValue());
            pcktToSendForOfferedLoadRate = SimTime(1) / offeredLoadRateSentInterval;
            emit(pcktToSendSignal,pcktToSendForOfferedLoadRate);

            if(pcktToSendForOfferedLoadRate > 0)
                scheduleAt(simTime() + offeredLoadRateSentInterval, msgOfferedLoadRateSentTrigger);
            else
                scheduleAt(simTime() + offeredLoadRefreshInterval, msgSentTrigger); //schedule next offered load rate generation time

        }else if (msg==msgOfferedLoadRateSentTrigger){

            sendData();
            pcktToSendForOfferedLoadRate--;

            if(pcktToSendForOfferedLoadRate > 0 )
                scheduleAt(simTime() + offeredLoadRateSentInterval, msgOfferedLoadRateSentTrigger);
            else
                scheduleAt(simTime() + offeredLoadRefreshInterval, msgSentTrigger); //schedule next offered load rate generation time
        }

    }else
        handleAppMessage(msg);

}


void DynamicTrafficRateApp::setOfferedLoadRefreshInterval(){

    double parOfferedLoadRateSentInterval = par("offeredLoadRefreshInterval").doubleValue() ;
    offeredLoadRefreshInterval = SimTime(parOfferedLoadRateSentInterval);

    //checking valid input of offeredLoadRefreshInterval parameter
    string  s = getEnvir()->getConfig()->getConfigValue("sim-time-limit");
    if(parOfferedLoadRateSentInterval >= std::stod(s.substr(0,s.length()-1)) )
        throw cRuntimeError("'offeredLoadRefreshInterval' parameter can not be greater than 'sim-time-limit' parameter") ;
}
void DynamicTrafficRateApp::sendData(){
    BasicMsg* basicMsg = BuildMsg("hetNets");
    send(basicMsg, toDecisionMaker);
    emit(BaseAppl::sentPacket,basicMsg);  // to record sent data
}

void DynamicTrafficRateApp::handleAppMessage(cMessage *msg)
{
    emit(BaseAppl::rcvdPacket, msg);
}

BasicMsg* DynamicTrafficRateApp::BuildMsg(std::string namePrefix)
{
    ControlMsg*  ctrlMsg=new ControlMsg();
    ctrlMsg->setName((namePrefix+string("-dynamicTrafficRate-")+std::to_string(ctrlMsg->getTreeId())).c_str());
    ctrlMsg->setNetworkId(interfaceId);
    ctrlMsg->setByteLength(msgLength);
    ctrlMsg->setApplId(appID);
    return  ctrlMsg;
}

