# Bibliotheken laden
import machine
import network
import sys
import time
import usocket as socket
import ustruct as struct

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

# WLAN-Verbindung herstellen
wlanConnect()

# Zeit setzen
setTimeRTC()

# Aktuelles Datum ausgeben
print()
print(machine.RTC().datetime())