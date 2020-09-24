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

#include "CollectStats.h"
#include <inet/common/ModuleAccess.h>
#include "stack/phy/packet/cbr_m.h"
#include <numeric>


static const simsignal_t packetSentToLowerSignal = cComponent::registerSignal("packetSentToLower");
static const simsignal_t packetReceivedFromLowerSignal = cComponent::registerSignal("packetReceivedFromLower");
static const  simsignal_t lteSentMsg = cComponent::registerSignal("sentMsg"); // LTE sent Msg
static const  simsignal_t lteRcvdMsg = cComponent::registerSignal("receivedMsg"); // LTE received Msg


Define_Module(CollectStats);

void CollectStats::initialize()
{

    cModule* host = inet::getContainingNode(this);
    // 802.11
    inet::ieee80211::Ieee80211Mac*  mLinkLayer80211 = inet::findModuleFromPar<inet::ieee80211::Ieee80211Mac>(par("macModule80211"), host);
    mLinkLayer80211->subscribe(packetSentToLowerSignal, this);
    mLinkLayer80211->subscribe(packetReceivedFromLowerSignal, this);
    // 802.15
    int numRadios=getAncestorPar("numRadios");
    if(numRadios>1)
    {
        CSMA* mLinkLayerNarrowBand=inet::findModuleFromPar<inet::CSMA>(par("macModule80215"), host);
        mLinkLayerNarrowBand->subscribe(packetSentToLowerSignal, this);
        mLinkLayerNarrowBand->subscribe(packetReceivedFromLowerSignal, this);
    }

    // Lte
    bool mode4=getAncestorPar("withMode4");
    if(mode4)
    {
        LtePhyVUeMode4* mRadioLte= inet::findModuleFromPar<LtePhyVUeMode4>(par("radioModuleLte"), host);
        mRadioLte->subscribe(lteSentMsg,this);
        mRadioLte->subscribe(lteRcvdMsg, this);
    }

    dtlMin=par("dtlMin").doubleValue();
    delay80211= registerSignal("latency80211");
    delay80215= registerSignal("latency80215");
    delayLte= registerSignal("latencyLte");
    criteriaListSignal=registerSignal("criteriaListSignal");


}




void CollectStats::computeThroughput(simtime_t now, unsigned long bits, double& throughput)
{
    numPackets++;
    numBits += bits;

    // packet should be counted to new interval
    if (intvlNumPackets >= batchSize || now - intvlStartTime >= maxInterval)
    {
        simtime_t duration = now - intvlStartTime;
        // record measurements
        throughput=intvlNumBits / duration.dbl();// bps
        // restart counters
        intvlStartTime = now;    // FIXME this should be *beginning* of tx of this packet, not end!
        intvlNumPackets = intvlNumBits = 0;
    }

    intvlNumPackets++;
    intvlNumBits += bits;
    intvlLastPkTime = now;

}


void CollectStats::recordStats(simsignal_t comingSignal, simsignal_t signalSent, simsignal_t signalRcv,cObject* msg, listOfCriteria& l)
{
    // local variables

    double latency=0;
    double th=0;
    double rel=0;

    if(comingSignal==signalSent)
    {
        sent++;
        l.sentPackets.push_back(sent);
    }

    if (comingSignal == signalRcv)
    {
        // Latency
        auto hetMsg=dynamic_cast<cMessage*>(msg);
        latency=(simTime()-hetMsg->getTimestamp()).dbl();
        l.latency.push_back(latency);
        // Throughput
        computeThroughput(simTime(), PK(hetMsg)->getBitLength(),th);
        l.throughput.push_back(th);
        // rcved packets
        rcvd++;
        l.receivedPackets.push_back(rcvd);
    }

    if(sent!=0)
    {
        rel=(rcvd/(sent+rcvd))*100;
        l.reliability.push_back(rel);
    }
}


double CollectStats::calculateCofficientOfVariation(std::vector<double> v)
{

    double n=v.size();
    double sum=0;
    double mean=0;
    double sq_sum=0;
    double stdev=0;

    if(n!=0)
    {
        sum = std::accumulate(v.begin(), v.end(), 0.0);
        mean = sum / v.size();

        sq_sum = std::inner_product(v.begin(), v.end(), v.begin(), 0.0);
        stdev = std::sqrt(sq_sum / v.size() - mean * mean);
    }


    return stdev/mean;
}


double CollectStats::getBeta(double n)
{
    return 2/(n+1);
}
std::vector<double> CollectStats::prepareData(std::vector<double> v1, std::vector<double> v2, std::vector<double> v3)
{
    std::vector<double>  v;
    if((v1.size()>2)&&(v2.size()>2) &&(v3.size()>2))
    {
        // v1.back(); --> give the last value
        //  TODO: search beta for EWA

        double l1=Utilities::calculateEWA_BiasCorrection(v1,getBeta(v1.size())); // TODO: verify beta
        double l2=Utilities::calculateEWA_BiasCorrection(v2,getBeta(v2.size()));;
        double l3=Utilities::calculateEWA_BiasCorrection(v3,getBeta(v3.size()));;
        double myarray [] = {l1,l2,l3};
        v.insert(v.begin(), myarray, myarray+3);
    }
    return v;
}

double CollectStats::updateDTL(double x)
{
    return exp(-1*x) + dtlMin;
}

double CollectStats::prepareNetAttributes()
{

   std::vector<double> vTh=prepareData(listOfCriteria80211.throughput,listOfCriteria80215.throughput,listOfCriteriaLte.throughput);
   std::vector<double> vLatency=prepareData(listOfCriteria80211.latency,listOfCriteria80215.latency,listOfCriteriaLte.latency);
   std::vector<double> vRel=prepareData(listOfCriteria80211.reliability,listOfCriteria80215.reliability,listOfCriteriaLte.reliability);

   double cv_th=0;
   double cv_l=0;
   double cv_rel=0;
   double cv=0;
   double dtl=0;

   if(!vTh.empty() && !vLatency.empty() && !vRel.empty())
   {
        allPathsCriteriaValues=McdaAlg::buildAllPathThreeCriteria(vTh, vLatency, vRel);
        // TODO: verify cv and dtl
        cv_th=calculateCofficientOfVariation(vTh);
        cv_l=calculateCofficientOfVariation(vLatency);
        cv_rel=calculateCofficientOfVariation(vRel);
        cv=(cv_th+cv_l+cv_rel)/3;
        dtl=updateDTL(cv);
   }

   // TODO: implement DTL adaptation

   return dtl;
}




void CollectStats::receiveSignal(cComponent* source, simsignal_t signal, cObject* msg,cObject *details)
{

    std::string ModuleName=source->getParentModule()->getFullName();
    int interfaceId=Utilities::extractNumber(ModuleName);

    // Wlan interfaces
    if(interfaceId==0)
    {
        recordStats(signal,packetSentToLowerSignal,packetReceivedFromLowerSignal,msg,listOfCriteria80211);

    }
    else if(interfaceId==1)
    {
        recordStats(signal,packetSentToLowerSignal,packetReceivedFromLowerSignal,msg,listOfCriteria80215);
    }
    else
    {
        EV_INFO<< "no valid interface" << endl;
    }

    // lte
    recordStats(signal,lteSentMsg,lteRcvdMsg,msg,listOfCriteriaLte);
    prepareNetAttributes();
   // emit(criteriaListSignal,allPathsCriteriaValues.c_str());

//    cv_l=calculateCofficientOfVariation(listOfCriteria80211.latency);
//    cv_th=calculateCofficientOfVariation(listOfCriteria80211.throughput);
//    cv_rel=calculateCofficientOfVariation(listOfCriteria80211.reliability);

//    cv_latency.record(cv_l);
//    cv_throughput.record(cv_th);
//    cv_reliability.record(cv_rel);
//
//    if(!listOfCriteria80211.latency.empty())
//        emit(delay80211, listOfCriteria80211.latency.back());
//
//    if(!listOfCriteria80215.latency.empty())
//            emit(delay80215, listOfCriteria80215.latency.back());
//
//    if(!listOfCriteriaLte.latency.empty())
//            emit(delayLte, listOfCriteriaLte.latency.back());

    //double test=Utilities::calculateEWA_BiasCorrection(listOfCriteria80211.latency,0.3);
    //std::cout<< "test ema = " << test;
    // verif EMA
    //std::cout << "latency= "  << listOfCriteria80211.latency;
    // TODO call prepareNetAttributes

}


void CollectStats::finish()
{


}

