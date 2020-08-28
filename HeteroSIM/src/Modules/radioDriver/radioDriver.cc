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

#include "../../Modules/radioDriver/radioDriver.h"

#include <inet/common/InitStages.h>
#include <inet/common/ModuleAccess.h>
#include <inet/linklayer/ieee80211/mac/Ieee80211Mac.h>
#include <inet/physicallayer/ieee80211/packetlevel/Ieee80211Radio.h>

Define_Module(RadioDriver);

using namespace omnetpp;

int RadioDriver::numInitStages() const
{
    return inet::InitStages::NUM_INIT_STAGES;
}
void RadioDriver::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL) {
            RadioDriverBase::initialize();
            cModule* host = inet::getContainingNode(this);
            mLinkLayer = inet::findModuleFromPar<inet::ieee80211::Ieee80211Mac>(par("macModule"), host);
            //mLinkLayer->subscribe(channelLoadSignal, this);
            mRadio = inet::findModuleFromPar<inet::ieee80211::Ieee80211Radio>(par("radioModule"), host);
            //mRadio->subscribe(radioChannelChangedSignal, this);
        } else if (stage == inet::InitStages::INITSTAGE_LINK_LAYER_2) {


        }
}


void RadioDriver::receiveSignal(cComponent* source, simsignal_t signal, double value, cObject*)
{
   // if (signal == channelLoadSignal) {
   //     emit(RadioDriverBase::ChannelLoadSignal, value);
   // }
}

void RadioDriver::handleDataIndication(cMessage* packet)
{

    indicateData(packet);
}
void RadioDriver::handleMessage(cMessage *msg)
{
    if (msg->getArrivalGate() == gate("lowerLayerIn")) {
           // receive indication
            handleDataIndication(msg);
        } else {
            RadioDriverBase::handleMessage(msg);
        }
}

void RadioDriver::handleDataRequest(cMessage * packet)
{

     send(packet,"lowerLayerOut");
}

