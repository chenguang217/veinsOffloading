import json
import math

import sumolib

size = (2600, 3000)

if __name__ == "__main__":
    net = sumolib.net.readNet('erlangen.net.xml')
    boundary = net.getBoundary()
    x = boundary[2] - boundary[0]
    y = boundary[3] - boundary[1]
    relay0 = []
    result = {}
    for node in net.getNodes():
        position = [node.getCoord()[0] - boundary[0], boundary[3] - node.getCoord()[1]]
        for i in range(round(size[0] / 1000)):
            for j in range(round(size[1] / 1000)):
                distance = math.sqrt((position[0] - i * 1000 - 500) ** 2 + (position[1] - j * 1000 - 500) ** 2)
                if distance <= 1000:
                    try:
                        result[str(position)].append([i * 1000 + 500, j * 1000 + 500])
                    except:
                        result[str(position)] = [[i * 1000 + 500, j * 1000 + 500]]
    print(result)
    with open('node2RSUtest.json', 'w', newline='\r\n') as file:
        file.write(json.dumps(result, indent='\t'))
    # print(str(deadPosition[0] - boundaries[0]), str(boundaries[3] - deadPosition[1]))
    # for i in range(round(size[0] / 1000)):
    #     for j in range(round(size[1] / 1000)):
    #         distance = math.sqrt((position[0] - i * 1000 + 500) ** 2 + (position[1] - j * 1000 + 500) ** 2)
    #         if distance <= 1000:
    #             relay0.append([i * 1000 + 500, j * 1000 + 500])

