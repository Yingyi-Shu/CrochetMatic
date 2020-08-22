"""
GCode Converter for CrochetMatic

Takes a Fusion360 GCode file, modifies it, and
return it as a new file understood by CrochetMatic

Argument 1: original GCode filename

Prerequisite: Input GCode file is in the same directory as this python script
"""

import sys

def removeZ(gcode):
    i=0
    while i<len(gcode):
        if gcode[i] == "Z":
            pos=i+1
            while gcode[pos] in ["1","2","3","4","5","6","7","8","9","0","."]:
            #while not gcode[pos].isalpha:
                pos+=1
            value=gcode[i+1:pos]
            if float(value)>60:
                gcode=gcode[:i]+gcode[pos:]
        i+=1
    return gcode

def changeF(gcode):
    i=0
    while i<len(gcode):
        if gcode[i] == 'F':
            pos=i+1
            gcode = gcode[:pos] + '6' + gcode[pos+2:]
            return gcode
        i+=1
    return gcode

def strip(gcode):
    while gcode.find("\n\n") !=-1:
        pos=gcode.find("\n\n")
        gcode=gcode[:pos]+gcode[pos+2:]
    return gcode

def addG1(gcode):
    i=0
    while gcode.find("\n",i+2) !=-1:
        pos=gcode.find("\n",i+2)
        #print(pos)
        if gcode[pos+1:pos+3]!="G1":
            gcode=gcode[:pos+1]+"G1 "+gcode[pos+1:]
        i=pos
    return gcode

#filename=input("What is the name of your .gcode file? (e.g. gcode.gcode)\n")
filename=sys.argv[1]
#This assumes the name of the gcode file to be changed is gcode.gcode
File=open(filename,"r")
gcode=File.read()
#Cuts everything before the first G0
gcode=gcode[gcode.find("G0"):]
#Cuts everything after second G0 and change first G0 to G1
gcode="G1"+gcode[2:gcode.find("G0",2)]
#Removes any Zvalue larger than 60
gcode=(removeZ(gcode))
#Strip extra lines
gcode=strip(gcode)
#print(repr(gcode))
#Add G1 before every coordinate line
gcode=addG1(gcode)
#Change F1000 to F600
gcode=changeF(gcode)

#export file
#exportname=input("What should the new file be called? (e.g export.gcode)\n")
exportname="%s-final.gcode" % (filename.split('.')[0])
output=open(exportname,"w")
output.write(gcode)
output.close()
print("New file exported as: "+exportname)
