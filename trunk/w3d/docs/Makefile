RM = rm -f

TARGET = main.ps

$(TARGET):
	$(RM) $(TARGET).gz
	latex $(TARGET:.ps=.tex)
	latex $(TARGET:.ps=.tex)
	dvips $(TARGET:.ps=.dvi) -o $(TARGET)
	gzip $(TARGET)

clean:
	$(RM) $(TARGET) $(TARGET).gz $(TARGET:.ps=.dvi) *.log *.aux
