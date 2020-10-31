/*
 * Builder.cpp
 *
 *  Created on: Aug 31, 2020
 *      Author: Mouna KAROUI
 */

#include "../base/Utilities.h"
using namespace inet;
namespace Utilities{



void calculateEMA(const vector<double> *vData, vector<double>& vEMA)
{
    double beta = 2.0 / (1.0 + vData->size());
    if (vData->size() > 0)
    {
    if (vEMA.size() == 0)
    {
     vEMA.resize(vData->size());
     vEMA.at(0) = vData->at(0);
    }

    for (unsigned int i = 1; i < vData->size(); i++)
    {
       vEMA.at(i) = beta * vData->at(i) + (1 - beta) * vEMA.at(i - 1);
    }
    }else{
        vEMA.push_back(vData->back());
    }

}


double calculateCofficientOfVariation(const vector<double> *v) {

    if (v->size() != 0)
        if(calculateMeanVec(v)!=0)
            return calculateStdVec(v) / calculateMeanVec(v);
        else
            return 0;
    else
        return 0;
}

int extractNumber(std::string input)
{
    size_t i = 0;
    for ( ; i < input.length(); i++ ){ if ( isdigit(input[i]) ) break; }
    // remove the first chars, which aren't digits
    input = input.substr(i, input.length() - i );
    // convert the remaining text to an integer
    int id = atoi(input.c_str());
    return id;
}

string getInterfaceNameFromFullPath(std::string pathName){
    return cStringTokenizer(pathName.c_str(),string(".").c_str()).asVector()[2];
}


Ieee802Ctrl* Ieee802CtrlInfo(std::string ModuleName)
{
    auto controlInfo = new Ieee802Ctrl();
    inet::MACAddress srcAdr;
    char* c = const_cast<char*>(ModuleName.c_str());
    srcAdr.setAddressBytes(c);
    controlInfo->setSourceAddress(srcAdr);

    inet::MACAddress destAdress;
    destAdress.setBroadcast();
    controlInfo->setDest(destAdress);
    return controlInfo;
}

FlowControlInfoNonIp* LteCtrlInfo(MacNodeId nodeId_)
{
    auto lteControlInfo = new FlowControlInfoNonIp();
    lteControlInfo->setSrcAddr(nodeId_);
    lteControlInfo->setDirection(D2D_MULTI);
    lteControlInfo->setDuration(1000);
    lteControlInfo->setPriority(3);
    lteControlInfo->setCreationTime(simTime());
    lteControlInfo->setMsgFlag(random());
    return lteControlInfo;
}

bool checkLteCtrlInfo(UserControlInfo* lteInfo)
{
    if (lteInfo)
        return (lteInfo->getFrameType() != HANDOVERPKT
                && lteInfo->getFrameType() != D2DMODESWITCHPKT
                && lteInfo->getFrameType() != RACPKT
                && lteInfo->getFrameType() != GRANTPKT
                && lteInfo->getFrameType() != HARQPKT);
}
double calculateStdVec(const std::vector<double> *v)
{
    accumulator_set<double, stats<tag::variance> > acc;
    for_each(v->begin(), v->end(), boost::bind<void>(boost::ref(acc), _1));
    return std::sqrt(variance(acc));
}

double calculateMeanVec(const std::vector<double> *v)
{
    accumulator_set<double, stats<tag::mean> > acc;
    for_each(v->begin(), v->end(), boost::bind<void>(boost::ref(acc), _1));
    return mean(acc);
}

double calculateRollingMean(std::vector<double> v, int windowSize)
{
    accumulator_set<int, stats<tag::rolling_mean> > acc(tag::rolling_window::window_size = windowSize);
    return rolling_mean(acc);
}


} /* namespace Builder */
