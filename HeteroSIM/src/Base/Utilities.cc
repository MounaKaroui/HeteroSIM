/*
 * Builder.cpp
 *
 *  Created on: Aug 31, 2020
 *      Author: Mouna KAROUI
 */

#include "Utilities.h"
using namespace inet;
namespace Utilities{


double calculateEMA(vector<double> v)
{
    double emaSample;
    double meanSample=calculateMeanVec(v);
    double beta=calculateBeta(v.size());
    if(v.size()>=1)
    {
        emaSample =(1-beta)*meanSample+beta*v.back();
    }

    return emaSample;
}

double calculateBeta(double n)
{
    return 2/(n+1);
}

double calculateCofficientOfVariation(vector<double> v)
{

    double n=v.size();
    double mean=0;
    double stdev=0;
    if(n!=0)
    {
        mean = calculateMeanVec(v);
        stdev =  calculateStdVec(v);
    }
    return stdev/mean;
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
double calculateStdVec(std::vector<double> v)
{
    accumulator_set<double, stats<tag::variance> > acc;
    for_each(v.begin(), v.end(), boost::bind<void>(boost::ref(acc), _1));
    return std::sqrt(variance(acc));
}

double calculateMeanVec(std::vector<double> v)
{
    accumulator_set<double, stats<tag::mean> > acc;
    for_each(v.begin(), v.end(), boost::bind<void>(boost::ref(acc), _1));
    return mean(acc);
}

double calculateRollingMean(std::vector<double> v, int windowSize)
{
    accumulator_set<int, stats<tag::rolling_mean> > acc(tag::rolling_window::window_size = windowSize);
    return rolling_mean(acc);
}


} /* namespace Builder */
