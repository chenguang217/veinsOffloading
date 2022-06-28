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
    std::ostringstream oss;      //创建一个格式化输出流
    oss<<t;             //把值传递到流中
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
        }
        std::cout << "Vehicle " << myId << " find RSU "<< sender << " at position " << pos << ", now have connectedRSUs " << connectedRSUs.size() << " RSUPositions " << RSUPositions.size() << std::endl;
        std::map<LAddress::L2Type, Coord>::iterator itCoord=RSUPositions.begin();
        for(it = connectedRSUs.begin(); it != connectedRSUs.end(); it++) {
            std::cout << "Vehicle " << myId << " now have RSU" << it->first << " at " << itCoord->second << std::endl;
            itCoord++;
        }
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
    for(it = connectedRSUs.begin(); it != connectedRSUs.end(); it++) {
        if (simTime() - it->second >= 5) {
            std::cout << "RSU " << it->first << " at " << itCoord->first << " didn't response in " << simTime() - it->second << " seconds" << std::endl;
            connectedRSUs.erase(it++);
            RSUPositions.erase(itCoord++);
            if (it == connectedRSUs.end()) {
                break;
            }
        }
        itCoord++;
    }
    if (U_Random() > 0.5){
        // generate Task Message
        std::cout<< "ExternalId" << mobility->getExternalId().c_str() << std::endl;
        std::cout<< "generate tasks" << std::endl;
        Task* task = new Task();
        populateWSM(task);
        task->setSenderAddress(myId);
        task->setCPU(U_Random() * 28 + 2);
        task->setMem(U_Random() * 900 + 300);
        task->setStorage(U_Random() * 900 + 300);

        std::string tmp = "";
        std::map<LAddress::L2Type, Coord>::iterator it;
        for(it = RSUPositions.begin(); it != RSUPositions.end(); it++) {
            if (it == RSUPositions.begin()) {
                tmp = tmp + toString(it->first) + "|" + toString(it->second) + ":true;";
            }
            else {
                tmp = tmp + toString(it->first) + "|" + toString(it->second) + ":false;";
            }
        }
        std::cout << "decisions " << tmp << std::endl;
        task->setDecision((char*)tmp.data());
        LPVOID pBuffer;
        std::string strMapName("global_share_memory");
        HANDLE hMap = NULL;

        hMap = OpenFileMapping(FILE_MAP_ALL_ACCESS, 0, "global_share_memory");
        if (hMap == NULL) {
            hMap = CreateFileMapping(NULL, NULL, PAGE_READWRITE, 0, 0X1000, "global_share_memory");
        }
        pBuffer = MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, 0);
        int result = system("D:\\scoop\\apps\\python38\\current\\python.exe test.py 123");
        if ((char*)pBuffer != NULL){
            std::cout << "Calling Python2 " << (char*)pBuffer << std::endl;
        }
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

int possion()  /* 产生一个泊松分布的随机数，Lamda为总体平均数*/
{
    int Lambda = 20, k = 0;
    long double p = 1.0;
    long double l=exp(-Lambda);  /* 为了精度，才定义为long double的，exp(-Lambda)是接近0的小数*/
    // printf("%.15Lfn",l);
    while (p>=l)
    {
        double u = U_Random();
        p *= u;
        k++;
    }
    return k-1;
}

double U_Random()   /* 产生一个0~1之间的随机数 */
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

