import collections
import copy
import json
import math
import mmap
import os
import random
import sys
import time
from turtle import pos

import numpy as np
import sumolib
from PythonParam import PythonParam

maxRate = 60
maxTime = 2000
alpha = 0.5

def send(s, file_name):
    s=s+100*' '
    infosize=len(s)+1
    byte=s.encode(encoding='UTF-8')
    #从头读取内存，不然的话会接着上次的内存位置继续写下去，这里是从头覆盖。
    shmem=mmap.mmap(0,1000,file_name,mmap.ACCESS_WRITE)
    shmem.write(byte)

def recieve(file_name):
    shmem=mmap.mmap(0,100,file_name,mmap.ACCESS_READ)
    s=str(shmem.read(shmem.size()).decode("utf-8"))
    #vs2012早期版本会有截断符和开始符号，需要提取有用字符串
    es='\\x00'#字符条件截断，还没有设计开始endstring
    if s.find(es)==-1:
        print(s)
    else:
        sn=s[:s.index(ss)]
        print(sn)

def relay(position, target):
    # relay0 = []
    # for i in range(round(size[0] / 1000)):
    #     for j in range(round(size[1] / 1000)):
    #         distance = math.sqrt((position[0] - i * 1000 - 500) ** 2 + (position[1] - j * 1000 - 500) ** 2)
    #         if distance <= 1000:
    #             relay0.append([i * 1000 + 500, j * 1000 + 500])
    # minJump = 16000
    # for point in relay0:
    #     if abs(target[0] - point[0] + target[1] - point[1]) < minJump:
    #         minJump = abs(target[0] - point[0] + target[1] - point[1])
    #         relayStart = point
    relayStart = position
    xlength = relayStart[0] - target[0]
    ylength = relayStart[1] - target[1]
    finalRelay = [relayStart]
    for i in range(abs(int(xlength / 1000))):
        if xlength > 0:
            finalRelay.append([finalRelay[-1][0] - 1000, finalRelay[-1][1]])
        else:
            finalRelay.append([finalRelay[-1][0] + 1000, finalRelay[-1][1]])
    for i in range(abs(int(ylength / 1000))):
        if ylength > 0:
            finalRelay.append([finalRelay[-1][0], finalRelay[-1][1] - 1000])
        else:
            finalRelay.append([finalRelay[-1][0], finalRelay[-1][1] + 1000])
    return finalRelay

def calDistance(node1, node2):
    return math.sqrt((node1[0] - node2[0]) ** 2 + (node1[1] - node2[1]) ** 2)

def calVariance(data):
    return np.var(data)

def calFairnessGain(rsu, rsuWaits, operationTime, variance):
    tmpWait = []
    for k, v in rsuWaits.items():
        if k == rsu.split(';')[0]:
            tmpWait.append(v + operationTime)
        else:
            tmpWait.append(v)
    tmpVariance = calVariance(tmpWait)
    return 1 / tmpVariance

def calServiceRate(target, rsuPos):
    serviceDist = calDistance(target, rsuPos)
    return (5 * math.log2(1 + 2500 / serviceDist)) / maxRate

def getRoadLength(roadId, net, boundaries):
    road = net.getEdge(roadId)
    # length = road.getLength()
    # fromNode = road.getFromNode().getCoord()
    # toNode = road.getToNode().getCoord()
    # fromNode = [fromNode[0] - boundaries[0], boundaries[3] - fromNode[1]]
    # toNode = [toNode[0] - boundaries[0], boundaries[3] - toNode[1]]
    shapeNodes = []
    for node in road.getShape():
        shapeNodes.append([node[0] - boundaries[0], boundaries[3] - node[1]])
    return shapeNodes

def calIntegralServiceRate(shapeNodes, rsuPos):
    xSum = 0
    ySum = 0
    for point in shapeNodes:
        xSum += point[0]
        ySum += point[1]
    servicePoint = [xSum / len(shapeNodes), ySum / len(shapeNodes)]
    return calServiceRate(servicePoint, rsuPos)

def CalMetric(serviceRate, waits, waitMax):

    if waitMax == 0:
        # means no ratio task allocate
        # print(serviceRate, 0)
        return alpha * serviceRate
    else:
        # print(serviceRate, calVariance(waits), calVariance(waitMax))
        # print(serviceRate, (calVariance(waits) / 100))
        return alpha * serviceRate + (1 - alpha) * (1 - calVariance(waits) / waitMax)
        # return alpha * serviceRate + (1 - alpha) * (1 - calVariance(waits) / 100)
        # return 0.5 * serviceRate

def calProfit(ratio, rsuIndex, allocateRatio, currentState, cpu, mem, rsuList, boundaries, serviceRoadList, originWait, transTime, proxyPos, etaRoad, etaFinal, waitMax, net):
    # print(ratio, rsuIndex, currentState, mem, rsuList)
    tmpState = copy.deepcopy(currentState[2])
    try:
        tmpState[list(rsuList.keys())[rsuIndex]][0] += ratio
    except:
        tmpState[list(rsuList.keys())[rsuIndex]] = [ratio, '']
    cpuTmp = rsuList[list(rsuList.keys())[rsuIndex]]['cpu']
    rsuPos = list(rsuList.keys())[rsuIndex].replace('(', '').replace(')', '')
    rsuPos = [int(rsuPos.split(',')[0]), int(rsuPos.split(',')[1])]
    operationTime = cpu / cpuTmp * tmpState[list(rsuList.keys())[rsuIndex]][0] + rsuList[list(rsuList.keys())[rsuIndex]]['wait']
    relays = relay(proxyPos, rsuPos)
    relayTime = 0
    for i in range(len(relays) - 1):
        relayTime += mem * tmpState[list(rsuList.keys())[rsuIndex]][0] * 8 / 1024 / maxRate
    tmpEtaFinal = []
    tmpEtaRoad = []
    for eta in range(len(etaFinal)):
        if calDistance(etaFinal[eta][0], rsuPos) <= 1000:
            tmpEtaFinal.append(etaFinal[eta])
            tmpEtaRoad.append(etaRoad[eta - 1])
    for eta in range(len(tmpEtaFinal)):
        if tmpEtaFinal[eta][1] > operationTime + transTime + relayTime:
            serviceRoad = tmpEtaRoad[eta][0]
            break
    else:
        serviceRoad = tmpEtaRoad[-1][0]
    tmpState[list(rsuList.keys())[rsuIndex]][1] = serviceRoad

    serviceRate = 0
    sumRatio = 0
    for rsu, property in tmpState.items():
        tmpPos = rsu.replace('(', '').replace(')', '')
        tmpPos = [int(tmpPos.split(',')[0]), int(tmpPos.split(',')[1])]
        serviceRate += calIntegralServiceRate(getRoadLength(property[1], net, boundaries), tmpPos) * property[0]
        sumRatio += property[0]
    # print(tmpWaitMax)
    # tmpWaitMax[tmpWaitMax.index(max(tmpWaitMax))] += cpu / list(rsuList.values())[tmpWaitMax.index(max(tmpWaitMax))]['cpu'] * (ratio + allocateRatio)
    tmpWait = copy.deepcopy(currentState[1])
    tmpWait[list(originWait.keys()).index(list(rsuList.keys())[rsuIndex])] += cpu / cpuTmp * ratio
    newState = [serviceRate, tmpWait, tmpState]
    # print([int(v[0] * 10) for k, v in tmpState.items()])
    if int(sum([v[0] for k, v in tmpState.items()]) * 10) == allocateRatio:
        metric = CalMetric(serviceRate, tmpWait, waitMax)
    else:
        metric = -1
    # print(tmpWait)
    # metric = CalMetric(serviceRate, tmpWait, waitMax)
    # print(serviceRate, calVariance(tmpWait), waitMax, '...')
    return newState, metric

def output(res, rsuList, maxRatio):
    with open('result.csv', 'w') as file:
        for i in range(11):
            file.write(str(res[0][i][0]) + ',')
        file.write('\n')
        for i in range(1, len(rsuList) + 1):
            for j in range(maxRatio[i - 1]):
                itemIndex = 1 + sum(maxRatio[:i - 1]) + j
                for k in range(11):
                    file.write(str(res[itemIndex][k][0]) + ',')
                file.write('\n')
        

def rsuDecision(param: PythonParam):
    rsuInfo = param.getString("rsuInfo")[:-1].split(';')
    vehPos = param.getString("vehPos").replace('(', '').replace(')', '')
    vehPos = [float(vehPos.split(',')[0]), float(vehPos.split(',')[1])]
    deadLinePos = param.getString("deadLinePos").replace('(', '').replace(')', '')
    deadLinePos = [float(deadLinePos.split(',')[0]), float(deadLinePos.split(',')[1])]

    cpu = param.getDouble("cpu")
    mem = param.getDouble("mem")
    externalId = param.getString("externalId")
    roadId = param.getString("roadId")
    proxyPos = param.getString("proxyPos").replace('(', '').replace(')', '')
    proxyPos = [float(proxyPos.split(',')[0]), float(proxyPos.split(',')[1])]
    taskName = param.getString("taskName")
    simTime = param.getDouble("simTime")
    rparam = PythonParam()
    net = sumolib.net.readNet('erlangen.net.xml')
    boundaries = net.getBoundary()
    # -----------rsuInfo parse-----------

    rsuList = {}
    rsuWaits = {}
    with open('rsus.csv', 'r') as file:
        while True:
            line = file.readline()
            if len(line) == 0:
                break
            line = line.strip().split(' ')
            if line[0] in rsuInfo:
                rsuList[line[0] + ';1'] = {
                    'cpu': float(line[5]), 
                    'mem': float(line[6]), 
                    'wait': max(0, float(line[10]) - simTime), 
                }
                rsuList[line[0] + ';2'] = {
                    'cpu': float(line[5]), 
                    'mem': float(line[7]), 
                    'wait': max(0, float(line[11]) - simTime), 
                }
                rsuList[line[0] + ';3'] = {
                    'cpu': float(line[5]), 
                    'mem': float(line[8]), 
                    'wait': max(0, float(line[12]) - simTime), 
                }
                rsuList[line[0] + ';4'] = {
                    'cpu': float(line[5]), 
                    'mem': float(line[9]), 
                    'wait': max(0, float(line[13]) - simTime), 
                }
            for i in range(10, len(line)):
                rsuWaits[line[0] + ';' + str(i - 9)] = (float(line[i]))

    # -----------eta calculation-----------
    etaRoad = []

    etaRaw = []
    etaFinal = []
    etaStart = 0
    with open('eta/' + externalId + '.csv', 'r') as file:
        while True:
            line = file.readline()
            if len(line) == 0:
                break
            etaRaw.append(line.strip().split(','))
    flag = False
    for i in range(len(etaRaw)):
        tmpNode = net.getEdge(etaRaw[i][0]).getFromNode().getCoord()
        tmpNode = [tmpNode[0] - boundaries[0], boundaries[3] - tmpNode[1]]
        if etaRaw[i][0] == roadId:
            etaFinal.append([tmpNode, 0])
            etaRoad.append([etaRaw[i][0],0])
            etaStart = float(etaRaw[i][1])
            flag = True
        elif abs(tmpNode[0] - deadLinePos[0]) < 0.05 and abs(tmpNode[1] - deadLinePos[1]) < 0.05:
            etaFinal.append([tmpNode, float(etaRaw[i][1]) - etaStart])
            etaRoad.append([etaRaw[i][0], float(etaRaw[i][1]) - etaStart])
            rparam.set("dead", etaRaw[i][0])
            break
        elif flag:
            etaFinal.append([tmpNode, float(etaRaw[i][1]) - etaStart])
            etaRoad.append([etaRaw[i][0], float(etaRaw[i][1]) - etaStart])

    # ---------------get rsu task list----------------

    decision = ''
    result = []
    distance = calDistance(proxyPos, vehPos)
    transRate = min(5 * math.log2(1 + 2500 / distance), maxRate)
    transTime = mem * 8 / 1024 / transRate
    # ----------------decision process----------------
    # -----here is a dynamic programing algorithm-----
    # -----step is 0.1, so ratio is mutiply by 10-----

    waitMax = 0
    for rsuCoord, wait in rsuWaits.items():
        tmpwaitMax = copy.deepcopy(rsuWaits)
        try:
            tmpwaitMax[rsuCoord] += cpu / rsuList[rsuCoord]['cpu']
            if calVariance(list(tmpwaitMax.values())) > waitMax:
                waitMax = calVariance(list(tmpwaitMax.values()))
        except:
            pass

    serviceRoadList = {}

    # print(etaFinal)
    maxRatio = []
    for rsu, property in rsuList.items():
        maxRatio.append(math.floor(min(rsuList[rsu]['mem'], mem) / mem * 10))
    res = [[[-1, list(rsuWaits.values()), {}] for k in range(11)]]
    for i in range(len(rsuList)):
        for j in range(maxRatio[i]):
            res.append([[-1, list(rsuWaits.values()), {}] for k in range(11)])
    for j in range(11):
        res[0][j][0] = 0
    for i in range(1, len(rsuList) + 1):
        for j in range(maxRatio[i - 1]):
            itemIndex = 1 + sum(maxRatio[:i - 1]) + j
            for k in range(1, 11):
                res[itemIndex][k] = res[itemIndex - 1][k]
                if k >= 1:
                    newState, tmpMetric = calProfit(0.1 , i - 1, k, res[itemIndex - 1][k - 1], cpu, mem, rsuList, boundaries, serviceRoadList, rsuWaits, transTime, proxyPos, etaRoad, etaFinal, waitMax, net)
                    if tmpMetric > CalMetric(res[itemIndex][k][0], res[itemIndex][k][1], waitMax):
                        res[itemIndex][k] = newState
    x = [False for i in range(sum(maxRatio))]
    # output(res, rsuList, maxRatio)
    w = 10
    for i in range(len(rsuList), 0, -1):
        for j in range(maxRatio[i - 1] - 1, -1, -1):
            itemIndex = 1 + sum(maxRatio[:i - 1]) + j
            # *******
            if CalMetric(res[itemIndex][w][0], res[itemIndex][w][1], waitMax) > CalMetric(res[itemIndex - 1][w][0], res[itemIndex - 1][w][1], waitMax):
                x[itemIndex - 1] = True
                w -= 1
    result = {}
    for i in range(len(rsuList)):
        for j in range(maxRatio[i]):
            itemIndex = 1 + sum(maxRatio[:i]) + j
            if x[itemIndex - 1]:
                # has ratio
                try:
                    result[list(rsuList.keys())[i]] += 0.1
                except:
                    result[list(rsuList.keys())[i]] = 0.1
    
    
    decision = ''
    resultRelay = ''
    serviceRoad = ''
    tmpLines = []
    with open('rsus.csv', 'r') as file:
        while True:
            line = file.readline()
            if len(line) == 0:
                break
            tmp = line.split(' ')
            tmpLines.append(tmp)

    for k, v in result.items():
        tmpResultRelay = ''
        decision += k + '*' + str(round(v, 2)) + '|'
        rsuPos = k.replace('(', '').replace(')', '')
        rsuPos = [float(rsuPos.split(',')[0]), float(rsuPos.split(',')[1])]
        tmpRelay = relay(proxyPos, rsuPos)
        for node in tmpRelay:
            if node != rsuPos and node != proxyPos:
                tmpResultRelay += '(' + str(int(node[0])) + ',' + str(int(node[1])) + ',3);'
        if len(tmpResultRelay) == 0:
            tmpResultRelay += 'NULL'
        for tmp in tmpLines:
            if tmp[0] == k.split(';')[0]:
                core = int(k.split(';')[1])
                tmpRelayTime = len(tmpRelay[1:]) * mem * round(v, 2) * 8 / 1024 / maxRate
                tmpOperationTime = cpu * round(v, 2) / rsuList[k]['cpu']
                if tmp[core] == '*':
                    tmp[core] = taskName + '(0(' + str(int(transTime + tmpRelayTime + simTime + 7)) + ';'
                else:
                    tmp[core] += taskName + '(0(' + str(int(transTime + tmpRelayTime + simTime + 7)) + ';'
                tmp[core + 5] = str(float(tmp[core + 5]) - round(mem * round(v, 2), 2))
                tmp[core + 9] = str(rsuList[k]['wait'] + transTime + tmpRelayTime + tmpOperationTime + simTime)
                if core == 4:
                    tmp[core + 9] += '\n'
                break
        resultRelay += tmpResultRelay + '|'
        serviceRoad += res[-1][-1][2][k][1] + '|'
    with open('rsus.csv', 'w') as file:
        file.write(''.join([' '.join(tmp) for tmp in tmpLines]))

    if len(result) == 0:
        with open('decision/' + taskName, 'w') as file:
            file.write('1')
    else:
        with open('decision/' + taskName, 'w') as file:
            file.write(str(proxyPos) + '\n' + decision + '\n' + resultRelay + '\n' + serviceRoad)
    rparam.set("decision", decision[:-1])
    rparam.set("relay", resultRelay[:-1])
    rparam.set("service", serviceRoad[:-1])
    return rparam

