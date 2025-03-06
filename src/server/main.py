# main.py
import logging
from server_utils import run_server

if __name__ == "__main__":
    try:
        run_server()
    except Exception as e:
  
        logging.exception("Server responded with an error")
        print("A fatal error occurred. Server is shutting down.")