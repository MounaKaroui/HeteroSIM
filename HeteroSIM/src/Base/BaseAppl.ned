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

package heterosim.src.Base;

//
// TODO auto-generated type
//
simple BaseAppl like IBaseAppl
{
     parameters:
     
         int appID=default(0);
         string trafficType=default("Interactive");
         int msgLength=default(50B) @unit(B);  // 
         
         double startTime=default(0.0s) @unit(s); // application start time (start of the first packet)
         double stopTime=default(-1 s) @unit(s);
         double sendInterval=default(0.1 s) @unit(s);
         
         @signal[sentPk](type=cPacket);
         @statistic[sentPk](title="packets sent"; source=sentPk; record=count,"sum(packetBytes)","vector(packetBytes)"; interpolationmode=none);
         
    gates:
        input  fromDecisionMaker;
        output toDecisionMaker;

}
