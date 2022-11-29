import collections
import copy
import json
import math
import mmap
import os
import random
import sys
from turtle import pos
from datetime import datetime
from typing import List, Iterable
from operator import itemgetter


import pandas as pd
import numpy as np
import sumolib


size = (2600, 3000)
maxRate = 60
maxTime = 200
bestallocation = []
resultserviceRoad = {}


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
        
        
def num_pieces(num, length):
    ot = list(range(1, length+1))[::-1]
    all_list = []
    for i in range(length-1):
        n = random.randint(1, num-ot[i])
        all_list.append(n)
        num -= n
    all_list.append(num) 
    return all_list


def CalMetric(serviceRate, waits):
    if calVariance(waits) == 0:
        return 0.5 * serviceRate
    else:
        # print("faireness is " + str(1 - calVariance(waits) / calVariance(waitMax)))
        # return 0.5 * serviceRate + 0.5 * (1 - calVariance(waits) / calVariance(waitMax))
        metric = 0.5 * serviceRate + 0.5 * (1 / calVariance(waits))
        # print("metric is " + str(metric))
        return metric


class Gene:
    def __init__(self, **data):
        self.__dict__.update(data)
        self.size = len(data['data'])


class GA:
    def __init__(self, parameter):
        self.parameter = parameter
        self.maxpiece = self.parameter[4]
        self.rsuList = self.parameter[5]
        self.cpu = self.parameter[6]
        self.mem = self.parameter[7]
        self.boundaries = self.parameter[8]
        self.etaRoad = self.parameter[10]
        self.etaFinal = self.parameter[11]
        self.rsuWaits = self.parameter[12]
        self.proxyPos = self.parameter[13]
        self.transTime = self.parameter[14]
        self.net = self.parameter[15]
        self.allnum = 100 
        
        pop = []
        
        count = 0
        nlength = len(self.rsuList)
        while count < self.parameter[3]:
            flag1 = 1
            temp = num_pieces(self.allnum, nlength)
            for i in range(nlength):
                if temp[i] > self.maxpiece[i]:
                    flag1 = 0
                    break
            if flag1 == 1:
                fitness = self.evaluate(temp)
                pop.append({'Gene': Gene(data=temp), 'fitness':fitness})
                # pop.append({'Gene': Gene(data=temp)})
                count += 1
            # print(temp)
        self.pop = pop
        # print(len(self.pop))
        self.bestindividual = self.selectBest(self.pop)
        # print(self.bestindividual)
        
    
    def evaluate(self, geneinfo):
        nlength = len(self.rsuList)
        relayTime = 0
        allserviceRoad = {}
        
        # Judge whether the individual meets the constraints
        for i in range(nlength):
            if geneinfo[i] > self.maxpiece[i]:
                allcost = 100000000
                return 1 / allcost
        
        tmpWait = []
        serviceRate = 0
        for items in self.rsuWaits.values():
            # print("item is " + str(items))        
            tmpWait.append(items)
        
        # parse and calculate the fitness    
        for i in range(nlength): 
            if geneinfo[i] == 0:
                continue
            rsuPos = list(self.rsuList.keys())[i].replace('(', '').replace(')', '')
            rsuPos = [int(rsuPos.split(',')[0]), int(rsuPos.split(',')[1])]
            cpuTmp = self.rsuList[list(self.rsuList.keys())[i]]['cpu']
            operationTime = self.cpu / (cpuTmp * self.allnum / geneinfo[i]) + self.rsuList[list(self.rsuList.keys())[i]]['wait']
            relays = relay(self.proxyPos, rsuPos)
            for j in range(len(relays) - 1):
                relayTime += self.mem * 8 / 1000 / maxRate
            for eta in range(len(self.etaFinal)):
                if self.etaFinal[eta][1] > operationTime + self.transTime + relayTime:
                    # print("transTime is " + str(transTime))
                    serviceRoad = etaRoad[eta - 1][0]
                    break
            else:
                serviceRoad = etaRoad[-1][0]
            allserviceRoad[list(rsuList.keys())[i]] = (i, serviceRoad)
            # print("here is one " + str(list(self.rsuWaits.keys())))
            # print("here is two " + str(list(self.rsuWaits.keys()).index(list(self.rsuList.keys())[i])))
            # print("here is three " + str(tmpWait))
            # print("here is four " + str(list(self.rsuList.keys())[i]))
            tmpWait[int(list(self.rsuWaits.keys()).index(list(self.rsuList.keys())[i]))] += (self.cpu * geneinfo[i]) / (cpuTmp * self.allnum)
        # print("tmpWait is " + str(tmpWait))
        # print("allserviceRoad is " + str(allserviceRoad))
        for rsu, property in allserviceRoad.items():
            tmpPos = rsu.replace('(', '').replace(')', '')
            tmpPos = [int(tmpPos.split(',')[0]), int(tmpPos.split(',')[1])]
            # print("property is " + str(property))
            serviceRate += calIntegralServiceRate(getRoadLength(property[1], self.net, self.boundaries), tmpPos) * geneinfo[property[0]] / self.allnum
        # print("serviceRate is " + str(serviceRate))    
        # tmpWaitMax = copy.deepcopy(list(self.rsuWaits.values()))
        # tmpWaitMax[tmpWaitMax.index(max(tmpWaitMax))] += self.cpu / list(self.rsuList.values())[tmpWaitMax.index(max(tmpWaitMax))]['cpu']
        # print("tmpWaitMax is " + str(tmpWaitMax))
        
        # calculate the fitness of this individual
        metric = CalMetric(serviceRate, tmpWait)
        
        return metric
    
    
    def getserviceRoad(self, geneinfo):
        nlength = len(self.rsuList)
        # print("geneinfo is " + str(geneinfo))
        # print("rsuList.keys() is " + str(list(rsuList.keys())))
        relayTime = 0
        allserviceRoad = {}
        for i in range(nlength): 
            if geneinfo[i] == 0:
                continue
            rsuPos = list(self.rsuList.keys())[i].replace('(', '').replace(')', '')
            rsuPos = [int(rsuPos.split(',')[0]), int(rsuPos.split(',')[1])]
            # print("rsuPos is " + str(rsuPos))
            cpuTmp = self.rsuList[list(self.rsuList.keys())[i]]['cpu']
            operationTime = self.cpu / (cpuTmp * self.allnum / geneinfo[i]) + self.rsuList[list(self.rsuList.keys())[i]]['wait']
            relays = relay(self.proxyPos, rsuPos)
            # print("self.etaFinal is " + str(self.etaFinal))
            for j in range(len(relays) - 1):
                relayTime += self.mem * 8 / 1000 / maxRate
            for eta in range(len(self.etaFinal)):
                if self.etaFinal[eta][1] > operationTime + self.transTime + relayTime:
                    # print("transTime is " + str(transTime))
                    serviceRoad = etaRoad[eta - 1][0]
                    break
            else:
                serviceRoad = etaRoad[-1][0]
            allserviceRoad[list(rsuList.keys())[i]] = (i, serviceRoad)
        print("allserviceRoad is " + str(allserviceRoad))
        return allserviceRoad
        
        
    def selectBest(self, pop):
        s_inds = sorted(pop, key=itemgetter("fitness"), reverse=True)
        return s_inds[0]
    
    
    def selection(self, individuals, k):
        # print("k is " + str(k))
        # print("individuals is " + str(len(individuals)))
        s_inds = sorted(individuals, key=itemgetter("fitness"), reverse=True)
        sum_fits = sum(ind['fitness'] for ind in individuals)
        # print("s_inds is " + str(len(s_inds)))
        # print("sum_fits is " + str(sum_fits))
        
        chosen = []
        for i in range(k):
            u = random.random() * sum_fits
            sum_ = 0
            for ind in s_inds:
                sum_ += ind['fitness']
                if sum_ >= u:
                    chosen.append(ind)
                    break
        chosen = sorted(chosen, key=itemgetter("fitness"), reverse=False)
        # print("chosen is " + str(len(chosen)))
        return chosen
    
    
    def crossoperate(self, offspring):
        newoff1 = Gene(data=[])
        newoff2 = Gene(data=[])
        
        geninfo1 = offspring[0]['Gene'].data
        geninfo2 = offspring[1]['Gene'].data
        
        edge_point1 = []
        edge_point2 = []
        
        temp_list1 = []
        temp_list2 = []
        
        flag1 = 0
        flag2 = 0
        
        # print(geninfo1)
        # print(geninfo2)
        
        for i in range(len(geninfo1)):
            temp_list1.append(int((geninfo1[i] + geninfo2[i]) / 2))
            temp_list2.append(int((geninfo1[len(geninfo1) - 1 - i] + geninfo2[len(geninfo1) - 1 - i]) / 2))
            
        for i in range(len(geninfo1)):
            flag1 += temp_list1[i]
            flag2 += temp_list2[i]
        div1 = self.allnum - flag1
        div2 = self.allnum - flag2
        
        for i in range(div1):
            temp_list1[i] += 1
            
        for i in range(div2):
            temp_list2[i] += 1
        
        newoff1.data = temp_list1
        newoff2.data = temp_list2
        return newoff1, newoff2
    
    
    def mutation(self, crossoff):
        a_gene = crossoff.data
        length = len(a_gene)
        a_flag = random.randint(0, length - 1)
        b_flag = random.randint(0, length - 1)
        
        temp = int(a_gene[a_flag] / 2)
        
        a_gene[a_flag] -= temp
        a_gene[b_flag] += temp
        
        crossoff.data = a_gene
       
        return crossoff
    
    
    def GA_main(self):
        popsize = self.parameter[3]
        # print(popsize)
        best_individual = []
        max_fit = 0
        print("Start of evolution")
        # Begin the evolution
        for g in range(NGEN):
            print("############### Generation {} ###############".format(g))
            
            # Apply selection based on their converted fitness
            elite_individual1 = self.selectBest(self.pop)
            self.pop.remove(elite_individual1)
            # print(len(self.pop))
            elite_individual2 = self.selectBest(self.pop)
            self.pop.remove(elite_individual2)
            selectpop = self.selection(self.pop, popsize-2)
            # print(len(selectpop))
            # print(len(self.pop))
            nextoff = []
            nextoff.append(elite_individual1)
            nextoff.append(elite_individual2)
            
            '''
                elite_individual1 = selectpop.pop()
                elite_individual2 = selectpop.pop()
                nextoff.append(elite_individual1)
                nextoff.append(elite_individual2)
            '''
            
            while len(nextoff) != popsize:
                # Apply crossover and mutation on the offspring
                
                # Select two individuals
                # print("selectpop is " + str(len(selectpop)))
                # print("nextoff is " + str(len(nextoff)))
                offspring = [selectpop.pop() for _ in range(2)]
                if random.random() < CXPB: # cross two individuals with probability CXPB
                    crossoff1, crossoff2 = self.crossoperate(offspring)
                    if random.random() < MUTPB: # mutate an individual with probability MUTPB
                        muteoff1 = self.mutation(crossoff1)
                        muteoff2 = self.mutation(crossoff2)
                        fit_muteoff1 = self.evaluate(muteoff1.data) # Evaluate the individuals
                        fit_muteoff2 = self.evaluate(muteoff2.data) # Evaluate the individuals
                        nextoff.append({'Gene':muteoff1, 'fitness':fit_muteoff1})
                        nextoff.append({'Gene':muteoff2, 'fitness':fit_muteoff2})
                    else:
                        fit_crossoff1 = self.evaluate(crossoff1.data) # Evaluate the individuals
                        fit_crossoff2 = self.evaluate(crossoff2.data) # Evaluate the individuals
                        nextoff.append({'Gene':crossoff1, 'fitness':fit_crossoff1})
                        nextoff.append({'Gene':crossoff2, 'fitness':fit_crossoff2})
                        
                else:
                    nextoff.extend(offspring)
            
            # The population is entirely replaced by the offspring
            self.pop = nextoff
            
            # Gather all the fitnesses in one list and print the stats
            fits = [ind['fitness'] for ind in self.pop]
            
            best_ind = self.selectBest(self.pop)
            
            if best_ind['fitness'] > self.bestindividual['fitness']:
                self.bestindividual = best_ind
                
            # print("Best individual found is {}, {}".format(self.bestindividual['Gene'].data,
            #                                            self.bestindividual['fitness']))
            print("  Max fitness of current pop: {}".format(max(fits)))
            print("  Min value of aim_func: {}".format(1 / max(fits)))
            if max_fit < max(fits):
                best_individual = self.bestindividual['Gene'].data
        print("------ End of (successful) evolution ------")
        '''
        with open('best_individual1.txt', 'w') as ft:
            ft.write(str(best_individual))
            ft.close()
        '''
        tmpserviceRoad = self.getserviceRoad(best_individual)
        # resultserviceRoad = tmpserviceRoad.copy()
        for key, value in tmpserviceRoad.items():
            resultserviceRoad[key] = value
        # print("resultserviceRoad is " + str(resultserviceRoad))
        for num in best_individual:
            bestallocation.append(num)
        

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
    # print("the finalRelay is " + str(finalRelay))
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


if __name__ == "__main__":
    # print("point 1")
    rsuInfo = sys.argv[1][:-1].split(';')
    vehPos = sys.argv[2].replace('(', '').replace(')','')
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
    # print("point 2")
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
    # print(rsuList)        
    
            
    # -----------eta calculation-----------
    # print("point 3")
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
            break
        elif flag:
            etaFinal.append([tmpNode, float(etaRaw[i][1]) - etaStart])
            etaRoad.append([etaRaw[i][0], float(etaRaw[i][1]) - etaStart])
    # print("etaFinal is " + str(etaFinal))
    
    # ---------------get rsu task list----------------
    # print("point 4")
    # decision = ''
    # result = []
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
    # -------here is a genetic algorithm-------
    # print("point 5")
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
                # print("transTime is " + str(transTime))
                target = etaFinal[eta - 1][0]
                serviceRoad = etaRoad[eta - 1][0]
                serviceRoadList[rsu] = serviceRoad
                break
        else:
            target = etaFinal[-1][0]
            serviceRoad = etaRoad[-1][0]
            serviceRoadList[rsu] = serviceRoad
    print("serviceRoadList is " + str(serviceRoadList))
    
    maxpiece = []
    for rsu, property in rsuList.items():
        maxpiece.append(math.floor(min(rsuList[rsu]['mem'], mem) / mem * 100))
    # print("maxpiece is " + str(maxpiece))
    
    # corenum = len(rsuList)
    # print(corenum)
    # allpiece = 100
    CXPB, MUTPB, NGEN, popsize = 0.9, 0.2, 100, 40
    # all_list = num_pieces(allpiece, corenum)
    # print(all_list)
    para1 = [CXPB, MUTPB, NGEN, popsize, maxpiece, rsuList, cpu, mem, boundaries, serviceRoadList, etaRoad, etaFinal, rsuWaits, 
             proxyPos, transTime, net]
    # print(para1)
    run = GA(para1)
    run.GA_main()
    
    
    decision = ''
    resultRelay = ''
    serviceRoad = ''
    # print("bestallocation is " + str(bestallocation))
    # print("resultserviceRoad is " + str(resultserviceRoad))
    for k in range(len(bestallocation)):
        tmpResultRelay = ''
        rsuPos = list(rsuList.keys())[k].replace('(', '').replace(')', '')
        rsuPos = [float(rsuPos.split(',')[0]), float(rsuPos.split(',')[1])]
        decision += str(list(rsuList.keys())[k]) + '*' + str(bestallocation[k] / 100) + '|'
        tmpRelay = relay(proxyPos, rsuPos)
        for node in tmpRelay:
            if node != rsuPos and node != proxyPos:
                tmpResultRelay += '(' + str(int(node[0])) + ',' + str(int(node[1])) + ',3);'
        if len(tmpResultRelay) == 0:
            tmpResultRelay += 'NULL'
        resultRelay += tmpResultRelay + '|'
    # serviceRoad += str(resultserviceRoad[list(rsuList.keys())[k]]) + '|'
    for key, value in resultserviceRoad.items():
        serviceRoad += value[1] + '|'
    

    # decision = '(1500,2500,3);4*1|'
    # resultRelay = '(1500,1500,3);|'
    # serviceRoad = '23339459|'
    print(decision)
    print(resultRelay)
    print(serviceRoad)
    send(decision, externalId + 'decision')
    send(resultRelay, externalId + 'relay')
    send(serviceRoad, externalId + 'service')
    
    

