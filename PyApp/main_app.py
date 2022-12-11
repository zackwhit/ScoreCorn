from PyQt5.QtWidgets import QApplication, QDialog, QMainWindow, QPushButton
from PyQt5.QtGui import QStandardItemModel, QStandardItem
from PyQt5.QtCore import QModelIndex
from PyQt5.QtCore import QTimer,QDateTime
from PyQt5 import uic
import serial
from serial.tools import list_ports

class MyGui(QMainWindow):

    def __init__(self):
        super(MyGui, self).__init__()
        uic.loadUi("QTApp.ui", self)
        self.show()

        self.listButton.clicked.connect(self.loadPorts)
        self.connectButton.clicked.connect(self.connectPort)
        self.disconnectButton.clicked.connect(self.disconnect)
        self.clearButton.clicked.connect(self.clearSerialData)
        self.throwButton.clicked.connect(self.sendThrow)
        self.resetButton.clicked.connect(self.resetMCU)
        self.printStateButton.clicked.connect(self.printState)
        self.tareButton.clicked.connect(self.tareCells)


        self.distSlider.valueChanged.connect(self.distSliderChanged)


        self.listModel = QStandardItemModel(self.listView)
        self.serialListModel = QStandardItemModel(self.listView_2)
        self.distancesListModel = QStandardItemModel(8, 1, self.listView_3)
        self.listView_2.setModel(self.serialListModel)
        self.listView_3.setModel(self.distancesListModel)

        # Setup read-serial timer
        self.readTimer = QTimer()
        self.readTimer.timeout.connect(self.readSerialData)
        self.readTimer.setInterval(100)

    def loadPorts(self):
        # Delete old stuff
        self.listModel.clear()
        # Load serial stuff
        ports = list_ports.comports()
        print(ports)
        # Load them into the app? 
        for port in ports:
            print(port)
            self.listModel.appendRow(QStandardItem(port.device))
        # Show
        self.listView.setModel(self.listModel)

    def connectPort(self):
        # Connect to the currently selected port
        print(F"Connecting to {self.listView.currentIndex().data()}")
        self.arduinoConnection = serial.Serial(
            port=self.listView.currentIndex().data(), 
            baudrate=115200, 
            timeout=0.1)
        #self.listButton.setEnabled(False)
        self.disconnectButton.setEnabled(True)
        self.connectButton.setEnabled(False)
        self.throwButton.setEnabled(True)
        self.readTimer.start()

    def disconnect(self):
        self.arduinoConnection.close()
        self.disconnectButton.setEnabled(False)
        self.connectButton.setEnabled(True)
        self.throwButton.setEnabled(False)
        self.readTimer.stop()

    def readSerialData(self):
        # Read lines and add them to the serial list view
        if (self.arduinoConnection.is_open and self.arduinoConnection.in_waiting):
            while(self.arduinoConnection.in_waiting):
                data = self.arduinoConnection.read_until(b'\n')
                #print(data.decode())
                #print('-'*20)
                # Breakup into lines
                splitBytes = data.split(b'\n')
                for line in splitBytes:
                    if (len(line) == 0): break
                    #print(line)
                    # Remove last byte (extra line return, basically)
                    #line = line[:len(line)-2]

                    # Breakup and examine data for a report
                    moreSplits = line.split(b':')
                    if(len(moreSplits) == 0):
                        return
                    # Check for a report
                    if (moreSplits[0] == b'R'):
                        addrIndex = int(moreSplits[1])
                        reportDist = float(moreSplits[2])
                        self.distancesListModel.setItem(addrIndex,
                            QStandardItem(F"{addrIndex}: {reportDist}m"))
                        if (self.checkBoxR.isChecked()):
                            self.serialListModel.appendRow(
                                QStandardItem(line.decode()))
                    elif (moreSplits[0].decode().find("Sum") != -1):
                        #print(moreSplits[0].decode())
                        weightOnBoard = float(moreSplits[1])
                        self.lcdNumber.display(weightOnBoard)
                        if (self.checkBoxSum.isChecked()):
                            self.serialListModel.appendRow(
                                QStandardItem(line.decode()))
                    else: 
                        # Add to display
                        self.serialListModel.appendRow(
                            QStandardItem(line.decode()))

        # Scroll to bottom
        self.listView_2.verticalScrollBar().setValue(self.listView_2.verticalScrollBar().maximum())


    def clearSerialData(self):
        self.serialListModel.clear()

    def distSliderChanged(self):
        dist = self.distSlider.value()
        self.landDistanceLabel.setText(F"Landing distance: {dist}")
        

    def sendThrow(self):
        # vXXXXiXX[n/y][n/y]
        print("Sending throw")
        dist = self.distSlider.value()
        serMsg = bytearray()
        distBytes = bytearray("v%04d" % dist, 'ascii')
        idBytes = bytearray("i%02d" % self.idEntryBox.value(),'ascii')

        flagBytes = None
        if self.rdBtnLineSensor.isChecked():
            flagBytes = bytearray('ny', 'ascii')
        elif self.rdBtnWeight.isChecked():
            flagBytes = bytearray('yn', 'ascii')
        elif self.rdBtnNeither.isChecked():
            flagBytes = bytearray('nn', 'ascii')

        # Concat all the bytes together and send them
        serMsg = serMsg + distBytes
        serMsg = serMsg + idBytes
        serMsg = serMsg + flagBytes

        print(F"Message length: {len(serMsg)}, final msg: {serMsg.decode()}")
        self.arduinoConnection.write(serMsg)

    def resetMCU(self):
        self.arduinoConnection.write(bytes('r', 'ascii'))
    
    def printState(self):
        self.arduinoConnection.write(bytes('p', 'ascii'))
    
    def tareCells(self):
        self.arduinoConnection.write(bytes('t', 'ascii'))
        

    def __del__(self):
        print("Cleaning up...")
        self.arduinoConnection.close()


def main():
    app = QApplication([])
    window = MyGui()
    window.loadPorts()
    app.exec_()



if __name__ == '__main__':
    main()
