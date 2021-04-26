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

package heterosim.src.base;

//
// TODO auto-generated type
//
simple BaseAppl like IBaseAppl
{
    parameters:

        int appID = default(0);
        string trafficType = default("Interactive");
        int msgLength @unit(B) = default(50B);  // 
        string destAddress = default("all"); // all means broadcast message

        double startTime @unit(s) = default(0.0s); // application start time (start of the first packet)
        double stopTime @unit(s) = default(-1 s);
        volatile double sendInterval @unit(s) = default(exponential(1 s));

		// sent stats
        @signal[sentPk](type=cPacket);
        @statistic[sentPk](title="packets sent"; source=sentPk; record=count,"sum(packetBytes)","vector(packetBytes)"; interpolationmode=none);
        // rcvd stats
        @signal[rcvdPk](type=cPacket);
        @statistic[rcvdPk](title="packets received"; source=rcvdPk; record=count,"sum(packetBytes)","vector(packetBytes)"; interpolationmode=none);
        @statistic[throughput](title="throughput"; unit=bps; source="throughput(rcvdPk)"; record=vector,mean);
        @statistic[delay](title="delay"; source="messageAge(rcvdPk)"; unit=s; record=stats,vector,mean; interpolationmode=none);


    gates:
        input fromDecisionMaker;
        output toDecisionMaker;

}
