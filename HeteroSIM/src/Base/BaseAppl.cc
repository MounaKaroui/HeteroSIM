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

#include "BaseAppl.h"
#include <inet/common/ModuleAccess.h>

Register_Abstract_Class(BaseAppl);

BaseAppl::BaseAppl()
{
}


void BaseAppl::initialize()
{

    toDecisionMaker =findGate("toDecisionMaker");
    fromDecisionMaker=findGate("toDecisionMaker");

    sendInterval=SimTime(par("sendInterval").doubleValue());
    startTime=SimTime(par("startTime").doubleValue());
    stopTime=SimTime(par("stopTime").doubleValue());

    msgLength=par("msgLength").intValue();
    appID=par("appID").intValue();
    setNodeId();

    sentPacket=registerSignal("sentPk");
    msgSentTrigger=new cMessage("Send trigger");

    if(startTime>=0)
        scheduleAt(startTime, msgSentTrigger);
    else
        throw cRuntimeError("Start time '%d' can not be negative.",SIMTIME_DBL(startTime));

}

void BaseAppl::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage()) {
        BasicMsg* basicMsg = BuildMsg("hetNets");
        send(basicMsg, toDecisionMaker);
        // to record sent data
        emit(sentPacket,basicMsg);
        if (stopTime >= simTime() || stopTime < 0){
            sendInterval= SimTime(par("sendInterval").doubleValue());
            scheduleAt(simTime() + sendInterval, msgSentTrigger);
        }
    }else
        handleAppMessage(msg);

}

void BaseAppl::setNodeId()
{
    cModule* host = inet::getContainingNode(this);
    std::string name=host->getFullName();
    nodeId=Utilities::extractNumber(name.c_str());
}


void BaseAppl::finish()
{
    cancelAndDelete(msgSentTrigger);
}



