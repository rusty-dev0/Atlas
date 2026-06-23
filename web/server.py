from fastapi import FastAPI, WebSocket, WebSocketDisconnect
from fastapi.responses import HTMLResponse
import asyncio
import json

app = FastAPI()

html = """
<html>
    <head>
        <title>WebSocket</title>
    </head>

    <body>
        <h1>WebSocket Controller</h1>
        <div id="metrics">im bored...</div>
        <button onclick="sendSomething()">do something</button>
        <script>
            const protocol = window.location.protocol === "https:" ? "wss" : "ws";
            const ws = new WebSocket(`${protocol}://${window.location.host}/ws`);

            ws.onopen = function(event) {
                console.log("WebSocket is open");
            };

            ws.onmessage = function(event) {
                const metrics = document.getElementById("metrics");
                metrics.textContent = event.data;
            };

            ws.onerror = function(event) {
                console.log("WebSocket error:", event);
            };

            ws.onclose = function(event) {
                console.log("WebSocket is closed");
            };

            function sendSomething() {
                ws.send(JSON.stringify({action: "heheheha"}));
            }
        </script>
    </body>
</html>
"""

@app.get("/")
async def get():
    return HTMLResponse(html)

@app.websocket("/ws")
async def websocket_endpoint(websocket: WebSocket):
    await websocket.accept()
    try:
        while True:
            try:
                data = await websocket.receive_text()
                message = json.loads(data)
                print(f"received something: {message}")
            except asyncio.TimeoutError:
                pass
            except json.JSONDecodeError:
                print("received some non-json bs")
                pass

            sensor_data = {"thing1": 123, "thing2": 456}
            await websocket.send_json(sensor_data)

            await asyncio.sleep(0.05)
    except WebSocketDisconnect:
        print("WebSocket disconnected")

if __name__ == "__main__":
    import uvicorn
    uvicorn.run("server:app", host="localhost", port=8000, reload=True)