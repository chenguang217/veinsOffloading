import sys
with open('simTime/' + sys.argv[1] + '.log', 'a') as file:
    file.write(sys.argv[2] + ',' + sys.argv[3] + '\n') 