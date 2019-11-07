
import argparse as ap
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
from PIL import Image

fps = 10

parser = ap.ArgumentParser()
parser.add_argument('input_files', type=str, nargs='*')
#parser.add_argument('output_file', type=str)
args = parser.parse_args()

width = 3280
height = 2464
shape = (height, width)

fig = plt.figure(constrained_layout=True)
gs = fig.add_gridspec(1, 2)

raw_ax = fig.add_subplot(gs[0, 0])
raw_img = raw_ax.imshow(np.zeros(shape, dtype=np.uint32), cmap='gray')
coadd_ax = fig.add_subplot(gs[0, 1])
coadd_img = coadd_ax.imshow(np.zeros(shape, dtype=np.uint32), cmap='gray')

coadd = np.zeros(shape, dtype=np.uint32)
count = 0
coadd_frames = 5
files = args.input_files
i = 0
def update(frame):
    global coadd
    global count
    global coadd_frames
    global i

    print(frame)

    rgb_img = np.array(Image.open(files[i]).convert('RGB'))
    print(rgb_img.shape)
    i += 1

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

