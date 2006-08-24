
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

databuffer.o:
	ghdl -a --ieee=synopsys  databuffer.vhd

expandblock.o:
	ghdl -a --ieee=synopsys  expandblock.vhd

reconframes.o:
	ghdl -a --ieee=synopsys  reconframes.vhd

copyrecon.o:
	ghdl -a --ieee=synopsys  copyrecon.vhd

lflimits.o:
	ghdl -a --ieee=synopsys  lflimits.vhd

loopfilter.o:
	ghdl -a --ieee=synopsys  loopfilter.vhd

UpdateUMV.o:
	ghdl -a --ieee=synopsys  UpdateUMV.vhd

reconrefframes.o: reconrefframes.vhd
	ghdl -a --ieee=synopsys  reconrefframes.vhd

tb_reconrefframes.o: tb_reconrefframes.vhd
	ghdl -a --ieee=synopsys  tb_reconrefframes.vhd

elaborate: clamp.o reconpixelindex.o dual_syncram.o idctslow.o syncram.o databuffer.o expandblock.o reconframes.o copyrecon.o lflimits.o loopfilter.o UpdateUMV.o reconrefframes.o tb_reconrefframes.o
	ghdl -e  --ieee=synopsys tb_reconrefframes


run: tb_reconrefframes
	@echo "Running... wait some minutes"
	@echo "Remember to do: make compare"
	ghdl -r tb_reconrefframes

compare:
	@if diff OUT.DUV golden/OUT.EXPECTED.TB ; then \
	  echo -e "\033[32;1m     ***       Simulation PASSED       *** \033[0m"; \
        else \
	  echo -e "\033[31;1m!!!!!!!!!   Simulation FAILED   !!!!!!!!!\033[0m"; \
        fi


runvcd: elaborate
	@echo "Running... wait some minutes"
	ghdl -r tb_reconrefframes  --vcd=dump.vcd

	@echo "VCD File wrote: dump.vcd"

clean:
	$(RM) OUT.DUV *.o *~ tb_reconrefframes *.cf \#*
	@echo "done"