/* Bridge from theorapy to pygame                                            */
/*                                                                           */
/* For questions regarding this program contact                              */
/* Daniel Holth <dholth@fastmail.fm>                                         */

/**
 * Render theora video to a pygame surface
 * 
 * @author Daniel Holth
 */

extern "C" {
#include <pygame/pygame.h>
}

#include <boost/python/detail/wrap_python.hpp>
#include <boost/python.hpp>
#include "theoracc.h"

using namespace boost::python;

class pygame_bridge {
    private:
        /// References to the Python objects:
        object info;
        object surface;

        theora_info *ti;

        SDL_Surface *surf;
        SDL_Overlay *yuv_overlay;
        SDL_Rect rect;

        int frame_width, frame_height;

    protected:
        /// Make sure overlay is the correct size
        int check_overlay(ogg::theora::info &info);

    public:
        pygame_bridge() { this->ti = NULL; this->surf = NULL; this->yuv_overlay = NULL; }
        
        int set_info(ogg::theora::info &info)
        {
            // this->info = info;   // not a python object
            this->ti = info.get_data();
            return this->check_overlay(info);
        }

        int is_surface(PyObject *surf) { return PySurface_Check(surf); }

        /// Make surface for testing.
        PyObject *make_surface()
        {
            SDL_Surface *surf = NULL;
            PyObject    *pysurf = NULL;

            surf = SDL_CreateRGBSurface(SDL_SWSURFACE, 128, 128, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);

            if(!surf)
            {
                SDL_FreeSurface(surf);
            }
            else
            {
                pysurf = PySurface_New(surf);
            }


            return pysurf;
        }

        int set_surface(boost::python::object surface);
        int render(ogg::theora::yuv_image &image);
};


/* This function is based on code from OggTheora's player_example.c  *
 * THE Theora SOURCE CODE IS COPYRIGHT (C) 2002-2003                 *
 * by the Xiph.Org Foundation http://www.xiph.org/                   */
static void video_write(yuv_buffer &yuv, theora_info &ti, SDL_Surface *screen, SDL_Overlay *yuv_overlay, SDL_Rect *rect) 
{
    int i;
    int crop_offset;

    /* Lock SDL_yuv_overlay */
    if ( SDL_MUSTLOCK(screen) ) {
        if ( SDL_LockSurface(screen) < 0 ) return;
    }
    if (SDL_LockYUVOverlay(yuv_overlay) < 0) return;

    /* let's draw the data (*yuv[3]) on a SDL screen (*screen) */
    /* deal with border stride */
    /* reverse u and v for SDL */
    /* and crop input properly, respecting the encoded frame rect */
    crop_offset=ti.offset_x+yuv.y_stride*ti.offset_y;
    for(i=0;i<yuv_overlay->h;i++)
        memcpy(yuv_overlay->pixels[0]+yuv_overlay->pitches[0]*i,
               yuv.y+crop_offset+yuv.y_stride*i,
               yuv_overlay->w);
    crop_offset=(ti.offset_x/2)+(yuv.uv_stride)*(ti.offset_y/2);
    for(i=0;i<yuv_overlay->h/2;i++){
        memcpy(yuv_overlay->pixels[1]+yuv_overlay->pitches[1]*i,
               yuv.v+crop_offset+yuv.uv_stride*i,
               yuv_overlay->w/2);
        memcpy(yuv_overlay->pixels[2]+yuv_overlay->pitches[2]*i,
               yuv.u+crop_offset+yuv.uv_stride*i,
               yuv_overlay->w/2);
    }

    /* Unlock SDL_yuv_overlay */
    if ( SDL_MUSTLOCK(screen) ) {
        SDL_UnlockSurface(screen);
    }
    SDL_UnlockYUVOverlay(yuv_overlay);


    /* Show, baby, show! */
    SDL_DisplayYUVOverlay(yuv_overlay, rect);
}


int pygame_bridge::check_overlay(ogg::theora::info &image)
{
    theora_info *ti = image.get_data();

    if(this->yuv_overlay == NULL)
    {
        this->yuv_overlay = SDL_CreateYUVOverlay(ti->frame_width, ti->frame_height, SDL_YV12_OVERLAY, this->surf);

        if(this->yuv_overlay != NULL)
        {
            rect.x = 0;
            rect.y = 0;
            rect.w = ti->frame_width;
            rect.h = ti->frame_height;

            SDL_DisplayYUVOverlay(this->yuv_overlay, &(this->rect));
        }
    }

    return (this->yuv_overlay == NULL) ? 0 : 1;
}


int pygame_bridge::render(ogg::theora::yuv_image &image)
{    
    video_write(*(image.get_data()), *(this->ti), this->surf, this->yuv_overlay, &(this->rect));
    return 1;
}


int pygame_bridge::set_surface(object display)
// int pygame_bridge::set_surface(PyObject *surfobj)
{
    PyObject* surfobj;
    surfobj = display.ptr();

    if(PySurface_Check(surfobj))
    {
        // this->surface = display;
        this->surf = PySurface_AsSurface(surfobj);
        return 1;
    }

    return 0;
}


BOOST_PYTHON_MODULE_INIT(pygame_bridge)
{

    class_< pygame_bridge > ("bridge", "Connect theorapy's yuv_image to pygame")
        .def("set_info", &pygame_bridge::set_info, "Set theora info to use (do this first")
        .def("set_surface", &pygame_bridge::set_surface, "Set SDL surface for image display")
        .def("render", &pygame_bridge::render, "Blit a frame to the SDL surface")
        .def("is_surface", &pygame_bridge::is_surface, "Do we think it's a SDL surface?")
        .def("make_surface", &pygame_bridge::make_surface, "Create something that's definiely a surface.")
        ;
    
    // Must import needed APIs:
    import_pygame_base();
    import_pygame_rect();
    import_pygame_surface();
}
