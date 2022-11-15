import collections
import copy
import json
import math
import mmap
import os
import random
import sys
from turtle import pos

import numpy as np
import sumolib

size = (2600, 3000)
maxRate = 60
maxTime = 200

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
    if calVariance(waitMax) == 0:
        # means no ratio task allocate
        return 0.5 * serviceRate
    else:
        return 0.5 * serviceRate + 0.5 * (1 - calVariance(waits) / calVariance(waitMax))
        # return 0.5 * serviceRate

def calProfit(ratio, rsuIndex, allocateRatio, currentState, cpu, mem, rsuList, boundaries, serviceRoadList, originWait, transTime, proxyPos, etaRoad, etaFinal):
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
        relayTime += mem * 8 / 1000 / maxRate
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
    # print(serviceRoad, end=" ")
    # for eta in range(len(etaFinal)):
    #     if etaFinal[eta][1] > operationTime + transTime + relayTime and calDistance(etaFinal[eta][0], rsuPos) < 1000:
    #         serviceRoad = etaRoad[eta][0]
    #         break
    # else:
    #     serviceRoad = etaRoad[-1][0]
    # print(serviceRoad)

    tmpState[list(rsuList.keys())[rsuIndex]][1] = serviceRoad

    serviceRate = 0
    for rsu, property in tmpState.items():
        tmpPos = rsu.replace('(', '').replace(')', '')
        tmpPos = [int(tmpPos.split(',')[0]), int(tmpPos.split(',')[1])]
        serviceRate += calIntegralServiceRate(getRoadLength(property[1], net, boundaries), tmpPos) * property[0]
    tmpWaitMax = copy.deepcopy(list(originWait.values()))
    tmpWaitMax[tmpWaitMax.index(max(tmpWaitMax))] += cpu / list(rsuList.values())[tmpWaitMax.index(max(tmpWaitMax))]['cpu']
    # tmpWaitMax[tmpWaitMax.index(max(tmpWaitMax))] += cpu / list(rsuList.values())[tmpWaitMax.index(max(tmpWaitMax))]['cpu'] * (ratio + allocateRatio)
    tmpWait = copy.deepcopy(currentState[1])
    tmpWait[list(originWait.keys()).index(list(rsuList.keys())[rsuIndex])] += cpu / cpuTmp * ratio
    newState = [serviceRate, tmpWait, tmpState]
    # print(tmpWait)
    metric = CalMetric(serviceRate, tmpWait, tmpWaitMax)
    # print(metric, newState)
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
        

if __name__ == "__main__":
    rsuInfo = sys.argv[1][:-1].split(';')
    vehPos = sys.argv[2].replace('(', '').replace(')', '')
    vehPos = [float(vehPos.split(',')[0]), float(vehPos.split(',')[1])]
    deadLinePos = sys.argv[3].replace('(', '').replace(')', '')
    deadLinePos = [float(deadLinePos.split(',')[0]), float(deadLinePos.split(',')[1])]

    cpu = float(sys.argv[4])
    mem = float(sys.argv[5])
    externalId = sys.argv[6]
    roadId = sys.argv[7]
    proxyPos = sys.argv[8].replace('(', '').replace(')', '')
    proxyPos = [float(proxyPos.split(',')[0]), float(proxyPos.split(',')[1])]
    net = sumolib.net.readNet('erlangen.net.xml')
    boundaries = net.getBoundary()

    # -----------rsuInfo parse-----------

    rsuList = {}
    for rsu in rsuInfo:
        if float(rsu.split(':')[1].split('*')[2]) < maxTime:
            rsuList[rsu.split(':')[0] + ';1'] = {
                'cpu': float(rsu.split(':')[1].split('*')[0]), 
                'mem': float(rsu.split(':')[1].split('*')[1]), 
                'wait': float(rsu.split(':')[1].split('*')[2]), 
            }
        if float(rsu.split(':')[1].split('*')[3]) < maxTime:
            rsuList[rsu.split(':')[0] + ';2'] = {
                'cpu': float(rsu.split(':')[1].split('*')[0]), 
                'mem': float(rsu.split(':')[1].split('*')[1]), 
                'wait': float(rsu.split(':')[1].split('*')[3]), 
            }
        if float(rsu.split(':')[1].split('*')[4]) < maxTime:
            rsuList[rsu.split(':')[0] + ';3'] = {
                'cpu': float(rsu.split(':')[1].split('*')[0]), 
                'mem': float(rsu.split(':')[1].split('*')[1]), 
                'wait': float(rsu.split(':')[1].split('*')[4]), 
            }
        if float(rsu.split(':')[1].split('*')[5]) < maxTime:
            rsuList[rsu.split(':')[0] + ';4'] = {
                'cpu': float(rsu.split(':')[1].split('*')[0]), 
                'mem': float(rsu.split(':')[1].split('*')[1]), 
                'wait': float(rsu.split(':')[1].split('*')[5]), 
            }

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
            send(etaRaw[i][0], externalId + 'dead')
            break
        elif flag:
            etaFinal.append([tmpNode, float(etaRaw[i][1]) - etaStart])
            etaRoad.append([etaRaw[i][0], float(etaRaw[i][1]) - etaStart])

    # ---------------get rsu task list----------------

    decision = ''
    result = []
    distance = calDistance(proxyPos, vehPos)
    transRate = min(5 * math.log2(1 + 2500 / distance), maxRate)
    transTime = mem * 8 / 1000 / transRate
    rsuWaits = {}
    with open('rsus.csv', 'r') as file:
        while True:
            line = file.readline()
            if len(line) == 0:
                break
            line = line.strip().split(' ')
            for i in range(3, len(line)):
                rsuWaits[line[0] + ';' + str(i - 2)] = (float(line[i]))
    # ----------------decision process----------------
    # -----here is a dynamic programing algorithm-----
    # -----step is 0.1, so ratio is mutiply by 10-----

    serviceRoadList = {}
    for rsu, property in rsuList.items():
        operationTime = cpu / property['cpu'] + property['wait']
        rsuPos = rsu.replace('(', '').replace(')', '')
        rsuPos = [float(rsuPos.split(',')[0]), float(rsuPos.split(',')[1])]
        core = int(rsu.split(';')[1])
        relays = relay(proxyPos, rsuPos)
        relayTime = 0
        for i in range(len(relays) - 1):
            relayTime += mem * 8 / 1000 / maxRate
        for eta in range(len(etaFinal)):
            if etaFinal[eta][1] > operationTime + transTime + relayTime:
                target = etaFinal[eta - 1][0]
                serviceRoad = etaRoad[eta - 1][0]
                serviceRoadList[rsu] = serviceRoad
                break
        else:
            target = etaFinal[-1][0]
            serviceRoad = etaRoad[-1][0]
            serviceRoadList[rsu] = serviceRoad

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
            # print(itemIndex, j + 1)
            for k in range(1, 11):
                res[itemIndex][k] = res[itemIndex - 1][k]
                # print(k, j + 1)
                if k >= j + 1:
                    newState, tmpMetric = calProfit((j + 1) / 10, i - 1, (k - j - 1) / 10, res[itemIndex - 1][k - j - 1], cpu, mem, rsuList, boundaries, serviceRoadList, rsuWaits, transTime, proxyPos, etaRoad, etaFinal)
                    tmpWaitMax = copy.deepcopy(list(rsuWaits.values()))
                    # tmpWaitMax[tmpWaitMax.index(max(tmpWaitMax))] += cpu / list(rsuList.values())[tmpWaitMax.index(max(tmpWaitMax))]['cpu'] * (k - j - 1) / 10
                    tmpWaitMax[tmpWaitMax.index(max(tmpWaitMax))] += cpu / list(rsuList.values())[tmpWaitMax.index(max(tmpWaitMax))]['cpu']
                    # print(tmpMetric)
                    if tmpMetric > CalMetric(res[itemIndex][k][0], res[itemIndex][k][1], tmpWaitMax):
                        res[itemIndex][k] = newState
            #     else:
            #         print(res[itemIndex - 1][k][2])
            # if j == 2:
            #     output(res, rsuList, maxRatio)
            #     print('---------')
            #     print(res[1][2][2])
            #     print(res[2][2][2])
            #     print(res[3][2][2])
            #     exit()
    
    x = [False for i in range(sum(maxRatio))]
    tmpWaitMax = copy.deepcopy(list(rsuWaits.values()))
    tmpWaitMax[tmpWaitMax.index(max(tmpWaitMax))] += cpu / list(rsuList.values())[tmpWaitMax.index(max(tmpWaitMax))]['cpu']
    # output(res, rsuList, maxRatio)
    w = 10
    for i in range(len(rsuList), 0, -1):
        for j in range(maxRatio[i - 1] - 1, -1, -1):
            itemIndex = 1 + sum(maxRatio[:i - 1]) + j
            if CalMetric(res[itemIndex][w][0], res[itemIndex][w][1], tmpWaitMax) > CalMetric(res[itemIndex - 1][w][0], res[itemIndex - 1][w][1], tmpWaitMax):
                x[itemIndex - 1] = True
                w -= j + 1
    result = {}
    for i in range(len(rsuList)):
        for j in range(maxRatio[i]):
            itemIndex = 1 + sum(maxRatio[:i]) + j
            if x[itemIndex - 1]:
                # has ratio
                try:
                    result[list(rsuList.keys())[i]] += (j + 1) / 10
                except:
                    result[list(rsuList.keys())[i]] = (j + 1) / 10
    
    # print(res[-1][-1][1])
    # print(CalMetric(res[-1][-1][0], res[-1][-1][1], tmpWaitMax))
    # print((res[-1][-1][0] + 1 / calVariance(res[-1][-1][1])) / 2)
    
    # print(res[-1][-1])
    
    decision = ''
    resultRelay = ''
    serviceRoad = ''
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
        resultRelay += tmpResultRelay + '|'
        # serviceRoad += serviceRoadList[k] + '|'
        serviceRoad += res[-1][-1][2][k][1] + '|'

    # decision = '(1500,2500,3);4*1|'
    # resultRelay = '(1500,1500,3);|'
    # serviceRoad = '23339459|'
    print(decision)
    print(resultRelay)
    print(serviceRoad)
    send(decision, externalId + 'decision')
    send(resultRelay, externalId + 'relay')
    send(serviceRoad, externalId + 'service')
