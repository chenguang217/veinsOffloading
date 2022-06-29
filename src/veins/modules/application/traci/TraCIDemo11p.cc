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
    std::ostringstream oss;      //´´½¨Ò»¸ö¸ñÊ½»¯Êä³öÁ÷
    oss<<t;             //°ÑÖµ´«µÝµ½Á÷ÖÐ
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
//    if (stage == 1) {
//        srand((unsigned)time(NULL));
//        int randNum = (rand()%10);
//        std::cout<< randNum << std::endl;
//        if (randNum > 5){
//            LPVOID pBuffer;
//            std::string strMapName("global_share_memory");
//            HANDLE hMap = NULL;
//
//            hMap = OpenFileMapping(FILE_MAP_ALL_ACCESS, 0, "global_share_memory");
//            if (hMap == NULL) {
//                hMap = CreateFileMapping(NULL, NULL, PAGE_READWRITE, 0, 0X1000, "global_share_memory");
//            }
//            pBuffer = MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, 0);
//            int result = system("D:\\scoop\\apps\\python38\\current\\python.exe F:\\test.py 123");
//            DemoBaseApplLayer::handlePositionUpdate(obj);
//            if ((char*)pBuffer != NULL)EV_DEBUG << "Calling Python2 " << (char*)pBuffer << std::endl;
//        }
//        scheduleAt(simTime() + uniform(0.01, 0.2), rm);
//    }
}

void TraCIDemo11p::onRM(ReportMessage* rm)
{
    if (rm->getSenderType() == 0) {
        lastReceiveAt = simTime();
        LAddress::L2Type sender = rm->getSenderAddress();
        Coord pos = rm->getSenderPos();
        long rsuCpu = rm->getCpu();
        long rsuMem = rm->getMem();
        double rsuWait = rm->getWait();
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
        std::cout << "Vehicle " << myId << " find RSU "<< sender << " at position " << pos << ", now have connectedRSUs " << connectedRSUs.size() << " RSUPositions " << RSUPositions.size() << std::endl;
        std::map<LAddress::L2Type, Coord>::iterator itCoord=RSUPositions.begin();
        for(it = connectedRSUs.begin(); it != connectedRSUs.end(); it++) {
            std::cout << "Vehicle " << myId << " now have RSU" << it->first << " at " << itCoord->second << std::endl;
            itCoord++;
        }
    }
}

void TraCIDemo11p::onTask(Task* frame)
{
    Task* newTask = check_and_cast<Task*>(frame);
    if (newTask->getSenderType() == 1) {
        std::cout << "task finished" << std::endl;
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
    std::map<LAddress::L2Type, long>::iterator itWait=RSUwaits.begin();
    for(it = connectedRSUs.begin(); it != connectedRSUs.end(); it++) {
        if (simTime() - it->second >= 5) {
            std::cout << "RSU " << it->first << " at " << itCoord->first << " didn't response in " << simTime() - it->second << " seconds" << std::endl;
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
    if (U_Random() > 0.5){
        // generate Task Message
        std::string externalId = mobility->getExternalId();
        std::string roadId = mobility->getRoadId();
        std::cout<< "generate tasks" << std::endl;
        Task* task = new Task();
        populateWSM(task);
        task->setSenderAddress(myId);
        task->setCPU(U_Random() * 28 + 2);
        task->setMem(U_Random() * 900 + 300);
        task->setStorage(U_Random() * 900 + 300);

        std::string tmp = "";
        std::map<LAddress::L2Type, Coord>::iterator it;
        // ****add cpu mem wait in decision string
        std::map<LAddress::L2Type, long>::iterator itCpu = RSUcpus.begin();
        std::map<LAddress::L2Type, long>::iterator itMem = RSUmems.begin();
        std::map<LAddress::L2Type, long>::iterator itWait = RSUwaits.begin();
        std::string RSUinfo = "";
        for(it = RSUPositions.begin(); it != RSUPositions.end(); it++) {
            RSUinfo = RSUinfo + toString(itCpu->first) + ":" + toString(it->second) + "*" + toString(itCpu->second) + "*" + toString(itMem->second) + "*" + toString(itWait->second) + ";";
            itCpu++;
            itMem++;
            itWait++;
            // if (it == RSUPositions.begin()) {
            //     tmp = tmp + toString(it->first) + "|" + toString(it->second) + ":true;";
            // }
            // else {
            //     tmp = tmp + toString(it->first) + "|" + toString(it->second) + ":false;";
            // }
        }
        // task->setDecision((char*)tmp.data());
        std::cout << "decisions " << tmp << std::endl;
        std::cout << "RSUInfo " << RSUinfo << std::endl;
        LPVOID pBuffer;
        std::string strMapName("global_share_memory");
        HANDLE hMap = NULL;

        hMap = OpenFileMapping(FILE_MAP_ALL_ACCESS, 0, "global_share_memory");
        if (hMap == NULL) {
            hMap = CreateFileMapping(NULL, NULL, PAGE_READWRITE, 0, 0X1000, "global_share_memory");
        }
        pBuffer = MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, 0);

        LPVOID pBufferDecision;
        HANDLE hMapDecision = NULL;

        hMapDecision = OpenFileMapping(FILE_MAP_ALL_ACCESS, 0, "decision");
        if (hMapDecision == NULL) {
            hMapDecision = CreateFileMapping(NULL, NULL, PAGE_READWRITE, 0, 0X1000, "decision");
        }
        pBufferDecision = MapViewOfFile(hMapDecision, FILE_MAP_ALL_ACCESS, 0, 0, 0);

        std::string command = "D:\\scoop\\apps\\python38\\current\\python.exe test.py 123 " + externalId + " " + roadId + " " + RSUinfo + " " + toString(curPosition);
        std::cout << "command " << command << std::endl;
        int result = system(command.c_str());
        if ((char*)pBuffer != NULL){
            std::cout << "Calling Python2 " << (char*)pBuffer << std::endl;
        }
        if ((char*)pBufferDecision != NULL){
            std::cout << "Calling Python3 " << (char*)pBufferDecision << std::endl;
        }
        task->setName((char*)pBuffer);
        task->setDecision((char*)pBufferDecision);
        scheduleAt(simTime() + uniform(0.01, 0.2), task->dup());
    }
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

int possion()  /* ï¿½ï¿½ï¿½ï¿½Ò»ï¿½ï¿½ï¿½ï¿½ï¿½É·Ö²ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½LamdaÎªï¿½ï¿½ï¿½ï¿½Æ½ï¿½ï¿½ï¿½ï¿½*/
{
    int Lambda = 20, k = 0;
    long double p = 1.0;
    long double l=exp(-Lambda);  /* Îªï¿½Ë¾ï¿½ï¿½È£ï¿½ï¿½Å¶ï¿½ï¿½ï¿½Îªlong doubleï¿½Ä£ï¿½exp(-Lambda)ï¿½Ç½Ó½ï¿½0ï¿½ï¿½Ð¡ï¿½ï¿½*/
    // printf("%.15Lfn",l);
    while (p>=l)
    {
        double u = U_Random();
        p *= u;
        k++;
    }
    return k-1;
}

double U_Random()   /* ï¿½ï¿½ï¿½ï¿½Ò»ï¿½ï¿½0~1Ö®ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ï¿½ */
{
    double f;
    f = (float)(rand() % 100);
    /* printf("%fn",f); */
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

