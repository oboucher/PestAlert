#import libraries
from adafruit_ble import BLERadio
from adafruit_ble.advertising.standard import ProvideServicesAdvertisement
from adafruit_ble.services.nordic import UARTService
import time, os, sqlite3, sys
from pathlib import Path
class PestCamera:

    def __init__(self):
        self.connection = sqlite3.connect('pest_alert.db')
        self.cursor = self.connection.cursor()

        # Define radio for finding deivice
        self.ble = BLERadio()
        # set global variable uart_connection, will be replaced with the service once connected
        self.uart_connection = None

        # creating the database, this needs to happen before the while loop
        command1 = """CREATE TABLE IF NOT EXISTS
        info(picture_path TEXT, humidity FLOAT, temperature FLOAT)"""

        self.cursor.execute(command1)
        # on launch, connect to the device and ask for input
        noCon = True
        while noCon:
            # imageName = 'IMAGE00.JPG'
            # If there is no uart connection yet, connect to an available device with a UART service,
            # TODO: Make it connect specifically to our project, and not any UART Device, not important
            if not self.uart_connection:
                print("Trying to connect...")
                for adv in self.ble.start_scan(ProvideServicesAdvertisement):
                    if UARTService in adv.services:
                        self.uart_connection = self.ble.connect(adv)
                        print("Connected")
                        break
                self.ble.stop_scan()
            # if there is a Uart device connected, ask the user for an input, then call the function corresponding to the input
            # TODO: Make this refresh automaticaly and with a button press, needed for CDR
            if self.uart_connection and self.uart_connection.connected:
                self.uart_service = self.uart_connection[UARTService]
                noCon = False

    # add image, temp, and humidity to database once all are recived
    # commented out for testing, will need to be working for actual project
    def addToDB(self, textString):
        vals = textString.split(", ")
        print(vals)
        self.cursor.execute("INSERT INTO info VALUES (?, ?, ?)", (vals[0], vals[1], vals[2]))
        self.connection.commit()
        self.cursor.execute("SELECT * FROM info")
        results = self.cursor.fetchall()
        # render_template('template.html', value=results)
        print(results)

    # Testing function to get text from the device, can be removed once we know everything works
    def getText(self):
        self.uart_service.write("w".encode("utf-8"))
        textString = self.uart_service.readline().decode("utf-8")
        print(textString)
        self.addToDB(textString)

    def runAll(self):
        BUFFSIZE = 4
        # bytes received, used to tell if we are getting all of the data
        byteNum = 0
        # send the user input that was passed thru as s, this tells the arduino what we want to do
        # happens in each run function to ensure command isnt sent before we are ready to get the data
        self.uart_service.write("r".encode("utf-8"))
        # wait until the arduino sends an c back, to tell the rpi we are about to get data
        while (True):
            val = self.uart_service.read(nbytes=1)
            if (val == None):
                continue
            if (val.decode("utf-8") == 'c'):
                break
        # debug statement to know we are getting the name first
        print("readyForImgName")
        # read the image name, used to be readLine, but for some reason that broke
        # When possible use read(nbytes) instead of readline to fix errors if size of data is known

        # arduino will send a line with the image name, temperature, and humidity seperated by a comma
        # TODO: textString is sending an unknown number of bytes, should be 19, but for somereason its not
        imageName = self.uart_service.read(nbytes=12).decode("utf-8")
        fileName = Path("static/"+imageName)
        fileName.touch(exist_ok=True)
        # print the name to make sure its the right one
        print(imageName)
        # textString = uart_service.readline().decode("utf-8")
        # a file with the name of the image that we will save data to
        f = open(fileName, 'wb+')
        # get the first 4 bytes, then delay, print statemtns are for debugging
        val = self.uart_service.read(nbytes=BUFFSIZE)
        print("delay")
        time.sleep(.01)
        print("dd")
        # Get the data from the uart service, then write them to teh file, 4 bytes at a time for now, seems to be reliable
        while (val != None):
            # print(val)
            f.write(val)
            val = self.uart_service.read(nbytes=BUFFSIZE)
            byteNum = byteNum + BUFFSIZE
            # we need this delay to allow the arduino to send data, if we dont we might read the same data in
            # time.sleep(.005)
        # print the number of bytes recived,

        # close the file
        f.close()
        # send request the next data again

        # print it so we know it works
        # print(textString)
        # add that data to the DB, this is how the webapp knows the image path, as well as the temp and humidity
        # right now it adds it to an incorrect path, we need to figure out how to get it to go into the static folder
        # TODO: save image to the static folder instead of the project folder. Need for CDR


if __name__ == "__main__":

    cam = PestCamera()
    running = True
    print(cam.__dict__)
    while running:
        s = input("q, r, w: ")
        if (s == 'r'):
            cam.runAll()
            # LITERALLY HAVE NO CLUE WHY TF THIS HAS TO BE HERE, FOR SOME REASON GETTING TEXT ONLY WORKS AFTER
            # HAVING A DELAY, CANT BE RUN IN THE runAll METHOD
            time.sleep(.1)
            cam.getText()
        if (s == 'q'):
            sys.exit()
        if (s == 'w'):
            cam.getText()
        if (s == 'x'):
            cam.uart_service.write(s.encode("utf-8"))

