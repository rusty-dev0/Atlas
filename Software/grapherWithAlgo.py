import time
import pygame
import json
import math
from pyrplidar import PyRPlidar

W, H = 800, 800

PW = 1

ZOOM = 2.5

def toScreenUnits(x : int, L : int, maxDist : int):
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
    

def setupLidar():
    lidar = PyRPlidar()
    lidar.connect(port="/dev/ttyUSB0", baudrate=460800, timeout=3)
    lidar.set_motor_pwm(500)
    time.sleep(2)
    return lidar

def setupPygame():
    pygame.init()
    screen = pygame.display.set_mode((W, H))
    pygame.display.set_caption("Polar Graphing Test")
    clock = pygame.time.Clock()
    return screen, clock

def loadDynamicPointCloudFromLidar():
    screen, clock = setupPygame()
    lidar = setupLidar()
    scan_generator = lidar.force_scan()
    running = True
    step = 45
    try:
        pointsToLoad : list[Point] = []
        polarPointsToLoad : list[PolarPoint] = []
        polarHistogram : dict[float, tuple[int, int]] = {} # total distance, count
        for i in range(8):
            polarHistogram[i] = (0, 0)
        prevAngle = None

        for scan in scan_generator():
            for event in pygame.event.get():
                if event.type == pygame.QUIT:
                    running = False
            
            pointsToLoad.append(cwPolarToCartesian(toScreenUnits(scan.distance, W // 2, 4000), scan.angle))
            
            ccwAngle = (-scan.angle) % 360

            bin_index = int(((ccwAngle + step / 2) % 360) // step) % 8

            total_dist, count = polarHistogram[bin_index]
            polarHistogram[bin_index] = (total_dist + scan.distance, count + 1)

            if prevAngle is not None and scan.angle < prevAngle:

                screen.fill((0,0,0))

                # draw circles for reference and grid
                for r in range(50, W // 2, 50):
                    pygame.draw.circle(screen, (50, 50, 50), (W // 2, H // 2), r, 1)
                    
                # center point
                pygame.draw.circle(screen, (0, 255, 0), (W // 2, H // 2), 5) 

                for p in pointsToLoad:
                    pygame.draw.circle(screen, (255, 0, 0), p.toScreen(), PW)
                # pygame.draw.circle(screen, (255, 0, 0), cwPolarToCartesian(100, 90).toScreen(), 5)
                bestDir : int = 0
                for i in range(8):
                    if polarHistogram[i][1] > 0 and polarHistogram[i][0] / polarHistogram[i][1] > polarHistogram[bestDir][0] / max(polarHistogram[bestDir][1], 1):
                        bestDir = i
                pygame.draw.line(screen, (0, 0, 255), (W // 2, H // 2), polarToCartesian(300, bestDir * step).toScreen(), 2)
                pygame.display.flip()
                clock.tick(60)
                pointsToLoad.clear()
                for i in range(8):
                    polarHistogram[i] = (0, 0)
            
            prevAngle = scan.angle

            if not running:
                break

    except KeyboardInterrupt:
        print("\nStopped")
    finally:
        lidar.stop()
        lidar.set_motor_pwm(0)
        lidar.disconnect()
        pygame.quit()


def loadStaticPointCloud():
    screen, clock = setupPygame()
    with open("lidar_sweep.json", "r") as f:
        data = json.load(f)
        pointsToLoad : list[Point] = []
        # print(data["points"])
        for point in data["points"]:
            angle = point["angle_deg"]
            distance = point["distance_mm"]
            pointsToLoad.append(cwPolarToCartesian(toScreenUnits(distance, W // 2, 4000), angle))
            # print(f"Angle: {angle}°, Distance: {distance}mm -> Cartesian: ({x:.2f}, {y:.2f})")
        running = True
        try:
            while running:
                for event in pygame.event.get():
                    if event.type == pygame.QUIT:
                        running = False
                screen.fill((0,0,0))

                # draw circles for reference and grid
                for r in range(50, W // 2, 50):
                    pygame.draw.circle(screen, (50, 50, 50), (W // 2, H // 2), r, 1)
                    
                # center point
                pygame.draw.circle(screen, (0, 255, 0), (W // 2, H // 2), 1) 

                for p in pointsToLoad:
                    pygame.draw.circle(screen, (255, 0, 0), p.toScreen(), PW)
                # pygame.draw.circle(screen, (255, 0, 0), cwPolarToCartesian(100, 90).toScreen(), 5)

                pygame.display.flip()
                clock.tick(60)
        except KeyboardInterrupt:
            print("\nStopped")
        finally:
            pygame.quit()

if __name__ == "__main__":
    loadDynamicPointCloudFromLidar()