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

package heterosim.src.modules.nodes;

import heterosim.src.base.IBaseAppl;
import heterosim.src.modules.decisionMaker.DecisionMaker;
import heterosim.src.modules.util.AddressResolver;

import inet.mobility.contract.IMobility;
import inet.networklayer.common.InterfaceTable;
import inet.linklayer.contract.IWirelessNic;
import inet.linklayer.ieee80211.mac.contract.IStatistics;



module GenericNode
{
    parameters:
        @display("i=block/wtx;bgb=1188.506,699.264;is=vs");
        @networkNode();
        @labels(node,ethernet-node,wireless-node);

        int numApps = default(1);
        string wlanType = default("Ieee80211Nic");
        int numRadios = default(1);
        string mobilityType = default("LinearMobility");

        *.interfaceTableModule = default(absPath(".interfaceTable"));
        *.addressResolver =default(absPath(".addressResolver"));

		//*.macModule80211 = absPath(".wlan[0].mac");


    gates:
        input radioIn[numRadios] @directIn;


    submodules:
        wlan[numRadios]: <default("Ieee80211Nic")> like IWirelessNic {
            @display("p=220.51999,402.3,row,100;q=queue");
			mgmtType=default("Ieee80211MgmtAdhoc");  // use adhoc management
        }
        mobility: <mobilityType> like IMobility {
            @display("p=1050.45,84.92999");
        }
        interfaceTable: InterfaceTable {
            @display("p=1066.84,435.08;is=s");
        }

        decisionMaker: DecisionMaker {
            @display("p=402.966,223.992");

        }
        applLayer[numApps]: <default("VanetApp")> like IBaseAppl {
            @display("p=402.3,38.739998,row,200;q=queue,i,block/app2");
        }
        collectStatistics: <default("CollectStats")> like IStatistics {
            parameters:
                @display("p=1050.45,223.49998;is=s");

        }

        addressResolver: AddressResolver {

            @display("p=1078.76,584.07996");
        }


    connections allowunconnected:
        for i=0..numApps -1 {
            decisionMaker.toApplication++ --> applLayer[i].fromDecisionMaker;
            applLayer[i].toDecisionMaker --> decisionMaker.fromApplication++;
        }



        for i=0..sizeof(radioIn)-1 {
            radioIn[i] --> { @display("m=s"); } --> wlan[i].radioIn;
            decisionMaker.toRadio++ --> wlan[i].upperLayerIn;
            wlan[i].upperLayerOut --> decisionMaker.fromRadio++;
        }



}
