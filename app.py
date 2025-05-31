# app.py
from flask import Flask, request, jsonify, render_template
import threading
import time

app = Flask(__name__)

# This variable will store the latest water level received.
# We use a Lock to ensure thread-safe updates, though for this simple case,
# it might not be strictly necessary, it's good practice for shared resources.
current_water_level = "N/A"
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
    global current_water_level
    level = request.args.get('level')

    if level is not None:
        with water_level_lock:
            current_water_level = level
        print(f"Received water level: {level}")
        return jsonify({"status": "success", "message": f"Water level updated to {level}"})
    else:
        print("Error: No 'level' parameter received.")
        return jsonify({"status": "error", "message": "Missing 'level' parameter"}), 400

@app.route('/get_waterlevel', methods=['GET'])
def get_waterlevel():
    """
    API endpoint for the webpage to fetch the current water level.
    """
    with water_level_lock:
        return jsonify({"water_level": current_water_level})

if __name__ == '__main__':
    # You might need to change '0.0.0.0' to your specific local IP address
    # if you're having trouble connecting from NodeMCU.
    # For example, if your computer's IP is 192.168.1.100, use host='192.168.1.100'.
    # You can find your IP address by running 'ipconfig' (Windows) or 'ifconfig' (Linux/macOS)
    # in your terminal.
    print("Flask server starting...")
    print("Visit http://127.0.0.1:5000/ in your browser.")
    print("NodeMCU should send GET requests to http://192.168.29.119:5000/update_waterlevel?level=YOUR_WATER_LEVEL")
    app.run(host='192.168.29.119', port=5000, debug=True)

