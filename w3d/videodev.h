#ifndef __VIDEODEV_H
#define __VIDEODEV_H

#include <stdlib.h>
#include <sys/ioctl.h>
#include <linux/videodev.h>


typedef struct {
   int                      fd;
   struct video_capability  cap;
   struct video_window      win;
   struct video_picture     pic;
} VideoDev;


extern VideoDev* video_device_new (char *dev_file);
extern int video_device_try_palette (VideoDev *dev, unsigned short palette);
extern int video_device_grab_frame (VideoDev *dev, void *buf);
extern void video_device_destroy (VideoDev *dev);


#endif

