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
int maxRate = 30;

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
        cpu = U_Random() * 8 + 2;
        mem = U_Random() * 9000 + 3000;
        wait = 0;
        populateWSM(rm);
        rm->setSenderAddress(myId);
        rm->setSenderPos(curPosition);
        rm->setSenderType(0);
        scheduleAt(simTime() + uniform(0.01, 0.2), rm);
        taskWait = 0;
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
        if (5 * log2(1 + 250 / distance) > 30){
            transmissionTime = newTask->getMem() * 8 / 1000 / 30;
        } else {
            transmissionTime = newTask->getMem() * 8 / 1000 / (5 * log2(1 + 250 / distance));
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
        // std::cout << "here4 " << decision << " relays" << std::endl;
        if(decision == toString(curPosition)){
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
            taskQueue.push_back(taskName);
            taskWait += operationTime;
            scheduleAt(taskWait, newTask->dup());
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
                    double relayTime = newTask->getMem() * 8 / 1000 / maxRate;
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
        if(simTime() > taskWait){
            taskWait = simTime();
        }
        rm->setSenderAddress(myId);
        rm->setSenderPos(curPosition);
        rm->setCpu(cpu);
        rm->setMem(mem);
        rm->setWait(taskWait - simTime());
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
        for(auto const &i: taskQueue) {
            command += i + ",";
        }
        command += + "\" " + toString(cpu) + " " + toString(mem) + " " + toString(taskWait) + " " + toString(simTime());
        int result = system(command.c_str());
    }
    else if (Task* newTask = dynamic_cast<Task*>(msg)) {
        std::string tmpDes = toString(newTask->getDecision());
        tmpDes.erase(std::remove(tmpDes.begin(), tmpDes.end(), ' '), tmpDes.end());
        if(newTask->getSenderType() == 1){
            // send back
            std::cout << "ready to send back" << std::endl;
            std::list<std::string>::iterator it;
            for(it = taskQueue.begin(); it != taskQueue.end(); ){
                if (*it == newTask->getName()){
                    it = taskQueue.erase(it);
                } else {
                    ++it;
                }
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
                    std::cout << "number of task is " << taskQueue.size() << " wait time is " << taskWait << std::endl;
                    sendDown(newTask->dup());
                }
            }
            if(flag){
                std::cout << "task fininshed, but vehicle not arrive" << std::endl;
                scheduleAt(simTime() + 1, newTask->dup());
            }
        }
        else if(tmpDes == toString(curPosition)){
            // just operate on this rsu
            std::string taskName = newTask->getName();
            std::cout << "operate here locally, task name is " << taskName << std::endl;
            double taskCpu = newTask->getCPU();
            double taskMem = newTask->getMem();
            double cpuTmp = cpu;
            mem -= newTask->getMem();
            double operationTime = taskCpu / cpuTmp;
            // wait += operationTime;
            newTask->setSenderType(1);
            newTask->setSenderPos(curPosition);
            newTask->setOperationTime(operationTime);
            taskQueue.push_back(taskName);
            std::cout << "operationTime " << operationTime << " taskWait " << taskWait << std::endl;
            if(simTime() > taskWait){
                taskWait = simTime() + operationTime;
            }
            else{
                taskWait += operationTime;
            }
            // std::cout << "here " << simTime() << " present position " << taskWait << std::endl;
            scheduleAt(taskWait, newTask);
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

