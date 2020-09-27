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

package heterosim.Modules.DecisionMaker;

import inet.common.queue.DropTailQueue;


//
// TODO auto-generated module
//
simple DecisionMaker
{
    parameters:
        @class(DecisionMaker);
        @display("i=old/telnet");
        
        bool mode4IsActive = default(false);
        int criteriaNum=default(3);
        // TODO: transform absolute path to relative
        string pathToConfigFiles=default("/home/mouna1/HeteroSimWorkSpace/HeteroSIM/src/Base/mcda/");
   
 		bool isDeciderActive=default(true); 
 		int dummyNetworkChoice=default(0);     
 		
 		
        gates:

        input fromApplication[];
        output toApplication[];

        input fromRadio[] @labels(Ieee802Ctrl/up);
        output toRadio[] @labels(Ieee802Ctrl/down);
        

        
//    submodules:
//
//
//        queue: DropTailQueue {
//            @display("p=177,113");
//        }
//        connections allowunconnected:
}

