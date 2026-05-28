#Import necessary libraries
import asyncio
import sys
from PyQt6.QtWidgets import QApplication, QWidget, QPushButton, QVBoxLayout, QHBoxLayout
from PyQt6.QtCore import QSize, Qt
import server
import random

clients = set()


# Set up window
class ToggleButtonsWindow(QWidget):

    # Set position and size of window for window
    def __init__(self):
        super().__init__()

        self.clients = server.clients
        self.setWindowTitle("Control Panel")
        self.setGeometry(100, 100, 500, 300)
        self.initUI()
        self.button_flags = {
            "Motor": False,
            "12V": False,
            "55V": False,
            "Green": False,
            "Blue": False,
            "Red": False,
            "Reset": False
        }

    
    # Creates horizontal and vertical layouts
    def initUI(self):
        # Layouts
        main_layout = QHBoxLayout()   # Horizontal: ESTOP | Right Panel
        left_layout = QVBoxLayout()   #left side estop
               
        # Create red circular estop button
        self.estop_btn = QPushButton("ESTOP")
        self.estop_btn.setCheckable(True)
        self.estop_btn.setFixedSize(QSize(80, 80))
        self.estop_btn.setStyleSheet(
            "QPushButton {"
            "border-radius: 60px;"
            "background-color: red;"
            "color: white;"
            "font-size: 18px;"
            "font-weight: bold;"
            "}"
            "QPushButton:checked {background-color: darkred;}"
        )
        self.estop_btn.clicked.connect(self.master_toggle)
        left_layout.addWidget(self.estop_btn, 0, alignment=Qt.AlignmentFlag.AlignCenter)
        left_layout.addStretch()
        main_layout.addLayout(left_layout)


        #right side
        right_layout = QVBoxLayout()
        #top toggle buttons
        row_layout = QHBoxLayout()  
  # Button names
        button_names = ["Motor", "12V", "55V", "Green", "Blue", "Red", "Reset"]
        self.buttons = []

        # Create 6 circular toggle buttons
        for name in button_names:
            btn = QPushButton(name)
            btn.setCheckable(True)
            btn.setChecked(False)  # Ensure they start OFF
            btn.setEnabled(True)  # always on, not controled by the big
            btn.setFixedSize(QSize(80, 80))  # Make square
            btn.setStyleSheet(
                "QPushButton {"
                "border-radius: 40px;"
                "background-color: lightgray;"
                "font-weight: bold;"
                "}"
                "QPushButton:checked {background-color: green; color: white;}"
                "QPushButton:disabled {background-color: gray; color: darkgray;}"
            )
            btn.clicked.connect(self.button_toggled)
            #btn.clicked.connect(lambda: asyncio.create_task(self.broadcast("Hello!")))
            self.buttons.append(btn)
            row_layout.addWidget(btn)
        
        right_layout.addLayout(row_layout)
        
        #voltage, current and bms value rows
        from PyQt6.QtWidgets import QLabel
        
        # volt_layout = QHBoxLayout()
        # title_label = QLabel("Voltages (V):  ")
        # title_label.setStyleSheet("font-weight: bold; font-size: 16pt;")
        # volt_layout.addWidget(title_label)        
        # self.volt_labels = []
        # for _ in range(4):
        #     lbl = QLabel("--")
        #     lbl.setFixedWidth(70)
        #     self.volt_labels.append(lbl)
        #     volt_layout.addWidget(lbl)

        # refresh_v_btn = QPushButton("Refresh")
        # refresh_v_btn.setFixedWidth(90)  
        # refresh_v_btn.clicked.connect(self.refresh_v)
        # volt_layout.addStretch()          # pushes button to right
        # volt_layout.addWidget(refresh_v_btn)

        # right_layout.addLayout(volt_layout)

        # # ---- Current Row ----
        # curr_layout = QHBoxLayout()
        # title_label = QLabel("Current (A):   ")
        # title_label.setStyleSheet("font-weight: bold; font-size: 16pt;")
        # curr_layout.addWidget(title_label)

        # self.curr_labels = []
        # for _ in range(3):
        #     lbl = QLabel("--")
        #     lbl.setFixedWidth(70)
        #     self.curr_labels.append(lbl)
        #     curr_layout.addWidget(lbl)

        # refresh_a_btn = QPushButton("Refresh")
        # refresh_a_btn.setFixedWidth(90)   # same width for all
        # refresh_a_btn.clicked.connect(self.refresh_a)
        # curr_layout.addStretch()          # pushes button to right
        # curr_layout.addWidget(refresh_a_btn)

        # right_layout.addLayout(curr_layout)

        # # ---- Battery Row ----
        # batt_layout = QHBoxLayout()
        # title_label = QLabel("Battery (V):   ")
        # title_label.setStyleSheet("font-weight: bold; font-size: 16pt;")

        # batt_layout.addWidget(title_label)

        # self.batt_labels = []
        # for _ in range(6):
        #     lbl = QLabel("--")
        #     lbl.setFixedWidth(100)
        #     lbl.setStyleSheet("font-size: 16px; font-weight: bold;")
        #     self.batt_labels.append(lbl)
        #     batt_layout.addWidget(lbl)

        # refresh_b_btn = QPushButton("Refresh")
        # refresh_b_btn.setFixedWidth(90)   # same width for all
        # refresh_b_btn.clicked.connect(self.refresh_b)
        # batt_layout.addStretch()          # pushes button to right
        # batt_layout.addWidget(refresh_b_btn)

        # right_layout.addLayout(batt_layout)

        # right_layout.addStretch()
        main_layout.addLayout(right_layout)

        self.setLayout(main_layout)    
        
    # Makes toggle mechanism for smaller buttons
    def button_toggled(self):

        
        btn = self.sender()  # the button that was clicked
        name = btn.text()
        checked = btn.isChecked()

            # Update flag
        self.button_flags[name] = checked

            # Look up the correct int command
        cmd = (
            self.BUTTON_COMMANDS[name].get(checked, 0)
            if isinstance(self.BUTTON_COMMANDS.get(name), dict)
            else self.BUTTON_COMMANDS.get(name, 0)
        )
        print(f"[GUI DEBUG] Button {name} = {checked}, sending {cmd}")

            # Broadcast it
        asyncio.create_task(self.broadcast(cmd))
        
    # Bitmask commands (same as your C defines)
    # in dec
    BUTTON_COMMANDS = {
        "Motor": {
            True: "4",   # MOTOR_ON_CMD
            False: "5",  # MOTOR_OFF_CMD
        },
        "12V": {
            True: "9",   # ON_12V_CMD
            False: "13", # OFF_12V_CMD
        },
        # hijacking the 24V port bc 55V got taken over by bms
        "55V": {
            True: "10",  # ON_24V_CMD
            False: "14", # OFF_24V_CMD
        },
        "Green": 18,
        "Blue": 19,
        "Red": 17,
        "ESTOP": 16,   # Special case: always send 16,
        "Reset": 20
    }

    
    def update_flag(self, name, checked):
        self.button_flags[name] = checked 
        # Look up the corresponding command
        message = self.BUTTON_COMMANDS.get(name, {}).get(checked, 0)

        # Debug
        print(f"[GUI DEBUG] Button {name} set to {checked}, sending command: {message}")
        
        
        # Schedule async send without blocking GUI
        asyncio.create_task(self.broadcast(message))
    
    async def broadcast(self, message):
        if not self.clients:
            print("No clients connected.")
            return

        print(f"Sending to {len(self.clients)} client(s): {message} ({message})")
        await asyncio.gather(
            *(client.send(str(message)) for client in self.clients),
            return_exceptions=True
        )


    # Makes toggle mechanism for estop
    def master_toggle(self):
        cmd = self.BUTTON_COMMANDS.get("ESTOP",16)
        # Simply send 0 to all websocket clients when ESTOP is clicked
        print(f"[GUI DEBUG] ESTOP clicked, sending {cmd}")
        asyncio.create_task(self.broadcast(0))
        
    # def refresh_v(self):
    #     print("Refreshing voltages...")
    #     values = [5.021, 11.965, 23.992, 54.935]  # Example data
    #     for i in range(4):  # indices 0 to 3
    #         values[i] += random.uniform(-0.01, 0.06)
        
    #     self.volt_labels[0].setText(f"5V: --")
    #     self.volt_labels[1].setText(f"12V: --")   
    #     self.volt_labels[2].setText(f"24V: {values[2]:.2f}") 
    #     self.volt_labels[3].setText(f"55V: --") 
               

    # def refresh_a(self):
    #     print("Refreshing current...")
    #     values = [0.000, 0.00, 0.000]  # Example data
    #     for i in range(3):  
    #         values[i] += random.uniform(0, 0.005)
        
    #     self.curr_labels[0].setText(f"5V: {values[0]:.2f}")
    #     self.curr_labels[1].setText(f"12V: {values[1]:.2f}")   
    #     self.curr_labels[2].setText(f"24V: {values[2]:.2f}") 
    #     # TODO: send websocket request for current

    # def refresh_b(self):
    #     print("Refreshing battery...")
    #     #print(f"sending 2")

    #         # Broadcast it
    #     asyncio.create_task(self.broadcast(2))
    #     values = [3.987, 4.015, 3.955, 4.002]  # Example data
    #            # for SAR
    #     for i in range(4):  # indices 0 to 3
    #         values[i] += random.uniform(-0.06, 0.06)
        
        
    #     self.batt_labels[0].setText(f"C1: {server.bms_mv[0]/1000:.3f}")  
    #     self.batt_labels[1].setText(f"C2: {server.bms_mv[1]/1000:.3f}")
    #     self.batt_labels[2].setText(f"C3: {server.bms_mv[2]/1000:.3f}")
    #     self.batt_labels[3].setText(f"C4: {server.bms_mv[3]/1000:.3f}")
        
    #     # total
    #     self.batt_labels[4].setText(f"Total: {(server.bms_mv[0]+server.bms_mv[1]+server.bms_mv[2]+server.bms_mv[3])/1000:.3f}")
    #     self.batt_labels[5].setText(f"SOC (%): 84") #hard coded for now
       


        
        


    

