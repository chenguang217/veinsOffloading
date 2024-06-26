//
// Copyright (C) 2016 David Eckhoff <david.eckhoff@fau.de>
//
// Documentation for these modules is at http://veins.car2x.org/
//
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//

#pragma once

#include <stdlib.h>
#include <Windows.h>
#include <ctime>
#include <time.h>
#include <sstream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <math.h>
#include <fstream>
#include <vector>

#include "veins/modules/application/ieee80211p/DemoBaseApplLayer.h"

namespace veins {

/**
 * Small RSU Demo using 11p
 */
class VEINS_API TraCIDemoRSU11p : public DemoBaseApplLayer {
public:
    void initialize(int stage) override;
protected:
    std::map<LAddress::L2Type, simtime_t> connectedNodes;
    std::map<LAddress::L2Type, Coord> NodePositions;
    std::map<LAddress::L2Type, std::string> NodeRoad;
    std::list<std::string>taskQueue1;
    std::list<std::string>taskQueue2;
    std::list<std::string>taskQueue3;
    std::list<std::string>taskQueue4;
    simtime_t taskWait1 = 0;
    simtime_t taskWait2 = 0;
    simtime_t taskWait3 = 0;
    simtime_t taskWait4 = 0;
    double mem;
    double cpu = 0;
    double wait;
protected:
    void onWSM(BaseFrame1609_4* wsm) override;
    void onWSA(DemoServiceAdvertisment* wsa) override;
    void onRM(ReportMessage* rm) override;
    void onTask(Task* frame) override;

    void handleSelfMsg(cMessage* msg) override;
};

} // namespace veins
