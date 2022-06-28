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

#include "veins/modules/application/traci/TraCIDemoRSU11p.h"

#include "veins/modules/application/traci/TraCIDemo11pMessage_m.h"

using namespace veins;

Define_Module(veins::TraCIDemoRSU11p);

void TraCIDemoRSU11p::initialize(int stage)
{
    DemoBaseApplLayer::initialize(stage);
    if (stage == 1) {
        ReportMessage* rm = new ReportMessage();
        populateWSM(rm);
        rm->setSenderAddress(myId);
        rm->setSenderPos(curPosition);
        rm->setSenderType(0);
        scheduleAt(simTime() + uniform(0.01, 0.2), rm);
    }
}

void TraCIDemoRSU11p::onRM(ReportMessage* frame)
{
    if (frame->getSenderType() == 1) {
        ReportMessage* rm = check_and_cast<ReportMessage*>(frame);
        LAddress::L2Type sender = rm->getSenderAddress();
        Coord pos = rm->getSenderPos();
        simtime_t time = simTime();
        std::map<LAddress::L2Type, simtime_t>::iterator it;
        it = connectedNodes.find(sender);
        if(it == connectedNodes.end()) {
            connectedNodes.insert(std::make_pair(sender, time));
            NodePositions.insert(std::make_pair(sender, pos));
        }
        else {
            it->second = time;
        }
        findHost()->getDisplayString().setTagArg("t", 0, connectedNodes.size());
    }
}

void TraCIDemoRSU11p::onTask(Task* frame)
{
    Task* newTask = check_and_cast<Task*>(frame);
    char *decision = new char[strlen(newTask->getDecision())+1];
    strcpy(decision, newTask->getDecision());
    char delims[] = ";";
    char *result = NULL;
    std::cout << "receive Decision " << newTask->getDecision() << ' ' << decision << std::endl;
    result = strtok( decision, delims );
    while( result != NULL ) {
//        printf( "result is \"%s\"\n", result );
        std::string tmp = result;
        size_t pos = tmp.find("|");
        std::string temp = tmp.substr(0, pos);
        std::cout<< "if myID " << myId << ' ' << temp << std::endl;
        if (myId == atol(temp.c_str())){
//            std::cout << "same with my ID" << std::endl;
            std::string ifRun = tmp.substr(pos + 1, tmp.length() - 1);
            size_t pos2 = ifRun.find(":");
            std::string des = ifRun.substr(pos2 + 1, ifRun.length() - 1);
            if(des == "true"){
                std::cout << "operate here" << endl;
            }
            break;
        }

//        char* result2 = NULL;
//        result2 = strtok(result, delims2);
//        std::cout << "if myID " << myId << ' ' << result2 << std::endl;
//        if (myId == atol(result2)){
//            std::cout << "same with my ID" << std::endl;
//            break;
//        }
        result = strtok( NULL, delims );
    }
    std::cout << "receive Task " << newTask->getSenderPos() << std::endl;
}


void TraCIDemoRSU11p::handleSelfMsg(cMessage* msg)
{
    ReportMessage* rm = dynamic_cast<ReportMessage*>(msg);
    // std::cout << curPosition << std::endl;
    rm->setSenderAddress(myId);
    rm->setSenderPos(curPosition);
    scheduleAt(simTime() + 2, rm);
    sendDown(rm->dup());

    std::map<LAddress::L2Type, simtime_t>::iterator it;
    std::map<LAddress::L2Type, Coord>::iterator itCoord=NodePositions.begin();
    for(it = connectedNodes.begin(); it != connectedNodes.end(); it++) {
        if (simTime() - it->second >= 3.5) {
            connectedNodes.erase(it++);
            NodePositions.erase(itCoord++);
            if (it == connectedNodes.end()) {
                break;
            }
        }
        itCoord++;
    }
    findHost()->getDisplayString().setTagArg("t", 0, connectedNodes.size());
}

void TraCIDemoRSU11p::onWSA(DemoServiceAdvertisment* wsa)
{
    // if this RSU receives a WSA for service 42, it will tune to the chan
    if (wsa->getPsid() == 42) {
        mac->changeServiceChannel(static_cast<Channel>(wsa->getTargetChannel()));
    }
}

void TraCIDemoRSU11p::onWSM(BaseFrame1609_4* frame)
{
    TraCIDemo11pMessage* wsm = check_and_cast<TraCIDemo11pMessage*>(frame);

    // this rsu repeats the received traffic update in 2 seconds plus some random delay
    sendDelayedDown(wsm->dup(), 2 + uniform(0.01, 0.2));
}

