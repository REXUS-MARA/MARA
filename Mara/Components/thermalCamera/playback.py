import struct
from PIL import Image, ImageDraw, ImageColor, ImageFont
import pygame
from pygame.locals import *
import time
import sys

#config
scale = 5
delay = 0.2

#global vars and inits
f = []
screen = pygame.display.set_mode((320*scale, 240*scale))
pygame.display.set_caption("Image")
file = open("file1.dat", "br")
#file = open("thermography-E6D72BF90909.csv", "br")
font = ImageFont.truetype("tratex.ttf", scale*6)
i = 0
baseTime = 0
buf = [[0.0 for i in range(240)] for i in range(320)]
oldTime = 0

while True:
    # create new image in PIL (needed for saving to .gif)
    img = Image.new("RGB", (320*scale, 240*scale), color="white")
    d = ImageDraw.Draw(img)

    # backend needed for pygame
    for event in pygame.event.get():
        if event.type == QUIT:
            pygame.quit()
            quit()
        if event.type == MOUSEBUTTONUP:
            pass

    # read image from file
    for y in range(240):
        for x in range(320):
            try:
                buf[x][y] = struct.unpack("f", file.read(4))[0]
            except:
                if "-s" in sys.argv:
                    f[0].save(
                        "data.gif",
                        save_all=True,
                        append_images=f[1:],
                        duration=100,
                        loop=0,
                        optimize=False
                    )
                    print(len(f))
                    print("saving and quitting")
                else:
                    print("quitting")
                quit()

    minVal = 1000
    maxVal = 0
    maxPos = (0,0)
    for y in range(240):
        if min(buf[y]) < minVal:
            minVal = min(buf[y])
        if max(buf[y]) > maxVal:
            maxVal = max(buf[y])
            maxPos = (buf[y].index(maxVal), y)

    # render image
    for y in range(240):
        for x in range(320):
            val = buf[x][y]
            val = ((val-minVal)/(maxVal-minVal))*255
#            d.point((x, y), fill =(int(val), int(val), int(val)))
            d.rectangle([x*scale, y*scale, x*scale+(scale-1), y*scale+(scale-1)], fill=(int(val), int(val), int(val)))

    # get relative timestamp
    timestamp = struct.unpack("d", file.read(8))[0]
    if 0 == baseTime:
        baseTime = timestamp
    timestamp -= baseTime

    # add info text
    d.text((scale*2,scale*2), str(i), font=font, fill=(255,0,0))
    d.text((scale*2,scale*8), str(int(timestamp*1000)) + " ms", font=font, fill=(255,0,0))
    d.text((maxPos[1]*scale, maxPos[0]*scale), str(round(maxVal,2)), font=font, fill=(255,0,0))

    #PIL ==> pygame integration
    rgb_image = img.tobytes("raw", "RGB")
    image = pygame.image.fromstring(rgb_image, img.size, "RGB")
    screen.blit(image, (0, 0))
    pygame.display.flip()
    print(timestamp-oldTime)
    oldTime = timestamp
    f.append(img)
    i += 1
    time.sleep(delay)
