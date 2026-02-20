#Import necessary libraries
import asyncio
import sys
from PyQt6.QtWidgets import QApplication, QWidget, QPushButton, QVBoxLayout, QHBoxLayout
from PyQt6.QtCore import QSize, Qt
import server

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
            "Arm": False,
            "5V": False,
            "12V": False,
            "24V": False,
            "55V": False,
        }

    
    # Creates horizontal and vertical layouts
    def initUI(self):
        # Layouts
        main_layout = QVBoxLayout()
        row_layout = QHBoxLayout()
        
        # Button names
        button_names = ["Motor", "Arm", "5V", "12V", "24V", "55V"]
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
        
        main_layout.addLayout(row_layout)
        
        # Create red circular estop button
        self.estop_btn = QPushButton("ESTOP")
        self.estop_btn.setCheckable(True)
        self.estop_btn.setFixedSize(QSize(120, 120))
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
        main_layout.addWidget(self.estop_btn, 0, alignment=Qt.AlignmentFlag.AlignCenter)
        
        self.setLayout(main_layout)
    
    # Makes toggle mechanism for smaller buttons
    def button_toggled(self):
        # Print states of small buttons
        #states = {btn.text(): btn.isChecked() for btn in self.buttons}
        #print("Button states:", states)
        
        btn = self.sender()  # the button that was clicked
        name = btn.text()
        checked = btn.isChecked()

            # Update flag
        self.button_flags[name] = checked

            # Look up the correct bitmask command
        cmd = self.BUTTON_COMMANDS.get(name, {}).get(checked, 0)

        print(f"[GUI DEBUG] Button {name} = {checked}, sending {hex(cmd)}")

            # Broadcast it
        asyncio.create_task(self.broadcast(cmd))
        
    # Bitmask commands (same as your C defines)
    BUTTON_COMMANDS = {
        "Motor": {
            True: 4,   # MOTOR_ON_CMD
            False: 5,  # MOTOR_OFF_CMD
        },
        "Arm": {
            True: 6,   # ARM_ON_CMD
            False: 7,  # ARM_OFF_CMD
        },
        "5V": {
            True: 8,   # ON_5V_CMD
            False: 12, # OFF_5V_CMD
        },
        "12V": {
            True: 9,   # ON_12V_CMD
            False: 13, # OFF_12V_CMD
        },
        "24V": {
            True: 10,  # ON_24V_CMD
            False: 14, # OFF_24V_CMD
        },
        "55V": {
            True: 11,  # ON_55V_CMD
            False: 15, # OFF_55V_CMD
        },
        "ESTOP": 0   # Special case: always send 0
    }

    
    def update_flag(self, name, checked):
        self.button_flags[name] = checked
        #message = f"{name}:{int(checked)}"

    # Determine message based on which button was pressed
    
    
        # match name:
        #     case "Motor":
        #         message = f"MOTOR_STATE:{int(checked)}"
        #     case "Arm":
        #         message = f"ARM_STATE:{int(checked)}"
        #     case "5V":
        #         message = f"POWER_5V:{int(checked)}"
        #     case "12V":
        #         message = f"POWER_12V:{int(checked)}"
        #     case "24V":
        #         message = f"POWER_24V:{int(checked)}"
        #     case "55V":
        #         message = f"POWER_55V:{int(checked)}"
        #     case _:
        #         message = f"{name}:{int(checked)}"  # fallback

        
        # Look up the corresponding command
        message = self.BUTTON_COMMANDS.get(name, {}).get(checked, 0)

        # Debug
        print(f"[GUI DEBUG] Button {name} set to {checked}, sending command: {hex(message)}")
        
        
        # Schedule async send without blocking GUI
        asyncio.create_task(self.broadcast(message))
    
    async def broadcast(self, message):
        if not self.clients:
            print("No clients connected.")
            return

        print(f"Sending to {len(self.clients)} client(s): {message} ({hex(message)})")
        await asyncio.gather(
            *(client.send(str(message)) for client in self.clients),
            return_exceptions=True
        )


    # Makes toggle mechanism for estop
    def master_toggle(self):
        # Enable/disable small buttons based on ESTOP state
        # state = self.estop_btn.isChecked()
        # for btn in self.buttons:
        #     btn.setEnabled(state)   # Enable when BIG ESTOP ON, disable when OFF
        #     if not state:
        #         btn.setChecked(False)  # Turn them off if ESTOP OFF
        
        # status = "ON" if state else "OFF"
        # print(f"ESTOP is {status}")
        
        # Simply send 0 to all websocket clients when ESTOP is clicked
        print("[GUI DEBUG] ESTOP clicked, sending 0")
        asyncio.create_task(self.broadcast(0))

# Show window onto screen
# if __name__ == "__main__":
#     app = QApplication(sys.argv)
#     window = ToggleButtonsWindow()
#     window.show()
#     sys.exit(app.exec())
    

