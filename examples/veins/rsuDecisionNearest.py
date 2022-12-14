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

def calDistance(node1, node2):
    return math.sqrt((node1[0] - node2[0]) ** 2 + (node1[1] - node2[1]) ** 2)

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

    core = random.randint(1, 4)
    decision = param.getString("proxyPos") + ';' + str(core)
    relay = 'NULL'
    serviceRoad = roadId
    distance = calDistance(proxyPos, vehPos)
    transRate = min(5 * math.log2(1 + 2500 / distance), maxRate)
    transTime = mem * 8 / 1024 / transRate

    tmpLines = []
    with open('rsus.csv', 'r') as file:
        while True:
            line = file.readline()
            if len(line) == 0:
                break
            tmp = line.split(' ')
            tmpLines.append(tmp)
    for tmp in tmpLines:
        if tmp[0] == param.getString("proxyPos"):
            tmpOperationTime = cpu / rsuList[decision]['cpu']
            if tmp[core] == '*':
                tmp[core] = taskName + '(0(' + str(int(transTime + simTime + 7)) + ';'
            else:
                tmp[core] += taskName + '(0(' + str(int(transTime + simTime + 7)) + ';'
            tmp[core + 5] = str(float(tmp[core + 5]) - mem)
            tmp[core + 9] = str(rsuList[decision]['wait'] + transTime + tmpOperationTime + simTime)
            if core == 4:
                tmp[core + 9] += '\n'
            break
    with open('rsus.csv', 'w') as file:
        file.write(''.join([' '.join(tmp) for tmp in tmpLines]))
    rparam.set("decision", decision + '*1')
    rparam.set("relay", relay)
    rparam.set("service", serviceRoad)
    return rparam

