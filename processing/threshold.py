#!/usr/bin/env python
# coding: utf-8


import argparse as ap

import numpy as np
from PIL import Image
import scipy.signal as spsignal
import matplotlib.pyplot as plt

parser = ap.ArgumentParser()
parser.add_argument('input_file', nargs='?', default='./out/469_bright_region.png')
parser.add_argument('-c', '--color-map', default='gray')

args = parser.parse_args()

unprocessed_rgb = np.array(Image.open(args.input_file))

unprocessed_rgb = np.delete(unprocessed_rgb, 3, 2) 
unprocessed = unprocessed_rgb.sum(axis=2, dtype=np.float)

ax1 = plt.subplot(1, 2, 1)
ax1.imshow(unprocessed, cmap=args.color_map)


thresh_hi = 200

filtered = np.ma.array(unprocessed, mask=unprocessed > thresh_hi).filled(0)


ax2 = plt.subplot(1, 2, 2)
ax2.imshow(filtered, cmap=args.color_map)

plt.show()
