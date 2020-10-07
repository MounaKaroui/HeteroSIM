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

#ifndef MODULES_CBRMEASUREMENT_CHANNELLOADCSMA_H_
#define MODULES_CBRMEASUREMENT_CHANNELLOADCSMA_H_

#include "inet/linklayer/csma/CSMA.h"
#include "util/ChannelLoadSampler.h"

class ChannelLoadCSMA : public  inet::CSMA{
public:
    ChannelLoadCSMA();
    virtual ~ChannelLoadCSMA();

    static const omnetpp::simsignal_t ChannelLoadSignal;
    simtime_t mChannelSensingInterval;
    simtime_t mChannelSensingDuration ;

protected:
    void initialize(int stage) override;
    void handleMessage(omnetpp::cMessage*) override;
    void senseMedium();

private:
    omnetpp::simtime_t mChannelReportInterval;
    omnetpp::cMessage* mChannelReportTrigger;
    omnetpp::cMessage* mChannelSenseTrigger;
    artery::ChannelLoadSampler mChannelLoadSampler;
};

#endif /* MODULES_CBRMEASUREMENT_CHANNELLOADCSMA_H_ */
