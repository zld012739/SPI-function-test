import serial.tools.list_ports
import tkinter
import tkinter.messagebox as tm
import gui.gui_main as gg
import gui.serialset_gui as gs

try:
    port_list = list(serial.tools.list_ports.comports())
    if len(port_list) == 0:
        raise
except Exception as e:
    gs.message_askokcancel('提示','没有发现串口，请插入串口设备！')
    
gg.gui_run()