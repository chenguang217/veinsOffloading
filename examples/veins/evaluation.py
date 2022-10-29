import os
import sumolib

if __name__ == '__main__':
    net = sumolib.net.readNet('erlangen.net.xml')
    boundaries = net.getBoundary()
    result = {}
    taskLogList = os.listdir('taskLog')
    for taskLog in taskLogList:
        if '_' not in taskLog:
            with open('taskLog/' + taskLog, 'r') as file:
                status = file.read().strip()
            if status == 'success':
                result[taskLog.replace('.json', '')] = {'status': True, 'QoS': 0, 'Fairness': 0}
    for taskName, property in result.items():
        print(taskName)


    # with open('taskLog/')