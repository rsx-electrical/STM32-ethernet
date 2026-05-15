import asyncio
import websockets
import re

clients = set()
bms_mv = [0, 0, 0, 0]


async def handler(websocket, window=None):
    clients.add(websocket)

    print("Connected!!!!")

    try:
        async for message in websocket:

            print(f"\nRX: {message}")

            if re.fullmatch(r"\s*\d+(\s*,\s*\d+)*\s*", message):

                values = [int(x.strip()) for x in message.split(",")]

                for i in range(min(4, len(values))):
                    bms_mv[i] = values[i]

                print("BMS:", bms_mv)

            else:
                print("RAW_MSG:", message)

    finally:
        clients.remove(websocket)
        print("Disconnected")

async def stdin_sender():
    loop = asyncio.get_running_loop()

    while True:
        try:
            # show prompt and wait for input without blocking asyncio
            msg = await loop.run_in_executor(None, input, "CMD> ")

            msg = msg.strip()

            if not msg:
                continue

            if not re.fullmatch(r"\d+", msg):
                print("Please enter a number")
                continue

            if not clients:
                print("No clients connected")
                continue

            print(f"SENT CMD: {msg}")

            await asyncio.gather(
                *(ws.send(msg) for ws in clients),
                return_exceptions=True
            )

        except Exception as e:
            print("stdin_sender error:", e)