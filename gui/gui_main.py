from tkinter import *
from tkinter import scrolledtext
import serial.tools.list_ports
from . import serialset_gui

class App:
    def __init__(self,master):
        self.master = master
        self.initwidget()

    def initwidget(self):
        self.menubar = Menu(self.master)
        #创建文件菜单
        filemenu = Menu(self.menubar,tearoff = 0)
        self.menubar.add_cascade(label='文件',menu = filemenu)
        filemenu.add_command(label = '选择测试文件',command = serialset_gui.file_select)
        filemenu.add_command(label = '选择验证文件',command = serialset_gui.vfile_select)
        filemenu.add_separator()
        filemenu.add_command(label = '退出',command = self.master.quit)
        
        #创建串口选择菜单
        portmenu = Menu(self.menubar,tearoff = 0)
        self.menubar.add_cascade(label = '串口',menu = portmenu)
        portmenu.add_command(label = '端口选择', command = self.port_set)
        self.master.config(menu = self.menubar)
        
        #LOG 显示窗口
        Label(self.master, text = 'LOG OF TEST:',height = 2).place(x = 20,y=36,width = 90,height = 30)
        # self.log = Text(self.master, width = 70, height = 35)
        # self.log.place(x = 20,y= 80)
        # scroll = Scrollbar(self.master, command = self.log.yview)
        # scroll.pack(side = RIGHT, fill = Y)
        # self.log.configure(yscrollcommand = scroll.set)
        self.scr = scrolledtext.ScrolledText(self.master, width = 70,height = 35,wrap = WORD)
        self.scr.place(x=20,y=80)
        self.scr.insert(END,'SPI fuction test log\n')
        self.scr.insert(END,'============================================================\n')
        
        
        
        
        #测试开始按钮
        start_wid = Button(self.master, width = 18,height = 2, text = '开始测试')
        start_wid.bind('<Button-1>', lambda event : serialset_gui.uart_open(self.scr))
        start_wid.place(x = 660, y = 480)
        
        
    def port_set(self):
        serialset_gui.serial_paraset()
  
def gui_run():  
    root = Tk()
    root.title('SPI功能测试')
    root.geometry('1020x600+100+100')
    root.iconbitmap('F:\\IMU383_SPI_AUTOTEST\\PICTURE\\spi.ico')
    ap_gui = App(root)
    root.mainloop()
    
if __name__ == '__main__':
    gui_run()