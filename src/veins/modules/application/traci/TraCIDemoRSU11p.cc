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
double U_Random();
int possion();
int maxRate = 60;

template<typename T> std::string toString(const T& t) {
    std::ostringstream oss;
    oss << t;
    return oss.str();
}

void TraCIDemoRSU11p::initialize(int stage)
{
    DemoBaseApplLayer::initialize(stage);
    if (stage == 1) {
        ReportMessage* rm = new ReportMessage();
        // adjust cpu core number by annotate the following
        cpu = U_Random() + 1;
        taskWait1 = 0;
        taskWait2 = 0;
        taskWait3 = 0;
        taskWait4 = 0;

        mem = U_Random() * 81920 + 71680;
        wait = 0;
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
    // std::cout << "here3 " << newTask->getTarget() << " " << curPosition  << std::endl;
    char *decision = "";
    
    // char *decision = new char[strlen(newTask->getDecision())+1];
    // strcpy(decision, newTask->getDecision());
    std::string taskName = newTask->getName();
    char *possibleRSUs = new char[strlen(newTask->getPossibleRSUs())+1];
    strcpy(possibleRSUs, newTask->getPossibleRSUs());
    char delims[] = ";";
    char *result = NULL;
    std::string rsuInfo = "";
    result = strtok( possibleRSUs, delims );
    if (toString(newTask->getTarget()) == toString(curPosition)){
        // proxyMode
        std::string externalId = newTask->getExternalId();
        while(result != NULL){
            std::ifstream infile("rsus.csv");
            std::string line;
            while (getline(infile, line)){
                std::stringstream ss(line);
                std::string token;
                ss >> token;
                if(token == result){
                    rsuInfo += token + ":";
                    ss >> token;
                    rsuInfo += token + "*";
                    ss >> token;
                    rsuInfo += token + "*";
                    ss >> token;
                    rsuInfo += token + "*";
                    ss >> token;
                    rsuInfo += token + "*";
                    ss >> token;
                    rsuInfo += token + "*";
                    ss >> token;
                    rsuInfo += token + ";";
                    break;
                }
            }
            result = strtok( NULL, delims );
        }
        LPVOID decision;
        HANDLE hMapDecision = NULL;
        hMapDecision = OpenFileMapping(FILE_MAP_ALL_ACCESS, 0, (externalId + "decision").c_str());
        if (hMapDecision == NULL) {
            hMapDecision = CreateFileMapping(NULL, NULL, PAGE_READWRITE, 0, 0X1000, (externalId + "decision").c_str());
        }
        decision = MapViewOfFile(hMapDecision, FILE_MAP_ALL_ACCESS, 0, 0, 0);

        LPVOID relay;
        HANDLE hMapRelay = NULL;
        hMapRelay = OpenFileMapping(FILE_MAP_ALL_ACCESS, 0, (externalId + "relay").c_str());
        if (hMapRelay == NULL) {
            hMapRelay = CreateFileMapping(NULL, NULL, PAGE_READWRITE, 0, 0X1000, (externalId + "relay").c_str());
        }
        relay = MapViewOfFile(hMapRelay, FILE_MAP_ALL_ACCESS, 0, 0, 0);

        std::string command = "D:\\scoop\\apps\\python38\\current\\python.exe rsuDecision.py " + rsuInfo + " " + toString(newTask->getSenderPos()) + " " + toString(newTask->getDeadlinePos()) + " " + toString(newTask->getCPU()) + " " + toString(newTask->getMem()) + " " + externalId + " " + newTask->getRoadId() + " " + toString(curPosition);
        std::cout << "rsuInfo " << command << std::endl;
        int result = system(command.c_str());
        newTask->setTarget("");
        newTask->setDecision((char*)decision);
        newTask->setRelay((char*)relay);
        double distance = sqrt(pow(newTask->getSenderPos().x - curPosition.x, 2) + pow(newTask->getSenderPos().y - curPosition.y, 2));
        double transmissionTime = 0;
        if (5 * log2(1 + 2500 / distance) > maxRate){
            transmissionTime = newTask->getMem() * 8 / 1024 / maxRate;
        } else {
            transmissionTime = newTask->getMem() * 8 / 1024 / (5 * log2(1 + 2500 / distance));
        }
        std::cout << "transmissionTime " << transmissionTime << " decision " << (char*)decision << std::endl;
        newTask->setTransmissionTime(transmissionTime);
        scheduleAt(simTime() + transmissionTime, newTask->dup());
    }
    else if (toString(newTask->getTarget()) == "" && newTask->getSenderType() == 0){
        // receive proxied task
        // std::string relays = newTask->getRelay();
        std::string decision = newTask->getDecision();
        decision.erase(std::remove(decision.begin(), decision.end(), ' '), decision.end());
        char *tmpDecision = new char[strlen(decision.c_str())+1];
        strcpy(tmpDecision, decision.c_str());
        char delims[] = ";";
        char *tmpResult = NULL;
        tmpResult = strtok( tmpDecision, delims );
        tmpResult = strtok( NULL, delims );
        // std::cout << "here4 " << tmpDecision << " relays " << tmpResult << std::endl;
        if(toString(tmpDecision) == toString(curPosition)){
            // operate here
            double taskCpu = newTask->getCPU();
            double taskMem = newTask->getMem();
            double cpuTmp = cpu;
            double operationTime = taskCpu / cpuTmp;
            mem -= taskMem;
            // wait += operationTime;
            std::cout << "operate here " << curPosition << " , task name is " << taskName << " operation time " << operationTime << endl;
            newTask->setSenderType(1);
            newTask->setOperationTime(operationTime);
            switch(atoi(toString(tmpResult).c_str())){
                case 1 :
                    std::cout << "operate on core 1" << std::endl;
                    taskQueue1.push_back(taskName);
                    if(taskWait1 >= simTime()){
                        taskWait1 += operationTime;
                    }
                    else{
                        taskWait1 = simTime() + operationTime;
                    }
                    scheduleAt(taskWait1, newTask->dup());
                    break;
                case 2 :
                    std::cout << "operate on core 2" << std::endl;
                    taskQueue2.push_back(taskName);
                    if(taskWait2 >= simTime()){
                        taskWait2 += operationTime;
                    }
                    else{
                        taskWait2 = simTime() + operationTime;
                    }
                    scheduleAt(taskWait2, newTask->dup());
                    break;
                case 3 :
                    std::cout << "operate on core 3" << std::endl;
                    taskQueue3.push_back(taskName);
                    if(taskWait3 >= simTime()){
                        taskWait3 += operationTime;
                    }
                    else{
                        taskWait3 = simTime() + operationTime;
                    }
                    scheduleAt(taskWait3, newTask->dup());
                    break;
                case 4 :
                    std::cout << "operate on core 4" << std::endl;
                    taskQueue4.push_back(taskName);
                    if(taskWait4 >= simTime()){
                        taskWait1 += operationTime;
                    }
                    else{
                        taskWait4 = simTime() + operationTime;
                    }
                    scheduleAt(taskWait4, newTask->dup());
                    break;
            }
        }
        else if (toString(newTask->getRelay()) != ""){
            // parse relay
            char *relays = new char[strlen(newTask->getRelay())+1];
            strcpy(relays, newTask->getRelay());
            
            result = strtok(relays, delims);
            while( result != NULL ) {
                std::string tmpRelay = toString(result);
                tmpRelay.erase(std::remove(tmpRelay.begin(), tmpRelay.end(), ' '), tmpRelay.end());
                // std::cout << "here4 " << tmpRelay << " " << toString(curPosition) << std::endl;
                if(tmpRelay == toString(curPosition)){
                    //relay node
                    double relayTime = newTask->getMem() * 8 / 1024 / maxRate;
                    std::cout << "relay here " << curPosition << " time " << relayTime << std::endl;
                    scheduleAt(simTime() + relayTime, newTask->dup());
                    break;
                }
                result = strtok( NULL, delims );
            }
        }
    }
}

void TraCIDemoRSU11p::handleSelfMsg(cMessage* msg)
{
    if (ReportMessage* rm = dynamic_cast<ReportMessage*>(msg)){
        // std::cout << curPosition << std::endl;
        if(simTime() > taskWait1){
            taskWait1 = simTime();
        }
        if(simTime() > taskWait2){
            taskWait2 = simTime();
        }
        if(simTime() > taskWait3){
            taskWait3 = simTime();
        }
        if(simTime() > taskWait4){
            taskWait4 = simTime();
        }
        rm->setSenderAddress(myId);
        rm->setSenderPos(curPosition);
        rm->setCpu(cpu);
        rm->setMem(mem);
        rm->setWait(taskWait1 - simTime());
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
        std::string command = "D:\\scoop\\apps\\python38\\current\\python.exe RSUState.py " + toString(curPosition) + " \"";
        // multi-core state record
        for(auto const &i: taskQueue1) {
            command += i + ",";
        }
        for(auto const &i: taskQueue2) {
            command += i + ",";
        }
        for(auto const &i: taskQueue3) {
            command += i + ",";
        }
        for(auto const &i: taskQueue4) {
            command += i + ",";
        }
        command += + "\" " + toString(cpu) + " " + toString(mem) + " " + toString(taskWait1) + " " + toString(taskWait2) + " " + toString(taskWait3) + " " + toString(taskWait4) + " " + toString(simTime());
        int result = system(command.c_str());
    }
    else if (Task* newTask = dynamic_cast<Task*>(msg)) {
        std::string tmpDes = toString(newTask->getDecision());
        tmpDes.erase(std::remove(tmpDes.begin(), tmpDes.end(), ' '), tmpDes.end());
        char *tmpDesLocal = new char[strlen(tmpDes.c_str())+1];
        strcpy(tmpDesLocal, tmpDes.c_str());
        char delims[] = ";";
        char *tmpResultLocal = NULL;
        tmpResultLocal = strtok( tmpDesLocal, delims );
        tmpResultLocal = strtok( NULL, delims );
        if(newTask->getSenderType() == 1){
            // send back
            std::cout << "ready to send back" << std::endl;
            std::list<std::string>::iterator it;
            switch(atoi(toString(tmpResultLocal).c_str())){
                case 1 :
                    for(it = taskQueue1.begin(); it != taskQueue1.end(); ){
                        if (*it == newTask->getName()){
                            it = taskQueue1.erase(it);
                        } else {
                            ++it;
                        }
                    }
                    break;
                case 2 :
                    for(it = taskQueue2.begin(); it != taskQueue2.end(); ){
                        if (*it == newTask->getName()){
                            it = taskQueue2.erase(it);
                        } else {
                            ++it;
                        }
                    }
                    break;
                case 3 :
                    for(it = taskQueue3.begin(); it != taskQueue3.end(); ){
                        if (*it == newTask->getName()){
                            it = taskQueue3.erase(it);
                        } else {
                            ++it;
                        }
                    }
                    break;
                case 4 :
                    for(it = taskQueue4.begin(); it != taskQueue4.end(); ){
                        if (*it == newTask->getName()){
                            it = taskQueue4.erase(it);
                        } else {
                            ++it;
                        }
                    }
                    break;
            }
            bool flag = true;
            std::map<LAddress::L2Type, Coord>::iterator it2;
            for(it2 = NodePositions.begin(); it2 != NodePositions.end(); it2++) {
                std::cout << "scan veh " << it2->first << std::endl;
                if(it2->first == newTask->getSenderAddress()){
                    flag = false;
                    newTask->setSenderPos(curPosition);
                    double operationTime = newTask->getOperationTime();
                    double transmissionTime = newTask->getTransmissionTime();
                    double taskMem = newTask->getMem();
                    mem += taskMem;
                    std::cout << "number of task is " << taskQueue1.size() << " wait time is " << taskWait1 << std::endl;
                    sendDown(newTask->dup());
                }
            }
            if(flag){
                std::cout << "task fininshed, but vehicle not arrive" << std::endl;
                scheduleAt(simTime() + 1, newTask->dup());
            }
        }
        else if(toString(tmpDesLocal) == toString(curPosition)){
            // just operate on this rsu
            std::string taskName = newTask->getName();
            double taskCpu = newTask->getCPU();
            double taskMem = newTask->getMem();
            double cpuTmp = cpu;
            mem -= newTask->getMem();
            double operationTime = taskCpu / cpuTmp;
            // wait += operationTime;
            newTask->setSenderType(1);
            newTask->setSenderPos(curPosition);
            newTask->setOperationTime(operationTime);
            std::cout << "operate here locally, task name is " << taskName << ", operationTime " << operationTime << " taskWait " << taskWait1 << std::endl;
            switch(atoi(toString(tmpResultLocal).c_str())){
                case 1 :
                    std::cout << "operate on core 1" << std::endl;
                    taskQueue1.push_back(taskName);
                    if(taskWait1 >= simTime()){
                        taskWait1 += operationTime;
                    }
                    else{
                        taskWait1 = simTime() + operationTime;
                    }
                    scheduleAt(taskWait1, newTask->dup());
                    break;
                case 2 :
                    std::cout << "operate on core 2" << std::endl;
                    taskQueue2.push_back(taskName);
                    if(taskWait2 >= simTime()){
                        taskWait2 += operationTime;
                    }
                    else{
                        taskWait2 = simTime() + operationTime;
                    }
                    scheduleAt(taskWait2, newTask->dup());
                    break;
                case 3 :
                    std::cout << "operate on core 3" << std::endl;
                    taskQueue3.push_back(taskName);
                    if(taskWait3 >= simTime()){
                        taskWait3 += operationTime;
                    }
                    else{
                        taskWait3 = simTime() + operationTime;
                    }
                    scheduleAt(taskWait3, newTask->dup());
                    break;
                case 4 :
                    std::cout << "operate on core 4" << std::endl;
                    taskQueue4.push_back(taskName);
                    if(taskWait4 >= simTime()){
                        taskWait4 += operationTime;
                    }
                    else{
                        taskWait4 = simTime() + operationTime;
                    }
                    scheduleAt(taskWait4, newTask->dup());
                    break;
            }
            // std::cout << "here " << simTime() << " present position " << taskWait << std::endl;
        }
        else{
            sendDown(newTask->dup());
        }
    }
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

