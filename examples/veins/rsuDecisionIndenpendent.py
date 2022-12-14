import json
import math
import mmap
import copy
import os
import random
import sys
from turtle import pos

import numpy as np
import sumolib
from PythonParam import PythonParam


size = (2600, 3000)
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
    shapeNodes = []
    for node in road.getShape():
        shapeNodes.append([node[0] - boundaries[0], boundaries[3] - node[1]])
    return shapeNodes

def calIntegralServiceRate(shapeNodes, rsuPos):
    tmpQos = 0
    for point in shapeNodes:
        tmpQos += calServiceRate(point, rsuPos)
    return tmpQos / len(shapeNodes)

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
    waitMax = 0
    for rsuCoord, wait in rsuWaits.items():
        tmpwaitMax = copy.deepcopy(rsuWaits)
        try:
            tmpwaitMax[rsuCoord] += cpu / rsuList[rsuCoord]['cpu']
            if calVariance(list(tmpwaitMax.values())) > waitMax:
                waitMax = calVariance(list(tmpwaitMax.values()))
        except:
            pass

    # ----------------decision process----------------
    # -----------here is a greedy algorithm-----------

    optimal = -1000
    for rsu, property in rsuList.items():
        if mem <= property['mem']:
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
                    break
            else:
                target = etaFinal[-1][0]
                serviceRoad = etaRoad[-1][0]
            # print(operationTime + transTime + relayTime,serviceRoad)
            # serviceRate = calServiceRate(target, rsuPos)
            waits = copy.deepcopy(rsuWaits)
            waits[rsu] += cpu / property['cpu']
            serviceRate = calIntegralServiceRate(getRoadLength(serviceRoad, net, boundaries), rsuPos)
            if CalMetric(serviceRate, list(waits.values()), waitMax) > optimal:
                optimal = CalMetric(serviceRate, list(waits.values()), waitMax)
                optimalRSU = rsuPos
                optimalRelay = relays
                optimalCore = core
                optimalServiceRoad = serviceRoad
    # print(optimalRSU, optimalRelay)
    tmpLines = []
    with open('rsus.csv', 'r') as file:
        while True:
            line = file.readline()
            if len(line) == 0:
                break
            tmp = line.split(' ')
            tmpLines.append(tmp)
    for tmp in tmpLines:
        if tmp[0] == '(' + str(int(optimalRSU[0])) + ',' + str(int(optimalRSU[1])) + ',3)':
            core = optimalCore
            tmpRelayTime = len(optimalRelay[1:]) * mem * 8 / 1024 / maxRate
            tmpOperationTime = cpu / rsuList['(' + str(int(optimalRSU[0])) + ',' + str(int(optimalRSU[1])) + ',3);' + str(optimalCore)]['cpu']
            if tmp[core] == '*':
                tmp[core] = taskName + '(0(' + str(int(transTime + tmpRelayTime + simTime + 7)) + ';'
            else:
                tmp[core] += taskName + '(0(' + str(int(transTime + tmpRelayTime + simTime + 7)) + ';'
            tmp[core + 5] = str(float(tmp[core + 5]) - mem)
            tmp[core + 9] = str(rsuList['(' + str(int(optimalRSU[0])) + ',' + str(int(optimalRSU[1])) + ',3);' + str(optimalCore)]['wait'] + transTime + tmpRelayTime + tmpOperationTime + simTime)
            if core == 4:
                tmp[core + 9] += '\n'
            break
    with open('rsus.csv', 'w') as file:
        file.write(''.join([' '.join(tmp) for tmp in tmpLines]))

    resultRSU = '(' + str(int(optimalRSU[0])) + ',' + str(int(optimalRSU[1])) + ',3);' + str(optimalCore) + '*1|'
    resultRelay = ''
    for node in optimalRelay:
        if node != optimalRSU and node != proxyPos:
            resultRelay += '(' + str(int(node[0])) + ',' + str(int(node[1])) + ',3);'
    if len(resultRelay) == 0:
        resultRelay += 'NULL'
    else:
        resultRelay = resultRelay[:-1]
    if len(result) == 0:
        with open('decision/' + taskName, 'w') as file:
            file.write('1')
    else:
        with open('decision/' + taskName, 'w') as file:
            file.write(str(proxyPos) + '\n' + resultRSU + '\n' + resultRelay + '\n' + optimalServiceRoad)
    rparam.set("decision", resultRSU)
    rparam.set("relay", resultRelay)
    rparam.set("service", optimalServiceRoad)
    return rparam



# if __name__ == "__main__":
#     rsuInfo = sys.argv[1][:-1].split(';')
#     vehPos = sys.argv[2].replace('(', '').replace(')', '')
#     vehPos = [float(vehPos.split(',')[0]), float(vehPos.split(',')[1])]
#     deadLinePos = sys.argv[3].replace('(', '').replace(')', '')
#     deadLinePos = [float(deadLinePos.split(',')[0]), float(deadLinePos.split(',')[1])]
#     cpu = float(sys.argv[4])
#     mem = float(sys.argv[5])
#     externalId = sys.argv[6]
#     roadId = sys.argv[7]
#     proxyPos = sys.argv[8].replace('(', '').replace(')', '')
#     proxyPos = [float(proxyPos.split(',')[0]), float(proxyPos.split(',')[1])]
#     net = sumolib.net.readNet('erlangen.net.xml')
#     boundaries = net.getBoundary()

#     # -----------rsuInfo parse-----------

#     rsuList = {}
#     for rsu in rsuInfo:
#         if float(rsu.split(':')[1].split('*')[2]) < maxTime:
#             rsuList[rsu.split(':')[0] + ';1'] = {
#                 'cpu': float(rsu.split(':')[1].split('*')[0]), 
#                 'mem': float(rsu.split(':')[1].split('*')[1]), 
#                 'wait': float(rsu.split(':')[1].split('*')[2]), 
#             }
#         if float(rsu.split(':')[1].split('*')[3]) < maxTime:
#             rsuList[rsu.split(':')[0] + ';2'] = {
#                 'cpu': float(rsu.split(':')[1].split('*')[0]), 
#                 'mem': float(rsu.split(':')[1].split('*')[1]), 
#                 'wait': float(rsu.split(':')[1].split('*')[3]), 
#             }
#         if float(rsu.split(':')[1].split('*')[4]) < maxTime:
#             rsuList[rsu.split(':')[0] + ';3'] = {
#                 'cpu': float(rsu.split(':')[1].split('*')[0]), 
#                 'mem': float(rsu.split(':')[1].split('*')[1]), 
#                 'wait': float(rsu.split(':')[1].split('*')[4]), 
#             }
#         if float(rsu.split(':')[1].split('*')[5]) < maxTime:
#             rsuList[rsu.split(':')[0] + ';4'] = {
#                 'cpu': float(rsu.split(':')[1].split('*')[0]), 
#                 'mem': float(rsu.split(':')[1].split('*')[1]), 
#                 'wait': float(rsu.split(':')[1].split('*')[5]), 
#             }

#     # -----------eta calculation-----------
#     etaRoad = []

#     etaRaw = []
#     etaFinal = []
#     etaStart = 0
#     with open('eta/' + externalId + '.csv', 'r') as file:
#         while True:
#             line = file.readline()
#             if len(line) == 0:
#                 break
#             etaRaw.append(line.strip().split(','))
#     flag = False
#     for i in range(len(etaRaw)):
#         tmpNode = net.getEdge(etaRaw[i][0]).getFromNode().getCoord()
#         tmpNode = [tmpNode[0] - boundaries[0], boundaries[3] - tmpNode[1]]
#         if etaRaw[i][0] == roadId:
#             etaFinal.append([tmpNode, 0])
#             etaRoad.append([etaRaw[i][0],0])
#             etaStart = float(etaRaw[i][1])
#             flag = True
#         elif abs(tmpNode[0] - deadLinePos[0]) < 0.05 and abs(tmpNode[1] - deadLinePos[1]) < 0.05:
#             etaFinal.append([tmpNode, float(etaRaw[i][1]) - etaStart])
#             etaRoad.append([etaRaw[i][0], float(etaRaw[i][1]) - etaStart])
#             break
#         elif flag:
#             etaFinal.append([tmpNode, float(etaRaw[i][1]) - etaStart])
#             etaRoad.append([etaRaw[i][0], float(etaRaw[i][1]) - etaStart])


#     # ----------------decision process----------------
#     # -----------here is a greedy algorithm-----------

#     decision = ''
#     result = []
#     distance = calDistance(proxyPos, vehPos)
#     transRate = min(5 * math.log2(1 + 2500 / distance), maxRate)
#     transTime = mem * 8 / 1000 / transRate
#     rsuWaits = {}
#     with open('rsus.csv', 'r') as file:
#         while True:
#             line = file.readline()
#             if len(line) == 0:
#                 break
#             line = line.strip().split(' ')
#             for i in range(3, len(line)):
#                 rsuWaits[line[0] + ';' + str(i - 2)] = (float(line[i]))
#     variance = calVariance(list(rsuWaits.values()))
#     optimal = -1000
#     for rsu, property in rsuList.items():
#         operationTime = cpu / property['cpu'] + property['wait']
#         rsuPos = rsu.replace('(', '').replace(')', '')
#         rsuPos = [float(rsuPos.split(',')[0]), float(rsuPos.split(',')[1])]
#         core = int(rsu.split(';')[1])
#         relays = relay(proxyPos, rsuPos)
#         relayTime = 0
#         for i in range(len(relays) - 1):
#             relayTime += mem * 8 / 1000 / maxRate
#         for eta in range(len(etaFinal)):
#             if etaFinal[eta][1] > operationTime + transTime + relayTime:
#                 target = etaFinal[eta - 1][0]
#                 serviceRoad = etaRoad[eta - 1][0]
#                 break
#         else:
#             target = etaFinal[-1][0]
#             serviceRoad = etaRoad[-1][0]
#         # print(operationTime + transTime + relayTime,serviceRoad)
#         varianceGain = calFairnessGain(rsu, rsuWaits, operationTime, variance)
#         # serviceRate = calServiceRate(target, rsuPos)
#         serviceRate = calIntegralServiceRate(getRoadLength(serviceRoad, net, boundaries), rsuPos)
#         if 0.5 * varianceGain + 0.5 * serviceRate > optimal:
#             optimal = 0.5 * varianceGain + 0.5 * serviceRate
#             optimalRSU = rsuPos
#             optimalRelay = relays
#             optimalCore = core
#             optimalServiceRoad = serviceRoad
#     # print(optimalRSU, optimalRelay)
#     resultRSU = '(' + str(int(optimalRSU[0])) + ',' + str(int(optimalRSU[1])) + ',3);' + str(optimalCore) + '*1|'
#     resultRelay = ''
#     for node in optimalRelay:
#         if node != optimalRSU and node != proxyPos:
#             resultRelay += '(' + str(int(node[0])) + ',' + str(int(node[1])) + ',3);'
#     if len(resultRelay) == 0:
#         resultRelay += 'NULL'
#     else:
#         resultRelay = resultRelay[:-1]
#     # print(resultRSU, resultRelay)
#     send(resultRSU, externalId + 'decision')
#     send(resultRelay, externalId + 'relay')
#     send(optimalServiceRoad, externalId + 'service')
#     print(resultRSU)
#     print(resultRelay)
#     print(optimalServiceRoad)

    

    
        
        
        