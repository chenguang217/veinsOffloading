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
    with open('taskLog/' + name + '.json', 'w', newline='\r\n') as file:
        result = {'position': position, 'deadLinePos': deadLinePos, 'servicePos': servicePos, 'cpu': cpu, 'mem': mem, 'operationTime': operationTime}
        file.write(json.dumps(result, indent='\t'))
