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
#include "inet/common/misc/ThruputMeter.h"
#include "inet/physicallayer/common/packetlevel/Radio.h"
#include "inet/physicallayer/ieee80211/packetlevel/Ieee80211Radio.h"
#include <inet/linklayer/ieee80211/mac/Ieee80211Mac.h>
#include <inet/linklayer/ieee80211/mac/Ieee80211Frame_m.h>
#include "inet/linklayer/csma/CSMA.h"
#include "stack/pdcp_rrc/layer/LtePdcpRrc.h"
#include <omnetpp.h>

#include "../../base/mcda/MCDM.h"
#include "../../base/Utilities.h"
#include "../../modules/messages/Messages_m.h"
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

        map<simtime_t,double>*  delayIndicator;
        map<simtime_t,double>*  throughputIndicator;
        map<simtime_t,double>* reliabilityIndicator;
    };

    typedef tuple<long,long>  LongIntegerPair;

    struct alternativeAttributes
    {
        double delayIndicator ;
        double throughputIndicator;
        double reliabilityIndicator;
    };

    struct listAlternativeAttributes
    {
        map<int,alternativeAttributes*> data;
    };


    map<int,listOfCriteria*> listOfCriteriaByInterfaceId;

    map<int,map<string,simtime_t>> packetFromUpperTimeStampsByInterfaceId; // To compute delays
    map<int,LongIntegerPair> attemptedToBeAndSuccessfullyTransmittedDataByInterfaceId; // To compute reliability and throughput metrics.
                                                                                        //Tuple <0> is for attempted to be transmitted data and Tuple<1> is successfully transmitted data

    map<int,map<string,cMessage*>> lastTransmittedFramesByInterfaceId ; // utility map to record statistics depending on whether transmitted is unicast or broadcast/multicast frame

    listAlternativeAttributes recentCriteriaStatsByInterfaceId;

    std::string  interfaceToProtocolMapping ;
    map<int,std::string> interfaceToProtocolMap;
    map<int,map<string,double>> dltByInterfaceIdByCriterion;

    template<typename T>
    void subscribeToSignal(std::string moduleName, simsignal_t sigName)
    {
        cModule* module = getModuleByPath(moduleName.c_str());
        T * mLinkLayer=dynamic_cast<T*>(module);
        mLinkLayer->subscribe(sigName, this);
    }
    template<typename T>
    void subscribeToSignal(std::string moduleName, const char * signalName)
    {
        cModule* module = getModuleByPath(moduleName.c_str());
        T * mLinkLayer=dynamic_cast<T*>(module);
        mLinkLayer->subscribe(signalName, this);
    }

    //Final decision data to be used as an input for decider
    CollectStats::listAlternativeAttributes* prepareNetAttributes();

    map<int,double> commonDLTMaxByInterfaceId;

protected:
    //NED parameters:
    std::string averageMethod;

    // Initialization and signal registration
    virtual void initialize(int stage);
    virtual int numInitStages() const { return inet::NUM_INIT_STAGES; }
    virtual void finish();
    void registerSignals();
    void initializeDLT();
    void setInterfaceToProtocolMap();
    void setCommonDltMax(std::string strValues);


    double getThroughputIndicator(int64_t dataBitLength, double radioFrameTime);
    virtual void receiveSignal(cComponent *source, simsignal_t signalID, cObject *obj, cObject *details);
    virtual void receiveSignal(cComponent *source, simsignal_t signalID, long value, cObject *details);
    void recordStatsForWlan(simsignal_t comingSignal, string sourceName ,cMessage* msg,  int interfaceId);
    void recordStatsForLte(simsignal_t comingSignal, cMessage* msg, int interfaceId);

    // Data Life Time calculation
    void updateDLT(listOfCriteria* list,int interfaceId);
    double getDLT(double CofficientOfVariation,int interfaceId);

    //Network attributes processing
    void recordStatTuple(int interfaceId, double delay, double transmissionRate, double queueVacancy);
    void insertStatTuple(listOfCriteria* list, simtime_t timestamp, double delayIndicator, double throughputIndicator, double reliabilityIndicator);
    void insertStatTuple(int interfaceId, simtime_t timestamp, double delayIndicator, double throughputIndicator, double reliabilityIndicator);

    listOfCriteria* getSublistByDLT(int interfaceID);
    map<simtime_t,double>* getSublistByDLTOfCriterion(map<simtime_t,double>* pCriterion,double pDlt);
    map<int,listOfCriteria*> getSublistByDLT();
    listAlternativeAttributes* applyAverageMethod(map<int,listOfCriteria*> dataSet);


    //Signals for stats
    simsignal_t throughputIndicator0Signal;
    simsignal_t throughputIndicator1Signal;

    simsignal_t delayIndicator0Signal;
    simsignal_t delayIndicator1Signal;

    simsignal_t reliabilityIndicator0Signal;
    simsignal_t reliabilityIndicator1Signal;

    //to record LTE related statistics
    simsignal_t lteMacSentPacketToLowerLayerSingal;
    simsignal_t ltePhyRecievedAirFrameSingal;
    MacNodeId lteInterfaceMacId_;
    simtime_t lastRacSendTimestamp;
    simtime_t lastRacReceptionTimestamp;
    simtime_t lastMacGrantObtentionDelay=0 ;

};

#endif

