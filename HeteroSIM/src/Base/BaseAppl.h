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

#ifndef BASE_BASEAPPL_H_
#define BASE_BASEAPPL_H_

#include <omnetpp.h>
#include "Modules/messages/Messages_m.h"
#include "Utilities.h"


class BaseAppl: public cSimpleModule {


protected:

    int toDecisionMaker ;
    int fromDecisionMaker;
    simtime_t sendInterval;
    simtime_t startTime;
    simtime_t stopTime;
    int nodeId;
    int  msgLength;
    int appID;
    cMessage* msgSentTrigger;

    simsignal_t sentPacket;

protected:

    virtual void initialize();
    virtual void handleMessage(cMessage *msg);
    void finish();

    virtual BasicMsg* BuildMsg(std::string namePrefix)=0;
    virtual void handleAppMessage(cMessage *msg)=0;


    void setNodeId();

};

#endif /* BASE_BASEAPPL_H_ */
