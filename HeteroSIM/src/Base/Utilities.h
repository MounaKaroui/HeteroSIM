/*
 * Builder.h
 *
 *  Created on: Aug 31, 2020
 *      Author: mouna1
 */

#ifndef BASE_UTILITIES_H_
#define BASE_UTILITIES_H_

#include "inet/linklayer/common/Ieee802Ctrl.h"
#include "common/LteControlInfo.h"
#include <inet/common/ModuleAccess.h>
#include <numeric>
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics.hpp>
#include <boost/accumulators/statistics/rolling_mean.hpp>
#include <iostream>
#include <boost/bind.hpp>
#include <boost/ref.hpp>
#include <map>
#include <string>
#include <iterator>
using namespace inet;
using namespace boost::accumulators;
using namespace std;

namespace Utilities {

Ieee802Ctrl* Ieee802CtrlInfo(std::string moduleName);
FlowControlInfoNonIp* LteCtrlInfo(MacNodeId nodeId_);
int extractNumber(std::string input);
string getInterfaceNameFromFullPath(std::string pathName);
template<typename K, typename V>
double calculateEMA(std::vector<double> v);
double calculateStdVec(std::vector<double> v);
double calculateMeanVec(vector<double> v);
double calculateRollingMean(std::vector<double> v, int windowSize);
double calculateCofficientOfVariation(std::vector<double> v);
double calculateBeta(double n);
bool checkLteCtrlInfo(UserControlInfo* lteInfo);


template<typename K, typename V>
bool findKeyByValue(std::vector<K> & keyVec, std::map<K, V> mapOfElements, V value)
{
    bool bResult=false;
    for (auto &it : mapOfElements) {
       if (it.second == value) {
           keyVec.push_back(it.first);
           bResult=true;
           break;
       }
    }
    return bResult;
}

} /* namespace Builder */

#endif /* BASE_UTILITIES_H_ */
