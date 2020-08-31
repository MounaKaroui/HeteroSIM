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

// The basic message for the heterogeneous network - it is used by the decision maker.
cplusplus{{
#define VANET 0
#define WSN 1
#define MODE4 2
#define LTE 3
}}
packet HeterogeneousMessage
{

	string sourceAddress;
	string destinationAddress;
	int networkType;
	int nodeId;
	simtime_t sendingTime;
	
}
