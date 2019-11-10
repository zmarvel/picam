#!/usr/bin/env python
# coding: utf-8


import argparse as ap

import numpy as np
from PIL import Image
import matplotlib.pyplot as plt

parser = ap.ArgumentParser()
parser.add_argument('input_file', nargs='?', default='./out/469_bright_region.png')
parser.add_argument('-c', '--color-map', default='gray')

args = parser.parse_args()

unprocessed_rgb = np.array(Image.open(args.input_file))

unprocessed_rgb = np.delete(unprocessed_rgb, 3, 2) 
unprocessed = unprocessed_rgb.sum(axis=2, dtype=np.float)

plt.imshow(unprocessed, cmap=args.color_map)

plt.show()
