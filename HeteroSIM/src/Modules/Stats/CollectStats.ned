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

package heterosim.Modules.Stats;

//
// TODO auto-generated module
//
import inet.linklayer.ieee80211.mac.contract.IStatistics;


simple CollectStats like IStatistics
{
    
    
    parameters:
          
          // 802.11
          string radioModule80211=default("");
          string macModule80211=default("");
          
          // 802.15
          string radioModule80215=default("");
          string macModule80215=default("");
          // Lte
          string radioModuleLte=default("");
}
