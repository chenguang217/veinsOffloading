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
int maxTask = 1;
double generateP = 1 - 0.2;


template<typename T> std::string toString(const T& t) {
    std::ostringstream oss;
    oss<<t;
    return oss.str();
}

std::string trim1(std::string s){
    int index = 0;
    if( !s.empty()){
        while( (index = s.find(' ',index)) != std::string::npos){
            s.erase(index,1);
        }
    }
    return s;
}


void TraCIDemo11p::initialize(int stage)
{
    DemoBaseApplLayer::initialize(stage);
    if (stage == 0) {
        sentMessage = false;
        lastDroveAt = simTime();
        currentSubscribedServiceId = -1;
        srand( (unsigned)time( NULL ) );
        pc = PythonCommunication::getInstance();
    }
}

void TraCIDemo11p::onRM(ReportMessage* rm)
{
    if (rm->getSenderType() == 0) {
        lastReceiveAt = simTime();
        LAddress::L2Type sender = rm->getSenderAddress();
        Coord pos = rm->getSenderPos();
        long rsuCpu = rm->getCpu();
        std::string rsuMem = rm->getMem();
        simtime_t rsuWait = rm->getWait();
        ReportMessage* newRM = new ReportMessage();
        populateWSM(newRM);
        newRM->setSenderAddress(myId);
        newRM->setSenderType(1);
        std::string roadIdTmp = mobility->getRoadId();
        newRM->setVehRoad(roadIdTmp.c_str());
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
    }
}

void TraCIDemo11p::onTask(Task* frame)
{
    Task* newTask = check_and_cast<Task*>(frame);
    if (newTask->getSenderType() == 1 && newTask->getSenderAddress() == myId) {
        taskList[trim1(toString(newTask->getName()))] = taskList[trim1(toString(newTask->getName()))] + newTask->getRatio();
        std::cout << "task finished" << std::endl;
        std::ofstream outfile;
        outfile.open("taskLog/" + trim1(toString(newTask->getName())) + "_" + toString(taskList[trim1(toString(newTask->getName()))]) + ".json");
        outfile << "{" << std::endl << "\t\"position\": \"" << toString(curPosition) << "\"," << std::endl;
        outfile << "\t\"deadLinePos\": \"" << toString(newTask->getDeadlinePos()) << "\"," << std::endl;
        outfile << "\t\"servicePos\": \"" << toString(newTask->getSenderPos()) << "\"," << std::endl;
        outfile << "\t\"cpu\": \"" << toString(newTask->getCPU()) << "\"," << std::endl;
        outfile << "\t\"mem\": \"" << toString(newTask->getMem()) << "\"," << std::endl;
        outfile << "\t\"operationTime\": \"" << toString(newTask->getOperationTime()) << "\"," << std::endl;
        outfile << "\t\"serviceMode\": \"0\"," << std::endl;
        outfile << "\t\"ratio\": \"" << toString(newTask->getRatio()) << "\"" << std::endl;
        outfile << "}";

        std::map<std::string, double>::const_iterator iteMap = taskList.begin();
        for(; iteMap != taskList.end(); ++ iteMap)
        {
            std::cout << iteMap->first;
            std::cout << ":";
            std::cout << iteMap->second << endl;
            if(iteMap->second >= 0.9999){
                std::cout << "this finished" << endl;
                taskList.erase(iteMap);
                std::ofstream outfile;
                outfile.open("taskLog/" + trim1(toString(newTask->getName())) + ".json");
                outfile << "success" << std::endl;
                break;
            }
        }
    }
    else if (newTask->getSenderType() == 2 && newTask->getSenderAddress() == myId) {
        taskList[trim1(toString(newTask->getName()))] = taskList[trim1(toString(newTask->getName()))] + newTask->getRatio();
        std::cout << "task finished" << std::endl;
        std::ofstream outfile;
        outfile.open("taskLog/" + trim1(toString(newTask->getName())) + "_" + toString(taskList[trim1(toString(newTask->getName()))]) + ".json");
        outfile << "{" << std::endl << "\t\"position\": \"" << toString(curPosition) << "\"," << std::endl;
        outfile << "\t\"deadLinePos\": \"" << toString(newTask->getDeadlinePos()) << "\"," << std::endl;
        outfile << "\t\"servicePos\": \"" << toString(newTask->getSenderPos()) << "\"," << std::endl;
        outfile << "\t\"cpu\": \"" << toString(newTask->getCPU()) << "\"," << std::endl;
        outfile << "\t\"mem\": \"" << toString(newTask->getMem()) << "\"," << std::endl;
        outfile << "\t\"operationTime\": \"" << toString(newTask->getOperationTime()) << "\"," << std::endl;
        outfile << "\t\"serviceMode\": \"" << trim1(toString(newTask->getService())) << "\"," << std::endl;
        outfile << "\t\"ratio\": \"" << toString(newTask->getRatio()) << "\"" << std::endl;
        outfile << "}";

        std::map<std::string, double>::const_iterator iteMap = taskList.begin();
        for(; iteMap != taskList.end(); ++ iteMap)
        {
            std::cout << iteMap->first;
            std::cout << ":";
            std::cout << iteMap->second << endl;
            if(iteMap->second >= 0.9999){
                std::cout << "this finished" << endl;
                taskList.erase(iteMap);
                std::ofstream outfile;
                outfile.open("taskLog/" + trim1(toString(newTask->getName())) + ".json");
                outfile << "success" << std::endl;
                break;
            }
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
    // pc->loadMoudle("test1");
    // PythonCommunication::PythonParam pp;
    // pp.set("add1", 3);
    // pp.set("add2", 5);
    // PythonCommunication::PythonParam *ppr = pc->call("sumTest", &pp);
    // int result1 = ppr->getInt("sumResult");
    // std::cout << "python call " << result1 << std::endl;

    
    std::map<LAddress::L2Type, simtime_t>::iterator it;
    std::map<LAddress::L2Type, Coord>::iterator itCoord=RSUPositions.begin();
    std::map<LAddress::L2Type, long>::iterator itCpu=RSUcpus.begin();
    std::map<LAddress::L2Type, std::string>::iterator itMem=RSUmems.begin();
    std::map<LAddress::L2Type, simtime_t>::iterator itWait=RSUwaits.begin();
    std::string roadIdTmp = mobility->getRoadId();
    std::string externalId = mobility->getExternalId();
    // std::ofstream outfile;
    // outfile.open("simTime/" + externalId + ".log", std::ios::app);
    // outfile << roadIdTmp << "," << toString(simTime()) << std::endl;
    for(it = connectedRSUs.begin(), itCoord = RSUPositions.begin(), itCpu = RSUcpus.begin(), itMem = RSUmems.begin(), itWait = RSUwaits.begin(); it != connectedRSUs.end() && itCoord != RSUPositions.end() && itCpu != RSUcpus.end() && itMem != RSUmems.end() && itWait != RSUwaits.end(); it++, itCoord++, itCpu++, itMem++, itWait++) {
        if (simTime() - it->second >= 10) {
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
        // itCoord++;
        // itCpu++;
        // itMem++;
        // itWait++;
    }
    // std::cout << "position x " << curPosition.x << " if equal " << std::endl;
    if (lastroadID == ""){
        lastroadID = mobility->getRoadId();
        // std::cout << myId << " is at " << lastroadID << std::endl;
    }
    if (startflag == 1 && startgenerate == 0){
    // if (startflag == 1){
        double gratio = U_Random();
        // std::cout << "the gratio is" << gratio << std::endl;
        if (gratio > generateP){
            startgenerate = 1;
        }
        else{
            startflag = 0;
        }
    }
    if (mobility->getRoadId() != lastroadID || startflag == 1){
        lastroadID = mobility->getRoadId();
        if (lastroadID.at(0) != ':'){
            std::cout << mobility->getExternalId() << " is at " << lastroadID << std::endl;
            // random generate tasks
            if ((U_Random() > generateP || startflag == 1) && ifSend < maxTask && RSUPositions.size() > 0){
                // generate Task Message
                std::string externalId = mobility->getExternalId();
                std::string roadId = mobility->getRoadId();
                std::cout<< "generate tasks" << std::endl;
                Task* task = new Task();
                populateWSM(task);
                task->setSenderAddress(myId);
                // task->setCPU(20);
                task->setMem(U_Random() * 5120 + 5120);
                task->setCPU(task->getMem() * 0.000008 * 1024);
                task->setStorage(task->getMem());
                task->setExternalId(externalId.c_str());
                task->setRoadId(roadId.c_str());
                task->setSenderType(0);

                // generate deadline road
                std::list<std::string> roadList = mobility->getVehicleCommandInterface()->getPlannedRoadIds();
                std::list<std::string>::iterator itRoad = roadList.begin();
                std::string roads = "";
                for(int i = 0; i < roadList.size(); i++){
                    roads = roads + *itRoad + ";";
                    itRoad++;
                }
                task->setRoads(roads.c_str());

                std::map<LAddress::L2Type, Coord>::iterator it = RSUPositions.begin();
                task->setTarget(toString(it->second).c_str());
                double minDist = 1000;
                for(it = RSUPositions.begin(); it != RSUPositions.end(); it++) {
                    double tmpDist = it->second.distance(curPosition);
                    // std::cout << it->second << std::endl;
                    if(tmpDist <= minDist){
                        minDist = tmpDist;
                        // std::cout << minDist << std::endl;
                        task->setTarget(toString(it->second).c_str());
                    }
                }
                // LPVOID pBuffer;
                // HANDLE hMap = NULL;
                // hMap = OpenFileMapping(FILE_MAP_ALL_ACCESS, 0, (externalId + "global_share_memory").c_str());
                // if (hMap == NULL) {
                //     hMap = CreateFileMapping(NULL, NULL, PAGE_READWRITE, 0, 0X1000, (externalId + "global_share_memory").c_str());
                // }
                // pBuffer = MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, 0);

                // LPVOID pBufferDecision;
                // HANDLE hMapDecision = NULL;
                // hMapDecision = OpenFileMapping(FILE_MAP_ALL_ACCESS, 0, (externalId + "possibleRSU").c_str());
                // if (hMapDecision == NULL) {
                //     hMapDecision = CreateFileMapping(NULL, NULL, PAGE_READWRITE, 0, 0X1000, (externalId + "possibleRSU").c_str());
                // }
                // pBufferDecision = MapViewOfFile(hMapDecision, FILE_MAP_ALL_ACCESS, 0, 0, 0);

                // LPVOID pBufferDeadlineX;
                // HANDLE hMapDeadlineX = NULL;
                // hMapDeadlineX = OpenFileMapping(FILE_MAP_ALL_ACCESS, 0, (externalId + "deadLinePosX").c_str());
                // if (hMapDeadlineX == NULL) {
                //     hMapDeadlineX = CreateFileMapping(NULL, NULL, PAGE_READWRITE, 0, 0X1000, (externalId + "deadLinePosX").c_str());
                // }
                // pBufferDeadlineX = MapViewOfFile(hMapDeadlineX, FILE_MAP_ALL_ACCESS, 0, 0, 0);

                // LPVOID pBufferDeadlineY;
                // HANDLE hMapDeadlineY = NULL;
                // hMapDeadlineY = OpenFileMapping(FILE_MAP_ALL_ACCESS, 0, (externalId + "deadLinePosY").c_str());
                // if (hMapDeadlineY == NULL) {
                //     hMapDeadlineY = CreateFileMapping(NULL, NULL, PAGE_READWRITE, 0, 0X1000, (externalId + "deadLinePosY").c_str());
                // }
                // pBufferDeadlineY = MapViewOfFile(hMapDeadlineY, FILE_MAP_ALL_ACCESS, 0, 0, 0);

                // std::string command = "D:\\scoop\\apps\\python38\\current\\python.exe vehDecision.py 123 " + externalId + " " + roadId + " " + toString(curPosition) + " " + toString(task->getCPU());
                // std::cout << "command " << command << std::endl;
                // int result = system(command.c_str());
                // if ((char*)pBufferDeadlineX != NULL && result == 0){
                //     task->setDeadlinePos(Coord(strtod((char*)pBufferDeadlineX, NULL), strtod((char*)pBufferDeadlineY, NULL), 0));
                //     task->setName((char*)pBuffer);
                //     task->setPossibleRSUs((char*)pBufferDecision);
                //     scheduleAt(simTime(), task->dup());
                //     ifSend++;
                // }
                // if (result == 0 && startflag == 1){
                //     startflag = 0;
                // }

                pc->setModuleName("vehDecisionCall");
                PythonCommunication::PythonParam pp;
                pp.set("externalID", externalId);
                pp.set("road", roadId);
                pp.set("position", toString(curPosition));
                pp.set("cpuRequirement", task->getMem() * 0.000008 * 1024);
                PythonCommunication::PythonParam *ppr = pc->call("vehDecision", &pp);
                std::string name;
                if(ppr == nullptr){
                    std::cout << "python call error" << std::endl;
                    name = "1";
                }
                else{
                    name = ppr->getString("name");
                }
                ifSend++;
                if (name == "1"){
                    std::cout << "deadline selection error or eta not exist" << std::endl;
                }
                else {
                    double deadLinePosX = ppr->getDouble("deadLinePosX");
                    double deadLinePosY = ppr->getDouble("deadLinePosY");
                    std::string possibleRSU = ppr->getString("possibleRSU");
                    task->setDeadlinePos(Coord(deadLinePosX, deadLinePosY, 0));
                    task->setName(name.c_str());
                    task->setPossibleRSUs(possibleRSU.c_str());
                    scheduleAt(simTime() + uniform(0.01, 0.2), task->dup());
                }
                if (startflag == 1){
                    startflag = 0;
                }
            }
        }
    }
    DemoBaseApplLayer::handlePositionUpdate(obj);
    lastDroveAt = simTime();
    // stopped for for at least 10s?
    // if (mobility->getSpeed() < 5) {
    //     if (simTime() - lastDroveAt >= 10 && sentMessage == false) {
    //         // findHost()->getDisplayString().setTagArg("i", 1, "red");
    //         sentMessage = true;

    //         // TraCIDemo11pMessage* wsm = new TraCIDemo11pMessage();
    //         // populateWSM(wsm);
    //         // wsm->setDemoData(mobility->getRoadId().c_str());

    //         // // host is standing still due to crash
    //         // if (dataOnSch) {
    //         //     startService(Channel::sch2, 42, "Traffic Information Service");
    //         //     // started service and server advertising, schedule message to self to send later
    //         //     scheduleAt(computeAsynchronousSendingTime(1, ChannelType::service), wsm);
    //         // }
    //         // else {
    //         //     // send right away on CCH, because channel switching is disabled
    //         //     sendDown(wsm);
    //         // }
    //     }
    // }
    // else {
    //     lastDroveAt = simTime();
    // }
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

