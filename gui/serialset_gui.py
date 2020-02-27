from tkinter import * 
from tkinter import ttk
import serial.tools.list_ports
import serial
import threading
import tkinter.filedialog
import re
import tkinter.messagebox as tm

filenames = ''
ser = 0
comport_new = ''
bpsport_new = '115200'
top = 0
buadrate_list = ['9600','19200','38400','115200','230400']
def message_askokcancel(title, info):
    file_warn = Tk()
    file_warn.withdraw()
    file_warn.update()
    #mes = tkinter.messagebox.askokcancel(title, info)
    tm.showinfo(title = title,message = info)
    file_warn.destroy()
    #return mes

def com_bpsset(com,bps):
    global comport_new, bpsport_new
    comport_new = com.get()
    bpsport_new = bps.get()
    top.destroy()

def  getcom():
    port_list = list(serial.tools.list_ports.comports())
    serial_com = []
    for index in range(len(port_list)):
        serial_com.append(port_list[index][0])
    return serial_com


def thread_recv(text_handel):
    global ser
    while True:
        try:
            read = ser.readall()
            if len(read) > 0:
                text_handel.insert(END, bytes(read).decode('gb2312')+'\n')
        except Exception as e:
            pass

def thread_send():
    global ser

    global filenames
    f = open(filenames)
    f_lines = f.readlines()
    f.close()
    for index in f_lines:
        ser.write(index.encode('gb2312'))
   
def uart_open(text_handel):
    global ser,bpsport_new,comport_new,filenames

    if ser != 0:
        ser.close()
    ser = serial.Serial(port = comport_new, baudrate = int(bpsport_new), timeout = 0.2)
    if ser.is_open:
        pass
    else:
        ser.open()
    # recv_data = threading.Thread(target = thread_recv,args = (text_handel,),name = 'T1')
    # recv_data.setDaemon(True)
    # recv_data.start()
    if re.match('.*txt',filenames):
        send_data = threading.Thread(target = thread_send,name = 'T2')
        send_data.setDaemon(True)
        send_data.start()
    else:
        file_selectwarn = threading.Thread(target = tm.showwarning,args = ('提示','文件选择错误！'),name = 'T3')
        file_selectwarn.start()

def serial_paraset():
    global top,bpsport_new,comport_new
    com_list = getcom()
    for index in range(len(buadrate_list)):
        if bpsport_new == buadrate_list[index]:
            buadrate_index = index
    
    if com_list == []:
        com_list.append('NO PORT')
        com_index = 0
    else:
        if comport_new in com_list:
            for index_1 in range(len(com_list)):
                if comport_new == com_list[index_1]:
                    com_index = index_1
        else:
            com_index = 0
        
    top = Tk()
    top.title('串口设置')
    top.geometry('720x500+150+150')
    label_com = Label(top, text = '串口号',height = 2).place(x = 20,y = 36,width = 40,height = 30)
    label_bps = Label(top, text = '波特率',height = 2).place(x = 20,y = 72,width = 40,height = 30)
    
    #串口设置
    varport = StringVar()
    combo_com = ttk.Combobox(top, textvariable = varport, width = 8, height = 2, justify = CENTER)
    combo_com['value'] = com_list
    combo_com.place(x = 80, y=36, width = 80, height = 30)
    combo_com.current(com_index)
    
    #波特率设置
    varbitrate = StringVar()
    combo_bps = ttk.Combobox(top, textvariable = varbitrate, width = 8 ,height = 2, justify = CENTER)
    combo_bps['value'] = buadrate_list
    combo_bps.place(x = 80, y = 72, width = 80, height = 30)
    combo_bps.current(buadrate_index)
    
    make_set = Button(top,text = '确定',width = 18, height = 2)
    make_set.bind('<Button-1>', lambda event : com_bpsset(combo_com,combo_bps))
    make_set.place(x = 520,y=420)
    
    #top.mainloop()
    

def file_select():
    global filenames
    filenames = tkinter.filedialog.askopenfilename()
    
    
