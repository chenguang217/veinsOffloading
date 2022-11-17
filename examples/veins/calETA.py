import json
import math
import mmap
import os
import random
import sys
from turtle import pos

import sumolib

size = (2600, 3000)


def calETA(net, begin, target):
    fromLon, fromLat = net.convertXY2LonLat(begin[0], begin[1])
    toLon, toLat = net.convertXY2LonLat(target[0], target[1])
    # 通过经纬度计算ETA, 待获取数据并计算结果
    return random.randint(0, 30)


if __name__ == "__main__":
    road = '-4099270#1'
    roads = []
    etaRaw = []
    with open('routesV/' + 'test2.csv', 'r') as file:
        while True:
            line = file.readline()
            if len(line) == 0:
                break
            roads.append(line.split(' ')[0].strip())
    # print(roads)
    net = sumolib.net.readNet('erlangen.net.xml')
    begin = net.getEdge(road).getToNode().getCoord()
    # print(begin)
    deadLinePosition = random.choice(roads[roads.index(road) + 3:])

    deadPosition = net.getEdge(deadLinePosition).getFromNode().getCoord()
    boundaries = net.getBoundary()
    deadPosition = [deadPosition[0] - boundaries[0], boundaries[3] - deadPosition[1]]

    # print(deadPosition)

    for road in roads[roads.index(road):roads.index(deadLinePosition)]:
        target = net.getEdge(road).getToNode().getCoord()
        eta = calETA(net, begin, target)
        print(eta)

    with open('eta/' + 'test1.csv', 'r') as file:
        while True:
            line = file.readline()
            if len(line) == 0:
                break
            etaRaw.append(line.strip().split(','))
    print(etaRaw)



