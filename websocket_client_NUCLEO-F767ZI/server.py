import asyncio
import websockets

async def handler(websocket):
    print("Connected!!!!")
    await websocket.send("FINALLY")
    async for message in websocket:
        print("TEST: ", message)
        await websocket.send("ACK: " + message)

async def main():
    async with websockets.serve(handler, "0.0.0.0", 8080):
        print("Server running on ws://192.168.0.10:8080")
        await asyncio.Future()  # Run forever

asyncio.run(main())