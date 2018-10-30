#!/usr/bin/env python3

'''
USAGE: track-DH5.py [<video_source>]
'''

# 1 - toggle HSV flow visualization
# 2 - toggle velocity show / save
# ESC - exit

# Python 2.7.13 and OpenCV 3.4.0
# mods by J.Beale

from __future__ import print_function

import numpy as np
import cv2 as cv
import imutils  # http://www.pyimagesearch.com/2015/02/02/
import time
from datetime import datetime
import os       # to flush buffer to disk
import math     # math.floor()

# ==============================================================

CNAME = 'DH5'
UTYPE = "rtsp://"
UPASS = "user:password"
IPADD = "192.168.1.45"
PORT = "554"
URL2 = "/cam/realmonitor?channel=1&subtype=2"  # Dahua IP Cameras

# parameters tuned assuming stream at 15 fps
FDRATE=1          # frame decimation rate: only display 1 in this many frames
XFWIDE = 176       # width of resized frame
vThreshold = 1100   # how many non-zero velocity pixels for event
saveCount = 0      # how many images saved so far
vThresh= 15        # minimum "significant" velocity (was 30)
runLimit=5         # how many frames of no motion ends a run
validRunLength= 7  # how many frames of motion for good event
show_hsv = False
show_vt = True    # display velocity threshold map
logFname = 'Log_' + CNAME + '.txt'
fPrefix= CNAME + '_'
tPrefix= 'thumbs/th_' + CNAME
thumbWidth= 120   # max pixels wide for thumb
thumbAspect = 1.333;  # maximum aspect ratio for thumbnail
moT = 1.1         # Calc. dx/dt motion threshold for saving event
vidname= UTYPE + UPASS + "@" + IPADD + ":" + PORT + URL2
# for Dahua: "/cam/realmonitor?channel=1&subtype=2"
# ================================================================

def draw_flow(img, flow, step=10):
    h, w = img.shape[:2]
    y, x = np.mgrid[step/2:h:step, step/2:w:step].reshape(2,-1).astype(int)
    fx, fy = flow[y,x].T
    lines = np.vstack([x, y, x+fx, y+fy]).T.reshape(-1, 2, 2)
    lines = np.int32(lines + 0.5)
    vis = cv.cvtColor(img, cv.COLOR_GRAY2BGR)
    cv.polylines(vis, lines, 0, (0, 255, 0))
    for (x1, y1), (_x2, _y2) in lines:
        cv.circle(vis, (x1, y1), 1, (0, 255, 0), -1)
    return vis

 
def calc_v(flow):
    h, w = flow.shape[:2]
    fx, fy = flow[:,:,0], flow[:,:,1]
    #vs = (fx*fx+fy*fy)
    vs = (fx*fx)    
    # gf = np.zeros((h, w), np.uint8)
    gray = np.minimum(vs*64, 255) # gray.shape = (99, 176), float32
    retval, grf = cv.threshold(gray, vThresh, 255, 0)
    gr = grf.astype(np.uint8)
    gr = cv.dilate(gr, None, iterations=2)
    gr = cv.erode(gr, None, iterations=4)  # was 8
    gr = cv.dilate(gr, None, iterations=7) # was 5
    return gr

def draw_hsv(flow):
    h, w = flow.shape[:2]
    fx, fy = flow[:,:,0], flow[:,:,1]
    ang = np.arctan2(fy, fx) + np.pi
    v = np.sqrt(fx*fx+fy*fy)
    hsv = np.zeros((h, w, 3), np.uint8)
    hsv[...,0] = ang*(180/np.pi/2)
    hsv[...,1] = 255
    #hsv[...,2] = np.minimum(v*4, 255)
    hsv[...,2] = np.minimum(v*64, 255)
    bgr = cv.cvtColor(hsv, cv.COLOR_HSV2BGR)
    return bgr

if __name__ == '__main__':
    import sys
    print(__doc__)
    try:
        fn = sys.argv[1]
    except IndexError:
        fn = 0

    cam = cv.VideoCapture(vidname)  # open a video file or stream

    logf = open(logFname, 'a')
    tnow = time.strftime("%Y-%m-%d %H:%M:%S", time.localtime())
    print("%s Start: %s" % (CNAME,tnow))
    s = "%s Start: %s\n" % (CNAME,tnow)
    logf.write(s)

    # cam = video.create_capture(fn)
    ret, prevRaw = cam.read()
    prev = imutils.resize(prevRaw, width=XFWIDE)
    bestImg = prevRaw
    tBest = time.localtime()

    w = int(cam.get(3))  # input image width
    h = int(cam.get(4))  # input image height
    fs = (1.0 * w) / XFWIDE # frame scaling factor
    print ("Image w,h = %d %d  Scale=%f" % (w, h, fs))


    motionNow = False  # if we have current motion
    mRun = 0           # how many motion frames in a row
    frameCount = 1     # total number of frames read
    lastFC = 0         # previous motion frame count
    prevgray = cv.cvtColor(prev, cv.COLOR_BGR2GRAY)
    cur_glitch = prev.copy()
    bsize=30           # buffer size, max elems in one run
    xpos=[]            # list of x positions empty
    xdelt=[]           # list of delta-x values
    s=[]               # list of strings starts empty
    rdist=[]           # radial distance to center frame
    xcent = XFWIDE*0.6           # X ideal position
    ycent = (XFWIDE*0.562)*0.4   # Y ideal position
    minR = 1000        # force it high
    maxY = 0           # maximum Y motion center of an event
    while True:
        #ret, img = cam.read()
        ret, imgRaw = cam.read()
        img = imutils.resize(imgRaw, width=XFWIDE)
        #w,h = cv.GetSize(img)  # width and height of image
        #xcent = w/2
        #ycent = h/2

        frameCount += 1
        gray = cv.cvtColor(img, cv.COLOR_BGR2GRAY)
        flow = cv.calcOpticalFlowFarneback(prevgray, gray, None, 0.5, 3, 15, 3, 7, 1.5, 0)
        prevgray = gray
        vt = calc_v(flow)  # returns threshold map (mask) but uint8
        #print (vt.dtype)
        #print (vt.shape)
        vt0 = vt.copy()
        mCount = cv.countNonZero(vt)
        fx = flow[:,:,0]
        deltaFC = frameCount - lastFC # 1 or 2 during an event
        if (mCount > vThreshold): # significant motion detected this frame
          motionNow = True
          im2,contours, hierarchy = cv.findContours(vt0, cv.RETR_EXTERNAL, cv.CHAIN_APPROX_SIMPLE)
          #im2, contours, hierarchy = cv.findContours(vt, cv.RETR_EXTERNAL, cv.CHAIN_APPROX_SIMPLE)
          contours = sorted(contours, key=cv.contourArea, reverse=True)[:2]
          # cv.drawContours(img, contours, -1, (0,255,0), 2)  # draw contours on image
          cnt = contours[0]  # select the largest contour
          M = cv.moments(cnt)
          Area = M['m00']       # area of contour
          cx = int(M['m10']/Area)
          cy = int(M['m01']/Area)
          dcx = (cx - xcent)
          dcy = (cy - ycent)
          r = np.sqrt(dcx*dcx + dcy*dcy)  # distance to center frame
          evR = cv.boundingRect(cnt)  # rect. around motion event
          y2=int(evR[1]+evR[3])   # y2 = y1 + height
          ttnow = time.localtime()
          # tnow = datetime.now().strftime('%Y-%m-%d %H:%M:%S.%f')[:-3] # time to msec
          # datetime.utcnow().strftime('%Y-%m-%d %H:%M:%S.%f')[:-3]
          tnow = datetime.now().strftime("%Y-%m-%d %H:%M:%S.%f")[:-3] # time to msec
          if (y2 > maxY):
              maxY = y2     # save largest Y value in this run
          if (r < minR):
              minR = r
              bestImg = imgRaw
              tBest = ttnow
              tsBest = tnow
              br = evR # rectangle around main contour
          #cx=0;
          fxSum,a,b,c = cv.sumElems(fx)
          fxAvg = fxSum / mCount
          xpos.append(cx)
          rdist.append(r)
          if (mRun > 0):
            xdelt.append(xpos[mRun]-xpos[mRun-1]) # remember each delta
          if (mRun >= validRunLength):
            v1 = np.mean(xdelt[0:3])
            v2 = np.mean(xdelt[mRun-4:mRun-1])
            if np.sign(v1) != np.sign(v2):
              deltaFC = runLimit+1 # force end of this run
              mCount = vThreshold - 1
          s.append("%s ,%d,%02d,%04d, %03d, %02d, %04d, %03d,%02d, %02d\n" % 
            (tnow,deltaFC,saveCount,mCount,int(fxAvg*10),
             mRun,int(Area),cx,cy, int(r)) )
          print(s[mRun],end='')  # without extra newline
          # logf.write(s)  # save in logfile
          lastFC = frameCount  # most recent frame with motion
          mRun += 1
          
        if (mCount <= vThreshold):
          if (deltaFC > runLimit):  # not currently in a motion event
            if (motionNow):  # was it active until just now??
              if mRun > 6:
                dxpos=[]
                for x in range(1, mRun):
                  dxpos.append(xpos[x]-xpos[x-1])
                xstd = np.std(dxpos)
                xavg = np.mean(dxpos)
                # dm1: average motion divided into 2 parts
                dm1 = (np.mean(xpos[mRun//2:mRun])
                   - np.mean(xpos[0:mRun//2])) / (0.5*mRun)
                mid2 = mRun//3  # find 1/3 and 2/3 points
                mid3 = mid2*2
                mn1 = np.mean(xpos[0:mid2])  # divided in 3 parts
                mn2 = np.mean(xpos[mid2:mid3])
                mn3 = np.mean(xpos[mid3:mRun])
                dm2 = (mn2-mn1)*3.0/mRun
                dm3 = (mn3-mn2)*3.0/mRun
                s1 = np.sign(dm1)  # real motion should be all same sign
                s2 = np.sign(dm2)
                s3 = np.sign(dm3)
                if (s1==s2 and s1==s3 and abs(dm1) > moT):
                  for x in s:
                    logf.write(x)                  
                  # dts = time.strftime("%Y-%m-%d %H:%M:%S", tBest) # time & date string
                  # dts = time.strftime("%Y-%m-%d %H:%M:%S.%f", tBest)[:-3] # date+time to msec
                  buf = "# " + tsBest 
                  logf.write(buf)
                  print(buf,end='')
                  buf = (" , %d, %.2f,%.2f,%.2f, %3.2f, %03d\n" % 
                     (mRun, dm2, dm1, dm3, xstd, maxY))
                  logf.write(buf)
                  print(buf,end='')  # without extra newline
                  #print(buf)
                  logf.flush()  # after event, actually write the buffered output to disk
                  os.fsync(logf.fileno())      
                  dt = time.strftime("%y%m%d_%H%M%S", tBest)
                  fname3 = fPrefix + dt + ".jpg"  # full-size image
                  fname4 = tPrefix + dt + ".png"  # thumbnail image
                  # fname4 = tPrefix + dt + ".jpg"

                  x1=int(br[0]*fs)
                  y1=int(br[1]*fs)
                  x2=int((br[0]+br[2])*fs)
                  y2=int((br[1]+br[3])*fs)
                  # imutils.resize will not constrain both width and height
                  # so if input is too tall, need to reduce target width
                  dYin = (y2-y1)
                  dXin = (x2-x1)
                  Aspect = (dXin / dYin)
                  if (Aspect > thumbAspect):
                    thumbWidthEff = thumbWidth
                  else:
                    thumbWidthEff = math.floor(0.5 + (thumbWidth * (Aspect / thumbAspect)))

                  print("%d,%d A=%5.3f width:%d" % (dXin, dYin, Aspect, thumbWidthEff))
                  thumbImg= imutils.resize(bestImg[y1:y2,x1:x2], 
                     width=thumbWidthEff)
                  cv.imwrite(fname4, thumbImg ) # active region
                  #cv.rectangle(bestImg,(x1,y1),(x2,y2),(0,255,0),1)  # draw rectangle on img
                  cv.imwrite(fname3, bestImg) # save best image

            motionNow = False
            mRun = 0  # count of consecutive motion frames
            s=[]      # empty out list
            xpos=[]
            xdelt=[]
            rdist=[]
            minR = 1000        # force it high
            maxY = 0           # force it low
            
        if (motionNow):
          if (mRun > 4) and ((mRun-5) % 3) == 0:
            saveCount += 1
            #fname1 = "imag%05d.jpg" % saveCount
            #fname2 = "mask%05d.jpg" % saveCount            
            #cv.imwrite(fname1, imgRaw) # frame as JPEG    
            #cv.imwrite(fname2, vt) # save mask
        
        if(frameCount % FDRATE) == 0:
          cv.imshow('Video_'+CNAME, img)  # raw image
          # cv.imshow('flow', draw_flow(gray, flow))
          if show_hsv:
            cv.imshow('HSV'+CNAME, draw_hsv(flow))
          if show_vt:
            cv.imshow('M_'+CNAME, vt)
          ch = cv.waitKey(1)
          
          #if ch == 27:
          #  break
          #if ch == ord('1'):
          #  show_hsv = not show_hsv
          #  print('HSV flow visualization is', ['off', 'on'][show_hsv])
          #if ch == ord('2'):
          #  show_vt = not show_vt
          #  print('VelThresh is', ['off', 'on'][show_vt])
            
    logf.flush()  # after event, actually write the buffered output to disk
    #os.fsync()            
    logf.close()            
    cv.destroyAllWindows()
