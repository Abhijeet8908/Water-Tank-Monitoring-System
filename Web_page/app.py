# app.py
from flask import Flask, request, jsonify, render_template, redirect, url_for, session
import threading
import time

app = Flask(__name__)
# IMPORTANT: Set a secret key for session management.
# In a production environment, this should be a strong, randomly generated key.
app.secret_key = 'your_super_secret_key_here_change_this_in_production'

# This variable will store the latest water level received.
current_water_level = "N/A"
# This variable will store the timestamp of the last successful water level update.
last_received_timestamp = 0 # Unix timestamp in seconds
water_level_lock = threading.Lock()

# Admin credentials
ADMIN_USERNAME = "Admin"
ADMIN_PASSWORD = "admin"

# Global flag to indicate if a reset is pending for NodeMCU
reset_pending = False
reset_lock = threading.Lock() # Lock for reset_pending flag

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

@app.route('/admin', methods=['GET', 'POST'])
def admin_panel():
    """
    Handles admin login and renders the admin panel.
    """
    if request.method == 'POST':
        username = request.form.get('username')
        password = request.form.get('password')

        if username == ADMIN_USERNAME and password == ADMIN_PASSWORD:
            session['logged_in'] = True
            return redirect(url_for('admin_panel')) # Redirect to GET /admin after successful login
        else:
            return render_template('admin.html', error="Invalid credentials")

    # For GET request
    if not session.get('logged_in'):
        return render_template('admin.html', show_login=True)
    return render_template('admin.html', show_login=False)

@app.route('/logout')
def logout():
    """
    Logs out the admin user.
    """
    session.pop('logged_in', None)
    return redirect(url_for('admin_panel'))

@app.route('/trigger_reset', methods=['POST'])
def trigger_reset():
    """
    Endpoint to set the reset flag, indicating NodeMCU should reset.
    Requires admin login.
    """
    if not session.get('logged_in'):
        return jsonify({"status": "error", "message": "Unauthorized"}), 401

    global reset_pending
    with reset_lock:
        reset_pending = True
    print("Reset signal triggered by admin.")
    return jsonify({"status": "success", "message": "Reset signal sent to NodeMCU."})

@app.route('/get_nodemcu_command', methods=['GET'])
def get_nodemcu_command():
    """
    Endpoint for NodeMCU to poll for commands (like reset).
    If a reset is pending, it sends the command and clears the flag.
    """
    global reset_pending
    command = {"reset": False}
    with reset_lock:
        if reset_pending:
            command["reset"] = True
            reset_pending = False # Clear the flag after sending the command
            print("NodeMCU reset command delivered and flag cleared.")
    return jsonify(command)


if __name__ == '__main__':
    print("Flask server starting...")
    print("Visit http://127.0.0.1:5000/ in your browser for the water level monitor.")
    print("Visit http://127.0.0.1:5000/admin for the admin panel.")
    print("NodeMCU should send GET requests to http://YOUR_FLASK_SERVER_IP:5000/update_waterlevel?level=YOUR_WATER_LEVEL")
    app.run(host='0.0.0.0', port=5000, debug=True)
