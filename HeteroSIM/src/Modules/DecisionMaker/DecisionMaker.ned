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

        bool debug = default(false);
        bool infoLogging = default(true);

        @signal[DSRCMessagesSent](type="long");
        @statistic[DSRCMessagesSent](title="DSRCMessagesSent"; source="DSRCMessagesSent"; record=vector,stats,histogram);
        @signal[lteMessagesSent](type="long");
        @statistic[lteMessagesSent](title="lteMessagesSent"; source="lteMessagesSent"; record=vector,stats,histogram);
        @signal[dontCareMessagesSent](type="long");
        @statistic[dontCareMessagesSent](title="dontCareMessagesSent"; source="dontCareMessagesSent"; record=vector,stats,histogram);
        @signal[DSRCMessagesReceived](type="long");
        @statistic[DSRCMessagesReceived](title="DSRCMessagesReceived"; source="DSRCMessagesReceived"; record=vector,stats,histogram);
        @signal[lteMessagesReceived](type="long");
        @statistic[lteMessagesReceived](title="lteMessagesReceived"; source="lteMessagesReceived"; record=vector,stats,histogram);

        @display("i=old/telnet");
    gates:
        input fromApplication;
        output toApplication;

        input fromLte;
        output toLte;

        input ifInRadio[];
        output ifOutRadio[];
}
