
all: clean elaborate
	@echo "PLEASE, DO: make run"

semaphore.o:
	ghdl -a --ieee=synopsys  semaphore.vhd

reconpixelindex.o:
	ghdl -a --ieee=synopsys  reconpixelindex.vhd

lflimits.o:
	ghdl -a --ieee=synopsys  lflimits.vhd

syncram.o:
	ghdl -a --ieee=synopsys  syncram.vhd

loopfilter.o: loopfilter.vhd
	ghdl -a --ieee=synopsys  loopfilter.vhd

tb_loopfilter.o: tb_loopfilter.vhd
	ghdl -a --ieee=synopsys  tb_loopfilter.vhd

elaborate: reconpixelindex.o lflimits.o syncram.o semaphore.o loopfilter.o tb_loopfilter.o
#elaborate: reconpixelindex.o lflimits.o syncram.o semaphore.o tb_loopfilter.o
#elaborate: reconpixelindex.o lflimits.o syncram.o loopfilter.o tb_loopfilter.o
	ghdl -e  --ieee=synopsys tb_loopfilter


run: tb_loopfilter
	@echo "Running... wait some minutes"
	@echo "Remember to do: make compare"
	ghdl -r tb_loopfilter

run2: tb_loopfilter
	@echo "Running... wait some minutes"
	@echo "Remember to do: make compare"
	ghdl -r tb_loopfilter --assert-level=warning


compare:
	@if diff OUT.DUV golden/OUT.EXPECTED.TB ; then \
	  echo -e "\033[32;1m     ***       Simulation PASSED       *** \033[0m"; \
        else \
	  echo -e "\033[31;1m!!!!!!!!!   Simulation FAILED   !!!!!!!!!\033[0m"; \
        fi


runvcd: elaborate
	@echo "Running... wait some minutes"
	ghdl -r tb_loopfilter  --vcd=dump.vcd

	@echo "VCD File wrote: dump.vcd"

runghw: elaborate
	@echo "Running... wait some minutes"
	ghdl -r tb_loopfilter  --wave=dump.ghw

	@echo "GHW File wrote: dump.ghw"


clean:
	$(RM) OUT.DUV *.o *~ tb_loopfilter *.cf
	@echo "done"