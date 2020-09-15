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

#ifndef __HETEROSIM_DECISIONMAKER_H_
#define __HETEROSIM_DECISIONMAKER_H_

#include <omnetpp.h>
#include "../../Modules/messages/HeterogeneousMessage_m.h"
#include "corenetwork/binder/LteBinder.h"
#include "Base/Builder.h"
#include "Base/mcda/MCDM.h"
using namespace omnetpp;
using namespace inet;

/**
 * TODO - Generated class
 */

// forward declaration
namespace inet {
namespace ieee80211 { class Ieee80211Mac; }
namespace physicallayer { class Ieee80211Radio; }
} // namespace inet


class DecisionMaker : public cSimpleModule, public cListener
{


public:

    ~DecisionMaker();

    int takeDecision(cMessage*  msg);
    void ctrlInfoWithRespectToNetType(cMessage* msg, int networkIndex);


    std::map<std::string,cMessage*> packetQueue;

  protected:
    //int numInitStages() const override;
    void initialize() override;

    virtual void handleMessage(cMessage *msg)  override;
    void sendToLower(cMessage* msg,int networkIndex);
    void sendToUpper(cMessage*  msg);

    void registerNodeToBinder();
    void handleLowerMsg(cMessage* msg);

    void storeUpperPackets();

    bool mode4;
    LteBinder* binder_;
    MacNodeId nodeId_;
    int critNumb;


  private:
    cMessage* selfMsg;


};

#endif
