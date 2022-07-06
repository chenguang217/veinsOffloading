import json
import math
import mmap
import os
import random
import sys

import sumolib


#写入一次
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

def calETA(net, begin, target, externalID):
    fromLon, fromLat = net.convertXY2LonLat(begin[0], begin[1])
    toLon, toLat = net.convertXY2LonLat(target[0], target[1])
    # 通过经纬度计算ETA, 待获取数据并计算结果
    return random.randint(0, 30)

RSUs = []

if __name__ == "__main__":
    info = sys.argv[1]
    net = sumolib.net.readNet('erlangen.net.xml')
    externalID = sys.argv[2]
    road = sys.argv[3]
    rsuInfo = sys.argv[4]
    position = sys.argv[5]
    rsuRaw = rsuInfo.split(';')
    for info in rsuRaw:
        if len(info) != 0:
            rsuId = info.split(':')[0]
            rsuLocation = info.split(':')[1].split('*')[0]
            rsuCpu = info.split(':')[1].split('*')[1]
            rsuMem = info.split(':')[1].split('*')[2]
            rsuWait = info.split(':')[1].split('*')[3]
            RSUs.append({'rsuId': rsuId, 'rsuLocation': rsuLocation, 'rsuCpu': rsuCpu, 'rsuMem': rsuMem, 'rsuWait': rsuWait})
    begin = net.getEdge(road).getFromNode().getCoord()
    roads = []
    with open('routesV/' + externalID + '.csv', 'r') as file:
        while True:
            line = file.readline()
            if len(line) == 0:
                break
            roads.append(line.split(' ')[0])
    # -----------generate deadline-----------

    # with open('node2RSU.json', 'r') as file:
    #     node2RSU = json.loads(file.read().strip())
    # deadLinePosition = random.choice(roads[roads.index(road):]) # 把deadline加入到新的共享内存中

    deadLinePosition = '23339459'
    deadPosition = net.getEdge(deadLinePosition).getFromNode().getCoord()
    boundaries = net.getBoundary()
    send(str(deadPosition[0] - boundaries[0]), 'deadLinePosX')
    send(str((boundaries[3] - boundaries[1]) - (deadPosition[1] - boundaries[1])), 'deadLinePosY')
    

    # for road in roads[roads.index(road):roads.index(deadLinePosition)]:
    #     target = net.getEdge(road).getToNode().getCoord()
    #     eta = calETA(net, begin, target, externalID)

    # -----------decision process-----------
    minimum = 10000
    rsuTarget = ''
    for rsu in RSUs:
        rsuX = float(rsu['rsuLocation'][1:-1].split(',')[0])
        rsuY = float(rsu['rsuLocation'][1:-1].split(',')[1])
        print(position)
        X = float(position[1:-1].split(',')[0])
        Y = float(position[1:-1].split(',')[1])
        distance = math.sqrt((rsuX - X) ** 2 + (rsuY - Y) ** 2)
        if minimum > distance:
            minimum = distance
            rsuTarget = rsu['rsuId']
    decision = ''
    for rsu in RSUs:
        decision += rsu['rsuId'] + '|' + rsu['rsuLocation'] + ':'
        if rsu['rsuId'] == rsuTarget:
            decision += 'true;'
        else:
            decision += 'false;'
    send(externalID + ''.join(random.sample('zyxwvutsrqponmlkjihgfedcba',5)), 'global_share_memory')
    send(decision, 'decision')
    exit()

    

    # print(roads)
    # print(roads[roads.index(road):roads.index(deadLinePosition)])
    




    # ETA到达时间 晚于 执行完成时间
    # 截止时间ETA位置 与其他RSU距离 选取最近的
    # 每个RSU都有一个执行完成的ETA位置，这个位置最好选取相对最近的，数据获取并渲染后可获得服务时的最低延迟。
