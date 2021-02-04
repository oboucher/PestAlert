#import libraries
from adafruit_ble import BLERadio
from adafruit_ble.advertising.standard import ProvideServicesAdvertisement
from adafruit_ble.services.nordic import UARTService
import re, time, os, sqlite3, sys

#define the DB connection, and place the cursor on it
connection = sqlite3.connect('pest_alert.db')
cursor = connection.cursor()

#Define radio for finding deivice
ble = BLERadio()
#set global variable uart_connection, will be replaced with the service once connected
uart_connection = None


imageNum = 0
byteNum = 0

#add image, temp, and humidity to database once all are recived
def addToDB(textString):
    vals = textString.split(", ")
    cursor.execute("INSERT INTO info VALUES (?, ?, ?)", (vals[0], vals[1], vals[2]))
    connection.commit()
    cursor.execute("SELECT * FROM info")
    results = cursor.fetchall()
    # render_template('template.html', value=results)
    print(results)


def runMain(s):
    byteNum = 0
    uart_service.write(s.encode("utf-8"))

    imageName = uart_service.readline().decode("utf-8")
    print(imageName)

    uart_service.write('n'.encode('utf-8'))
    f = open(imageName, 'wb+')
    val = uart_service.read(nbytes=4)
    print("delay")
    time.sleep(.01)
    print("dd")
    lval = val
    while (val != None):
        # print(val)
        f.write(val)
        val = uart_service.read(nbytes=4)
        byteNum = byteNum + 4
        time.sleep(.005)
    # uart_service.write('n'.encode("utf-8"))
    print(byteNum)
    f.close()
    # clearFile(imageName)

    uart_service.write('n'.encode("utf-8"))
    textString = uart_service.readline().decode("utf-8")
    print(textString)
    addToDB(textString)

#Testing function to get text from the device, can be removed once we know everything works
def getText(s):
    uart_service.write(s.encode("utf-8"))
    text = uart_service.readline().decode("utf-8")
    print(text)
    return

def runAll(s):
    byteNum = 0
    uart_service.write(s.encode("utf-8"))
    while (True):
        val = uart_service.read(nbytes=1)
        if (val == None):
            continue
        if (val.decode("utf-8") == 'c'):
            break

    print("readyForImgName")
    imageName = uart_service.readline().decode("utf-8")
    print(imageName)

    uart_service.write('n'.encode('utf-8'))
    f = open(imageName, 'wb+')
    val = uart_service.read(nbytes=4)
    print("delay")
    time.sleep(.01)
    print("dd")
    lval = val
    while (val != None):
        # print(val)
        f.write(val)
        val = uart_service.read(nbytes=4)
        byteNum = byteNum + 4
        time.sleep(.005)
    # uart_service.write('n'.encode("utf-8"))
    print(byteNum)
    f.close()
    # clearFile(imageName)

    uart_service.write('n'.encode("utf-8"))
    textString = uart_service.readline().decode("utf-8")
    print(textString)
    addToDB(textString)


#creating the database, this needs to happen before the while loop
command1 = """CREATE TABLE IF NOT EXISTS
info(picture_path TEXT, humidity FLOAT, temperature FLOAT)"""

cursor.execute(command1)

# on launch, connect to the device and ask for input
while True:
    # imageName = 'IMAGE00.JPG'
    # If there is no uart connection yet, connect to an available device with a UART service,
    # TODO: Make it connect specifically to our project, and not any UART Device, not important
    if not uart_connection:
        print("Trying to connect...")
        for adv in ble.start_scan(ProvideServicesAdvertisement):
            if UARTService in adv.services:
                uart_connection = ble.connect(adv)
                print("Connected")
                break
        ble.stop_scan()
    #if there is a Uart device connected, ask the user for an input, then call the function corresponding to the input
    #TODO: Make this refresh automaticaly and with a button press, needed for CDR
    if uart_connection and uart_connection.connected:
        uart_service = uart_connection[UARTService]
        while uart_connection.connected:
            # x takes picture, t recives text, p requests ALL images as jpgs
            s = input("p, r, q: ")
            if (s == 'r'):
                runAll(s)
            if (s == 'p'):
                runMain(s)
            if (s == 'q'):
                sys.exit()
            if (s == 'w'):
                getText(s)
            if (s=='x'):
                uart_service.write(s.encode("utf-8"))



