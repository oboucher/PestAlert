# imports
# TBH, this doesnt really need comments, it just gets data from the database and displays it in the template
from flask import Flask, request,render_template
import sqlite3
# TODO add the ability to request images on the web app need a button that runs the BLE_interface.runAll() method
app = Flask(__name__)

@app.route('/')
def home():
	conn = sqlite3.connect('pest_alert.db')
	cursor =conn.cursor()
	cursor.execute("SELECT * FROM info")
	data = cursor.fetchall()
	return render_template('template.html', data=data)

if __name__ == 'main':
	app.run(port=80, host = '192.168.1.250')
