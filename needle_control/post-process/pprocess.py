import glob
import os
import random
import time
import tkFileDialog
from Tkinter import *

class PProcess:

    def __init__(self, window):
        """
        Initialization of GUI widgets
        """
        self.window = window

        self.menu = Menu(self.window)
        self.menu_file = Menu(self.menu)      # selecting files
        self.menu_settings = Menu(self.menu)  # changing settings
        
        # The following is how you connect functions to the widgets you create.
        # command=self.functionName
        #
        # self.menu_file.add_command(label='', command=$$$)

        self.menu.add_cascade(label='File', menu=self.menu_file)
        self.menu.add_cascade(label='Settings', menu=self.menu_settings)

        self.window.config(menu=self.menu)

        # buttons
        self.btn_frame = Frame(self.window)
        self.btn_select_file = Button(self.btn_frame, text='Select GCode file', command=self.openFile)
        self.btn_init_process = Button(self.btn_frame, text='Initial Post-Processing', command=self.clicked)
        self.lbl_extruder = Label(self.btn_frame, text='Extruder Options')

        self.sep = Frame(master=self.btn_frame, height=2, bd=1, relief=SUNKEN)

        self.EXTRUDERS = [
          "T1",
          "T2",
          "T3"
        ]
        self.lbl_select_extruder = StringVar(self.btn_frame)
        self.lbl_select_extruder.set("Select an extruder")
        self.btn_select_extruder = OptionMenu(self.btn_frame, self.lbl_select_extruder, *self.EXTRUDERS)

        self.entry_start = Entry(self.btn_frame) # start value of the extruding range
        self.entry_end = Entry(self.btn_frame)   # end value of the extruding range

        # ... and so on
        # eventually need to make another frame that displays
        # each line of code in the GCode file for selecting

        self.btn_frame.pack(anchor='nw')
        self.btn_select_file.pack(fill=X)
        self.btn_init_process.pack(fill=X)
        self.sep.pack(fill=X, padx=5, pady=5)
        self.lbl_extruder.pack(fill=X)
        self.btn_select_extruder.pack(fill=X)
        self.entry_start.pack(fill=X)
        self.entry_end.pack(fill=X)



    # DEFINE FUNCTIONS HERE! Make sure to document. vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
    def clicked(self):
        print('You clicked something!')

    def openFile(self):
        self.window.file_path = tkFileDialog.askopenfilename(
            initialdir=os.getcwd(), 
            title="Select file", 
            filetypes=(("gcode files","*.gcode"),("all files","*.*")))
        self.updateWindowTitle()
        # print(self.window.file_path)

    def updateWindowTitle(self):
        self.window.title = self.window.file_path

    '''
    main functions we need (and feel free to make helpers):
    - selecting a gcode file from a directory 
    - initial postprocessing, make repetier compatible (see gcodeconverter.py)
    - be able to select lines of gccode we imported, type in parameters for adding
      extruder commands, then post-process 
    '''