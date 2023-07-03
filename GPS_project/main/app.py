from flask import Flask, render_template, send_file, make_response
from flask import request
import json
import socket
import os
app = Flask(__name__)
app.template_folder = 'templates'
app.static_folder = 'static'


Lat, Lon, time = 0,0,"00:00:00"
Status, Speed, Course,bounding_area_available = 0, 0, 0, False
stored_data = False
#create a list to store the data
stored_coordinates = []
curr_v = 0


@app.route('/')
def index():
    
    return render_template('index.html')

@app.route('/fetch_update', methods=['GET'])
def fetch_update():
    vrsn = int(request.headers.get('Version'))
    print("this is the esp version: ", vrsn)
    print("this is the cur version: ", curr_v)
    file_path = '../build/GPS_project.bin'
    response = send_file(file_path, as_attachment=True)
    content_length = os.path.getsize(file_path)
    response.headers["Content-Length"] = str(content_length)
    print("this is content length: ", content_length)
    if (vrsn >= curr_v):
        response = make_response()
        response.status_code = 204

    return response


@app.route('/api', methods=['POST'])
def receive_data():
    data = request.get_json()
    global bounding_area_available, Lat, Lon, time, Status, Speed, Course

    # process the GPS data as JSON  
    print(type(data))
    # iterate as dict
    # get the value of the key 'lat'
    Lat = data['Latitude']
    Lon = data['Longitude']
    Time_H = data['Time_H']
    Time_M = data['Time_M']
    Time_S = data['Time_S']
    Status = data['Status']
    Speed = data['Speed']
    Course = data['Course']


    # create time string using string formatting
    time = '{:02d}:{:02d}:{:02d}'.format(Time_H, Time_M, Time_S)

    GPS_info = [Lat, Lon, time, Status, Speed, Course]
    if (Status == 65):
        print("can create bounding Area")
        bounding_area_available = True
    
    print(GPS_info)

    return 'Data received'

@app.route('/stored_data', methods=['POST'])
def get_stored_data():
    data = request.get_json()
    global stored_coordinates, stored_data

    lat = data['Latitude']
    lon = data['Longitude']
    Time_H = data['Time_H']
    Time_M = data['Time_M']
    Time_S = data['Time_S']
    Status = data['Status']
    Speed = data['Speed']
    Course = data['Course']
    Last = data['Last']

    # create time string using string formatting
    time = '{:02d}:{:02d}:{:02d}'.format(Time_H, Time_M, Time_S)
    stored_coordinates.append([lat, lon, time, Status, Speed, Course])


    if (Last == 1):
        stored_data = True


    return 'Data received'


@app.route('/get_data', methods=['GET'])
def send_data():
    global bounding_area_available,stored_data
    # Send data to the client
    # always increase the longitude before sending
    
    global Lon, Lat, time, Status, Speed, Course    

    data = {'Latitude': Lat, 'Longitude': Lon, 'Time': time, 'BoundingArea': bounding_area_available, 'Status': Status, 'Speed': Speed, 'Course': Course,'Stored_Data': stored_data}
    print("this is stored data flag:", stored_data)

    return json.dumps(data)

@app.route('/get_stored_data', methods=['GET'])
def send_stored_data():
    global stored_coordinates,stored_data
    stored_data = False
    
    data2 = {'Coordinates': stored_coordinates}


    return json.dumps(data2)



if __name__ == '__main__':
    host = socket.gethostbyname(socket.gethostname())
    app.run(host='0.0.0.0', port=5000, debug=True)
