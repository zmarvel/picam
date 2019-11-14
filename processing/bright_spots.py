#!/usr/bin/env python
# coding: utf-8


from math import pi

import numpy as np
from PIL import Image
import matplotlib.pyplot as plt
import scipy.signal as spsig


unprocessed_rgb = np.array(Image.open('./out/469_smaller.png'))
unprocessed_rgb = np.delete(unprocessed_rgb, 3, 2)
unprocessed = unprocessed_rgb.sum(axis=2, dtype=np.float)


#img = plt.imshow(unprocessed, cmap='gray')
#plt.show()


# Blur the image to detect bright regions

def distance(x1, x2):
    return np.sqrt(x1*x1 + x2*x2)


def gaussian(mu, sigma, xs):
    return (1 / np.sqrt(2 * pi * sigma**2)) * np.exp(-(xs - mu)**2 / 2*sigma**2)

kernel_size = 19

xs = np.zeros((kernel_size, kernel_size))
for row in range(kernel_size):
    for col in range(kernel_size):
        xs[row][col] = distance(kernel_size // 2 - row, kernel_size // 2 - col)

kernel = gaussian(0, 0.5, xs)

#plt.imshow(kernel)
#plt.show()

ax = plt.subplot(1, 2, 1)
ax.imshow(unprocessed)

blurred = spsig.convolve2d(unprocessed, kernel)
ax = plt.subplot(1, 2, 2)
ax.imshow(blurred)
plt.show()


# Flood fill

# flood stores a color number, so different regions are filled with different
# colors
flood = np.zeros(edges.shape)

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

        for dr, dc in zip(range(-2, 2), range(-2, 2)):
        #for dr, dc in NEIGHBORS:
            r, c = (row+dr, col+dc)
            if is_in_bounds(mat, r, c) and out[r][c] == 0 and mat[r][c] > 0:
                q.append((r, c))

flood_test = np.array([
    [0, 0, 0, 0, 0, 0, 0, 0],
    [0, 0, 0, 0, 0, 0, 0, 0],
    [0, 0, 1, 1, 0, 0, 0, 0],
    [0, 0, 1, 1, 0, 0, 0, 0],
    [0, 0, 0, 0, 0, 0, 0, 0],
    [0, 0, 0, 0, 0, 0, 0, 0],
    [0, 0, 0, 0, 0, 0, 0, 0],
    [0, 0, 0, 0, 0, 0, 0, 0],
])

flood_test_out = np.zeros((8, 8))

#flood_fill(1, flood_test, flood_test_out, (2, 2))
#print(flood_test_out)

#fill_color = np.array([0xff, 0, 0, 0xff])
#flooded = np.zeros((*flood.shape, 4))
fill_color = 1

for r in range(edges_filled.shape[0]):
    for c in range(edges_filled.shape[1]):
        # If the cell is above the threshold and has not already been seen,
        # run flood_fill on it
        if not edges.mask[r][c] and flood[r][c] == 0:
            flood_fill(fill_color, edges_filled, flood, (r, c))
            fill_color += 1

print(f'Filled {fill_color} distinct regions')

#flood_fill(1, edges_filled, flood, (264, 269))






plt.imshow(flood, cmap='gist_ncar')

plt.show()
