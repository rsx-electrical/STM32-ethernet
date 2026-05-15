import sys
import asyncio
from PyQt6.QtWidgets import QApplication
from qasync import QEventLoop
import gui
import server
from functools import partial
import websockets


async def start_server(window):

    async with websockets.serve(
        partial(server.handler, window=window),
        "0.0.0.0",
        8080
    ):

        print("Server running on ws://192.168.0.10:8080")

        # Start terminal sender task
        asyncio.create_task(server.stdin_sender())

        await asyncio.Future()


if __name__ == "__main__":

    app = gui.QApplication(sys.argv)

    loop = QEventLoop(app)
    asyncio.set_event_loop(loop)

    window = gui.ToggleButtonsWindow()
    window.show()

    with loop:
        loop.create_task(start_server(window))
        loop.run_forever()