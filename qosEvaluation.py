import os
import json
import sumolib
import xmltodict
import math

maxRate = 60

def calDistance(node1, node2):
    return math.sqrt((node1[0] - node2[0]) ** 2 + (node1[1] - node2[1]) ** 2)

def calQoS(src, dest):
    serviceDist = calDistance(src, dest)
    # print(serviceDist)
    return (5 * math.log2(1 + 2500 / serviceDist)) / maxRate

if __name__ == "__main__":
    net = sumolib.net.readNet('erlangen.net.xml')
    boundaries = net.getBoundary()
    routes = xmltodict.parse(open('erlangen.rou.xml').read())
    vehicles = {}
    result = {}
    taskDes = {}
    for route in routes['routes']['vehicle']:
        vehicles[route['@id']] = route['route']['@edges'].split(' ')
    taskSet = set()
    receiveNotComplete = set()
    files = os.listdir('taskLog')
    for name in files:
        if os.path.exists('taskLog/' + name.replace('.json', '').split('_')[0] + '.json'):
            with open('taskLog/' + name.replace('.json', '').split('_')[0] + '.json', 'r') as file:
                if file.read().strip() == 'success':
                    taskSet.add(name.replace('.json', '').split('_')[0])
        else:
            receiveNotComplete.add(name.replace('.json', '').split('_')[0])    
    decisions = os.listdir('decision')
    for name in decisions:
        with open('decision/' + name, 'r') as file:
            file.readline()
            tmp = file.readline().strip()[:-1].split('|')
            taskDes[name] = []
            for part in tmp:
                taskDes[name].append([part.split('*')[0], float(part.split('*')[1])])
    # handle receive complete task
    for task in taskSet:
        for name in [name for name in files if task in name]:
            if '_' in name:
                taskLog = json.load(open('taskLog/' + name))
                position = [float(taskLog['position'][1:-1].split(',')[0]), float(taskLog['position'][1:-1].split(',')[1])]
                servicePos = [float(taskLog['servicePos'][1:-1].split(',')[0]), float(taskLog['servicePos'][1:-1].split(',')[1])]
                if taskLog['serviceMode'] == '0':
                    minEdge = ['0', 50]
                    edges = net.getNeighboringEdges(position[0] + boundaries[0], boundaries[3] - position[1], 50)
                    for edge in edges:
                        if edge[0].getID() in vehicles[task[:-5]] and edge[1] < minEdge[1]:
                            minEdge = [edge[0].getID(), edge[1]]
                    shapes = net.getEdge(minEdge[0]).getShape()
                    tmpSum = 0
                    for point in shapes:
                        tmpSum += calQoS([point[0] + boundaries[0], boundaries[3] - point[1]], servicePos)
                    qos = tmpSum / len(shapes)
                    try:
                        result[task] += qos * float(taskLog['ratio'])
                    except:
                        result[task] = qos * float(taskLog['ratio'])
                elif '2+' in taskLog['serviceMode']:
                    minEdge = ['0', 50]
                    edges = net.getNeighboringEdges(position[0] + boundaries[0], boundaries[3] - position[1], 50)
                    for edge in edges:
                        if edge[0].getID() in vehicles[task[:-5]] and edge[1] < minEdge[1]:
                            minEdge = [edge[0].getID(), edge[1]]
                    shapes = net.getEdge(minEdge[0]).getShape()
                    tmpNodeList = []
                    tmpLength = 0
                    tmpSum = 0
                    for i in range(len(shapes) - 1):
                        if calDistance(shapes[i], shapes[i + 1]) + tmpLength <= float(taskLog['serviceMode'][2:]):
                            tmpLength += calDistance(shapes[i], shapes[i + 1])
                            tmpNodeList.append(shapes[i])
                            tmpNodeList.append(shapes[i + 1])
                        else:
                            tmpNodeList.append(shapes[i])
                            tmpRatio = (float(taskLog['serviceMode'][2:]) - tmpLength) / calDistance(shapes[i], shapes[i + 1])
                            tmpNodeList.append([shapes[i][0] + tmpRatio * (shapes[i + 1][0] - shapes[i][0]), shapes[i][1] + tmpRatio * (shapes[i + 1][1] - shapes[i][1])])
                            tmpLength = float(taskLog['serviceMode'][2:])
                        if tmpLength == float(taskLog['serviceMode'][2:]):
                            break
                    if tmpLength < float(taskLog['serviceMode'][2:]):
                        for i in range(vehicles[task[:-5]].index(minEdge[0]) + 1, len(vehicles[task[:-5]])):
                            shapes = net.getEdge(vehicles[task[:-5]][i]).getShape()
                            for i in range(len(shapes) - 1):
                                if calDistance(shapes[i], shapes[i + 1]) + tmpLength <= float(taskLog['serviceMode'][2:]):
                                    tmpLength += calDistance(shapes[i], shapes[i + 1])
                                    tmpNodeList.append(shapes[i])
                                    tmpNodeList.append(shapes[i + 1])
                                else:
                                    tmpNodeList.append(shapes[i])
                                    tmpRatio = (float(taskLog['serviceMode'][2:]) - tmpLength) / calDistance(shapes[i], shapes[i + 1])
                                    tmpNodeList.append([shapes[i][0] + tmpRatio * (shapes[i + 1][0] - shapes[i][0]), shapes[i][1] + tmpRatio * (shapes[i + 1][1] - shapes[i][1])])
                                    tmpLength = float(taskLog['serviceMode'][2:])
                                if tmpLength == float(taskLog['serviceMode'][2:]):
                                    break
                            if tmpLength == float(taskLog['serviceMode'][2:]):
                                break
                    for point in tmpNodeList:
                        tmpSum += calQoS([point[0] + boundaries[0], boundaries[3] - point[1]], servicePos)
                    qos = tmpSum / len(tmpNodeList)
                    try:
                        result[task] += qos * float(taskLog['ratio'])
                    except:
                        result[task] = qos * float(taskLog['ratio'])
    # handle not receive complete tasks
    for task in receiveNotComplete:
        tmpRatioSum = 1
        tmpQos = 0
        tmpReceived = {}
        for name in [name for name in files if task in name]:
            taskLog = json.load(open('taskLog/' + name))
            tmpRatioSum -= float(taskLog['ratio'])
            try:
                tmpReceived[taskLog['servicePos']].append(float(taskLog['ratio']))
            except:
                tmpReceived[taskLog['servicePos']] = [float(taskLog['ratio'])]
            position = [float(taskLog['position'][1:-1].split(',')[0]), float(taskLog['position'][1:-1].split(',')[1])]
            servicePos = [float(taskLog['servicePos'][1:-1].split(',')[0]), float(taskLog['servicePos'][1:-1].split(',')[1])]
            if taskLog['serviceMode'] == '0':
                minEdge = ['0', 50]
                edges = net.getNeighboringEdges(position[0] + boundaries[0], boundaries[3] - position[1], 50)
                for edge in edges:
                    if edge[0].getID() in vehicles[task[:-5]] and edge[1] < minEdge[1]:
                        minEdge = [edge[0].getID(), edge[1]]
                shapes = net.getEdge(minEdge[0]).getShape()
                tmpSum = 0
                for point in shapes:
                    tmpSum += calQoS([point[0] + boundaries[0], boundaries[3] - point[1]], servicePos)
                tmpQos += tmpSum / len(shapes) * float(taskLog['ratio'])
            elif '2+' in taskLog['serviceMode']:
                minEdge = ['0', 50]
                edges = net.getNeighboringEdges(position[0] + boundaries[0], boundaries[3] - position[1], 50)
                for edge in edges:
                    if edge[0].getID() in vehicles[task[:-5]] and edge[1] < minEdge[1]:
                        minEdge = [edge[0].getID(), edge[1]]
                shapes = net.getEdge(minEdge[0]).getShape()
                tmpNodeList = []
                tmpLength = 0
                tmpSum = 0
                for i in range(len(shapes) - 1):
                    if calDistance(shapes[i], shapes[i + 1]) + tmpLength <= float(taskLog['serviceMode'][2:]):
                        tmpLength += calDistance(shapes[i], shapes[i + 1])
                        tmpNodeList.append(shapes[i])
                        tmpNodeList.append(shapes[i + 1])
                    else:
                        tmpNodeList.append(shapes[i])
                        tmpRatio = (float(taskLog['serviceMode'][2:]) - tmpLength) / calDistance(shapes[i], shapes[i + 1])
                        tmpNodeList.append([shapes[i][0] + tmpRatio * (shapes[i + 1][0] - shapes[i][0]), shapes[i][1] + tmpRatio * (shapes[i + 1][1] - shapes[i][1])])
                        tmpLength = float(taskLog['serviceMode'][2:])
                    if tmpLength == float(taskLog['serviceMode'][2:]):
                        break
                if tmpLength < float(taskLog['serviceMode'][2:]):
                    for i in range(vehicles[task[:-5]].index(minEdge[0]) + 1, len(vehicles[task[:-5]])):
                        shapes = net.getEdge(vehicles[task[:-5]][i]).getShape()
                        for i in range(len(shapes) - 1):
                            if calDistance(shapes[i], shapes[i + 1]) + tmpLength <= float(taskLog['serviceMode'][2:]):
                                tmpLength += calDistance(shapes[i], shapes[i + 1])
                                tmpNodeList.append(shapes[i])
                                tmpNodeList.append(shapes[i + 1])
                            else:
                                tmpNodeList.append(shapes[i])
                                tmpRatio = (float(taskLog['serviceMode'][2:]) - tmpLength) / calDistance(shapes[i], shapes[i + 1])
                                tmpNodeList.append([shapes[i][0] + tmpRatio * (shapes[i + 1][0] - shapes[i][0]), shapes[i][1] + tmpRatio * (shapes[i + 1][1] - shapes[i][1])])
                                tmpLength = float(taskLog['serviceMode'][2:])
                            if tmpLength == float(taskLog['serviceMode'][2:]):
                                break
                        if tmpLength == float(taskLog['serviceMode'][2:]):
                            break
                for point in tmpNodeList:
                    tmpSum += calQoS([point[0] + boundaries[0], boundaries[3] - point[1]], servicePos)
                tmpQos += tmpSum / len(tmpNodeList) * float(taskLog['ratio'])
        # print(result, round(tmpRatioSum, 2), tmpReceived)
        # print(tmpReceived['(2500,4500,3)'])
        for part in taskDes[task]:
            if len(tmpReceived[part[0].split(';')[0]]) == 4:
                continue
            # print(tmpReceived[part[0].split(';')[0]], part[1])
            if part[1] in tmpReceived[part[0].split(';')[0]]:
                tmpIndex = tmpReceived[part[0].split(';')[0]].index(part[1])
                tmpReceived[part[0].split(';')[0]] = tmpReceived[part[0].split(';')[0]][:tmpIndex] + tmpReceived[part[0].split(';')[0]][tmpIndex + 1:]
            else:
                tmpRatioSum -= part[1]
                servicePos = part[0].split(';')[0]
                servicePos = [float(servicePos[1:-1].split(',')[0]), float(servicePos[1:-1].split(',')[1])]
                with open('sendbackLog/' + part[0], 'r') as file:
                    while True:
                        line = file.readline().strip()
                        if len(line) == 0:
                            break
                        tmp = line.split(',')
                        if tmp[0] == task:
                            shapes = net.getEdge(tmp[-1]).getShape()
                            tmpSum = 0
                            for point in shapes:
                                tmpSum += calQoS([point[0] + boundaries[0], boundaries[3] - point[1]], servicePos)
                            tmpQos += tmpSum / len(shapes) * part[1]
                            break
        if round(tmpRatioSum, 2) == 0:
            result[task] = tmpQos
        else:
            print('task ' + task + ' is not finished')
    result = json.dumps(result, indent='\t')
    with open('qosResult.json', 'w', newline='\r\n') as file:
        file.write(result)

                # print(position)
