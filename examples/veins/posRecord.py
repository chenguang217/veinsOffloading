import sys
with open('trace.log', 'a') as file:
    file.write(sys.argv[1] + '\n') 