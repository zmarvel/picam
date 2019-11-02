
import argparse as ap
import cv2
import numpy as np
import matplotlib.pyplot as plt

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

#ax.set_xlim(0, cols)
#ax.set_ylim(0, rows)

out_image = np.zeros(shape)
ok = True
while ok:
    ok, rgb_img = vid.read()
    if ok:
        # Convert to grayscale 
        img_l = rgb_img.sum(axis=2, dtype=np.float)
        out_image += img_l


print(out_image.min(), out_image.mean(), out_image.std(), out_image.max())
img = plt.imshow(out_image, cmap='gray')
img.autoscale()

plt.show()

