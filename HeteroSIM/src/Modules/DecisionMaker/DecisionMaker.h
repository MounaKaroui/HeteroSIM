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
#include <inet/common/ModuleAccess.h>
#include "inet/linklayer/common/Ieee802Ctrl.h"
#include <inet/common/InitStages.h>
#include <inet/common/ModuleAccess.h>
#include "common/LteControlInfo.h"
#include "corenetwork/binder/LteBinder.h"

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

    void filterMsg(HeterogeneousMessage *heterogeneousMessage);
  protected:
    //int numInitStages() const override;
    void initialize() override;

    virtual void handleMessage(cMessage *msg)  override;
    void sendToWlanRadio(cMessage* msg,int networkIndex);
    void sendToApp(cMessage*  msg);

    void sendToMode4(cPacket* packet);
    void sendMsg(int networkType, cMessage* msg);
    void finish();

    void registerNodeToBinder();
    void handleLowerMessages(cMessage* msg);
    Ieee802Ctrl* buildCtrlInfo();
    // statistics
    simsignal_t G5MessagesSent;
    simsignal_t G5MessagesReceived;

    simsignal_t mode4MsgSent;
    simsignal_t mode4MsgReceived;

    simsignal_t WSNMessagesSent;
    simsignal_t WSNMessagesReceived;

    bool mode4;
    int toMode4;
    int fromMode4;

    int toVanetRadio;
    int fromVanetRadio;

    int to80215;
    int from80215;

    LteBinder* binder_;
    MacNodeId nodeId_;


  private:
    cMessage* selfMsg;


};

#endif
