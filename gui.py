from tkinter import *
from tkinter import ttk
import socket
from threading import Thread, Lock
from queue import Queue


regs = dict()
lock = Lock()
msg_queue = Queue()
client_running = True


HOST, PORT = "localhost", 50007
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect((HOST, PORT))


def toggle_valve():
  id = 2
  reg = "valve_open"
  lock.acquire()
  val = regs[f"hw{id}_{reg}"].get()
  val = int(not val)
  regs[f"hw{id}_{reg}"].set(val)
  lock.release()
  msg_queue.put(f"WriteReg {reg} {id} {val}")


def run_client():
  client_running_helper = True
  while client_running_helper:
    if msg_queue.empty():
      msg_queue.put(f"GetNext 0 0 0")
    msg = msg_queue.get()
    msg = msg.encode('utf-8')

    s.sendall(msg)
    #print(f"DEBUG: SocketClient Sent: {msg.decode('utf-8')}")

    data = s.recv(1024)
    data = data.decode('utf-8')
    #print(f"DEBUG: SocketClient Received: {data}")

    if data:
      cmd, reg, id, val = data.split()
      if cmd == "RegUpdated" or cmd == "WriteReg":
        lock.acquire()
        if client_running:
          regs[f"hw{id}_{reg}"].set(int(val))
        lock.release()

    lock.acquire()
    client_running_helper = client_running
    lock.release()


root = Tk()
root.title("GUI")

for id in (1, 2):
  for reg in ("pwr", "temp", "lvl", "valve_open"):
    regs[f"hw{id}_{reg}"] = IntVar(value=0)

mainframe = ttk.Frame(root, padding="10 10 10 10")
mainframe.grid(column=0, row=0, sticky=(N, W, E, S))
root.columnconfigure(0, weight=1)
root.rowconfigure(0, weight=1)

ttk.Label(mainframe, text="hw1_pwr").grid(column=0, row=0, sticky=W)
ttk.Label(mainframe, textvariable=regs["hw1_pwr"]).grid(column=1, row=0, sticky=W)
ttk.Label(mainframe, text="hw1_temp").grid(column=0, row=1, sticky=W)
ttk.Label(mainframe, textvariable=regs["hw1_temp"]).grid(column=1, row=1, sticky=W)

ttk.Label(mainframe, text="hw2_pwr").grid(column=0, row=2, sticky=W)
ttk.Label(mainframe, textvariable=regs["hw2_pwr"]).grid(column=1, row=2, sticky=W)
ttk.Label(mainframe, text="hw2_temp").grid(column=0, row=3, sticky=W)
ttk.Label(mainframe, textvariable=regs["hw2_temp"]).grid(column=1, row=3, sticky=W)

ttk.Label(mainframe, text="hw2_lvl").grid(column=0, row=4, sticky=W)
ttk.Label(mainframe, textvariable=regs["hw2_lvl"]).grid(column=1, row=4, sticky=W)
ttk.Label(mainframe, text="hw2_valve_open").grid(column=0, row=5, sticky=W)
ttk.Label(mainframe, textvariable=regs["hw2_valve_open"]).grid(column=1, row=5, sticky=W)

ttk.Button(mainframe, text="Toogle Valve", command=toggle_valve).grid(column=1, row=6)

for child in mainframe.winfo_children():
    child.grid_configure(padx=5, pady=5)


thread = Thread(target=run_client)
thread.start()
try:
  root.mainloop()
except Exception as e:
  print("ERROR: ", e)
lock.acquire()
client_running = False
lock.release()
thread.join()
s.close()
