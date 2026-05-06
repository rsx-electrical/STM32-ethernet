import asyncio
import websockets
import gui
import sys
from qasync import QEventLoop
import re

##############################################
# from rsx.h:
#define ESTOP_CMD	(1U << 0)
#define MEASURE_V_CMD (1U << 1)
#define	MEASURE_B_CMD (1U << 2)
#define	MEASURE_A_CMD (1U << 3)
#define	MOTOR_ON_CMD (1U << 4)
#define	MOTOR_OFF_CMD (1U << 5)
#define	ARM_ON_CMD (1U << 6)
#define	ARM_OFF_CMD (1U << 7)
#define	ON_5V_CMD (1U << 8)
#define	ON_12V_CMD (1U << 9)
#define	ON_24V_CMD (1U << 10)
#define	ON_55V_CMD (1U << 11)
#define	OFF_5V_CMD (1U << 12)
#define	OFF_12V_CMD (1U << 13)
#define	OFF_24V_CMD (1U << 14)
#define	OFF_55V_CMD (1U << 15)
###############################################
clients = set()
bms_mv = [0,0,0,0]
async def handler(websocket, window):
    clients.add(websocket)
    print("Connected!!!!")
    try:       
        await websocket.send("FINALLY")
        async for message in websocket:
            if re.fullmatch(r"\s*\d+(\s*,\s*\d+)*\s*", message):
                values = [int(x) for x in message.split(",") if x]
                for i in range(min(4, len(values))):
                    bms_mv[i] = values[i]
                print("BMS:", bms_mv)
                await websocket.send("ACK: " + message)
            else:
                # If parsing fails, treat as plain string
                print("RAW_MSG: ", message)
    finally:
        clients.remove(websocket) 

# async def stdin_sender():
#     loop = asyncio.get_running_loop()

#     while True:
#         msg = await loop.run_in_executor(None, input)
#         for ws in  clients:
#             print("SENT CMD: ", msg)
#             await ws.send(msg)

