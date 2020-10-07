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

#include "ChannelLoadCSMA.h"
#include <cmath>

const simsignal_t ChannelLoadCSMA::ChannelLoadSignal = cComponent::registerSignal("ChannelLoad");

Define_Module(ChannelLoadCSMA)

ChannelLoadCSMA::ChannelLoadCSMA() {
}

ChannelLoadCSMA::~ChannelLoadCSMA() {
    cancelAndDelete(mChannelReportTrigger);
}

void ChannelLoadCSMA::initialize(int stage)
{
    CSMA::initialize(stage);
    if (stage == inet::INITSTAGE_LOCAL) {

        mChannelReportInterval = SimTime(par("channelReportInterval").doubleValue());
        mChannelSensingInterval = SimTime(par("channelSensingInterval").doubleValue());
        mChannelSensingDuration = SimTime(par("channelSensingDuration").doubleValue());

        mChannelReportTrigger = new cMessage("report CL");
        mChannelSenseTrigger = new cMessage("sense channel");

        if (par("asyncChannelReport").boolValue()) {
            scheduleAt(simTime() + mChannelReportInterval, mChannelReportTrigger);
        } else {
            double cycle = simTime() / mChannelReportInterval;
            scheduleAt((1.0 + std::ceil(cycle)) * mChannelReportInterval, mChannelReportTrigger);
        }
        omnetpp::createWatch("channelLoadSampler", mChannelLoadSampler);

        scheduleAt(simTime() + mChannelSensingInterval, mChannelSenseTrigger);
    }
}

void ChannelLoadCSMA::handleMessage(cMessage* msg)
{
    if (msg == mChannelReportTrigger) {
        emit(ChannelLoadSignal, mChannelLoadSampler.cbr());
        scheduleAt(simTime() + mChannelReportInterval, mChannelReportTrigger);
        return ;
    }

    if (msg == mChannelSenseTrigger) {
        senseMedium();
        scheduleAt(simTime() + mChannelSensingInterval, mChannelSenseTrigger);
        return ;
    }

    CSMA::handleMessage(msg);
}

void ChannelLoadCSMA::senseMedium()
{
    simtime_t now = simTime();
    bool channelState = radio->getReceptionState(now-mChannelSensingDuration,now) == inet::physicallayer::IRadio::RECEPTION_STATE_IDLE;
    mChannelLoadSampler.busy(channelState);
}
