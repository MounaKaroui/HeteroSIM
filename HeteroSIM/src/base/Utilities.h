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
//FlowControlInfoNonIp* LteCtrlInfo(MacNodeId nodeId_);
int extractNumber(std::string input);
int extractNumber(const char * input);
string getInterfaceNameFromFullPath(std::string pathName);
void  calculateEMA(const vector<double>* vData, vector<double>& vEMA);

double calculateStdVec(const std::vector<double> *v);
double calculateMeanVec(const std::vector<double> *v);
double calculateRollingMean(std::vector<double> v, int windowSize);
double calculateCofficientOfVariation(const std::vector<double> *v);
bool checkLteCtrlInfo(UserControlInfo* lteInfo);

template<typename K, typename V>
vector<V>* retrieveValues(const std::map<K, V>* mapOfElements){
    vector<V>* rVector = new vector<V>();
    for(auto it = mapOfElements->begin(); it != mapOfElements->end(); ++it)
        rVector->push_back(it->second);
    return rVector ;
}
template<typename K, typename V>
vector<K>* retrieveKeys(const std::map<K, V>* mapOfElements){
    vector<K>* rVector = new vector<K>();
    for(auto it = mapOfElements->begin(); it != mapOfElements->end(); ++it)
        rVector->push_back(it->first);
    return rVector ;
}

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
