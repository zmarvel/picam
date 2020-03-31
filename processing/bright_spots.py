#!/usr/bin/env python
# coding: utf-8
from math import pi
import numpy as np
from PIL import Image
import matplotlib.pyplot as plt
import scipy.signal as spsig

# +

# %matplotlib tk
# -

# # Load the image(s)
#
# We'll add a couple of images together in an attempt to filter out camera noise.
#
# TODO: Base thresholds on number of added images instead of hard-coding the values (or figure out what the thresholds should be through a histogram).

# unprocessed_rgb = np.array(Image.open('./out/469_smaller.png'))
unprocessed_rgb = (np.array(Image.open('./out/469.png'))
                   + np.array(Image.open('./out/470.png'))
                  )
unprocessed_rgb = np.delete(unprocessed_rgb, 3, 2)
unprocessed = unprocessed_rgb.sum(axis=2, dtype=np.float)


# # Calculate magnitude of edges
#
# Calculate the difference of each pixel with its neighbors. Then take the absolute value of the sum of these differences. This way we get large values for pixels with dramatic differences in all directions, whether positive or negative.
#
# TODO: perhaps it's best not to use absolute value *after* the sum--a star is probably larger than one pixel.

# +
n_kernel = [
    [0, -1, 0],
    [0, 1, 0],
    [0, 0, 0],
]

e_kernel = [
    [0, 0, 0],
    [0, 1, -1],
    [0, 0, 0],
]

s_kernel = [
    [0, 0, 0],
    [0, 1, 0],
    [0, -1, 0],
]

w_kernel = [
    [0, 0, 0],
    [-1, 1, 0],
    [0, 0, 0],
]
# -

n_delta = spsig.convolve2d(unprocessed, n_kernel)
e_delta = spsig.convolve2d(unprocessed, e_kernel)
w_delta = spsig.convolve2d(unprocessed, w_kernel)
s_delta = spsig.convolve2d(unprocessed, s_kernel)

# +

plt.figure()
plt.imshow(unprocessed)
plt.title('Unprocessed')
plt.show()
# -

sum_delta = np.absolute(n_delta + e_delta + w_delta + s_delta)
sum_delta

plt.figure()
plt.imshow(sum_delta)
plt.title('Absolute value of sum of neighbor delta')
plt.show()

# ## Threshold the sum of difference

# +
upper_threshold = 1000
lower_threshold = 100

thresholded = np.copy(sum_delta)
thresholded[(sum_delta < lower_threshold) | (sum_delta > upper_threshold)] = 0
# -

plt.figure()
plt.set_cmap('magma')
plt.imshow(thresholded)
plt.title('Thresholded sum of delta')
plt.show()

# +
# upper_threshold = 600
# lower_threshold = 100

# thresholded = np.copy(unprocessed)
# thresholded[(unprocessed < lower_threshold) | (unprocessed > upper_threshold)] = 0

# +
# plt.figure()
# plt.set_cmap('gist_ncar')
# plt.imshow(thresholded)
# plt.show()
# -

# # Blur the image
#
# This helps us detect bright regions.
#
# NOTE: It's pretty expensive to compute the blurred image with a large kernel. I bet scikit-image has a built-in gaussian blur that would be faster.

# +
kernel_size = 100

def distance(x1, x2):
    return np.sqrt(x1*x1 + x2*x2)

xs = np.zeros((kernel_size, kernel_size))
for row in range(kernel_size):
    for col in range(kernel_size):
        xs[row][col] = distance(kernel_size // 2 - row, kernel_size // 2 - col)


# +
def gaussian(mu, sigma, xs):
    return (1 / np.sqrt(2 * pi * sigma**2)) * np.exp(-(1/2) * ((xs - mu) / (2*sigma))**2)

kernel = gaussian(0, 2, xs)
# -

plt.figure()
plt.imshow(kernel)
plt.show()

# %%time
# blurred = spsig.convolve2d(unprocessed, kernel)
# blurred = spsig.convolve2d(thresholded, kernel)
blurred = spsig.convolve2d(sum_delta, kernel)


tw, th = thresholded.shape
bw, bh = blurred.shape
dw = bw - tw
dh = bh - th
blurred_cropped = blurred[dh//2:-dh//2,dw//2:-dw//2]

plt.figure()
plt.imshow(blurred_cropped)
plt.title('Blurred (density)')
plt.show()

# ## Threshold the sum of differences again based on the differences
#
# We attempt to exclude bright regions this way, but it doesn't work perfectly. What can we do to make it work better? Perhaps increase sigma.

# +
upper = 2000
lower = 500

blurred_thresholded = np.copy(thresholded)
blurred_thresholded[(blurred_cropped > lower) & (blurred_cropped < upper)] = 0
# -

plt.figure()
plt.imshow(blurred_thresholded)
plt.title('Thresholded by blurred (density) image')
plt.show()


# # Identify connected components
#
# Use a simple DFS to create a matrix where each non-zero cell identifies which connected component a pixel belongs to.

# +

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

#         for dr, dc in zip(range(-2, 2), range(-2, 2)):
        for dr, dc in NEIGHBORS:
            r, c = (row+dr, col+dc)
            if is_in_bounds(mat, r, c) and out[r][c] == 0 and mat[r][c] > 0:
                q.append((r, c))


# -

# flood stores a color number, so different regions are filled with different
# colors
flood = np.zeros(blurred_thresholded.shape)

# +
fill_color = 0

for r in range(blurred_thresholded.shape[0]):
    for c in range(blurred_thresholded.shape[1]):
        # If the cell is above the threshold and has not already been seen,
        # run flood_fill on it
        if blurred_thresholded[r][c] > 0 and flood[r][c] == 0:
            flood_fill(fill_color, blurred_thresholded, flood, (r, c))
            fill_color += 1
# -

plt.set_cmap('Set1')
plt.imshow(flood)
print(f'{fill_color} regions')

# Now let's look at region size and filter too-small and too-large regions.

