import asyncio
import websockets
clients = set()

async def handler(websocket):
    print("Connected!!!!")
    await websocket.send("FINALLY")
    async for message in websocket:
        print("TEST: ", message)
        await websocket.send("ACK: " + message)

async def stdin_sender():
    loop = asyncio.get_running_loop()

    while True:
        msg = await loop.run_in_executor(None, input)
        for ws in clients:
            await ws.send(msg)

async def main():
    async with websockets.serve(handler, "0.0.0.0", 8080):
        print("Server running on ws://192.168.0.10:8080")
        await asyncio.gather(
            stdin_sender(),
            asyncio.Future(),   # run forever
        ) 

asyncio.run(main())