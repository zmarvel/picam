#!/usr/bin/env python
# coding: utf-8


import argparse as ap
from math import pi, sqrt, exp

import numpy as np
from PIL import Image
import matplotlib.pyplot as plt
#import skimage.feature as skfeature
import scipy.signal as spsignal

parser = ap.ArgumentParser()
parser.add_argument('input_file', nargs='?', default='./out/469_bright_region.png')
parser.add_argument('-c', '--color-map', default='gray')

args = parser.parse_args()

unprocessed_rgb = np.array(Image.open(args.input_file))

unprocessed_rgb = np.delete(unprocessed_rgb, 3, 2) 
unprocessed = unprocessed_rgb.sum(axis=2, dtype=np.float)

ax = plt.subplot(1, 2, 1)
ax.imshow(unprocessed, cmap=args.color_map)

mu, sigma = (0, 5)
var = sigma**2

kernel_size = 5

xs = np.arange(-kernel_size // 2, kernel_size // 2)

ys = 1 / sqrt(2 * pi * var) * np.exp(-(xs - mu)**2 / (2*var))

kernel = np.zeros((kernel_size, kernel_size))
kernel[:,kernel_size // 2] = ys
kernel[kernel_size // 2,:] = ys

#for x in xs:
#    print((1 / sqrt(2 * pi * var)) * exp(-(x - mu)**2 / (2 * var)))
blurred = spsignal.convolve2d(unprocessed, kernel)
ax = plt.subplot(1, 2, 2)
ax.imshow(blurred, cmap=args.color_map)

plt.show()
