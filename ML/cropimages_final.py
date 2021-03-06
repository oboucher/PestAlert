# -*- coding: utf-8 -*-
"""cropimages.ipynb

Automatically generated by Colaboratory.

Original file is located at
    https://colab.research.google.com/drive/1XNMTNHEpksxbHAWPQkTWQVHCKNto8Qlk
"""

import cv2
import imutils
import numpy as np

image = cv2.imread('/content/fivebugs.png') # path = path to your file
#cv2_imshow(image)

image_count = 0
bin = cv2.inRange(image, (0, 0, 0), (100, 100, 100))
cv2.bitwise_not(bin, bin)
cnts = cv2.findContours(bin.copy(), cv2.RETR_TREE, cv2.CHAIN_APPROX_SIMPLE)
cnts = imutils.grab_contours(cnts)
cnts = sorted(cnts, key = cv2.contourArea, reverse = True)
image2 = image.copy()

while (1):
  rect = cv2.boundingRect(cnts[image_count])
  x,y,w,h = rect
  #print(rect)
  if (rect[3] <= 20):
    break
  
  if (w <= 50 or h <=50):
      x -= 15
      y -= 15
      w += 50
      h += 50
  cv2.rectangle(image2, (x,y,w,h), (0,255,0), 1)
  #cv2_imshow(image2)

  ROI = image[y:y+h, x:x+w]
  cv2.imwrite("ROI_{}.png".format(image_count), ROI)
  image_count+=1
