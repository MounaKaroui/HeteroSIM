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

//
// TODO auto-generated module
//
simple DecisionMaker
{
    parameters:
        @class(DecisionMaker);

     	// G5 
        @signal[G5MessagesSent](type="long");
        @statistic[G5MessagesSent](title="G5MessagesSent"; source="G5MessagesSent"; record=vector,stats,histogram);  
        @signal[G5MessagesReceived](type="long");
        @statistic[G5MessagesReceived](title="G5MessagesReceived"; source="G5MessagesReceived"; record=vector,stats,histogram);
      
        
        // Mode 4
        @signal[mode4MsgReceived](type="long");
        @statistic[mode4MsgReceived](title="mode4MsgReceived"; source="mode4MsgReceived"; record=vector,stats,histogram);
        @signal[mode4MsgSent](type="long");
        @statistic[mode4MsgSent](title="mode4MsgSent"; source="mode4MsgSent"; record=vector,stats,histogram);

     	bool mode4IsActive=default(false);
     	
        
        @display("i=old/telnet");
    gates:
        
        input  fromApplication[];
        output toApplication[];

        input  fromRadio[] ; //@labels(Ieee802Ctrl/up);
        output toRadio[] ; //@labels(Ieee802Ctrl/down);
        
        
        //output toMode4;
        //input  fromMode4;
}
