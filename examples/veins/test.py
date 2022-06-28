import mmap
import json
import sys
import random
import sumolib
import os

#写入一次
def send(s):
    s=s+100*' '
    infosize=len(s)+1
    byte=s.encode(encoding='UTF-8')
    #从头读取内存，不然的话会接着上次的内存位置继续写下去，这里是从头覆盖。
    shmem=mmap.mmap(0,1000,file_name,mmap.ACCESS_WRITE)
    shmem.write(byte)

def recieve():
    shmem=mmap.mmap(0,100,file_name,mmap.ACCESS_READ)
    s=str(shmem.read(shmem.size()).decode("utf-8"))
    #vs2012早期版本会有截断符和开始符号，需要提取有用字符串
    es='\\x00'#字符条件截断，还没有设计开始endstring
    if s.find(es)==-1:
        print(s)
    else:
        sn=s[:s.index(ss)]
        print(sn)

#首先需要开内存，在函数中会自动回收内存。
file_name='global_share_memory'

if __name__ == "__main__":
    info = sys.argv[1]
    net = sumolib.net.readNet('erlangen.net.xml')
    externalID = sys.argv[2]
    road = sys.argv[3]
    roads = []
    with open('routesV/' + externalID + '.csv', 'r') as file:
        while True:
            line = file.readline()
            if len(line) == 0:
                break
            roads.append(line.split(' ')[0])
    print(random.choice(roads[roads.index(road):]))
    # ETA到达时间 晚于 执行完成时间
    # 截止时间ETA位置 与其他RSU距离 选取最近的
    # 每个RSU都有一个执行完成的ETA位置，这个位置最好选取相对最近的，数据获取并渲染后可获得服务时的最低延迟。
    send(info + ''.join(random.sample('zyxwvutsrqponmlkjihgfedcba',5)) + os.getcwd())
