# Fuck Automake
# Fuck the horse it rode in on
# and Fuck its little dog Libtool too

TARGET  = gPlanarity
CC      = gcc 
LD      = gcc
INSTALL = install
STRIP   = strip
PREFIX  = /usr/local
BINDIR  = $(PREFIX)/bin
ETCDIR  = /etc/$(TARGET)
MANDIR  = $(PREFIX)/man

# the very long LIBS entry is to build an executable that will
# work for on most modern linux distros without any of the
# bleeding-edge GTK 2.7/2.8 libs installed.

SRC  = dialog_finish.c gameboard_logic.c dialog_pause.c gameboard_logic_button.c\
	gameboard.c gameboard_logic_buttonbar.c gameboard_draw_box.c\
	gameboard_logic_mouse.c gameboard_draw_button.c gameboard_logic_push.c\
	gameboard_draw_buttonbar.c graph.c gameboard_draw_curtain.c\
	graph_score.c graph_arrange.c gameboard_draw_edge.c graph_generate.c\
	gameboard_draw_intersection.c graph_generate_mesh1.c gameboard_draw_main.c\
	gameboard_draw_score.c main.c gameboard_draw_selection.c\
	timer.c gameboard_draw_vertex.c levelstate.c dialog_level.c\
	dialog_level_icons.c gameboard_draw_text.c random.c graph_generate_data.c\
	gameboard_logic_fade.c
OBJ  = dialog_finish.o gameboard_logic.o dialog_pause.o gameboard_logic_button.o\
	gameboard.o gameboard_logic_buttonbar.o gameboard_draw_box.o\
	gameboard_logic_mouse.o gameboard_draw_button.o gameboard_logic_push.o\
	gameboard_draw_buttonbar.o graph.o gameboard_draw_curtain.o\
	graph_score.o graph_arrange.o gameboard_draw_edge.o graph_generate.o\
	gameboard_draw_intersection.o graph_generate_mesh1.o gameboard_draw_main.o\
	gameboard_draw_score.o main.o gameboard_draw_selection.o\
	timer.o gameboard_draw_vertex.o levelstate.o dialog_level.o\
	dialog_level_icons.o gameboard_draw_text.o random.o graph_generate_data.o\
	gameboard_logic_fade.o
GCF  = `pkg-config --static --cflags "gtk+-2.0 >= 2.7.2"`

# uncomment below for a normal dynamic build

LDF  = `pkg-config --static --libs "gtk+-2.0 >= 2.7.2"`

# uncomment below for a build with static Gtk 2.7+; this generally
# requires that all of the Gtk dependencies were built with static
# dependencies also pointing to the local install

SLDF  = `pkg-config --static --libs-only-L "gtk+-2.0 >= 2.7.2"`
GTK   = /usr/local/lib
EXPAT= /usr/lib/libexpat.a
XINERAMA = /usr/X11R6/lib/libXinerama.a
PNG = /usr/lib/libpng.a
PIXMAN = /usr/lib/libpixman.a
SLIBS = $(GTK)/libgtk-x11-2.0.a \
	$(GTK)/libgdk-x11-2.0.a $(GTK)/libgdk_pixbuf-2.0.a \
	$(GTK)/libpangocairo-1.0.a $(GTK)/libpangoft2-1.0.a \
	$(GTK)/libpangox-1.0.a $(GTK)/libpangoxft-1.0.a \
	$(GTK)/libpango-1.0.a $(GTK)/libcairo.a \
	$(GTK)/libatk-1.0.a $(GTK)/libgmodule-2.0.a \
	$(GTK)/libgobject-2.0.a $(GTK)/libglib-2.0.a \
	-lpthread $(EXPAT) -lfreetype -lz -lm -lc \
	-lXext -lXrandr $(XINERAMA) -lXcursor -lX11 $(PIXMAN)\
	-lXft $(PNG)

all:    
	$(MAKE) target CFLAGS="-O2 -ffast-math $(GCF) $(ADD_DEF)"
	$(STRIP) $(TARGET)

static:    
	$(MAKE) static-target CFLAGS="-O2 -ffast-math $(GCF) $(ADD_DEF) "	
	$(STRIP) $(TARGET)

debug:
	$(MAKE) target CFLAGS="-g -Wall -W -Wno-unused-parameter -D__NO_MATH_INLINES $(GCF) $(ADD_DEF)"

profile:
	$(MAKE) target CFLAGS="-pg -g -O2 -ffast-math $(GCF) $(ADD_DEF)" LIBS="$(LIBS) -lgprof-helper "

clean:
	rm -f $(OBJ) *.d *.d.* gmon.out $(TARGET)

distclean: clean
	rm -f *~

%.d: %.c
	$(CC) -M $(CFLAGS) $< > $@.$$$$; sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; rm -f $@.$$$$

ifeq ($(MAKECMDGOALS),target)
include $(SRC:.c=.d)
endif

ifeq ($(MAKECMDGOALS),static-target)
include $(SRC:.c=.d)
endif

target:  $(OBJ) 
	./touch-version
	$(LD) $(OBJ) $(CFLAGS) -o $(TARGET) $(LIBS) $(LDF)

static-target: $(OBJ)
	./touch-version
	$(LD) $(OBJ) $(CFLAGS) -o $(TARGET) $(SLIBS) $(SLDF) 

install: target
	$(INSTALL) -d -m 0755 $(BINDIR)
	$(INSTALL) -m 0755 $(TARGET) $(BINDIR)
