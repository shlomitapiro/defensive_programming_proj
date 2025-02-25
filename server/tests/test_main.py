import os
import subprocess
import time

def test_server_start():
    """Tests if the server starts without crashing."""
    server_process = subprocess.Popen(["python", "server/main.py"])
    time.sleep(2)
    assert server_process.poll() is None
    server_process.terminate()

def test_load_port():
    """Tests if the server loads the port correctly from myport.info."""
    config_dir = "server/config"
    port_file_path = os.path.join(config_dir, "myport.info")

    os.makedirs(config_dir, exist_ok=True)

    with open(port_file_path, "w") as f:
        f.write("5000")

    # ✅ במקום לייבא `main` ולהפעיל את השרת, נקרא רק לפונקציה שטוענת את הפורט
    from main import load_port  # ודא שהפונקציה קיימת ב-main.py

    port = load_port()
    assert port == 5000  # ✅ השרת אמור לטעון את הפורט 5000

    os.remove(port_file_path)

    port = load_port()
    assert port == 1357  # ✅ ללא הקובץ, השרת אמור להשתמש ב-1357