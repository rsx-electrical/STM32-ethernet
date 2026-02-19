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
            btn.setEnabled(False)  # Disabled until BIG POWER ON
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
            btn.clicked.connect(lambda: asyncio.create_task(self.broadcast("Hello!")))
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
        states = {btn.text(): btn.isChecked() for btn in self.buttons}
        print("Button states:", states)
    
    def update_flag(self, name, checked):
        self.button_flags[name] = checked
        message = f"{name}:{int(checked)}"
        
        # Schedule async send without blocking GUI
        asyncio.create_task(self.broadcast(message))
    
    async def broadcast(self, message):
        if not self.clients:
            print("[WS DEBUG] No clients connected.")
            return
        print(f"[WS DEBUG] Sending to {len(self.clients)} client(s): {message}")
        await asyncio.gather(*(client.send(message) for client in self.clients), return_exceptions=True)


    # Makes toggle mechanism for estop
    def master_toggle(self):
        # Enable/disable small buttons based on ESTOP state
        state = self.estop_btn.isChecked()
        for btn in self.buttons:
            btn.setEnabled(state)   # Enable when BIG ESTOP ON, disable when OFF
            if not state:
                btn.setChecked(False)  # Turn them off if ESTOP OFF
        
        status = "ON" if state else "OFF"
        print(f"ESTOP is {status}")

# Show window onto screen
# if __name__ == "__main__":
#     app = QApplication(sys.argv)
#     window = ToggleButtonsWindow()
#     window.show()
#     sys.exit(app.exec())
