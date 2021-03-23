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

#include "corenetwork/binder/LteBinder.h"
#include <iostream>


#include "../../base/mcda/MCDM.h"
#include "../../base/Utilities.h"
#include "../../modules/messages/Messages_m.h"
#include "../stats/CollectStats.h"
#include "modules/util/contract/IAddressResolver.h"

#include <random>

using namespace omnetpp;
using namespace inet;
using namespace std;
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

    static simsignal_t decisionSignal ;

  protected:

    void initialize() override;
    virtual void handleMessage(cMessage *msg)  override;
    void sendToLower(cMessage* msg,int networkIndex);
    void sendToUpper(cMessage*  msg);

    string getNetworkProtocolName(int networkIndex);
    std::string convertListOfCriteriaToString(CollectStats::listAlternativeAttributes* listOfAlternativeAttributes);

    // Ping-pong methods
    int reducePingPongEffects(int newDecision, CollectStats::listAlternativeAttributes* newDecisionData);
    double normalizeTh(double x1, double x2);
    double calculateWeightedThresholdAverage(CollectStats::listAlternativeAttributes* newDecisionData);
    //Decision method
    int takeDecision(cMessage*  msg);
    void setCtrlInfoWithRespectToNetType(cMessage* msg, int networkIndex);
    void setIeee802CtrlInfo(cMessage* msg,int networkIndex);

    bool withMovingDLT;
    double hysteresisTh;
    bool lteInterfaceIsActive;
    bool isDeciderActive;
    int dummyNetworkChoice;
    bool isRandomDecision;

    std::string simpleWeights;
    std::string criteriaType;

    LteBinder* binder_;
    MacNodeId nodeId_;
    int critNumb;
    std::string pathToConfigFiles;

    CollectStats::listAlternativeAttributes* lastDecisionData;
    int lastDecision;
    bool isPingPongReductionActive;

    std::default_random_engine generator;

    std::bernoulli_distribution distribution;

    IAddressResolver* addressResolver;


  private:
    cMessage* selfMsg;


};

#endif

