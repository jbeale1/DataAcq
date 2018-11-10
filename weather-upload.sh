#!/bin/bash

# get latest still image, crop, rescale, and
# upload JPEG to Wunderground webcam page
# Can be run every 5 min from crontab with:
#  */5 * * * * /dev/shm/weather-upload.sh

# Below two commands should go in /etc/rc.local for startup
#    copy weather-camera upload script to ramdisk
# cp /home/pi/wcam-upload.sh /dev/shm/wcam-upload.sh
#    start up PiKrellCam to run the camera
# su pi -c '(sleep 5; /usr/local/bin/pikrellcam) &'



# Original PKC image size with this cam is 3280x2464 
XS="3150"  # cropped X size before rescale
YS="2464"  # cropped Y size before rescale

WDIR="/dev/shm"                          # ramdisk working dir
DIR="/home/pi/pikrellcam/media/stills"   # get image from here
FIFO="/home/pi/pikrellcam/www/FIFO"      # control camera here

HOST="webcam.wunderground.com"           # upload to WU here
USER="my_username"                       # my assigned WU username
PASSWD="my_password"                     # my assigned WU password

INFILE="latest.jpg"                      # local temp filename
OUTFILE="image.jpg"                      # required filename for WU

# send command into PiKrellCam FIFO to capture a still image right now:
echo "still" > $FIFO

# wait for PKC command to be executed and file to be written to disk
sleep 4

# find most recent file in PKC stills directory
unset -v latest
for file in "$DIR"/*.jpg; do
  [[ $file -nt $latest ]] && latest=$file
done

# copy latest image to working directory
cp $file $INFILE

# crop off right-hand edge, rescale to 25% size, and force size to be within 150 kB
convert $WDIR/$INFILE -crop "$XS"x"$YS"+0+0 -resize 25% -define jpeg:extent=150000 $WDIR/$OUTFILE

# send image to remote host via FTP
ftp -n -v $HOST << EOT
user $USER $PASSWD
prompt
binary
put $OUTFILE 
bye
EOT

exit 0
