
all: clean elaborate
	@echo "PLEASE, DO: make run"

clamp.o:
	ghdl -a --ieee=synopsys  clamp.vhd

reconpixelindex.o:
	ghdl -a --ieee=synopsys  reconpixelindex.vhd

dual_syncram.o:
	ghdl -a --ieee=synopsys  dual_syncram.vhd

idctslow.o:
	ghdl -a --ieee=synopsys  idctslow.vhd

syncram.o:
	ghdl -a --ieee=synopsys  syncram.vhd

semaphore.o:
	ghdl -a --ieee=synopsys  semaphore.vhd

expandblock.o: expandblock.vhd
	ghdl -a --ieee=synopsys  expandblock.vhd

tb_expandblock.o: tb_expandblock.vhd
	ghdl -a --ieee=synopsys  tb_expandblock.vhd

elaborate: clamp.o reconpixelindex.o dual_syncram.o idctslow.o syncram.o semaphore.o expandblock.o tb_expandblock.o
	ghdl -e  --ieee=synopsys tb_expandblock


run: tb_expandblock
	@echo "Running... wait some minutes"
	@echo "Remember to do: make compare"
	ghdl -r tb_expandblock

compare:
	@if diff OUT.DUV golden/OUT.EXPECTED.TB ; then \
	  echo -e "\033[32;1m     ***       Simulation PASSED       *** \033[0m"; \
        else \
	  echo -e "\033[31;1m!!!!!!!!!   Simulation FAILED   !!!!!!!!!\033[0m"; \
        fi


runvcd: elaborate
	@echo "Running... wait some minutes"
	ghdl -r tb_expandblock  --vcd=dump.vcd

	@echo "VCD File wrote: dump.vcd"

clean:
	$(RM) OUT.DUV *.o *~ tb_expandblock *.cf \#*
	@echo "done"