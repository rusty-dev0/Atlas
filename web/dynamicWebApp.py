import asyncio
import json
import math
import time
import sys
from contextlib import asynccontextmanager
import anyio
from fastapi import FastAPI, WebSocket, WebSocketDisconnect
from fastapi.responses import HTMLResponse
from pyrplidar import PyRPlidar

@asynccontextmanager
async def lifespan(app: FastAPI):
    print("Starting up: Offloading LiDAR data fetching to a separate worker thread...")
    lidar_task = asyncio.create_task(anyio.to_thread.run_sync(lidarStreamWorker))
    yield
    print("Shutting down: Stopping background tasks...")
    lidar_task.cancel()
    try:
        await lidar_task
    except asyncio.CancelledError:
        pass
    print("Shutdown complete.")

app = FastAPI(lifespan=lifespan)

latestScanData = []
bestDirection = 0

W, H = 800, 800
ZOOM = 1
STEP = 45

def toScreenUnits(x: int, L: int = W, maxDist: int = 4000):
    return int((x / maxDist) * (L // 2) * ZOOM)

def cwPolarToCartesian(r, t):
    return polarToCartesian(r, -t)

def polarToCartesian(r, t):
    x = r * math.cos(math.radians(t))
    y = r * math.sin(math.radians(t))
    return Point(x, y)

class Point:
    def __init__(self, x: int, y: int):
        self.x = x
        self.y = y

    def toScreen(self):
        return W // 2 + self.x, H // 2 - self.y

class PolarPoint:
    def __init__(self, r: float, t: float):
        self.r = r
        self.t = t

def lidarStreamWorker():
    global latestScanData, bestDirection

    lidar = PyRPlidar()
    try:
        lidar.connect(port="/dev/ttyUSB0", baudrate=460800, timeout=3)
        lidar.set_motor_pwm(500)
        time.sleep(2)
        
        scanGenerator = lidar.force_scan()
        pointsToLoad: list[tuple[int, int]] = []
        polarHistogram: dict[int, tuple[int, int]] = {i: (0, 0) for i in range(8)}
        prevAngle = None

        print("LiDAR connected. Beginning data capture loop...")
        for scan in scanGenerator():
            screenR = toScreenUnits(scan.distance)
            coords = cwPolarToCartesian(screenR, scan.angle)
            pointsToLoad.append(coords.toScreen())

            ccwAngle = (-scan.angle) % 360
            binIndex = int(((ccwAngle + STEP / 2) % 360) // STEP) % 8
            total_distance, count = polarHistogram[binIndex]
            polarHistogram[binIndex] = (total_distance + scan.distance, count + 1)

            if prevAngle is not None and scan.angle < prevAngle:
                currBestDirection = 0
                for i in range(8):
                    currAvg = polarHistogram[i][0] / polarHistogram[i][1] if polarHistogram[i][1] > 0 else 0
                    bestAvg = polarHistogram[currBestDirection][0] / polarHistogram[currBestDirection][1] if polarHistogram[currBestDirection][1] > 0 else 0
                    if currAvg > bestAvg:
                        currBestDirection = i
                
                latestScanData = list(pointsToLoad)
                bestDirection = currBestDirection * STEP
                pointsToLoad.clear()
                for i in range(8):
                    polarHistogram[i] = (0, 0)
            
            prevAngle = scan.angle
            
    except Exception as e:
        print(f"\nCRITICAL ERROR in lidarStreamWorker: {e}", file=sys.stderr)
    finally:
        print("Cleaning up LiDAR hardware connections...")
        try:
            lidar.stop()
            lidar.set_motor_pwm(0)
            lidar.disconnect()
        except Exception:
            pass

# the web UI here is mainly AI generated boilerplate for now
html = """<!DOCTYPE html>
<html>
<head>
    <title>LiDAR Web Canvas</title>
    <style>
        body { background: #111; color: white; text-align: center; font-family: sans-serif; }
        canvas { background: #000; border: 2px solid #333; margin-top: 10px; }
    </style>
</head>
<body>
    <h1>LiDAR Map</h1>
    <div id="status">Connecting to LiDAR...</div>
    <canvas id="radarCanvas" width="800" height="800"></canvas>

    <script>
        const canvas = document.getElementById("radarCanvas");
        const ctx = canvas.getContext("2d");
        const cx = canvas.width / 2;
        const cy = canvas.height / 2;
        
        const ws = new WebSocket("ws://localhost:8000/ws");
        
        ws.onopen = () => document.getElementById("status").innerText = "Streaming Data";
        ws.onclose = () => document.getElementById("status").innerText = "Disconnected";

        ws.onmessage = (event) => {
            const data = JSON.parse(event.data);
            drawRadar(data.points, data.best_angle);
        };

        function drawRadar(points, bestAngle) {
            ctx.clearRect(0, 0, canvas.width, canvas.height);
            
            // 1. Draw Grid Rings
            ctx.strokeStyle = "#222222";
            ctx.lineWidth = 1;
            for (let r = 50; r < cx; r += 50) {
                ctx.beginPath();
                ctx.arc(cx, cy, r, 0, 2 * Math.PI);
                ctx.stroke();
            }
            
            // 2. Center Node
            ctx.fillStyle = "#00ff00";
            ctx.beginPath();
            ctx.arc(cx, cy, 5, 0, 2 * Math.PI);
            ctx.fill();

            // 3. Draw Point Cloud
            ctx.fillStyle = "#ff0000";
            points.forEach(p => {
                ctx.beginPath();
                ctx.arc(p.x, p.y, 1.5, 0, 2 * Math.PI);
                ctx.fill();
            });

            // 4. Draw Best Direction Line
            if (bestAngle !== undefined) {
                const rad = bestAngle * (Math.PI / 180);
                const targetX = cx + 300 * Math.cos(rad);
                const targetY = cy - 300 * Math.sin(rad);
                
                ctx.strokeStyle = "#0000ff";
                ctx.lineWidth = 3;
                ctx.beginPath();
                ctx.moveTo(cx, cy);
                ctx.lineTo(targetX, targetY);
                ctx.stroke();
            }
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
                await asyncio.wait_for(websocket.receive_text(), timeout=0.005)
            except asyncio.TimeoutError:
                pass

            payload = {
                "points": [{"x": p[0], "y": p[1]} for p in latestScanData],
                "best_angle": bestDirection
            }

            await websocket.send_json(payload)
            await asyncio.sleep(0.05)

    except WebSocketDisconnect:
        print("WebSocket disconnected")

if __name__ == "__main__":
    import uvicorn
    uvicorn.run("dynamicWebApp:app", host="localhost", port=8000, reload=True)
