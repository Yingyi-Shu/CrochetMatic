from Tkinter import Tk
from pprocess import PProcess

def main():
    window = Tk()
    window.title('GCode -> Repetier Post Processing')
    window.geometry('800x600')
    window.option_add('*tearOff', False) # avoid dotted line
    run = PProcess(window)
    window.mainloop()

main()