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

    bool isDeciderActived(){
        return isDeciderActive;
    }
    bool isNaiveSingleCriterionBasedDecision(){
        return naiveSingleCriterionBasedDecision ;
    }

  protected:

    virtual void initialize(int stage);
    virtual int numInitStages() const { return inet::NUM_INIT_STAGES; }
    virtual void handleMessage(cMessage *msg)  override;
    virtual void finish();
    void sendToLower(cMessage* msg,int networkIndex);
    void sendToUpper(cMessage*  msg);

    string getNetworkProtocolName(int networkIndex);
    std::string convertListOfCriteriaToString(CollectStats::listAlternativeAttributes* listOfAlternativeAttributes);

    //Decision method
    int takeDecision(cMessage*  msg);
    int takeNaiveSingleCriterionBasedDecision();
    void setCtrlInfoWithRespectToNetType(cMessage* msg, int networkIndex);
    void setIeee802CtrlInfo(cMessage* msg,int networkIndex);
    void setLteCtrlInfo(cMessage* msg); //"networkIndex" parameter is not needed parameter because multiple LTE interfaces within a node is note supported

    bool lteInterfaceIsActive;
    bool isDeciderActive;
    bool naiveSingleCriterionBasedDecision;
    int naiveSingleCriterionBasedDecisionChoice ;
    int dummyNetworkChoice;
    bool isRandomDecision;

    std::string simpleWeights;
    std::string criteriaType;

    LteBinder* lteBinder_;
    MacNodeId lteInterfaceUpperLayerAddress_;
    int critNumb;
    std::string pathToConfigFiles;

    std::default_random_engine generator;

    std::bernoulli_distribution distribution;

    IAddressResolver* addressResolver;


  private:
    cMessage* selfMsg;


};

#endif

