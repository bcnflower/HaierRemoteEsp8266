import sys
import glob
import serial
import time
from decode_utils import *

arduino = None

def serial_ports():
    """ Lists serial port names

        :raises EnvironmentError:
            On unsupported or unknown platforms
        :returns:
            A list of the serial ports available on the system
    """
    if sys.platform.startswith('win'):
        ports = ['COM%s' % (i + 1) for i in range(256)]
    elif sys.platform.startswith('linux') or sys.platform.startswith('cygwin'):
        # this excludes your current terminal "/dev/tty"
        ports = glob.glob('/dev/tty[A-Za-z]*')
    elif sys.platform.startswith('darwin'):
        ports = glob.glob('/dev/tty.*')
    else:
        raise EnvironmentError('Unsupported platform')

    result = []
    for port in ports:
        try:
            s = serial.Serial(port)
            s.close()
            result.append(port)
        except (OSError, serial.SerialException):
            pass
    return result



def receive_ir_pronto():
    # time.sleep(5)
    arduino.write(bytes('receive\r', 'utf-8'))
    while True:
        time.sleep(0.1)
        out = arduino.readline().decode()
        # print('out=',out)
        if len(out)<1:
            continue
        if out[0] == '+':
            return out
        elif out[0] == '.':
            # print("Timeout")
            return None
    # data = arduino.readline()
    # print(len(data))
    # return data


try:
    arduino = serial.Serial(port=serial_ports()[0], baudrate=115200, timeout=.1)
except:
    print("Available Ports =",serial_ports())
    exit()

print("Connecting...")
data = arduino.readline().decode()
while "GirsLite" not in data:
    data = arduino.readline().decode()
print("Connected")
print(data)

while True:
    pronto = receive_ir_pronto()
    # print(len(pronto))
    if pronto == None or len(pronto)<1000:
        continue
    # print(pronto) 
    decoded = decode_pronto(pronto,550,1700)

    # ck = calcChecksum(decoded,12)[-8:]
    ck = calcChecksum(decoded,13)[-8:]
    ck2 = calcChecksum(decoded,5,14)[-8:]
    # print(ck,ck in decoded)
    # print(ck2,ck2 in decoded)

    if ck not in decoded:
        print("Sum1=",ck,"Not Matched")
    if ck2 not in decoded:
        print("Sum2=",ck2,"Not Matched")
    dbytes = seperateBits(decoded,[8 for x in range(19)])

    decoded = seperateBits(decoded,[8,4,4,3,5,3,3,1,2,1,6,3,5,1,1,6,3,5,1,1,6,11,1,14,1,5,8,8,1,1,30])
    # decoded = seperateBits(decoded,[8 for x in range(19)])

    sp = decoded.split(" ")
    sp[0] = str(hex(int(sp[0],2)))
    sp[1] = str(int(sp[1],2)+16)+"C"
    sp[2] = "SwingV="+str(int(sp[2],2)).zfill(2)
    sp[3] = "SwingH="+str(int(sp[3],2))

    sp[5] = "TMode="+str(int(sp[5],2))

    sp[7] = "HLTH="+sp[7] 

    sp[9] = "PWR="+sp[9] 

    sp[11] = "Fan="+str(int(sp[11],2))
    sp[12] = "OffH="+str(int(sp[12],2)).zfill(2)
    sp[13] = "QIT="+sp[13] 
    sp[14] = "TRB="+sp[14] 
    sp[15] = "OffM="+str(int(sp[15],2)).zfill(2)
    sp[16] = "Mode="+str(int(sp[16],2))
    sp[17] = "OnH="+str(int(sp[17],2)).zfill(2)
    sp[18] = "SLP="+sp[18] 

    sp[20] = "OnM="+str(int(sp[20],2)).zfill(2)

    sp[22] = "CL="+sp[22] 

    sp[24] = "LK="+sp[24]
    sp[25] = "Btn="+str(int(sp[25],2)).zfill(2)
    sp[26] = "Sum=0x"+str(hex(int(sp[26],2)))[2:].zfill(2)




    for d in sp:
        print(d+' ',end="")
    print("")
    # print('\''+decoded+'\',')
    print(dbytes)

# p = "+3000 -3050 +3050 -4450 +500 -1700 +550 -600 +500 -1700 +550 -550 +550 -550 +550 -1700 +550 -1700 +500 -600 +500 -1750 +500 -1700 +550 -550 +550 -1700 +500 -600 +500 -600 +500 -600 +500 -600 +500 -1750 +500 -1700 +550 -1700 +500 -600 +500 -600 +500 -600 +500 -600 +500 -600 +500 -600 +500 -600 +500 -600 +500 -550 +550 -550 +550 -550 +550 -550 +550 -550 +500 -600 +500 -600 +500 -600 +500 -600 +500 -600 +500 -600 +500 -600 +500 -600 +500 -550 +550 -550 +550 -1700 +500 -600 +500 -600 +500 -600 +500 -600 +500 -600 +500 -600 +500 -600 +500 -600 +500 -550 +550 -550 +550 -550 +550 -550 +550 -550 +500 -1700 +550 -600 +500 -600 +500 -600 +500 -600 +500 -600 +500 -550 +550 -550 +550 -550 +550 -550 +500 -600 +500 -600 +500 -600 +500 -600 +500 -600 +500 -550 +550 -600 +500 -600 +500 -550 +550 -550 +550 -550 +550 -550 +550 -550 +500 -600 +500 -600 +500 -600 +500 -600 +500 -600 +500 -600 +500 -600 +500 -550 +550 -550 +550 -550 +550 -550 +550 -550 +500 -600 +500 -600 +500 -600 +500 -600 +500 -600 +500 -600 +500 -600 +500 -600 +500 -550 +550 -550 +550 -1700 +500 -600 +500 -1750 +500 -1750 +500 -1700 +550 -1700 +550 -1700 +500 -1700 +550 -600 +500 -1700 +550 -1700 +550 -1700 +500 -600 +500 -1750 +500 -1700 +550 -550 +550 -1700 +500 -600 +500 -1750 +500 -600 +500 -600 +500 -600 +500 -600 +500 -550 +550 -550 +550 -550 +550 -550 +550 -550 +500 -600 +500 -600 +500 -600 +500 -600 +500 -600 +500 -600 +500 -600 +500 -550 +550 -550 +550 -550 +550 -550 +500 -600 +500 -600 +500 -600 +500 -600 +500 -600 +500 -600 +500 -600 +500 -600 +500 -550 +550 -550 +550 -550 +550 -550 +550 -1700 +500 -600 +500 -1750 +500 -1700 +550 -550 +550 -1700 +500 -600 +500 -1750 +500 -50100"
# p = "+3000 -3100 +3000 -4450 +550 -600 +500 -1700 +550 -550 +550 -1700 +500 -1700 +550 -600 +500 -600 +500 -1750 +500 -1700 +550 -1700 +500 -1750 +500 -600 +500 -1750 +500 -1750 +500 -550 +550 -550 +550 -1700 +500 -600 +500 -1700 +550 -550 +550 -600 +500 -600 +500 -600 +500 -600 +500 -600 +500 -550 +500 -600 +550 -550 +550 -550 +500 -600 +500 -1750 +500 -600 +500 -600 +500 -1750 +500 -550 +500 -600 +550 -550 +550 -550 +550 -550 +500 -600 +500 -600 +500 -1750 +500 -1700 +550 -600 +500 -550 +550 -550 +550 -550 +550 -550 +500 -600 +500 -600 +500 -600 +500 -600 +500 -600 +500 -600 +500 -600 +500 -600 +500 -600 +500 -550 +550 -1700 +500 -600 +500 -600 +500 -600 +500 -600 +500 -600 +500 -600 +500 -600 +450 -650 +500 -600 +500 -550 +550 -550 +550 -550 +550 -550 +500 -600 +500 -600 +500 -600 +500 -600 +500 -600 +500 -600 +500 -600 +500 -600 +500 -600 +500 -550 +550 -550 +550 -550 +550 -550 +500 -600 +500 -600 +500 -600 +500 -600 +500 -600 +500 -600 +500 -600 +500 -600 +500 -600 +500 -550 +550 -550 +550 -550 +500 -600 +500 -600 +500 -600 +500 -600 +500 -600 +500 -600 +500 -600 +500 -1750 +500 -550 +550 -1700 +550 -550 +500 -600 +500 -1750 +500 -1750 +500 -1750 +500 -1700 +550 -550 +550 -1700 +500 -1750 +500 -600 +500 -1700 +550 -600 +500 -1700 +550 -550 +500 -600 +500 -600 +500 -600 +500 -600 +500 -600 +500 -600 +500 -600 +500 -600 +500 -600 +500 -550 +500 -600 +550 -550 +550 -550 +500 -600 +500 -600 +500 -600 +500 -600 +500 -600 +500 -600 +500 -600 +500 -600 +500 -600 +500 -550 +550 -550 +550 -550 +550 -550 +500 -600 +500 -600 +500 -600 +500 -550 +550 -600 +500 -1750 +500 -600 +500 -1700 +550 -1700 +500 -600 +500 -1700 +550 -600 +500 -1750 +500 -50100"
# decode_pronto(p,550,1700)

# Prefix(8)-Temp(4)-SwingV(4)-SwingH(3)-?(5)-TimerMode(3)-?(3)-Health(1)-?(2)-Power(1)-?(6)-Fan(3)-OffHours(5)-Quiet(1)-Turbo(1)-OffMinutes(6)-Mode(3)-OnHour(5)-OnMinutes(6)-?(13)-SelfClean(1)-?(15)-Button(5)-Sum(8)-Prefix2(8)-?(1)-Clear(1)-?(30)-(8)?
