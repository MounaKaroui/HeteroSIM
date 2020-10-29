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
        
        string interfaceToProtocolMapping = default("0:80211,1:80211,2:mode4");
        string averageMethod = default("ema"); // simple average or with ema
        
        
        @signal[tr0];
        @statistic[tr0](title="tr0"; source="tr0"; record=vector);
        
        @signal[tr1];
        @statistic[tr1](title="tr1"; source="tr1"; record=vector);
        
        @signal[delay0];
        @statistic[delay0](title="delay0"; source="delay0"; record=vector);
        
        @signal[delay1];
        @statistic[delay1](title="delay1"; source="delay1"; record=vector);
        
        //string freshnessFactor=default("");
        
}

