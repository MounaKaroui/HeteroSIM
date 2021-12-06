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

package heterosim.src.modules.stats;

//
// TODO auto-generated module
//
import inet.linklayer.ieee80211.mac.contract.IStatistics;


simple CollectStats like IStatistics
{
    
    parameters:    
        string interfaceToProtocolMapping = default("0:WiFi,1:LTE");
        string averageMethod = default("ema"); // simple average or with ema
        string dltMaxByAppByInterfaceId=default("0:0:1;0:1:1"); //TODO configure depending the umber of deployer apps; // ; separated v1:v2:v3 values where v1 is the app ID, v2 is the interface ID, and v3 the maximun Data Life Time (in second) for each of its criteria.
        double controlTrafficSendInterval @unit(s) =default(0.1s) ; // TODO separate per interface
        string minNumOfControlTrafficPktInDLTByApp = default("0:5") ; //TODO separate per interface
        //int minDLT
        
        // Statistics
        @signal[throughputIndicator0];
        @statistic[throughputIndicator0](title="throughputIndicator0"; source="throughputIndicator0"; record=vector,mean);
        
        @signal[throughputIndicator1];
        @statistic[throughputIndicator1](title="throughputIndicator1"; source="throughputIndicator1"; record=vector,mean);
        
        @signal[delayIndicator0];
        @statistic[delayIndicator0](title="delayIndicator0"; source="delayIndicator0"; record=vector,mean);
        
        @signal[delayIndicator1];
        @statistic[delayIndicator1](title="delayIndicator1"; source="delayIndicator1"; record=vector,mean);
              
        @signal[reliabilityIndicator0];
        @statistic[reliabilityIndicator0](title="reliabilityIndicator0"; source="reliabilityIndicator0"; record=vector,mean);
        
        @signal[reliabilityIndicator1];
        @statistic[reliabilityIndicator1](title="reliabilityIndicator1"; source="reliabilityIndicator1"; record=vector,mean);
}

