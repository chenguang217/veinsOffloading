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
cplusplus {{
#include "veins/base/utils/Coord.h"
#include "veins/modules/messages/BaseFrame1609_4_m.h"
#include "veins/base/utils/SimpleAddress.h"
#include <map>
}}

namespace veins;

class BaseFrame1609_4;
class noncobject Coord;
class LAddress::L2Type extends void;

packet Task extends BaseFrame1609_4 {
    Coord senderPos;
    LAddress::L2Type senderAddress = -1;
    int senderType = 0;
    string name;
    double CPU;
    double mem;
    double storage;
    double operationTime;
    double transmissionTime;
    string externalId;
    string roadId;
    string target;
    string decision;
    string possibleRSUs;
    string relay;
    string service;
    Coord deadlinePos;
    double ratio;
}