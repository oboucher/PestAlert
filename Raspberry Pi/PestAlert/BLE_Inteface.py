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
nameBytes = 11
tsBytes = nameBytes + 3


#add image, temp, and humidity to database once all are recived
def addToDB(textString):
    vals = textString.split(", ")
    print(vals)
    #cursor.execute("INSERT INTO info VALUES (?, ?, ?)", (vals[0], vals[1], vals[2]))
    #connection.commit()
    #cursor.execute("SELECT * FROM info")
    #results = cursor.fetchall()
    # render_template('template.html', value=results)
    print(results)

# I honestly dont know what the difference is between run main and run all, they seem identical but there was a reason
# for there to be 2, leaving it for now incase we figure out why
# comments should be the same and im lazy, so refer to runAll for how it works
def runMain(s):
    byteNum = 0
    uart_service.write(s.encode("utf-8"))

    imageName = uart_service.read(nbytes=nameBytes).decode("utf-8")
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
    textString = uart_service.readline()
    print(textString)
    addToDB(textString)

# Testing function to get text from the device, can be removed once we know everything works
def getText(s):
    uart_service.write(s.encode("utf-8"))
    text = uart_service.readline().decode("utf-8")
    print(text)
    return

def runAll(s):
    # bytes received, used to tell if we are getting all of the data
    byteNum = 0
    # send the user input that was passed thru as s, this tells the arduino what we want to do
    # happens in each run function to ensure command isnt sent before we are ready to get the data
    uart_service.write(s.encode("utf-8"))
    # wait until the arduino sends an c back, to tell the rpi we are about to get data
    while (True):
        val = uart_service.read(nbytes=1)
        if (val == None):
            continue
        if (val.decode("utf-8") == 'c'):
            break
    # debug statement to know we are getting the name first
    print("readyForImgName")
    # read the image name, used to be readLine, but for some reason that broke
    #When possible use read(nbytes) instead of readline to fix errors if size of data is known
    imageName = uart_service.read(nbytes=nameBytes).decode("utf-8")
    # print the name to make sure its the right one
    print(imageName)

    # send an n to the arduino to tell it we are ready to go to the next step, and get the image data
    uart_service.write('n'.encode('utf-8'))
    # a file with the name of the image that we will save data to
    f = open(imageName, 'wb+')
    # get the first 4 bytes, then delay, print statemtns are for debugging
    val = uart_service.read(nbytes=4)
    print("delay")
    time.sleep(.01)
    print("dd")
    # Get the data from the uart service, then write them to teh file, 4 bytes at a time for now, seems to be reliable
    while (val != None):
        # print(val)
        f.write(val)
        val = uart_service.read(nbytes=4)
        byteNum = byteNum + 4
        # we need this delay to allow the arduino to send data, if we dont we might read the same data in
        time.sleep(.005)
    # print the number of bytes recived,
    print(byteNum)
    # close the file
    f.close()
    # send request the next data again
    uart_service.write('n'.encode("utf-8"))
    # arduino will send a line with the image name, temperature, and humidity seperated by a comma
    #TODO: textString is sending an unknown number of bytes, should be 19, but for somereason its not
    textString = uart_service.read(nbytes = tsBytes).decode("utf-8")
    #print it so we know it works
    print(textString)
    # add that data to the DB, this is how the webapp knows the image path, as well as the temp and humidity
    # right now it adds it to an incorrect path, we need to figure out how to get it to go into the static folder
    #TODO: save image to the static folder instead of the project folder. Need for CDR
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



