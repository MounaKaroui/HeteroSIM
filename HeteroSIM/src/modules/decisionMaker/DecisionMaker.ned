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
        
        string addressResolver ;
                
        //weights vectors for applications, each separated by ";". The first vector is for app with ID=0, the second vector is for app with ID=1, and so on...
        //In a vector of form v1,v2,V3 :  v1 is the weight for delay indicator, v2 is the weight for throughput indicator and v3 is the weight for reliability indicator.
        string simpleWeights=default("0.4,0.4,0.2;0.2,0.4,0.4");
        // criteria type for normalization 1--> "the larger the better", 0--> "the smaller the better" 
        string criteriaType=default("0,1,1");
        // pathToConfigFiles only used to read pair wise matrix
        string pathToConfigFiles=default("../../HeteroSIM/src/Base/mcda"); 
 		bool isDeciderActive=default(true); 
 		bool naiveSingleCriterionBasedDecision= default(false); // this parameter is considered when "isDeciderActive" is true
 		// Naive for applications, each separated by ";". The first choice is for app with ID=0, the second choice is for app with ID=1, and so on ...
 		string naiveSingleCriterionBasedDecisionChoice=default("0;1;2"); // choice 0 is for delay indicator, 1 for throughput indicator, and 2 for reliability indicator
 		
 		bool isRandomDecision=default(true);
 		
 		string dummyNetworkChoice=default("0;0;0");
 		
 		
 		@signal[decision];
 		@statistic[decision](title="decision";source="decision"; record=vector,stats); 
 		@signal[decision0];
 		@statistic[decision0](title="decision0";source="decision0"; record=vector,stats); 
 		@signal[decision1];
 		@statistic[decision1](title="decision1";source="decision1"; record=vector,stats); 
 		@signal[decision2];
 		@statistic[decision2](title="decision2";source="decision2"; record=vector,stats); 
 		     
 		     
        gates:
        input fromApplication[];
        output toApplication[];
        input fromRadio[] @labels(Ieee802Ctrl/up);
        output toRadio[] @labels(Ieee802Ctrl/down);
   
        
       
}

