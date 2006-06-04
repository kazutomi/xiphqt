
all: tb_idctslow
	@echo "PLEASE, DO: make run"


tb_idctslow: idctslow.vhd tb_idctslow.vhd
	ghdl -a --ieee=synopsys  idctslow.vhd
	ghdl -a --ieee=synopsys  tb_idctslow.vhd
	ghdl -e  --ieee=synopsys tb_idctslow


run: tb_idctslow
	@echo "Running... wait some minutes"
	@echo "Remember to do: make compare"
	ghdl -r tb_idctslow

compare:
	@if diff OUT.DUV golden/OUT.EXPECTED.TB ; then \
	  echo -e "\033[32;1m     ***       Simulation PASSED       *** \033[0m"; \
        else \
	  echo -e "\033[31;1m!!!!!!!!!   Simulation FAILED   !!!!!!!!!\033[0m"; \
        fi


runvcd: elaborate
	@echo "Running... wait some minutes"
	ghdl -r tb_idctslow  --vcd=dump.vcd

	@echo "VCD File wrote: dump.vcd"
clean:
	rm OUT.DUV
	rm *.o
	rm tb_idctslow
	@echo "done"