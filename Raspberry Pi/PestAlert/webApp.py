from flask import Flask, request,render_template
import sqlite3

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
