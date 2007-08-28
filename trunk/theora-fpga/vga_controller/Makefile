
all: clean elaborate
	@echo "PLEASE, DO: make run"

clamp.o:
	ghdl -a --ieee=synopsys  clamp.vhd

divider.o:
	ghdl -a --ieee=synopsys  divider.vhd

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

interface_vga.o: interface_vga.vhd
	ghdl -a --ieee=synopsys  interface_vga.vhd

YCbCr2RGB.o: YCbCr2RGB.vhd
	ghdl -a --ieee=synopsys  YCbCr2RGB.vhd

reconrefframes.o: reconrefframes.vhd
	ghdl -a --ieee=synopsys  reconrefframes.vhd

theora_hardware.o: theora_hardware.vhd
	ghdl -a --ieee=synopsys  theora_hardware.vhd

tb_toplevel.o: tb_toplevel.vhd
	ghdl -a --ieee=synopsys  tb_toplevel.vhd

elaborate: clamp.o divider.o reconpixelindex.o dual_syncram.o idctslow.o syncram.o databuffer.o expandblock.o reconframes.o copyrecon.o lflimits.o loopfilter.o UpdateUMV.o interface_vga.o YCbCr2RGB.o reconrefframes.o theora_hardware.o tb_toplevel.o
	ghdl -e  --ieee=synopsys tb_toplevel


run: tb_toplevel
	@echo "Running... wait some minutes"
	@echo "Remember to do: make compare"
	ghdl -r tb_toplevel

compare:
	@if diff OUT.DUV golden/OUT.EXPECTED.TB ; then \
	  echo -e "\033[32;1m     ***       Simulation PASSED       *** \033[0m"; \
        else \
	  echo -e "\033[31;1m!!!!!!!!!   Simulation FAILED   !!!!!!!!!\033[0m"; \
        fi


runvcd: elaborate
	@echo "Running... wait some minutes"
	ghdl -r tb_toplevel  --vcd=dump.vcd

	@echo "VCD File wrote: dump.vcd"

runwave: elaborate
	@echo "Running... wait some minutes"
	ghdl -r tb_toplevel  --wave=dump.ghw

	@echo "GHW File wrote: dump.ghw"


clean:
	$(RM) OUT.DUV *.o *~ tb_toplevel *.cf \#*
	@echo "done"
