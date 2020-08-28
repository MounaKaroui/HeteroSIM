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

#include "../../Modules/messages/HeterogeneousMessage_m.h"


Define_Module(DecisionMaker);

void DecisionMaker::initialize()
{

    // find application gates
    fromApplication=findGate("fromApplication");
    toApplication=findGate("toApplication");

    // from Radio
    fromRadio=findGate("fromRadio");
    toRadio=findGate("toRadio");


}



void DecisionMaker::sendToRadio(cMessage*  msg)
{
    // TODO implement send to radio


}

void DecisionMaker::handleMessage(cMessage *msg)
{
    int arrivalGate = msg->getArrivalGateId();
    if (arrivalGate == fromApplication) {
        HeterogeneousMessage *heterogeneousMessage =dynamic_cast<HeterogeneousMessage *>(msg);
        if (!heterogeneousMessage) {
            std::cout<<"Message " << msg->getFullName() << " is not a HeterogeneousMessage, but a " << msg->getClassName();
            delete msg;
            return;
        }
        switch (heterogeneousMessage->getNetworkType())
        {

        case WLAN:
            //emit(DSRCMessagesSent, 1);
            sendToRadio(heterogeneousMessage);
            break;
        case LTE:
            // TODO LTE
            //emit(lteMessagesSent, 1);
            //sendLteMessage(heterogeneousMessage);
            break;

        case OTHER:
            //TODO
            //emit(dontCareMessagesSent, 1);
            //sendDontCareMessage(heterogeneousMessage);
            break;
        }
    } else {
        // handleLowerMessage(msg);
    }


}

void DecisionMaker::finish()
{


}
