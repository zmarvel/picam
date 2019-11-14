#!/usr/bin/env python
# coding: utf-8


import argparse as ap
from math import pi, sqrt, exp
import itertools as it

import numpy as np
from PIL import Image
import matplotlib.pyplot as plt
#import skimage.feature as skfeature
import scipy.signal as spsignal

parser = ap.ArgumentParser()
parser.add_argument('input_file', nargs='?', default='./out/469_smaller_cropped.png')
parser.add_argument('-c', '--color-map', default='gray')
parser.add_argument('-o', '--output-file')

args = parser.parse_args()

unprocessed_rgb = np.array(Image.open(args.input_file))

if unprocessed_rgb.shape[2] == 2:
    # Grayscale image with alpha
    unprocessed = np.delete(unprocessed_rgb, 1, 2).squeeze()
elif unprocessed_rgb.shape[2] == 4:
    # RGBA image
    unprocessed_rgb = np.delete(unprocessed_rgb, 3, 2) 
    unprocessed = unprocessed_rgb.sum(axis=2, dtype=np.float)

threshold_lo = 50
unprocessed = np.ma.array(unprocessed, mask=unprocessed < threshold_lo).filled(0)


def is_in_bounds(mat, row, col):
    return row > 0 and row < mat.shape[0] - 1 and col > 0 and col < mat.shape[1] - 1

NEIGHBORS = (
    (-1, 0),
    (1, 0),
    (0, -1),
    (0, 1),
    (-1, -1),
    (-1, 1),
    (1, 1),
    (1, -1),
)
def flood_fill(fill_color, mat, out, start):
    q = [start]
    while len(q) > 0:
        row, col = q.pop()
        if mat[row][col] > 0:
            out[row][col] = fill_color

        for dr, dc in it.combinations(range(-2, 2), 2):
        #for dr, dc in NEIGHBORS:
            r, c = (row+dr, col+dc)
            if is_in_bounds(mat, r, c) and out[r][c] == 0 and mat[r][c] > 0:
                q.append((r, c))

shape = unprocessed.shape
flood = np.zeros(shape)
fill_color = 1
for r in range(shape[0]):
    for c in range(shape[1]):
        if unprocessed[r][c] > 0 and flood[r][c] == 0:
            flood_fill(fill_color, unprocessed, flood, (r, c))
            fill_color += 1

print(f'Identified {fill_color} regions')

ax = plt.subplot(1, 1, 1)
ax.imshow(flood, cmap='gist_ncar')

#blurred = spsignal.convolve2d(unprocessed, kernel)
#ax = plt.subplot(1, 2, 2)
#ax.imshow(blurred, cmap=args.color_map)

if args.output_file:
    plt.savefig(args.output_file, dpi=800)
else:
    plt.show()
