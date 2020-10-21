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

#ifndef __HETEROSIM_COLLECTSTATS_H_
#define __HETEROSIM_COLLECTSTATS_H_
#include "inet/common/LayeredProtocolBase.h"
#include "../../Modules/messages/Messages_m.h"
#include "inet/common/misc/ThruputMeter.h"
#include "inet/physicallayer/common/packetlevel/Radio.h"
#include "inet/physicallayer/ieee80211/packetlevel/Ieee80211Radio.h"
#include <inet/linklayer/ieee80211/mac/Ieee80211Mac.h>
#include "Base/mcda/MCDM.h"
#include "inet/linklayer/csma/CSMA.h"
#include "stack/pdcp_rrc/layer/LtePdcpRrc.h"
#include <omnetpp.h>
#include "../../Base/Utilities.h"
#include "stack/phy/layer/LtePhyVUeMode4.h"
#include "stack/mac/layer/LteMacVUeMode4.h"
using namespace omnetpp;
using namespace inet;
using namespace inet::physicallayer;
/**
 * TODO - Generated class
 */

class CollectStats : public cListener, public cSimpleModule
{

public:
    struct listOfCriteria{

        vector<simtime_t> timeStamp;
        vector<double>  delay;
        vector<double>  transmissionRate;
        vector<double> queueVacancy; // TODO compute queueVacancy
    };

    struct alternativeAttributes
    {
        double delay ;
        double transmissionRate;
        double queueVacancy;
    };

    struct listAlternativeAttributes
    {
        map<int,alternativeAttributes*> data;
    };


    map<int,listOfCriteria*> listOfCriteriaByInterfaceId;
    map<int,map<string,simtime_t>> packetFromUpperTimeStampsByInterfaceId; // To compute delays
    std::string  interfaceToProtocolMapping ;
    map<int,std::string> interfaceToProtocolMap;
    map<int,double> dltByInterfaceId;
    std::string averageMethod;

    template<typename T>
    void subscribeToSignal(std::string moduleName, simsignal_t sigName)
    {
        cModule* module = getModuleByPath(moduleName.c_str());
        T * mLinkLayer=dynamic_cast<T*>(module);
        mLinkLayer->subscribe(sigName, this);
    }

    void printMsg(std::string type, cMessage*  msg);
    double getAvailableBandwidth(int64_t dataLength, double sendInterval, double cbr);
    void setInterfaceToProtocolMap();
    void updateDLT(listOfCriteria* list,int interfaceId);
    void initializeDLT();

    //TODO change to protected or private
    map<int,listOfCriteria*> getSublistByDLT();
    listAlternativeAttributes applyAverageMethod(map<int,listOfCriteria*> dataSet);
    std::string convertListOfCriteriaToString(listAlternativeAttributes a);
    std::string prepareNetAttributes();
    double extractQueueVacancy(int interfaceId);
    double extractBufferOccupancy();
    double getWlanCBR(int interfaceId);
    double extractLteBufferVacancy();
    double getLteCBR();

    simsignal_t signalCritereNet1;
    simsignal_t signalCritereNet2;

protected:

    virtual void initialize();
    void registerSignals();
    virtual void receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj, cObject *details);

    void recordStatsForWlan(simsignal_t comingSignal, string sourceName ,cMessage* msg,  int interfaceId);
    void recordStatsForLte(simsignal_t comingSignal, cMessage* msg, int interfaceId);
    void recordStatTuple(int interfaceId, double delay, double transmissionRate, double queueVacancy);
    void insertStatTuple(listOfCriteria* list, simtime_t timestamp, double delay, double transmissionRate, double queueVacancy);

    listOfCriteria* getSublistByDLT(int interfaceID);

};

#endif

