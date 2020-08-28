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

#ifndef __HETEROSIM_RADIODRIVER_H_
#define __HETEROSIM_RADIODRIVER_H_

#include <omnetpp.h>
#include <omnetpp/clistener.h>

#include "../../Modules/radioDriverBase/RadioDriverBase.h"


// forward declaration
namespace inet {
namespace ieee80211 { class Ieee80211Mac; }
namespace physicallayer { class Ieee80211Radio; }
} // namespace inet


/**
 * TODO - Generated class
 */

class RadioDriver : public RadioDriverBase, public omnetpp::cListener
{

public:
    int numInitStages() const override;
    void initialize(int stage) override;
    void handleMessage(omnetpp::cMessage*) override;

protected:
    void receiveSignal(omnetpp::cComponent*, omnetpp::simsignal_t, double, omnetpp::cObject*) override;
    void handleDataRequest(omnetpp::cMessage*) override;
    void handleDataIndication(omnetpp::cMessage*);

private:
       inet::ieee80211::Ieee80211Mac* mLinkLayer = nullptr;
       inet::physicallayer::Ieee80211Radio* mRadio = nullptr;


};

#endif
