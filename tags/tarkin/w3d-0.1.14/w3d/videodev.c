#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#include "videodev.h"


#define  LOG(x...)   fprintf(stderr, x)
#define  ELOG(x...)  fprintf(stderr, "ERROR: " x)



VideoDev* video_device_new (char *dev_file)
{
   VideoDev *dev = (VideoDev*) malloc (sizeof (VideoDev));

   dev->fd = open (dev_file, O_RDONLY);

   if (dev->fd < 0) {
      ELOG ("error opening '%s'\n", dev_file);
      free (dev);
      return NULL;
   }

   if (ioctl (dev->fd, VIDIOCGCAP, &dev->cap) < 0 ||
       ioctl (dev->fd, VIDIOCGWIN, &dev->win) < 0 ||
       ioctl (dev->fd, VIDIOCGPICT, &dev->pic) < 0)
   {
      ELOG ("ioctl (VIDIOCGCAP, VIDIOCGWIN, VIDIOCGPICT) failed.\n");
      ELOG ("('%s' not a videodevice ?)\n", dev_file);
      close (dev->fd);
      free (dev);
      return NULL;
   }

   LOG ("%s: cap.name == %s\n", __FUNCTION__, dev->cap.name);
   LOG ("%s: cap.type == %i\n", __FUNCTION__, dev->cap.type);
   LOG ("%s: cap.minwidth == %i\n", __FUNCTION__, dev->cap.minwidth);
   LOG ("%s: cap.maxwidth == %i\n", __FUNCTION__, dev->cap.maxwidth);
   LOG ("%s: cap.minheight == %i\n", __FUNCTION__, dev->cap.minheight);
   LOG ("%s: cap.maxheight == %i\n", __FUNCTION__, dev->cap.maxheight);

   dev->win.width  = dev->cap.maxwidth;
   dev->win.height = dev->cap.maxheight;

   if (ioctl (dev->fd, VIDIOCSWIN, & dev->win) < 0)
   {
      ELOG ("ioctl (VIDIOCSWIN) size request failed.\n");
      close (dev->fd);
      free (dev);
      return NULL;
   }

   return dev;
}



int video_device_try_palette (VideoDev *dev, unsigned short palette)
{
   unsigned short old_pal = dev->pic.palette;

   dev->pic.palette = palette;

   switch (palette) {
      case VIDEO_PALETTE_GREY:    dev->pic.depth = 8; break;
      case VIDEO_PALETTE_YUYV:
      case VIDEO_PALETTE_UYVY:
      case VIDEO_PALETTE_YUV422:  dev->pic.depth = 16; break;
      case VIDEO_PALETTE_RGB32:   dev->pic.depth = 32; break;
      case VIDEO_PALETTE_RGB24:   dev->pic.depth = 24; break;
      default:
         ELOG ("%s: unsupported palette.\n", __FUNCTION__);
   };

   if (ioctl (dev->fd, VIDIOCSPICT, & dev->pic) < 0 ||
       ioctl (dev->fd, VIDIOCGPICT, & dev->pic) < 0 ||
       dev->pic.palette != palette)
   {
      ELOG ("%s: failed setting palette %u @ depth %u; restore old mode...\n",
            __FUNCTION__, dev->pic.palette, dev->pic.depth);
      dev->pic.palette = old_pal;
      ioctl (dev->fd, VIDIOCSPICT, & dev->pic); 
      return -1;
   }

   return 0;
}



int video_device_grab_frame (VideoDev *dev, void *buf)
{
   return read (dev->fd, buf, dev->win.width * dev->win.height * dev->pic.depth / 8);
}



void video_device_destroy (VideoDev *dev)
{
   close (dev->fd);
   free (dev);
}


