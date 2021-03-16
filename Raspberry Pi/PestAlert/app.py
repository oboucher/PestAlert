# imports
from flask import Flask, request,render_template
from PestCamera import PestCamera
# TODO: Use AJAX to request an update on the device with the [PestCamera].runAll() command without refreshing the page, once the database is updated refresh the page
app = Flask(__name__)


# steps for running program:
#		Initialize ble and connection
#		run the pestAlert.runall() command
#		delay for brief period (THIS IS IMPORTANT FOR SOME REASON SOMETIMES)
#		run the pestAlert.getText() command

@app.before_first_request
def before_first_request():
	global pestAlert
	pestAlert = PestCamera()
	app.logger.info("before_first_request")

@app.route('/')
def home():
	pestAlert.cursor.execute("SELECT * FROM info")
	data = pestAlert.cursor.fetchall()
	return render_template('template.html', data=data)


if __name__ == 'main':
	#initialize Camera
	#run 'server'
	app.run(port=80, host = '192.168.1.250')