
import argparse as ap
import cv2
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation

fps = 10

parser = ap.ArgumentParser()
parser.add_argument('input_file', type=str)
#parser.add_argument('output_file', type=str)
args = parser.parse_args()

vid = cv2.VideoCapture(args.input_file)

width = 1920
height = 1080
rows, cols = (height, width)
shape = (rows, cols)

fig = plt.figure(constrained_layout=True)
gs = fig.add_gridspec(2, 1)

raw_ax = fig.add_subplot(gs[0, 0])
raw_img = raw_ax.imshow(np.zeros(shape, dtype=np.uint32), cmap='gray')
coadd_ax = fig.add_subplot(gs[1, 0])
coadd_img = coadd_ax.imshow(np.zeros(shape, dtype=np.uint32), cmap='gray')

#ax.set_xlim(0, cols)
#ax.set_ylim(0, rows)

#raw_images = []
#coadd_images = []
#coadd = np.zeros(shape)
#count = 0
#coadd_frames = 30
#ok = True
#while ok:
#    ok, rgb_img = vid.read()
#    if ok:
#        # Convert to grayscale 
#        img_l = rgb_img.sum(axis=2, dtype=np.uint32) / 3
#        raw_images.append(img_l)
#        coadd += img_l
#        count += 1
#        if count == coadd_frames:
#            coadd_images.append(coadd)
#            coadd = np.zeros(shape)
#            count = 0


coadd = np.zeros(shape)
count = 0
coadd_frames = 30
def update(frame):
    global coadd
    global count
    global coadd_frames
    ok, rgb_img = vid.read()
    print(frame)
    img_l = rgb_img.sum(axis=2, dtype=np.uint32)
    #raw_images.append(img_l)
    coadd += img_l
    count += 1
    #ax.imshow(coadd_images[frame], cmap='gray')
    #plt.draw()
    raw_img.set_data(img_l)
    raw_img.autoscale()
    if count == coadd_frames:
        coadd_img.set_data(coadd)
        coadd_img.autoscale()
        coadd = np.zeros(shape)
        count = 0
    #plt.draw()
    return raw_img, coadd_img

ani = FuncAnimation(fig, update, frames=300*fps, interval=(1000/fps))

plt.show()

