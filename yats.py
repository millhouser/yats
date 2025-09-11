import machine
# from machine import Pin,SPI
import framebuf
import time
from dht import DHT22
import network
import sys
import usocket as socket
import ustruct as struct

# for the display
DC = 8
RST = 12
MOSI = 11
SCK = 10
CS = 9

# WLAN-Konfiguration
wlanSSID = 'home'
wlanPW = 'HilpeTritsche'
network.country('DE')

# Winterzeit / Sommerzeit
#GMT_OFFSET = 3600 * 1 # 3600 = 1 h (Winterzeit)
GMT_OFFSET = 3600 * 2 # 3600 = 1 h (Sommerzeit)

# Status-LED
led_onboard = machine.Pin('LED', machine.Pin.OUT, value=0)

# NTP-Host
NTP_HOST = 'pool.ntp.org'

# Funktion: WLAN-Verbindung
def wlanConnect():
    wlan = network.WLAN(network.STA_IF)
    if not wlan.isconnected():
        print('WLAN-Verbindung herstellen')
        wlan.active(True)
        wlan.connect(wlanSSID, wlanPW)
        for i in range(10):
            if wlan.status() < 0 or wlan.status() >= 3:
                break
            led_onboard.toggle()
            print('.')
            time.sleep(1)
    if wlan.isconnected():
        print('WLAN-Verbindung hergestellt / WLAN-Status:', wlan.status())
        led_onboard.on()
    else:
        print('Keine WLAN-Verbindung')
        led_onboard.off()
        print('WLAN-Status:', wlan.status())

# Funktion: Zeit per NTP holen
def getTimeNTP():
    NTP_DELTA = 2208988800
    NTP_QUERY = bytearray(48)
    NTP_QUERY[0] = 0x1B
    addr = socket.getaddrinfo(NTP_HOST, 123)[0][-1]
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        s.settimeout(1)
        res = s.sendto(NTP_QUERY, addr)
        msg = s.recv(48)
    finally:
        s.close()
    ntp_time = struct.unpack("!I", msg[40:44])[0]
    return time.gmtime(ntp_time - NTP_DELTA + GMT_OFFSET)

# Funktion: RTC-Zeit setzen
def setTimeRTC():
    # NTP-Zeit holen
    tm = getTimeNTP()
    machine.RTC().datetime((tm[0], tm[1], tm[2], tm[6] + 1, tm[3], tm[4], tm[5], 0))

class OLED_1inch3(framebuf.FrameBuffer):
    def __init__(self):
        self.width = 128
        self.height = 64
        
        self.rotate = 180 #only 0 and 180 
        
        self.cs = machine.Pin(CS,machine.Pin.OUT)
        self.rst = machine.Pin(RST,machine.Pin.OUT)
        
        self.cs(1)
        self.spi = machine.SPI(1)
        self.spi = machine.SPI(1,2000_000)
        self.spi = machine.SPI(1,20000_000,polarity=0, phase=0,sck=machine.Pin(SCK),mosi=machine.Pin(MOSI),miso=None)
        self.dc = machine.Pin(DC,machine.Pin.OUT)
        self.dc(1)
        self.buffer = bytearray(self.height * self.width // 8)
        super().__init__(self.buffer, self.width, self.height, framebuf.MONO_HMSB)
        self.init_display()
        
        self.white =   0xffff
        self.black =   0x0000
        
    def write_cmd(self, cmd):
        self.cs(1)
        self.dc(0)
        self.cs(0)
        self.spi.write(bytearray([cmd]))
        self.cs(1)

    def write_data(self, buf):
        self.cs(1)
        self.dc(1)
        self.cs(0)
        self.spi.write(bytearray([buf]))
        self.cs(1)

    def init_display(self):
        """Initialize dispaly"""  
        self.rst(1)
        time.sleep(0.001)
        self.rst(0)
        time.sleep(0.01)
        self.rst(1)
        
        self.write_cmd(0xAE)#turn off OLED display

        self.write_cmd(0x00)   #set lower column address
        self.write_cmd(0x10)   #set higher column address 

        self.write_cmd(0xB0)   #set page address 
      
        self.write_cmd(0xdc)    #et display start line 
        self.write_cmd(0x00) 
        self.write_cmd(0x81)    #contract control 
        self.write_cmd(0x6f)    #128
        self.write_cmd(0x21)    # Set Memory addressing mode (0x20/0x21) #
        if self.rotate == 0:
            self.write_cmd(0xa0)    #set segment remap
        elif self.rotate == 180:
            self.write_cmd(0xa1)
        self.write_cmd(0xc0)    #Com scan direction
        self.write_cmd(0xa4)   #Disable Entire Display On (0xA4/0xA5) 

        self.write_cmd(0xa6)    #normal / reverse
        self.write_cmd(0xa8)    #multiplex ratio 
        self.write_cmd(0x3f)    #duty = 1/64
  
        self.write_cmd(0xd3)    #set display offset 
        self.write_cmd(0x60)

        self.write_cmd(0xd5)    #set osc division 
        self.write_cmd(0x41)
    
        self.write_cmd(0xd9)    #set pre-charge period
        self.write_cmd(0x22)   

        self.write_cmd(0xdb)    #set vcomh 
        self.write_cmd(0x35)  
    
        self.write_cmd(0xad)    #set charge pump enable 
        self.write_cmd(0x8a)    #Set DC-DC enable (a=0:disable; a=1:enable)
        self.write_cmd(0XAF)
    
    def show(self):
        self.write_cmd(0xb0)
        for page in range(0,64):
            if self.rotate == 0:
                self.column =  63 - page    #set segment remap
            elif self.rotate == 180:
                self.column =  page
                          
            self.write_cmd(0x00 + (self.column & 0x0f))
            self.write_cmd(0x10 + (self.column >> 4))
            for num in range(0,16):
                self.write_data(self.buffer[page*16+num])
                
if __name__=='__main__':
    # init display
    disp = OLED_1inch3()
    
    disp.fill(disp.black)
    disp.text("yats",0,27,disp.white)
    disp.show()
    time.sleep(1)

    keyA = machine.Pin(15,machine.Pin.IN,machine.Pin.PULL_UP)
    keyB = machine.Pin(17,machine.Pin.IN,machine.Pin.PULL_UP)

    # init temperature/humidity sensor
    dht22 = DHT22(machine.Pin(16, machine.Pin.IN, machine.Pin.PULL_UP))

    # init RTC
    # rtc = machine.RTC()
    machine.RTC().datetime((1977, 3, 17, 3, 17, 45, 0, 0))
    print("Startzeit: ", machine.RTC().datetime())
    # wlanConnect()
    # setTimeRTC()
    # print(machine.RTC().datetime())

    # for the state machine
    state = 1
    newstate = 0
    keys_locked = False

    currenttime = time.time()
    lasttime5s = 0
    lasttime10min = 0
    
    while(1):
        #if state != newstate:
        state = newstate
        if state == 0:
            disp.fill(disp.black)
            #disp.text("0",0,27,disp.white)
            
            currenttime = time.time()
                
            # check WLAN connection and reset RTC every 10 min
            if currenttime - lasttime10min >= 600:
                lasttime10min = currenttime
                wlanConnect()
                setTimeRTC()
                print("Zeit gesetzt: ", machine.RTC().datetime())

            # measure tempereature and humidity every 5s
            if currenttime - lasttime5s >= 5:
                lasttime5s = currenttime
    
                try:                
                    dht22.measure()
                    temp = dht22.temperature()
                    humi = dht22.humidity()
                except:
                    print('Lesefehler DHT22!')

                text = str(temp) + "°C " + str(humi) + "%"
                disp.text(text,0,27,disp.white)
                disp.show()
                print('Time: ', machine.RTC().datetime(), ', Temp: ', temp, '°C, Humi: ', humi, '%')

        if state == 1:
            disp.fill(disp.black)
            disp.text("1",0,27,disp.white)
            disp.show()

        if state == 2:
            disp.fill(disp.black)
            disp.text("2",0,27,disp.white)
            disp.show()
        
        if state == 3:
            disp.fill(disp.black)
            disp.text("3",0,27,disp.white)
            disp.show()
        
        if state == 4:
            disp.fill(disp.black)
            disp.text("4",0,27,disp.white)
            disp.show()
        
        if(keyB.value() == 0):
            keys_locked = True
            newstate = state - 1
            if newstate < 0:
                newstate = 4
            time.sleep(0.2)
        else:
            keys_locked = False
            
        if(keyA.value() == 0):
            keys_locked = True
            newstate = state + 1
            if newstate > 4:
                newstate = 0
            time.sleep(0.2)
        else:
            keys_locked = False
