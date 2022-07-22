import json
import math
import mmap
import os
import random
import sys
from turtle import pos

import numpy as np
import sumolib
from zmq import proxy

size = (2600, 3000)
maxRate = 30

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
        if k == rsu:
            tmpWait.append(v + operationTime)
        else:
            tmpWait.append(v)
    tmpVariance = calVariance(tmpWait)
    return variance - tmpVariance

def calServiceRate(target, rsuPos):
    serviceDist = calDistance(target, rsuPos)
    return 5 * math.log2(1 + 250 / serviceDist)

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
        rsuList[rsu.split(':')[0]] = {
            'cpu': float(rsu.split(':')[1].split('*')[0]), 
            'mem': float(rsu.split(':')[1].split('*')[1]), 
            'wait': float(rsu.split(':')[1].split('*')[2]), 
        }
        # if rsu.split(':')[0] == '(2500,1500,3)':
        #     rsuList[rsu.split(':')[0]]['cpu'] = 2

    # -----------eta calculation-----------

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
            etaStart = float(etaRaw[i][1])
            flag = True
        elif abs(tmpNode[0] - deadLinePos[0]) < 0.05 and abs(tmpNode[1] - deadLinePos[1]) < 0.05:
            etaFinal.append([tmpNode, float(etaRaw[i][1]) - etaStart])
            break
        elif flag:
            etaFinal.append([tmpNode, float(etaRaw[i][1]) - etaStart])

    # -----------decision process-----------
    # -----------here is a greedy algorithm-----------

    decision = ''
    result = []
    distance = calDistance(proxyPos, vehPos)
    transRate = min(5 * math.log2(1 + 250 / distance), maxRate)
    transTime = mem * 8 / 1000 / transRate
    rsuWaits = {}
    with open('rsus.csv', 'r') as file:
        while True:
            line = file.readline()
            if len(line) == 0:
                break
            line = line.strip().split(' ')
            rsuWaits[line[0]] = (float(line[3]))
    variance = calVariance(list(rsuWaits.values()))
    optimal = -1000
    optimalRSU = []
    optimalRelay = []
    for rsu, property in rsuList.items():
        operationTime = cpu / property['cpu'] + property['wait']
        rsuPos = rsu.replace('(', '').replace(')', '')
        rsuPos = [float(rsuPos.split(',')[0]), float(rsuPos.split(',')[1])]
        relays = relay(proxyPos, rsuPos)
        relayTime = 0
        for i in range(len(relays) - 1):
            relayTime += mem * 8 / 1000 / maxRate
        for eta in etaFinal:
            if eta[1] > operationTime + transTime + relayTime:
                target = eta[0]
                break
        else:
            target = etaFinal[-1][0]
        varianceGain = calFairnessGain(rsu, rsuWaits, operationTime, variance)
        serviceRate = calServiceRate(target, rsuPos)
        if 0.5 * varianceGain + 0.5 * serviceRate > optimal:
            optimal = 0.5 * varianceGain + 0.5 * serviceRate
            optimalRSU = rsuPos
            optimalRelay = relays
    # print(optimalRSU, optimalRelay)
    resultRSU = '(' + str(int(optimalRSU[0])) + ',' + str(int(optimalRSU[1])) + ',3)'
    resultRelay = ''
    for node in optimalRelay:
        if node != optimalRSU and node != proxyPos:
            resultRelay += '(' + str(int(node[0])) + ',' + str(int(node[1])) + ',3);'
    # print(resultRSU, resultRelay)
    send(resultRSU, externalId + 'decision')
    send(resultRelay[:-1], externalId + 'relay')
    

    
        
        
        