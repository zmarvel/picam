#!/usr/bin/env python
# coding: utf-8

# In[11]:


import numpy as np
from PIL import Image

unprocessed_rgb = np.array(Image.open('./out/469_bright_region.png'))


# In[12]:


unprocessed_rgb = np.delete(unprocessed_rgb, 3, 2) 
unprocessed = unprocessed_rgb.sum(axis=2, dtype=np.float)


# In[38]:


center = unprocessed[1:-1,1:-1]
left = center - unprocessed[1:-1,0:-2]
left[left < 0] = 0

right = center - unprocessed[1:-1,2:]
right[right < 0] = 0

up = center - unprocessed[0:-2,1:-1]
up[up < 0] = 0

down = center - unprocessed[2:,1:-1]
down[down < 0] = 0


# In[39]:


edges = left + right + up + down
#edges.set_fill_value(0)


# In[41]:


import matplotlib.pyplot as plt

#plt.subplots(figsize=(20,20))
img = plt.imshow(np.ma.array(edges, mask=edges < 75).filled(0), cmap='gray')

plt.show()



# Flood fill



# In[ ]:




