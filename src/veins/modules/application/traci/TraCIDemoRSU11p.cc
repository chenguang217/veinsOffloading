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

std::string trim(std::string s){
    int index = 0;
    if( !s.empty()){
        while( (index = s.find(' ',index)) != std::string::npos){
            s.erase(index,1);
        }
    }
    return s;
}

std::vector<std::string> split(const std::string& str, const std::string& delim) {
	std::vector<std::string> res;
	if("" == str) return res;
	//先将要切割的字符串从string类型转换为char*类型
	char * strs = new char[str.length() + 1] ; //不要忘了
	strcpy(strs, str.c_str()); 
 
	char * d = new char[delim.length() + 1];
	strcpy(d, delim.c_str());
 
	char *p = strtok(strs, d);
	while(p) {
		std::string s = p; //分割得到的字符串转换为string类型
		res.push_back(s); //存入结果数组
		p = strtok(NULL, d);
	}
 
	return res;
}

void TraCIDemoRSU11p::csv2variables(){
    std::ifstream infile("rsus.csv");
    if(!infile){
        std::cout << "open file error" << std::endl;
    }
    std::string line;
    while (getline(infile, line)){
        std::stringstream ss(line);
        std::string token;
        double memTmp;
        double tmpWait;
        ss >> token;
        if(token == toString(curPosition)){
            ss >> token;
            taskQueue1.clear();
            taskQueue2.clear();
            taskQueue3.clear();
            taskQueue4.clear();
            std::vector<std::string> tmpQueue;
            if(token != "*"){
                tmpQueue = split(token, ";");
                for(int i = 0; i < tmpQueue.size(); i++){
                    std::vector<std::string> nameState = split(tmpQueue[i].substr(0, tmpQueue[i].length() - 1), "(");
                    taskQueue1.push_back(nameState);
                }
            }
            ss >> token;
            if(token != "*"){
                tmpQueue = split(token, ";");
                for(int i = 0; i < tmpQueue.size(); i++){
                    std::vector<std::string> nameState = split(tmpQueue[i].substr(0, tmpQueue[i].length() - 1), "(");
                    taskQueue2.push_back(nameState);
                }
            }
            ss >> token;
            if(token != "*"){
                tmpQueue = split(token, ";");
                for(int i = 0; i < tmpQueue.size(); i++){
                    std::vector<std::string> nameState = split(tmpQueue[i].substr(0, tmpQueue[i].length() - 1), "(");
                    taskQueue3.push_back(nameState);
                }
            }
            ss >> token;
            if(token != "*"){
                tmpQueue = split(token, ";");
                for(int i = 0; i < tmpQueue.size(); i++){
                    std::vector<std::string> nameState = split(tmpQueue[i].substr(0, tmpQueue[i].length() - 1), "(");
                    taskQueue4.push_back(nameState);
                }
            }
            ss >> token;
            ss >> memTmp;
            mem1 = memTmp;
            ss >> memTmp;
            mem2 = memTmp;
            ss >> memTmp;
            mem3 = memTmp;
            ss >> memTmp;
            mem4 = memTmp;
            ss >> tmpWait;
            if(simTime() < tmpWait){
                taskWait1 = tmpWait;
            }
            else{
                taskWait1 = simTime();
            }
            ss >> tmpWait;
            if(simTime() < tmpWait){
                taskWait2 = tmpWait;
            }
            else{
                taskWait2 = simTime();
            }
            ss >> tmpWait;
            if(simTime() < tmpWait){
                taskWait3 = tmpWait;
            }
            else{
                taskWait3 = simTime();
            }
            ss >> tmpWait;
            if(simTime() < tmpWait){
                taskWait4 = tmpWait;
            }
            else{
                taskWait4 = simTime();
            }
        }
    }
}

void TraCIDemoRSU11p::variables2csv(){
    std::ifstream in("rsus.csv");
    std::string strFileData = "";
    std::string tmpLineData = "";
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
    while (std::getline(in, tmpLineData)){
        std::vector<std::string> tmpString = split(tmpLineData, " ");
        // std::cout << tmpString[0] << " " << toString(curPosition) << std::endl;
        if (toString(curPosition) == tmpString[0]){
            strFileData += tmpString[0] + " ";
            if(taskQueue1.size() == 0){
                strFileData += "*";
            }
            for(int i = 0; i < taskQueue1.size(); i++){
                strFileData += taskQueue1[i][0] + "(" + taskQueue1[i][1] + ");";
            }
            strFileData += " ";
            if(taskQueue2.size() == 0){
                strFileData += "*";
            }
            for(int i = 0; i < taskQueue2.size(); i++){
                strFileData += taskQueue2[i][0] + "(" + taskQueue2[i][1] + ");";
            }
            strFileData += " ";
            if(taskQueue3.size() == 0){
                strFileData += "*";
            }
            for(int i = 0; i < taskQueue3.size(); i++){
                strFileData += taskQueue3[i][0] + "(" + taskQueue3[i][1] + ");";
            }
            strFileData += " ";
            if(taskQueue4.size() == 0){
                strFileData += "*";
            }
            for(int i = 0; i < taskQueue4.size(); i++){
                strFileData += taskQueue4[i][0] + "(" + taskQueue4[i][1] + ");";
            }
            strFileData += " " + toString(cpu) + " " + toString(mem1) + " " + toString(mem2) + " " + toString(mem3) + " " + toString(mem4) + " " + toString(taskWait1) + " " + toString(taskWait2) + " " + toString(taskWait3) + " " + toString(taskWait4);
            // std::cout << strFileData << std::endl;
            strFileData += "\n";
        }
        else{
            strFileData += tmpLineData;
            strFileData += "\n";
        }
    }
    in.close();
    std::ofstream out("rsus.csv");
    out.flush();
    out<<strFileData;
    out.close();
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
        mem1 = (int) mem / 4;
        mem2 = (int) mem / 4;
        mem3 = (int) mem / 4;
        mem4 = (int) mem / 4;
        populateWSM(rm);
        rm->setSenderAddress(myId);
        rm->setSenderPos(curPosition);
        rm->setSenderType(0);
        scheduleAt(simTime() + uniform(1.01, 1.2), rm);
        pc = PythonCommunication::getInstance();

    }
}

void TraCIDemoRSU11p::onRM(ReportMessage* frame)
{
    if (frame->getSenderType() == 1)  {
        // csv2variables();
        ReportMessage* rm = check_and_cast<ReportMessage*>(frame);
        LAddress::L2Type sender = rm->getSenderAddress();
        Coord pos = rm->getSenderPos();
        std::string roadId = rm->getVehRoad();
        // std::cout << "simTime " << time << std::endl;
        std::map<LAddress::L2Type, simtime_t>::iterator it = connectedNodes.begin();
        it = connectedNodes.find(sender);
        if(it == connectedNodes.end()) {
            connectedNodes.insert(std::make_pair(sender, simTime()));
            // NodePositions.insert(std::make_pair(sender, pos));
            NodeRoad.insert(std::make_pair(sender, roadId));
        }
        else {
            connectedNodes[sender] = simTime();
            NodeRoad[sender] = roadId;
        }
        // delete rm;
        findHost()->getDisplayString().setTagArg("t", 0, connectedNodes.size());
    }
}

void TraCIDemoRSU11p::onTask(Task* frame)
{
    Task* newTask = check_and_cast<Task*>(frame);
    std::string taskName = trim(newTask->getName());
    std::cout << "received task " << taskName << std::endl;
    // ReportMessage* rm = new ReportMessage();
    // rm->setSenderAddress(myId);
    // rm->setSenderPos(curPosition);
    // rm->setSenderType(0);
    // rm->setCpu(cpu);
    // rm->setMem((toString(mem1) + "|" + toString(mem2) + "|" + toString(mem3) + "|" + toString(mem4)).c_str());
    // rm->setWait(taskWait1 - simTime());
    // sendDown(rm->dup());
    // delete rm;
    if (toString(newTask->getTarget()) == toString(curPosition)){
        // proxyMode
        csv2variables();
        std::string externalId = newTask->getExternalId();
        std::string possibleRSU = newTask->getPossibleRSUs();

        pc->loadMoudle("rsuDecisionDynamicProgramCall");
        PythonCommunication::PythonParam pp;
        pp.set("rsuInfo", toString(newTask->getPossibleRSUs()));
        pp.set("vehPos", toString(newTask->getSenderPos()));
        pp.set("deadLinePos", toString(newTask->getDeadlinePos()));
        pp.set("cpu", newTask->getCPU());
        pp.set("mem", newTask->getMem());
        pp.set("externalId", externalId);
        pp.set("roadId", toString(newTask->getRoadId()));
        pp.set("proxyPos", toString(curPosition));
        pp.set("taskName", toString(newTask->getName()));
        pp.set("simTime", simTime().dbl());
        PythonCommunication::PythonParam *ppr = pc->call("rsuDecision", &pp);
        std::string tmpDead = ppr->getString("dead");
        std::string MainDecision = ppr->getString("decision");
        std::string MainRelay = ppr->getString("relay");
        std::string MainRoad = ppr->getString("service");
        std::vector<std::string> tmpMainDecisionVector = split(trim(MainDecision), "|");
        std::vector<std::string> tmpMainRelayVector = split(trim(MainRelay), "|");
        std::vector<std::string> tmpMainRoadVector = split(trim(MainRoad), "|");
        // LPVOID decision;
        // HANDLE hMapDecision = NULL;
        // hMapDecision = OpenFileMapping(FILE_MAP_ALL_ACCESS, 0, (externalId + "decision").c_str());
        // if (hMapDecision == NULL) {
        //     hMapDecision = CreateFileMapping(NULL, NULL, PAGE_READWRITE, 0, 0X1000, (externalId + "decision").c_str());
        // }
        // decision = MapViewOfFile(hMapDecision, FILE_MAP_ALL_ACCESS, 0, 0, 0);

        // LPVOID relay;
        // HANDLE hMapRelay = NULL;
        // hMapRelay = OpenFileMapping(FILE_MAP_ALL_ACCESS, 0, (externalId + "relay").c_str());
        // if (hMapRelay == NULL) {
        //     hMapRelay = CreateFileMapping(NULL, NULL, PAGE_READWRITE, 0, 0X1000, (externalId + "relay").c_str());
        // }
        // relay = MapViewOfFile(hMapRelay, FILE_MAP_ALL_ACCESS, 0, 0, 0);

        // LPVOID serviceRoad;
        // HANDLE hMapServiceRoad = NULL;
        // hMapServiceRoad = OpenFileMapping(FILE_MAP_ALL_ACCESS, 0, (externalId + "service").c_str());
        // if (hMapServiceRoad == NULL) {
        //     hMapServiceRoad = CreateFileMapping(NULL, NULL, PAGE_READWRITE, 0, 0X1000, (externalId + "service").c_str());
        // }
        // serviceRoad = MapViewOfFile(hMapServiceRoad, FILE_MAP_ALL_ACCESS, 0, 0, 0);

        // LPVOID deadRoad;
        // HANDLE hMapDeadRoad = NULL;
        // hMapDeadRoad = OpenFileMapping(FILE_MAP_ALL_ACCESS, 0, (externalId + "dead").c_str());
        // if (hMapDeadRoad == NULL) {
        //     hMapDeadRoad = CreateFileMapping(NULL, NULL, PAGE_READWRITE, 0, 0X1000, (externalId + "dead").c_str());
        // }
        // deadRoad = MapViewOfFile(hMapDeadRoad, FILE_MAP_ALL_ACCESS, 0, 0, 0);

        // std::string command = "D:\\scoop\\apps\\python38\\current\\python.exe rsuDecisionDynamicProgram.py " + trim(toString(newTask->getPossibleRSUs())) + " " + toString(newTask->getSenderPos()) + " " + toString(newTask->getDeadlinePos()) + " " + toString(newTask->getCPU()) + " " + toString(newTask->getMem()) + " " + externalId + " " + newTask->getRoadId() + " " + toString(curPosition) + " " + toString(newTask->getName()) + " " + toString(simTime());
        // std::cout << "rsuInfo " << command << std::endl;
        // int result = system(command.c_str());
        // std::string tmpMainDecision((char*)decision);
        // std::string tmpMainRelay((char*)relay);
        // std::string tmpMainRoad((char*)serviceRoad);
        // std::string tmpDead((char*)deadRoad);
        // std::vector<std::string> tmpMainDecisionVector = split(trim(tmpMainDecision), "|");
        // std::vector<std::string> tmpMainRelayVector = split(trim(tmpMainRelay), "|");
        // std::vector<std::string> tmpMainRoadVector = split(trim(tmpMainRoad), "|");

        // double distance = sqrt(pow(newTask->getSenderPos().x - curPosition.x, 2) + pow(newTask->getSenderPos().y - curPosition.y, 2));
        double distance = newTask->getSenderPos().distance(curPosition);
        double transmissionTime = 0;
        if (5 * log2(1 + 2500 / distance) > maxRate){
            transmissionTime = newTask->getMem() * 8 / 1024 / maxRate;
        } else {
            transmissionTime = newTask->getMem() * 8 / 1024 / (5 * log2(1 + 2500 / distance));
        }
        for (int i = 0; i < tmpMainDecisionVector.size(); ++i)
        {
            std::cout << tmpMainDecisionVector[i] << " " << tmpMainRelayVector[i] << " " << tmpMainRoadVector[i] << std::endl;
            Task* taskSplit = newTask->dup();
            taskSplit->setTarget("");
            std::vector<std::string> ratioResult = split(tmpMainDecisionVector[i], "*");
            if(ratioResult[0].length() == 0){
                std::cout << "decision parse error " << taskName << " decision is " << tmpMainDecision << std::endl;
                break;
            }
            taskSplit->setDecision(ratioResult[0].c_str());
            taskSplit->setRatio(strtod(toString(ratioResult[1]).c_str(),NULL));
            if(tmpMainRelayVector[i] == "NULL"){
                taskSplit->setRelay("");
            } 
            else{
                taskSplit->setRelay(tmpMainRelayVector[i].c_str());
            }
            taskSplit->setService(tmpMainRoadVector[i].c_str());
            taskSplit->setTransmissionTime(transmissionTime);
            taskSplit->setDead(trim(tmpDead).c_str());
            std::cout << "task part, transmissionTime " << transmissionTime << " decision " << tmpMainDecisionVector[i] << " ratio " << taskSplit->getRatio() << std::endl;
            scheduleAt(simTime() + transmissionTime, taskSplit->dup());
        }
    }
    else if (toString(newTask->getTarget()) == "" && newTask->getSenderType() == 0){
        // receive proxied task
        std::vector<std::string> tmpResult;
        tmpResult = split(trim(newTask->getDecision()), ";");
        std::cout << "here4 " << trim(newTask->getDecision()) << " relays " << trim(newTask->getRelay()) << std::endl;
        if(tmpResult[0] == toString(curPosition)){
            // operate here
            csv2variables();
            double taskCpu = newTask->getCPU() * newTask->getRatio();
            double taskMem = newTask->getMem() * newTask->getRatio();
            double cpuTmp = cpu;
            double operationTime = taskCpu / cpuTmp;
            // wait += operationTime;
            std::cout << "operate here " << curPosition << " , task name is " << taskName << " operation time " << operationTime << endl;
            newTask->setOperationTime(operationTime);
            switch(atoi(toString(tmpResult[1]).c_str())){
                case 1 :
                    std::cout << "operate on core 1" << std::endl;
                    // mem1 -= taskMem;
                    for(int i = 0; i < taskQueue1.size(); i++){
                        if(taskQueue1[i][0] == taskName){
                            if(i == 0){
                                taskQueue1[i][1] = "2";
                                // if(taskWait1 >= simTime()){
                                //     taskWait1 += operationTime;
                                // }
                                // else{
                                //     taskWait1 = simTime() + operationTime;
                                // }
                                newTask->setSenderType(1);
                                scheduleAt(simTime() + operationTime, newTask->dup());
                            }
                            else if(taskQueue1[i - 1][1] == "0" || taskQueue1[i - 1][1] == "1" || taskQueue1[i - 1][1] == "2"){
                                taskQueue1[i][1] = "1";
                                // *** need update task state ****
                                scheduleAt(simTime() + 1, newTask->dup());
                            }
                            else if(taskQueue1[i - 1][1] == "3"){
                                taskQueue1[i][1] = "2";
                                // if(taskWait1 >= simTime()){
                                //     taskWait1 += operationTime;
                                // }
                                // else{
                                //     taskWait1 = simTime() + operationTime;
                                // }
                                newTask->setSenderType(1);
                                scheduleAt(simTime() + operationTime, newTask->dup());
                            }
                            break;
                        }
                    }
                    break;
                case 2 :
                    std::cout << "operate on core 2" << std::endl;
                    // mem2 -= taskMem;
                    for(int i = 0; i < taskQueue2.size(); i++){
                        if(taskQueue2[i][0] == taskName){
                            if(i == 0){
                                taskQueue2[i][1] = "2";
                                // if(taskWait2 >= simTime()){
                                //     taskWait2 += operationTime;
                                // }
                                // else{
                                //     taskWait2 = simTime() + operationTime;
                                // }
                                newTask->setSenderType(1);
                                scheduleAt(simTime() + operationTime, newTask->dup());
                            }
                            else if(taskQueue2[i - 1][1] == "0" || taskQueue2[i - 1][1] == "1" || taskQueue2[i - 1][1] == "2"){
                                taskQueue2[i][1] = "1";
                                // *** need update task state ****
                                scheduleAt(simTime() + 1, newTask->dup());
                            }
                            else if(taskQueue2[i - 1][1] == "3"){
                                taskQueue2[i][1] = "2";
                                // if(taskWait2 >= simTime()){
                                //     taskWait2 += operationTime;
                                // }
                                // else{
                                //     taskWait2 = simTime() + operationTime;
                                // }
                                newTask->setSenderType(1);
                                scheduleAt(simTime() + operationTime, newTask->dup());
                            }
                            break;
                        }
                    }
                    break;
                case 3 :
                    std::cout << "operate on core 3" << std::endl;
                    // mem3 -= taskMem;
                    for(int i = 0; i < taskQueue3.size(); i++){
                        if(taskQueue3[i][0] == taskName){
                            if(i == 0){
                                taskQueue3[i][1] = "2";
                                // if(taskWait3 >= simTime()){
                                //     taskWait3 += operationTime;
                                // }
                                // else{
                                //     taskWait3 = simTime() + operationTime;
                                // }
                                newTask->setSenderType(1);
                                scheduleAt(simTime() + operationTime, newTask->dup());
                            }
                            else if(taskQueue3[i - 1][1] == "0" || taskQueue3[i - 1][1] == "1" || taskQueue3[i - 1][1] == "2"){
                                taskQueue3[i][1] = "1";
                                // *** need update task state ****
                                scheduleAt(simTime() + 1, newTask->dup());
                            }
                            else if(taskQueue3[i - 1][1] == "3"){
                                taskQueue3[i][1] = "2";
                                // if(taskWait3 >= simTime()){
                                //     taskWait3 += operationTime;
                                // }
                                // else{
                                //     taskWait3 = simTime() + operationTime;
                                // }
                                newTask->setSenderType(1);
                                scheduleAt(simTime() + operationTime, newTask->dup());
                            }
                            break;
                        }
                    }
                    break;
                case 4 :
                    std::cout << "operate on core 4" << std::endl;
                    // mem4 -= taskMem;
                    for(int i = 0; i < taskQueue4.size(); i++){
                        if(taskQueue4[i][0] == taskName){
                            if(i == 0){
                                taskQueue4[i][1] = "2";
                                // if(taskWait4 >= simTime()){
                                //     taskWait4 += operationTime;
                                // }
                                // else{
                                //     taskWait4 = simTime() + operationTime;
                                // }
                                newTask->setSenderType(1);
                                scheduleAt(simTime() + operationTime, newTask->dup());
                            }
                            else if(taskQueue4[i - 1][1] == "0" || taskQueue4[i - 1][1] == "1" || taskQueue4[i - 1][1] == "2"){
                                taskQueue4[i][1] = "1";
                                // *** need update task state ****
                                scheduleAt(simTime() + 1, newTask->dup());
                            }
                            else if(taskQueue4[i - 1][1] == "3"){
                                taskQueue4[i][1] = "2";
                                // if(taskWait4 >= simTime()){
                                //     taskWait4 += operationTime;
                                // }
                                // else{
                                //     taskWait4 = simTime() + operationTime;
                                // }
                                newTask->setSenderType(1);
                                scheduleAt(simTime() + operationTime, newTask->dup());
                            }
                            break;
                        }
                    }
                    break;
            }
            variables2csv();
        }
        else if (toString(newTask->getRelay()) != ""){
            // parse relay
            std::vector<std::string> result = split(trim(newTask->getRelay()), ";");
            for(int i = 0; i < result.size(); i++) {
                std::string tmpRelay = trim(toString(result[i]));
                if(tmpRelay == toString(curPosition)){
                    //relay node
                    double relayTime = newTask->getMem() * 8 / 1024 / maxRate;
                    //change relay decision
                    std::string newRelay = trim(toString(newTask->getRelay()));
                    int tmpPos = newRelay.find(toString(curPosition) + ";");
                    // std::cout << "here5 " << tmpPos << std::endl;
                    if (tmpPos > -1)
                    {
                        newRelay.erase(tmpPos, toString(curPosition).length() + 1);
                    }
                    // std::cout << "here4 " << newRelay << " " << toString(curPosition) << std::endl;
                    newTask->setRelay(newRelay.c_str());

                    std::cout << "relay here " << curPosition << " time " << relayTime << std::endl;
                    scheduleAt(simTime() + relayTime, newTask->dup());
                    break;
                }
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
        rm->setMem((toString(mem1) + "|" + toString(mem2) + "|" + toString(mem3) + "|" + toString(mem4)).c_str());
        rm->setWait(taskWait1 - simTime());
        scheduleAt(simTime() + 5, rm);
        sendDown(rm->dup());

        std::map<LAddress::L2Type, simtime_t>::iterator it;
        // std::map<LAddress::L2Type, Coord>::iterator itCoord=NodePositions.begin();
        std::map<LAddress::L2Type, std::string>::iterator itRoad;
        for(it = connectedNodes.begin(), itRoad=NodeRoad.begin(); it != connectedNodes.end() && itRoad != NodeRoad.end(); it++, itRoad++) {
            if (simTime() - it->second >= 10) {
                // std::cout << "erase " << it->first << " " << itRoad->first << std::endl;
                if (it == connectedNodes.end() || itRoad == NodeRoad.end()){
                    // std::cout << "end " << std::endl;
                    connectedNodes.erase(it++);
                    NodeRoad.erase(itRoad++);
                    break;
                }
                else{
                    // std::cout << "not end " << connectedNodes.end()->first << std::endl;
                    connectedNodes.erase(it++);
                    NodeRoad.erase(itRoad++);
                }
                // std::cout << "here " << connectedNodes.end()->first << " " << it->first << std::endl;
                if (it == connectedNodes.end() || itRoad == NodeRoad.end()) {
                    break;
                }
            }
        }
        findHost()->getDisplayString().setTagArg("t", 0, connectedNodes.size());
        // saving woring state, replacing python call
        if(simTime() < 5){
            variables2csv();
        }
        std::ofstream outfile("RSUlog\\" + toString(curPosition), std::ios::app);
        for(int i = 0; i < taskQueue1.size(); i++){
            outfile << taskQueue1[i][0] << "(" << taskQueue1[i][1] << ")" << ",";
        }
        for(int i = 0; i < taskQueue2.size(); i++){
            outfile << taskQueue2[i][0] << "(" << taskQueue2[i][1] << ")" << ",";
        }
        for(int i = 0; i < taskQueue3.size(); i++){
            outfile << taskQueue3[i][0] << "(" << taskQueue3[i][1] << ")" << ",";
        }
        for(int i = 0; i < taskQueue4.size(); i++){
            outfile << taskQueue4[i][0] << "(" << taskQueue4[i][1] << ")" << ",";
        }
        // for(auto const &i: taskQueue1) {
        //     outfile << trim(i) + ",";
        // }
        // for(auto const &i: taskQueue2) {
        //     outfile << trim(i) + ",";
        // }
        // for(auto const &i: taskQueue3) {
        //     outfile << trim(i) + ",";
        // }
        // for(auto const &i: taskQueue4) {
        //     outfile << trim(i) + ",";
        // }
        outfile << ";" + toString(mem1) + "," + toString(mem2) + "," + toString(mem3) + "," + toString(mem4) + "," + toString(taskWait1 - simTime()) + "," + toString(taskWait2 - simTime()) + "," + toString(taskWait3 - simTime()) + "," + toString(taskWait4 - simTime()) + "," + toString(simTime()) << std::endl;
        outfile.close();
    }
    else if (Task* newTask = dynamic_cast<Task*>(msg)) {
        std::vector<std::string> tmpResultLocal = split(trim(newTask->getDecision()), ";");
        if(newTask->getSenderType() == 1){
            // send back
            std::cout << "ready to send back " << newTask->getName() << std::endl;
            // std::map<LAddress::L2Type, Coord>::iterator it2;
            std::map<LAddress::L2Type, std::string>::iterator itRoad = NodeRoad.begin();
            bool ifSend = 0;
            switch(atoi(toString(tmpResultLocal[1]).c_str())){
                case 1 :
                    for(int i = 0; i < taskQueue1.size(); i++){
                        if (taskQueue1[i][0] == newTask->getName()){
                            taskQueue1[i][1] = "3";
                            break;
                        }
                    }
                    break;
                case 2 :
                    for(int i = 0; i < taskQueue2.size(); i++){
                        if (taskQueue2[i][0] == newTask->getName()){
                            taskQueue2[i][1] = "3";
                            break;
                        }
                    }
                    break;
                case 3 :
                    for(int i = 0; i < taskQueue3.size(); i++){
                        if (taskQueue3[i][0] == newTask->getName()){
                            taskQueue3[i][1] = "3";
                            break;
                        }
                    }
                    break;
                case 4 :
                    for(int i = 0; i < taskQueue4.size(); i++){
                        if (taskQueue4[i][0] == newTask->getName()){
                            taskQueue4[i][1] = "3";
                            break;
                            }
                        }
                    break;
            }
            variables2csv();
            // need a new strategy
            for(itRoad; itRoad != NodeRoad.end(); itRoad++) {
                // std::cout << "scan veh " << itRoad->first << " " << itRoad->second << std::endl;
                if(itRoad->first == newTask->getSenderAddress()){
                    std::string roads = newTask->getRoads();
                    std::vector<std::string> roadList = split(roads, ";");
                    int presentIndex = -1;
                    int serviceIndex = -1;
                    int deadlineIndex = -1;
                    // std::cout << "serviceRoad " << newTask->getService() << std::endl;
                    for (int i = 0; i < roadList.size(); ++i){
                        // std::cout << "roadListReceived " << roadList[i] << std::endl;
                        if(itRoad->second == roadList[i]){
                            presentIndex = i;
                        }
                        if(trim(toString(newTask->getService())) == roadList[i]){
                            serviceIndex = i;
                        }
                        if(toString(newTask->getDead()) == roadList[i]){
                            deadlineIndex = i;
                        }
                    }
                    std::cout << "indexs " << presentIndex << " " << serviceIndex << " " << deadlineIndex << std::endl;
                    // if(presentIndex == -1 || serviceIndex == -1 || deadlineIndex == -1){
                    //     break;
                    // }
                    if(presentIndex < serviceIndex){
                        std::cout << "task fininshed, but vehicle not arrive" << std::endl;
                        break;
                    }
                    else if(presentIndex == serviceIndex){
                        ifSend = 1;
                        newTask->setSenderPos(curPosition);
                        std::cout << "number of task is " << taskQueue1.size() << " wait time is " << taskWait1 << std::endl;
                        sendDown(newTask->dup());
                        break;
                    }
                    else if(presentIndex > deadlineIndex){
                        ifSend = 1;
                        std::cout << "task didn't finish on time, vehicle have pass through deadline" << std::endl;
                        std::ofstream outfile;
                        outfile.open("taskLog/" + trim(toString(newTask->getName())) + ".json");
                        outfile << "failed" << std::endl;
                        outfile.close();
                        break;
                    }
                    else{
                        ifSend = 1;
                        LPVOID backDecision;
                        HANDLE hMapBackDecision = NULL;
                        hMapBackDecision = OpenFileMapping(FILE_MAP_ALL_ACCESS, 0, (trim(toString(newTask->getName())) + "sendback").c_str());
                        if (hMapBackDecision == NULL) {
                            hMapBackDecision = CreateFileMapping(NULL, NULL, PAGE_READWRITE, 0, 0X1000, (trim(toString(newTask->getName())) + "sendback").c_str());
                        }
                        backDecision = MapViewOfFile(hMapBackDecision, FILE_MAP_ALL_ACCESS, 0, 0, 0);
                        //already scan vehicle, needs findout the service location
                        std::string command = "D:\\scoop\\apps\\python38\\current\\python.exe rsuSendback.py " + itRoad->second + " " + newTask->getExternalId() + " " + trim(toString(newTask->getName())) + " " + toString(newTask->getDeadlinePos()) + " " + trim(toString(newTask->getService()));
                        std::cout << "send back command " << command << std::endl;
                        int result = system(command.c_str());
                        std::string tmpSendback((char*)backDecision);
                        std::cout << "task finish later than service road, need record task sendback length" << std::endl;
                        newTask->setSenderPos(curPosition);
                        double operationTime = newTask->getOperationTime();
                        double transmissionTime = newTask->getTransmissionTime();
                        double taskMem = newTask->getMem();
                        //set length and set sender type
                        newTask->setService((char*)backDecision);
                        newTask->setSenderType(2);
                        sendDown(newTask->dup());
                    }
                }
            }
            if(ifSend == 0){
                scheduleAt(simTime() + 3, newTask->dup());
            }
            else{
                switch(atoi(toString(tmpResultLocal[1]).c_str())){
                    case 1 :
                        mem1 += newTask->getMem() * newTask->getRatio();
                        for(int i = 0; i < taskQueue1.size(); i++){
                            if (taskQueue1[i][0] == newTask->getName()){
                                taskQueue1.erase(taskQueue1.begin() + i);
                                break;
                            }
                        }
                        break;
                    case 2 :
                        mem2 += newTask->getMem() * newTask->getRatio();;
                        for(int i = 0; i < taskQueue2.size(); i++){
                            if (taskQueue2[i][0] == newTask->getName()){
                                taskQueue2.erase(taskQueue2.begin() + i);
                                break;
                            }
                        }
                        break;
                    case 3 :
                        mem3 += newTask->getMem() * newTask->getRatio();;
                        for(int i = 0; i < taskQueue3.size(); i++){
                            if (taskQueue3[i][0] == newTask->getName()){
                                taskQueue3.erase(taskQueue3.begin() + i);
                                break;
                            }
                        }
                        break;
                    case 4 :
                        mem4 += newTask->getMem() * newTask->getRatio();;
                        for(int i = 0; i < taskQueue4.size(); i++){
                            if (taskQueue4[i][0] == newTask->getName()){
                                taskQueue4.erase(taskQueue4.begin() + i);
                                break;
                            }
                        }
                        break;
                }
            }
            variables2csv();
        }
        else if(tmpResultLocal[0] == toString(curPosition)){
            // just operate on this rsu
            csv2variables();
            std::string taskName = trim(newTask->getName());
            double taskCpu = newTask->getCPU() * newTask->getRatio();
            double taskMem = newTask->getMem() * newTask->getRatio();
            double cpuTmp = cpu;
            double operationTime = taskCpu / cpuTmp;
            // wait += operationTime;
            // newTask->setSenderType(1);
            // newTask->setSenderPos(curPosition);
            newTask->setOperationTime(operationTime);
            std::cout << "operate here locally, task name is " << taskName << ", operationTime " << operationTime << " taskWait " << taskWait1 << std::endl;
            switch(atoi(toString(tmpResultLocal[1]).c_str())){
                case 1 :
                    std::cout << "operate on core 1" << std::endl;
                    // mem1 -= taskMem;
                    for(int i = 0; i < taskQueue1.size(); i++){
                        if(taskQueue1[i][0] == taskName){
                            if(i == 0){
                                taskQueue1[i][1] = "2";
                                // if(taskWait1 >= simTime()){
                                //     taskWait1 += operationTime;
                                // }
                                // else{
                                //     taskWait1 = simTime() + operationTime;
                                // }
                                newTask->setSenderType(1);
                                scheduleAt(simTime() + operationTime, newTask->dup());
                            }
                            else if(taskQueue1[i - 1][1] == "0" || taskQueue1[i - 1][1] == "1" || taskQueue1[i - 1][1] == "2"){
                                taskQueue1[i][1] = "1";
                                // *** need update task state ****
                                scheduleAt(simTime() + 1, newTask->dup());
                            }
                            else if(taskQueue1[i - 1][1] == "3"){
                                taskQueue1[i][1] = "2";
                                // if(taskWait1 >= simTime()){
                                //     taskWait1 += operationTime;
                                // }
                                // else{
                                //     taskWait1 = simTime() + operationTime;
                                // }
                                newTask->setSenderType(1);
                                scheduleAt(simTime() + operationTime, newTask->dup());
                            }
                            break;
                        }
                    }
                    break;
                case 2 :
                    std::cout << "operate on core 2" << std::endl;
                    // mem2 -= taskMem;
                    for(int i = 0; i < taskQueue2.size(); i++){
                        if(taskQueue2[i][0] == taskName){
                            if(i == 0){
                                taskQueue2[i][1] = "2";
                                // if(taskWait2 >= simTime()){
                                //     taskWait2 += operationTime;
                                // }
                                // else{
                                //     taskWait2 = simTime() + operationTime;
                                // }
                                newTask->setSenderType(1);
                                scheduleAt(simTime() + operationTime, newTask->dup());
                            }
                            else if(taskQueue2[i - 1][1] == "0" || taskQueue2[i - 1][1] == "1" || taskQueue2[i - 1][1] == "2"){
                                taskQueue2[i][1] = "1";
                                // *** need update task state ****
                                scheduleAt(simTime() + 1, newTask->dup());
                            }
                            else if(taskQueue2[i - 1][1] == "3"){
                                taskQueue2[i][1] = "2";
                                // if(taskWait2 >= simTime()){
                                //     taskWait2 += operationTime;
                                // }
                                // else{
                                //     taskWait2 = simTime() + operationTime;
                                // }
                                newTask->setSenderType(1);
                                scheduleAt(simTime() + operationTime, newTask->dup());
                            }
                            break;
                        }
                    }
                    break;
                case 3 :
                    std::cout << "operate on core 3" << std::endl;
                    // mem3 -= taskMem;
                    for(int i = 0; i < taskQueue3.size(); i++){
                        if(taskQueue3[i][0] == taskName){
                            if(i == 0){
                                taskQueue3[i][1] = "2";
                                // if(taskWait3 >= simTime()){
                                //     taskWait3 += operationTime;
                                // }
                                // else{
                                //     taskWait3 = simTime() + operationTime;
                                // }
                                newTask->setSenderType(1);
                                scheduleAt(simTime() + operationTime, newTask->dup());
                            }
                            else if(taskQueue3[i - 1][1] == "0" || taskQueue3[i - 1][1] == "1" || taskQueue3[i - 1][1] == "2"){
                                taskQueue3[i][1] = "1";
                                // *** need update task state ****
                                scheduleAt(simTime() + 1, newTask->dup());
                            }
                            else if(taskQueue3[i - 1][1] == "3"){
                                taskQueue3[i][1] = "2";
                                // if(taskWait3 >= simTime()){
                                //     taskWait3 += operationTime;
                                // }
                                // else{
                                //     taskWait3 = simTime() + operationTime;
                                // }
                                newTask->setSenderType(1);
                                scheduleAt(simTime() + operationTime, newTask->dup());
                            }
                            break;
                        }
                    }
                    break;
                case 4 :
                    std::cout << "operate on core 4" << std::endl;
                    // mem4 -= taskMem;
                    for(int i = 0; i < taskQueue4.size(); i++){
                        if(taskQueue4[i][0] == taskName){
                            if(i == 0){
                                taskQueue4[i][1] = "2";
                                // if(taskWait4 >= simTime()){
                                //     taskWait4 += operationTime;
                                // }
                                // else{
                                //     taskWait4 = simTime() + operationTime;
                                // }
                                newTask->setSenderType(1);
                                scheduleAt(simTime() + operationTime, newTask->dup());
                            }
                            else if(taskQueue4[i - 1][1] == "0" || taskQueue4[i - 1][1] == "1" || taskQueue4[i - 1][1] == "2"){
                                taskQueue4[i][1] = "1";
                                // *** need update task state ****
                                scheduleAt(simTime() + 1, newTask->dup());
                            }
                            else if(taskQueue4[i - 1][1] == "3"){
                                taskQueue4[i][1] = "2";
                                // if(taskWait4 >= simTime()){
                                //     taskWait4 += operationTime;
                                // }
                                // else{
                                //     taskWait4 = simTime() + operationTime;
                                // }
                                newTask->setSenderType(1);
                                scheduleAt(simTime() + operationTime, newTask->dup());
                            }
                            break;
                        }
                    }
                    break;
            }
            std::cout << "here1 " << simTime() << " present position " << std::endl;
            variables2csv();
            std::cout << "here2 " << simTime() << " present position " << std::endl;
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

