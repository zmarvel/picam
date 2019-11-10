#!/usr/bin/env python
# coding: utf-8


import argparse as ap

import numpy as np
from PIL import Image
import scipy.signal as spsignal
import matplotlib.pyplot as plt

parser = ap.ArgumentParser()
parser.add_argument('input_file', nargs='?', default='./out/469_bright_region.png')

args = parser.parse_args()

unprocessed_rgb = np.array(Image.open(args.input_file))

unprocessed_rgb = np.delete(unprocessed_rgb, 3, 2) 
unprocessed = unprocessed_rgb.sum(axis=2, dtype=np.float)

filtered = spsignal.medfilt2d(unprocessed)

ax1 = plt.subplot(1, 2, 1)
ax1.imshow(unprocessed, cmap='gray')

ax2 = plt.subplot(1, 2, 2)
ax2.imshow(filtered, cmap='gray')

plt.show()
