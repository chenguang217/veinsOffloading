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


# Record Mem usage
# Record task list and delays

if __name__ == "__main__":
    position = sys.argv[1]
    taskList = sys.argv[2].replace(' ', '')
    cpu = sys.argv[3]
    mem = sys.argv[4]
    wait1 = sys.argv[5]
    wait2 = sys.argv[6]
    wait3 = sys.argv[7]
    wait4 = sys.argv[8]
    simTime = sys.argv[9]
    with open('RSUlog/' + position, 'a') as file:
        # print(bytes(taskList, 'utf-8'))
        file.write(taskList.replace(' ', '') + ',' + mem + ',' + str(float(wait1) - float(simTime)) + str(float(wait2) - float(simTime)) + str(float(wait3) - float(simTime)) + str(float(wait4) - float(simTime)) + ',' + simTime + '\n')
    with open('rsus.csv', 'r') as file:
        rsuList = file.readlines()
    for i in range(len(rsuList)):
        if rsuList[i].split(' ')[0] == position:
            rsuList[i] = position + ' ' + cpu + ' ' + mem + ' ' + str(float(wait1) - float(simTime)) + ' ' + str(float(wait2) - float(simTime)) + ' ' + str(float(wait3) - float(simTime)) + ' ' + str(float(wait4) - float(simTime)) + '\n'
    with open('rsus.csv', 'w+') as file:
        file.writelines(rsuList)
                
        