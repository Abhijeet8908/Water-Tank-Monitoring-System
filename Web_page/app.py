# app.py
from flask import Flask, request, jsonify, render_template
import threading
import time

app = Flask(__name__)

# This variable will store the latest water level received.
current_water_level = "N/A"
# This variable will store the timestamp of the last successful water level update.
last_received_timestamp = 0 # Unix timestamp in seconds
water_level_lock = threading.Lock()

@app.route('/')
def index():
    """
    Renders the main HTML page that displays the water level.
    """
    return render_template('index.html')

@app.route('/update_waterlevel', methods=['GET'])
def update_waterlevel():
    """
    API endpoint for NodeMCU to send water level data.
    It expects a 'level' query parameter (e.g., /update_waterlevel?level=123).
    """
    global current_water_level, last_received_timestamp
    level = request.args.get('level')

    if level is not None:
        with water_level_lock:
            current_water_level = level
            last_received_timestamp = int(time.time()) # Update timestamp on successful reception
        print(f"Received water level: {level} at {time.ctime(last_received_timestamp)}")
        return jsonify({"status": "success", "message": f"Water level updated to {level}"})
    else:
        print("Error: No 'level' parameter received.")
        return jsonify({"status": "error", "message": "Missing 'level' parameter"}), 400

@app.route('/get_waterlevel', methods=['GET'])
def get_waterlevel():
    """
    API endpoint for the webpage to fetch the current water level and connection status.
    """
    with water_level_lock:
        return jsonify({
            "water_level": current_water_level,
            "last_received_timestamp": last_received_timestamp
        })

if __name__ == '__main__':
    print("Flask server starting...")
    print("Visit http://127.0.0.1:5000/ in your browser.")
    print("NodeMCU should send GET requests to http://YOUR_FLASK_SERVER_IP:5000/update_waterlevel?level=YOUR_WATER_LEVEL")
    app.run(host='192.168.29.119', port=5000, debug=True)
