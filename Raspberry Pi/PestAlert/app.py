# imports
from flask import Flask, request,render_template
from PestCamera import PestCamera
# TODO add the ability to request images on the web app need a button that runs the BLE_interface.runAll() method
app = Flask(__name__)


# steps for running program:
#		Initialize ble and connection
#		run the runall command
#		delay for brief period (THIS IS IMPORTANT FOR SOME REASON SOMETIMES)
#		run the getText command



@app.route('/')
def home():
	pestAlert = PestCamera()
	pestAlert.cursor.execute("SELECT * FROM info")
	data = pestAlert.cursor.fetchall()
	return render_template('template.html', data=data)


if __name__ == 'main':
	#initialize Camera
	#run 'server'
	app.run(port=80, host = '192.168.1.250')