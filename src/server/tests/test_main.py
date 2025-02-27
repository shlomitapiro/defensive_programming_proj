import os
import subprocess
import time

def test_server_start():
    """Tests if the server starts without crashing."""
    server_path = os.path.abspath(os.path.join(os.path.dirname(__file__), "../main.py"))
    server_process = subprocess.Popen(["python", server_path])
    time.sleep(2)
    assert server_process.poll() is None
    server_process.terminate()

def test_load_port():
    """Tests if the server loads the port correctly from myport.info."""
    config_dir = os.path.abspath(os.path.join(os.path.dirname(__file__), "../config"))
    port_file_path = os.path.join(config_dir, "myport.info")

    os.makedirs(config_dir, exist_ok=True)

    with open(port_file_path, "w") as f:
        f.write("5000")

    from main import load_port

    port = load_port()

    print(f"Loaded port: {port}, Expected: 5000")
    print(f"Created myport.info at: {port_file_path}")

    assert port == 5000

    os.remove(port_file_path)