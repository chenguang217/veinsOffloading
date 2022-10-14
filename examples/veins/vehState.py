import json
import math
import mmap
import os
import random
import sys

import sumolib

if __name__ == "__main__":
    position = sys.argv[1]
    deadLinePos = sys.argv[2]
    servicePos = sys.argv[3]
    cpu = sys.argv[4]
    mem = sys.argv[5]
    operationTime = sys.argv[6]
    name = sys.argv[7]
    serviceMode = sys.argv[8]
    ratio = sys.argv[9]
    ratioSum = sys.argv[10]
    if serviceMode == '0':
        with open('taskLog/' + name + '_' + ratioSum + '.json', 'w', newline='\r\n') as file:
            result = {'position': position, 'deadLinePos': deadLinePos, 'servicePos': servicePos, 'cpu': cpu, 'mem': mem, 'operationTime': operationTime, 'serviceMode': 0, 'ratio': ratio}
            file.write(json.dumps(result, indent='\t'))
    else:
        with open('taskLog/' + name + '_' + ratioSum + '.json', 'w', newline='\r\n') as file:
            result = {'position': position, 'deadLinePos': deadLinePos, 'servicePos': servicePos, 'cpu': cpu, 'mem': mem, 'operationTime': operationTime, 'serviceMode': serviceMode, 'ratio': ratio}
            file.write(json.dumps(result, indent='\t'))
