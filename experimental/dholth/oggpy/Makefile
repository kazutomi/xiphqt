PYTHON_HEADER_DIR=/usr/include/python2.3
BOOST_HEADER_DIR=/home/dholth/opt/include/boost-1_31

all:
	./setup.py build

#multiple: oggpy.pyste vorbispy.pyste theorapy.pyste
#	pyste --multiple --module=oggpy --out=oggpy -I. -I$(PYTHON_HEADER_DIR) -I$(BOOST_HEADER_DIR) -I$(HOME)/include/theora \
#	oggpy.pyste vorbispy.pyste theorapy.pyste

oggpy.cpp: oggpy_wrappers.h oggcc.cc oggcc.h oggpy.pyste
	pyste --module=oggpy --out=oggpy.cpp -I. -I$(PYTHON_HEADER_DIR) -I$(BOOST_HEADER_DIR) oggpy.pyste

vorbispy.cpp: vorbispy_wrappers.h oggcc.h vorbiscc.h vorbispy.pyste
	pyste --module=vorbispy --out=vorbispy.cpp -I. -I$(PYTHON_HEADER_DIR) -I$(BOOST_HEADER_DIR) vorbispy.pyste

theorapy.cpp: oggcc.cc oggcc.h theoracc.h theorapy.pyste
	pyste --module=theorapy --out=theorapy.cpp -I. \
	-I$(PYTHON_HEADER_DIR) -I$(BOOST_HEADER_DIR) -I/usr/include/  theorapy.pyste

flacpy.cpp: flacpy.pyste
	pyste --module=flacpy --out=flacpy.cpp -I. \
	-I$(PYTHON_HEADER_DIR) -I$(BOOST_HEADER_DIR) -I/usr/include/ $<

module: theorapy.cpp oggpy.cpp vorbispy.cpp flacpy.cpp
	python ./setup.py build

bridge:
	python ./setup.py build

webcode: tests/*.py

.py .html:
	code2html $< $@

doc: oggcc.h vorbiscc.h theoracc.h
	doxygen
