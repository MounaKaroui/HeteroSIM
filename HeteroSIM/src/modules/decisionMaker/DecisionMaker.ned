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

package heterosim.src.modules.decisionMaker;



//
// TODO auto-generated module
//
simple DecisionMaker
{
    parameters:
        @class(DecisionMaker);
        @display("i=old/telnet");
        
        bool mode4IsActive = default(false);
        //weights in this order TransmissionRate,Delay,Cbr,QueueVacancy
        string simpleWeights=default("0.4,0.4,0.2");
        // criteria type for normalization 1--> "the larger the better", 0--> "the smaller the better" 
        string criteriaType=default("1,0,1");
        // pathToConfigFiles only used to read pair wise matrix
        string pathToConfigFiles=default("../../HeteroSIM/src/Base/mcda"); 
 		bool isDeciderActive=default(true); 
 		int dummyNetworkChoice=default(0);     
 		@signal[decision];
 		@statistic[decision](title="decision";source="decision"; record=vector); 
 		     
        gates:
        input fromApplication[];
        output toApplication[];
        input fromRadio[] @labels(Ieee802Ctrl/up);
        output toRadio[] @labels(Ieee802Ctrl/down);
   
        
       
}

