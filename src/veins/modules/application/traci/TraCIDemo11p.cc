//
// Copyright (C) 2006-2011 Christoph Sommer <christoph.sommer@uibk.ac.at>
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

#include "veins/modules/application/traci/TraCIDemo11p.h"

#include "veins/modules/application/traci/TraCIDemo11pMessage_m.h"

using namespace veins;

Define_Module(veins::TraCIDemo11p);
double U_Random();
int possion();

template<typename T> std::string toString(const T& t) {
    std::ostringstream oss;
    oss<<t;
    return oss.str();
}

void TraCIDemo11p::initialize(int stage)
{
    DemoBaseApplLayer::initialize(stage);
    if (stage == 0) {
        sentMessage = false;
        lastDroveAt = simTime();
        currentSubscribedServiceId = -1;
        srand( (unsigned)time( NULL ) );
    }
}

void TraCIDemo11p::onRM(ReportMessage* rm)
{
    if (rm->getSenderType() == 0) {
        lastReceiveAt = simTime();
        LAddress::L2Type sender = rm->getSenderAddress();
        Coord pos = rm->getSenderPos();
        long rsuCpu = rm->getCpu();
        long rsuMem = rm->getMem();
        simtime_t rsuWait = rm->getWait();
        ReportMessage* newRM = new ReportMessage();
        populateWSM(newRM);
        newRM->setSenderAddress(myId);
        newRM->setSenderType(1);
        scheduleAt(simTime() + uniform(0.01, 0.2), newRM->dup());
        delete newRM;
        std::map<LAddress::L2Type, simtime_t>::iterator it;
        it = connectedRSUs.find(sender);
        if(it == connectedRSUs.end()) {
            connectedRSUs.insert(std::make_pair(sender, lastReceiveAt));
            RSUPositions.insert(std::make_pair(sender, pos));
            RSUcpus.insert(std::make_pair(sender, rsuCpu));
            RSUmems.insert(std::make_pair(sender, rsuMem));
            RSUwaits.insert(std::make_pair(sender, rsuWait));
            
        }
        // std::cout << "Vehicle " << myId << " find RSU "<< sender << " at position " << pos << ", now have connectedRSUs " << connectedRSUs.size() << " RSUPositions " << RSUPositions.size() << std::endl;
        // std::map<LAddress::L2Type, Coord>::iterator itCoord=RSUPositions.begin();
        // for(it = connectedRSUs.begin(); it != connectedRSUs.end(); it++) {
        //     std::cout << "Vehicle " << myId << " now have RSU" << it->first << " at " << itCoord->second << std::endl;
        //     itCoord++;
        // }

    }
}

void TraCIDemo11p::onTask(Task* frame)
{
    Task* newTask = check_and_cast<Task*>(frame);
    if (newTask->getSenderType() == 1 && newTask->getSenderAddress() == myId) {
        std::cout << "task finished" << std::endl;
        std::string command = "D:\\scoop\\apps\\python38\\current\\python.exe vehState.py " + toString(curPosition) + " " + toString(newTask->getDeadlinePos()) + " " + toString(newTask->getSenderPos()) + " " + toString(newTask->getCPU()) + " " + toString(newTask->getMem()) + " " + toString(newTask->getOperationTime()) + " " + newTask->getName();
        std::cout << "finished " << command << std::endl;
        int result = system(command.c_str());
    }
}

void TraCIDemo11p::handleSelfMsg(cMessage* msg)
{
    if (TraCIDemo11pMessage* wsm = dynamic_cast<TraCIDemo11pMessage*>(msg)) {
        // send this message on the service channel until the counter is 3 or higher.
        // this code only runs when channel switching is enabled
        sendDown(wsm->dup());
        wsm->setSerial(wsm->getSerial() + 1);
        if (wsm->getSerial() >= 3) {
            // stop service advertisements
            stopService();
            delete (wsm);
        }
        else {
            scheduleAt(simTime() + 1, wsm);
        }
    }
    else if (ReportMessage* rm = dynamic_cast<ReportMessage*>(msg)) {
        rm->setSenderPos(curPosition);
        sendDown(rm->dup());
        delete rm;
    }
    else if (Task* task = dynamic_cast<Task*>(msg)) {
        task->setSenderPos(curPosition);
        sendDown(task->dup());
        delete task;
    }
    else {
        DemoBaseApplLayer::handleSelfMsg(msg);
    }
    // std::cout << "There are " << connectedRSUs.size() << " connected " << myId << std::endl;
}

void TraCIDemo11p::handlePositionUpdate(cObject* obj)
{
    std::map<LAddress::L2Type, simtime_t>::iterator it;
    std::map<LAddress::L2Type, Coord>::iterator itCoord=RSUPositions.begin();
    std::map<LAddress::L2Type, long>::iterator itCpu=RSUcpus.begin();
    std::map<LAddress::L2Type, long>::iterator itMem=RSUmems.begin();
    std::map<LAddress::L2Type, simtime_t>::iterator itWait=RSUwaits.begin();
    // std::string roadIdTmp = mobility->getRoadId();
    // std::string command = "D:\\scoop\\apps\\python38\\current\\python.exe posRecord.py " + roadIdTmp;
    // int result = system(command.c_str());
    for(it = connectedRSUs.begin(); it != connectedRSUs.end(); it++) {
        if (simTime() - it->second >= 5) {
            // std::cout << "RSU " << it->first << " at " << itCoord->first << " didn't response in " << simTime() - it->second << " seconds" << std::endl;
            connectedRSUs.erase(it++);
            RSUPositions.erase(itCoord++);
            RSUcpus.erase(itCpu++);
            RSUmems.erase(itMem++);
            RSUwaits.erase(itWait++);
            if (it == connectedRSUs.end()) {
                break;
            }
        }
        itCoord++;
        itCpu++;
        itMem++;
        itWait++;
    }
    // std::cout << "position x " << curPosition.x << " if equal " << std::endl;
    if (lastroadID == ""){
        lastroadID = mobility->getRoadId();
        std::cout << myId << " is at " << lastroadID << std::endl;
    }
    if (mobility->getRoadId() != lastroadID){
        lastroadID = mobility->getRoadId();
        if (lastroadID.at(0) != ':'){
            std::cout << myId << " is at " << lastroadID << std::endl;
            // random generate tasks
            if (U_Random() > 0.5 && ifSend <= 1){
                // generate Task Message
                std::string externalId = mobility->getExternalId();
                std::string roadId = mobility->getRoadId();
                std::cout<< "generate tasks" << std::endl;
                Task* task = new Task();
                populateWSM(task);
                task->setSenderAddress(myId);
                task->setCPU(U_Random() * 28 + 2);
                // task->setCPU(20);
                task->setMem(U_Random() * 900 + 300);
                task->setStorage(U_Random() * 900 + 300);
                task->setExternalId(externalId.c_str());
                task->setRoadId(roadId.c_str());
                task->setSenderType(0);

                std::string tmp = "";
                std::map<LAddress::L2Type, Coord>::iterator it;
                // add cpu mem wait in decision string
                std::map<LAddress::L2Type, long>::iterator itCpu = RSUcpus.begin();
                std::map<LAddress::L2Type, long>::iterator itMem = RSUmems.begin();
                std::map<LAddress::L2Type, simtime_t>::iterator itWait = RSUwaits.begin();
                double minDist = 1000;
                std::string RSUinfo = "";
                for(it = RSUPositions.begin(); it != RSUPositions.end(); it++) {
                    RSUinfo = RSUinfo + toString(itCpu->first) + ":" + toString(it->second) + "*" + toString(itCpu->second) + "*" + toString(itMem->second) + "*" + toString(itWait->second) + ";";
                    double tmpDist = it->second.distance(curPosition);
                    if(tmpDist <= minDist){
                        minDist = tmpDist;
                        // std::cout << toString(it->second) << std::endl;
                        task->setTarget(toString(it->second).c_str());
                    }
                    itCpu++;
                    itMem++;
                    itWait++;
                }
                LPVOID pBuffer;
                HANDLE hMap = NULL;
                hMap = OpenFileMapping(FILE_MAP_ALL_ACCESS, 0, (externalId + "global_share_memory").c_str());
                if (hMap == NULL) {
                    hMap = CreateFileMapping(NULL, NULL, PAGE_READWRITE, 0, 0X1000, (externalId + "global_share_memory").c_str());
                }
                pBuffer = MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, 0);

                LPVOID pBufferDecision;
                HANDLE hMapDecision = NULL;
                hMapDecision = OpenFileMapping(FILE_MAP_ALL_ACCESS, 0, (externalId + "possibleRSU").c_str());
                if (hMapDecision == NULL) {
                    hMapDecision = CreateFileMapping(NULL, NULL, PAGE_READWRITE, 0, 0X1000, (externalId + "possibleRSU").c_str());
                }
                pBufferDecision = MapViewOfFile(hMapDecision, FILE_MAP_ALL_ACCESS, 0, 0, 0);

                LPVOID pBufferDeadlineX;
                HANDLE hMapDeadlineX = NULL;
                hMapDeadlineX = OpenFileMapping(FILE_MAP_ALL_ACCESS, 0, (externalId + "deadLinePosX").c_str());
                if (hMapDeadlineX == NULL) {
                    hMapDeadlineX = CreateFileMapping(NULL, NULL, PAGE_READWRITE, 0, 0X1000, (externalId + "deadLinePosX").c_str());
                }
                pBufferDeadlineX = MapViewOfFile(hMapDeadlineX, FILE_MAP_ALL_ACCESS, 0, 0, 0);

                LPVOID pBufferDeadlineY;
                HANDLE hMapDeadlineY = NULL;
                hMapDeadlineY = OpenFileMapping(FILE_MAP_ALL_ACCESS, 0, (externalId + "deadLinePosY").c_str());
                if (hMapDeadlineY == NULL) {
                    hMapDeadlineY = CreateFileMapping(NULL, NULL, PAGE_READWRITE, 0, 0X1000, (externalId + "deadLinePosY").c_str());
                }
                pBufferDeadlineY = MapViewOfFile(hMapDeadlineY, FILE_MAP_ALL_ACCESS, 0, 0, 0);

                std::string command = "D:\\scoop\\apps\\python38\\current\\python.exe vehDecision.py 123 " + externalId + " " + roadId + " " + RSUinfo + " " + toString(curPosition) + " " + toString(task->getCPU());
                std::cout << "command " << command << std::endl;
                int result = system(command.c_str());
                if ((char*)pBufferDeadlineX != NULL){
                    task->setDeadlinePos(Coord(strtod((char*)pBufferDeadlineX, NULL), strtod((char*)pBufferDeadlineY, NULL), 0));
                    ifSend++;
                }
                task->setName((char*)pBuffer);
                task->setPossibleRSUs((char*)pBufferDecision);
                scheduleAt(simTime() + uniform(0.01, 0.2), task->dup());
            }
        }
    }
    // if (curPosition.x >= 2407.16 && ifSend <= 1){
        
    //     // generate Task Message
    //     std::string externalId = mobility->getExternalId();
    //     std::string roadId = mobility->getRoadId();
    //     std::cout<< "generate tasks" << std::endl;
    //     Task* task = new Task();
    //     populateWSM(task);
    //     task->setSenderAddress(myId);
    //     task->setCPU(U_Random() * 28 + 2);
    //     // task->setCPU(20);
    //     task->setMem(U_Random() * 900 + 300);
    //     task->setStorage(U_Random() * 900 + 300);
    //     task->setExternalId(externalId.c_str());
    //     task->setRoadId(roadId.c_str());
    //     task->setSenderType(0);

    //     std::string tmp = "";
    //     std::map<LAddress::L2Type, Coord>::iterator it;
    //     // add cpu mem wait in decision string
    //     std::map<LAddress::L2Type, long>::iterator itCpu = RSUcpus.begin();
    //     std::map<LAddress::L2Type, long>::iterator itMem = RSUmems.begin();
    //     std::map<LAddress::L2Type, simtime_t>::iterator itWait = RSUwaits.begin();
    //     double minDist = 1000;
    //     std::string RSUinfo = "";
    //     for(it = RSUPositions.begin(); it != RSUPositions.end(); it++) {
    //         RSUinfo = RSUinfo + toString(itCpu->first) + ":" + toString(it->second) + "*" + toString(itCpu->second) + "*" + toString(itMem->second) + "*" + toString(itWait->second) + ";";
    //         double tmpDist = it->second.distance(curPosition);
    //         if(tmpDist <= minDist){
    //             minDist = tmpDist;
    //             // std::cout << toString(it->second) << std::endl;
    //             task->setTarget(toString(it->second).c_str());
    //         }
    //         itCpu++;
    //         itMem++;
    //         itWait++;

    //         // if (it == RSUPositions.begin()) {
    //         //     tmp = tmp + toString(it->first) + "|" + toString(it->second) + ":true;";
    //         // }
    //         // else {
    //         //     tmp = tmp + toString(it->first) + "|" + toString(it->second) + ":false;";
    //         // }
    //     }
    //     // task->setDecision((char*)tmp.data());
    //     LPVOID pBuffer;
    //     HANDLE hMap = NULL;
    //     hMap = OpenFileMapping(FILE_MAP_ALL_ACCESS, 0, (externalId + "global_share_memory").c_str());
    //     if (hMap == NULL) {
    //         hMap = CreateFileMapping(NULL, NULL, PAGE_READWRITE, 0, 0X1000, (externalId + "global_share_memory").c_str());
    //     }
    //     pBuffer = MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, 0);

    //     LPVOID pBufferDecision;
    //     HANDLE hMapDecision = NULL;
    //     hMapDecision = OpenFileMapping(FILE_MAP_ALL_ACCESS, 0, (externalId + "possibleRSU").c_str());
    //     if (hMapDecision == NULL) {
    //         hMapDecision = CreateFileMapping(NULL, NULL, PAGE_READWRITE, 0, 0X1000, (externalId + "possibleRSU").c_str());
    //     }
    //     pBufferDecision = MapViewOfFile(hMapDecision, FILE_MAP_ALL_ACCESS, 0, 0, 0);

    //     LPVOID pBufferDeadlineX;
    //     HANDLE hMapDeadlineX = NULL;
    //     hMapDeadlineX = OpenFileMapping(FILE_MAP_ALL_ACCESS, 0, (externalId + "deadLinePosX").c_str());
    //     if (hMapDeadlineX == NULL) {
    //         hMapDeadlineX = CreateFileMapping(NULL, NULL, PAGE_READWRITE, 0, 0X1000, (externalId + "deadLinePosX").c_str());
    //     }
    //     pBufferDeadlineX = MapViewOfFile(hMapDeadlineX, FILE_MAP_ALL_ACCESS, 0, 0, 0);

    //     LPVOID pBufferDeadlineY;
    //     HANDLE hMapDeadlineY = NULL;
    //     hMapDeadlineY = OpenFileMapping(FILE_MAP_ALL_ACCESS, 0, (externalId + "deadLinePosY").c_str());
    //     if (hMapDeadlineY == NULL) {
    //         hMapDeadlineY = CreateFileMapping(NULL, NULL, PAGE_READWRITE, 0, 0X1000, (externalId + "deadLinePosY").c_str());
    //     }
    //     pBufferDeadlineY = MapViewOfFile(hMapDeadlineY, FILE_MAP_ALL_ACCESS, 0, 0, 0);

    //     std::string command = "D:\\scoop\\apps\\python38\\current\\python.exe vehDecision.py 123 " + externalId + " " + roadId + " " + RSUinfo + " " + toString(curPosition) + " " + toString(task->getCPU());
    //     std::cout << "command " << command << std::endl;
    //     int result = system(command.c_str());
    //     if ((char*)pBufferDeadlineX != NULL){
    //         // (char*)pBufferDeadline[1:-1];
    //         task->setDeadlinePos(Coord(strtod((char*)pBufferDeadlineX, NULL), strtod((char*)pBufferDeadlineY, NULL), 0));
    //         ifSend++;
    //     }
    //     task->setName((char*)pBuffer);
    //     task->setPossibleRSUs((char*)pBufferDecision);
    //     scheduleAt(simTime() + uniform(0.01, 0.2), task->dup());
    // }
    DemoBaseApplLayer::handlePositionUpdate(obj);
    // stopped for for at least 10s?
    if (mobility->getSpeed() < 5) {
        if (simTime() - lastDroveAt >= 10 && sentMessage == false) {
            findHost()->getDisplayString().setTagArg("i", 1, "red");
            sentMessage = true;

            TraCIDemo11pMessage* wsm = new TraCIDemo11pMessage();
            populateWSM(wsm);
            wsm->setDemoData(mobility->getRoadId().c_str());

            // host is standing still due to crash
            if (dataOnSch) {
                startService(Channel::sch2, 42, "Traffic Information Service");
                // started service and server advertising, schedule message to self to send later
                scheduleAt(computeAsynchronousSendingTime(1, ChannelType::service), wsm);
            }
            else {
                // send right away on CCH, because channel switching is disabled
                sendDown(wsm);
            }
        }
    }
    else {
        lastDroveAt = simTime();
    }
}

int possion()
{
    int Lambda = 20, k = 0;
    long double p = 1.0;
    long double l=exp(-Lambda);
    while (p>=l)
    {
        double u = U_Random();
        p *= u;
        k++;
    }
    return k-1;
}

double U_Random()
{
    double f;
    f = (float)(rand() % 100);
    return f/100;
}

void TraCIDemo11p::onWSA(DemoServiceAdvertisment* wsa)
{
    if (currentSubscribedServiceId == -1) {
        mac->changeServiceChannel(static_cast<Channel>(wsa->getTargetChannel()));
        currentSubscribedServiceId = wsa->getPsid();
        if (currentOfferedServiceId != wsa->getPsid()) {
            stopService();
            startService(static_cast<Channel>(wsa->getTargetChannel()), wsa->getPsid(), "Mirrored Traffic Service");
        }
    }
}

void TraCIDemo11p::onWSM(BaseFrame1609_4* frame)
{
    TraCIDemo11pMessage* wsm = check_and_cast<TraCIDemo11pMessage*>(frame);

    findHost()->getDisplayString().setTagArg("i", 1, "green");

    if (mobility->getRoadId()[0] != ':') traciVehicle->changeRoute(wsm->getDemoData(), 9999);
    if (!sentMessage) {
        sentMessage = true;
        // repeat the received traffic update once in 2 seconds plus some random delay
        wsm->setSenderAddress(myId);
        wsm->setSerial(3);
        scheduleAt(simTime() + 2 + uniform(0.01, 0.2), wsm->dup());
    }
}

