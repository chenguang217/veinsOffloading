import sys
import mmap
import sumolib
from PythonParam import PythonParam

def send(s, file_name):
    s=s+100*' '
    infosize=len(s)+1
    byte=s.encode(encoding='UTF-8')
    #从头读取内存，不然的话会接着上次的内存位置继续写下去，这里是从头覆盖。
    shmem=mmap.mmap(0,1000,file_name,mmap.ACCESS_WRITE)
    shmem.write(byte)

def getRoadLength(roadId, net):
    road = net.getEdge(roadId)
    length = road.getLength()
    return str(length)

def sendbackDes(param: PythonParam):
    roadId = param.getString("roadId")
    vehId = param.getString("vehId")
    taskName = param.getString("taskName")
    deadLinePos = param.getString("deadLinePos").replace('(', '').replace(')', '')
    deadLinePos = [float(deadLinePos.split(',')[0]), float(deadLinePos.split(',')[1])]
    serviceRoad = param.getString("serviceRoad")
    res = PythonParam()

    net = sumolib.net.readNet('erlangen.net.xml')
    boundaries = net.getBoundary()
    # get routes
    routesId = []
    with open('eta/' + vehId + '.csv', 'r') as file:
        while True:
            line = file.readline().strip()
            if len(line) == 0:
                break
            tmpNode = net.getEdge(line.split(',')[0]).getFromNode().getCoord()
            tmpNode = [tmpNode[0] - boundaries[0], boundaries[3] - tmpNode[1]]
            if tmpNode[0] - deadLinePos[0] < 0.05 and tmpNode[1] - deadLinePos[1] < 0.05:
                deadLineIndex = len(routesId) - 1
            routesId.append(line.split(',')[0])
    # print(routesId.index(roadId), routesId[deadLineIndex])
    if routesId.index(roadId) < routesId.index(serviceRoad):
        # print('not in service road')
        res.set('sendback', '0')
    elif routesId.index(roadId) == routesId.index(serviceRoad):
        # print('send back now')
        res.set('sendback', '1')
    elif deadLineIndex > routesId.index(roadId) > routesId.index(serviceRoad):
        # print('violation service road, but before deadline')
        res.set('sendback', '2+' + getRoadLength(serviceRoad, net))
    elif routesId.index(roadId) > deadLineIndex:
        # print('violation deadline')
        res.set('sendback', '3')
    return res

# if __name__ == "__main__":
    # roadId = sys.argv[1]
    # vehId = sys.argv[2]
    # taskName = sys.argv[3]
    # deadLinePos = sys.argv[4].replace('(', '').replace(')', '')
    # deadLinePos = [float(deadLinePos.split(',')[0]), float(deadLinePos.split(',')[1])]
    # serviceRoad = sys.argv[5]
    # net = sumolib.net.readNet('erlangen.net.xml')
    # boundaries = net.getBoundary()

    # # get routes
    # routesId = []
    # with open('eta/' + vehId + '.csv', 'r') as file:
        # while True:
            # line = file.readline().strip()
            # if len(line) == 0:
                # break
            # tmpNode = net.getEdge(line.split(',')[0]).getFromNode().getCoord()
            # tmpNode = [tmpNode[0] - boundaries[0], boundaries[3] - tmpNode[1]]
            # if tmpNode[0] - deadLinePos[0] < 0.05 and tmpNode[1] - deadLinePos[1] < 0.05:
                # deadLineIndex = len(routesId) - 1
            # routesId.append(line.split(',')[0])
    # # print(routesId.index(roadId), routesId[deadLineIndex])
    # if routesId.index(roadId) < routesId.index(serviceRoad):
        # # print('not in service road')
        # res.set('sendback', '0')
    # elif routesId.index(roadId) == routesId.index(serviceRoad):
        # # print('send back now')
        # res.set('sendback', '1')
    # elif deadLineIndex > routesId.index(roadId) > routesId.index(serviceRoad):
        # # print('violation service road, but before deadline')
        # res.set('sendback', '2' + getRoadLength(serviceRoad, net))
    # elif routesId.index(roadId) > deadLineIndex:
        # # print('violation deadline')
        # res.set('sendback', '3')
