# Fuck Automake
# Fuck the horse it rode in on
# and Fuck its little dog Libtool too

TARGET  = gPlanarity
CC      = gcc
LD      = gcc
export INSTALL = install
PREFIX  = /usr/local
BINDIR  = $(PREFIX)/bin
ETCDIR  = /etc/$(TARGET)
MANDIR  = $(PREFIX)/man

#DISABLE_NLS = true
# White space delimited list of locales you want to support. Defaults to all
# available ones.
#export WANT_LINGUAS := cs

# All subdirs with Makefiles

SUBDIRS = 

# By default, enable NLS
ifeq ($(origin DISABLE_NLS), undefined)
export GT_DOMAIN = $(TARGET)
export GT_DIR = $(PREFIX)/share/locale
ADD_DEF += -DUGT_DOMAIN="$(GT_DOMAIN)" -DUGT_DIR="$(GT_DIR)" -DENABLE_NLS
SUBDIRS += po
endif

SRC  = dialog_finish.c gameboard_logic.c dialog_pause.c gameboard_logic_button.c\
	gameboard.c gameboard_logic_buttonbar.c gameboard_draw_box.c\
	gameboard_logic_mouse.c gameboard_draw_button.c gameboard_logic_push.c\
	gameboard_draw_buttonbar.c graph.c gameboard_draw_curtain.c\
	graph_score.c graph_arrange.c gameboard_draw_edge.c graph_generate.c\
	gameboard_draw_intersection.c graph_generate_mesh1.c gameboard_draw_main.c\
	gameboard_draw_score.c main.c gameboard_draw_selection.c\
	timer.c gameboard_draw_vertex.c levelstate.c dialog_level.c\
	dialog_level_icons.c gameboard_draw_text.c random.c graph_generate_data.c\
	gameboard_logic_fade.c graph_generate_mesh2.c graph_region.c
OBJ  = dialog_finish.o gameboard_logic.o dialog_pause.o gameboard_logic_button.o\
	gameboard.o gameboard_logic_buttonbar.o gameboard_draw_box.o\
	gameboard_logic_mouse.o gameboard_draw_button.o gameboard_logic_push.o\
	gameboard_draw_buttonbar.o graph.o gameboard_draw_curtain.o\
	graph_score.o graph_arrange.o gameboard_draw_edge.o graph_generate.o\
	gameboard_draw_intersection.o graph_generate_mesh1.o gameboard_draw_main.o\
	gameboard_draw_score.o main.o gameboard_draw_selection.o\
	timer.o gameboard_draw_vertex.o levelstate.o dialog_level.o\
	dialog_level_icons.o gameboard_draw_text.o random.o graph_generate_data.o\
	gameboard_logic_fade.o graph_generate_mesh2.o graph_region.o
CAIROVER =  >= 1.0.0
GTKVER   =  >= 2.7.2
GCF  = `pkg-config --cflags "gtk+-2.0 $(GTKVER) cairo $(CAIROVER) freetype2"`
LDF  = `pkg-config --libs "gtk+-2.0 $(GTKVER) cairo $(CAIROVER) freetype2"`

all: all-local all-recursive

all-local:    
	pkg-config --cflags "gtk+-2.0 $(GTKVER) cairo $(CAIROVER) freetype2" 1>/dev/null
	$(MAKE) target CFLAGS='-O2 -ffast-math $(GCF) $(ADD_DEF)'

all-recursive:
	for D in $(SUBDIRS); do make -C $$D || exit 1; done

debug:
	pkg-config --cflags "gtk+-2.0 $(GTKVER) cairo $(CAIROVER) freetype2" 1>/dev/null
	$(MAKE) target CFLAGS='-g -Wall -W -Wno-unused-parameter -D__NO_MATH_INLINES $(GCF) $(ADD_DEF) $(CLFAGS)'

profile:
	pkg-config --cflags "gtk+-2.0 $(GTKVER) cairo $(CAIROVER) freetype2" 1>/dev/null
	$(MAKE) target CFLAGS='-pg -g -O2 -ffast-math $(GCF) $(ADD_DEF) $(CFLAGS)'

clean: clean-local clean-recursive

clean-local:
	rm -f $(OBJ) *.d *.d.* gmon.out $(TARGET)

clean-recursive:
	for D in $(SUBDIRS); do make -C $$D clean || exit 1; done


distclean: clean
	rm -f *~

%.d: %.c
	$(CC) -M $(CFLAGS) $< > $@.$$$$; sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; rm -f $@.$$$$

ifeq ($(MAKECMDGOALS),target)
include $(SRC:.c=.d)
endif

target:  $(OBJ) 
	./touch-version
	$(LD) $(OBJ) $(CFLAGS) -o $(TARGET) $(LIBS) -lm $(LDF)

install: install-local install-recursive

install-local: target
	$(INSTALL) -d -m 0755 $(BINDIR)
	$(INSTALL) -m 0755 $(TARGET) $(BINDIR)

install-recursive:
	for D in $(SUBDIRS); do make -C $$D install || exit 1; done
