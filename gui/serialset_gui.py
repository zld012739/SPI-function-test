from tkinter import * 
from tkinter import ttk
import serial.tools.list_ports
import serial
import threading
import tkinter.filedialog


filenames = ''
ser = 0

def  getcom():
    port_list = list(serial.tools.list_ports.comports())
    serial_com = []
    for index in range(len(port_list)):
        serial_com.append(port_list[index][0])
    return serial_com
com_list = getcom()

def thread_recv(text_handel):
    global ser
    while True:
        try:
            read = ser.readall()
            if len(read) > 0:
                text_handel.insert(END, bytes(read).decode('gb2312'))
        except Exception as e:
            pass
    
def uart_open(com,bps,text_handel):
    global ser
    ser = serial.Serial(port = com, baudrate = int(bps), timeout = 0.2)
    if ser.is_open:
        pass
    else:
        ser.open()
    recv_data = threading.Thread(target = thread_recv,args = (text_handel,),name = 'T1')
    recv_data.start()
    # try:
        # ser = serial.Serial(port = com, baudrate = int(bps), timeout = 0.2)
        # if ser.iso_open:
            # pass
        # else:
            # ser.open()
        # recv_data = threading.Thread(target = thread_recv,args = (text_handel,),name = 'T1')
        # recv_data.start()
    # except Exception:
        # pass
        
    print(com,bps)


# def serial_paraset(text_handel):
    # top = Toplevel()
    # top.title('串口设置')
    # top.geometry('720x500+150+150')
    # label_com = Label(top, text = '串口号',height = 2).place(x = 20,y = 36,width = 40,height = 30)
    # label_bps = Label(top, text = '波特率',height = 2).place(x = 20,y = 72,width = 40,height = 30)
    
    # #串口设置
    # varport = StringVar()
    # combo_com = ttk.Combobox(top, textvariable = varport, width = 8, height = 2, justify = CENTER)
    # combo_com['value'] = com_list
    # combo_com.current(1)
    # combo_com.place(x = 80, y=36, width = 80, height = 30)
    
    # #波特率设置
    # varbitrate = StringVar()
    # combo_bps = ttk.Combobox(top, textvariable = varbitrate, width = 8 ,height = 2, justify = CENTER)
    # combo_bps['value'] = ('9600','19200','38400','115200','230400')
    # combo_bps.current(1)
    # combo_bps.place(x = 80, y = 72, width = 80, height = 30)
    
    # make_set = Button(top,text = '确定',width = 18, height = 2)
    # make_set.bind('<Button-1>', lambda event : uart_open(combo_com.get(),combo_bps.get(),text_handel))
    # make_set.place(x = 520,y=420)
def serial_paraset():
    top = Toplevel()
    top.title('串口设置')
    top.geometry('720x500+150+150')
    label_com = Label(top, text = '串口号',height = 2).place(x = 20,y = 36,width = 40,height = 30)
    label_bps = Label(top, text = '波特率',height = 2).place(x = 20,y = 72,width = 40,height = 30)
    
    #串口设置
    varport = StringVar()
    combo_com = ttk.Combobox(top, textvariable = varport, width = 8, height = 2, justify = CENTER)
    combo_com['value'] = com_list
    combo_com.current(1)
    combo_com.place(x = 80, y=36, width = 80, height = 30)
    
    #波特率设置
    varbitrate = StringVar()
    combo_bps = ttk.Combobox(top, textvariable = varbitrate, width = 8 ,height = 2, justify = CENTER)
    combo_bps['value'] = ('9600','19200','38400','115200','230400')
    combo_bps.current(1)
    combo_bps.place(x = 80, y = 72, width = 80, height = 30)
    
    make_set = Button(top,text = '确定',width = 18, height = 2)
    make_set.bind('<Button-1>', lambda event : uart_open(combo_com.get(),combo_bps.get(),text_handel))
    make_set.place(x = 520,y=420)

def file_select():
    global filenames
    filenames = tkinter.filedialog.askopenfilename()
    
# def uart_close(ser):
    # if ser.isClose():
        # pass
    # else:
        # ser.close()

def uart_send():
    global ser
    try:
        global filenames
        f = open(filenames)
        f_lines = f.readlines()
        f.close()
        for index in f_lines:
            ser.write(var.encode('gb2312'))
    except Exception:
        pass
        

    
