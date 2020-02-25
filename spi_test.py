import serial.tools.list_ports
import tkinter
import tkinter.messagebox as tm
import gui.gui_main as gg

def message_askokcancel(title, info):
    top = tkinter.Tk()
    top.withdraw()
    top.update()
    #mes = tkinter.messagebox.askokcancel(title, info)
    tm.showinfo(title = title,message = info)
    top.destroy()
    #return mes

try:
    port_list = list(serial.tools.list_ports.comports())
    if len(port_list) == 0:
        raise
except Exception as e:
    message_askokcancel('提示','没有发现串口，请插入串口设备！')
    
gg.gui_run()