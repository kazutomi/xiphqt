library std;
library ieee;

use ieee.std_logic_1164.all;
--use ieee.std_logic_arith.all;
--use ieee.std_logic_unsigned.all;
--use ieee.std_logic_signed.all;
use ieee.numeric_std.all;
--use ieee.std_logic_textio.all;
use std.textio.all;


entity tb_IDctSlow is
  
end tb_IDctSlow;


architecture behavior of tb_IDctSlow is

  constant clk_period : time := 100 ns;  -- Clock period
  constant delta : time := clk_period / 4;

  file DataInFile        : text open read_mode is "golden/IN.TB";  -- Input file
  file QuantMatInFile        : text open read_mode is "golden/IN_QUANT.TB";  -- Input file
  file OutFile       : text open write_mode is "OUT.DUV";  -- Output file
--  file FullFile      : text open write_mode is "quadratura.full.output";  -- Full output

  signal end_of_file : boolean;  	-- End of File indicator

  signal clk	     : std_logic := '0';
  signal resetn      : std_logic;

  signal out_requested : std_logic;
  signal out_valid : std_logic := '0';
  signal out_data : signed(15 downto 0);
  signal out_quantmat : signed(15 downto 0);

  signal in_request : std_logic := '0';
  signal in_valid : std_logic;
  signal in_data : signed(15 downto 0);


  signal samples : integer := 0;
  signal clock_cycles : integer := 0;
  
begin  -- behavior

  idctslow0: entity work.IDctSlow
    port map(clk, resetn, out_requested, out_valid, out_data, out_quantmat,
             in_request, in_valid, in_data );


  clk <= not clk after clk_period / 2;
  resetn <= '0', '1' after 7 * clk_period;

  
  Input : process(clk, resetn)

    variable input_line	  : line;
    variable aux : integer;
  begin  -- process ReadInput
    if (resetn = '0') then
      end_of_file <= false;

    elsif clk'EVENT and clk = '1' then      
      if ( EndFile(DataInFile) or EndFile(QuantMatInFile) ) then
	end_of_file <= true;
        report "Latency = "&integer'image( clock_cycles/samples )&
          " clock cycles per data sample." severity note;
      else

        if( out_requested = '1' )then
          ReadLine( DataInFile, input_line);
          Read( input_line, aux );
          out_data <= to_signed(aux,16) after delta;
        
          ReadLine( QuantMatInFile, input_line);
          Read( input_line, aux );
          out_quantmat <= to_signed(aux,16) after delta;
          out_valid <= '1' after delta;
  
        end if;
      end if;
    end if;

  end process Input;





  Output : process(clk, resetn)

    variable output_line	  : line;
  begin  -- process ReadInput
    if (resetn = '0') then
    elsif clk'EVENT and clk = '1' then
      clock_cycles <= clock_cycles + 1;

      in_request <= '1' after delta;
      if( in_request = '1' and in_valid = '1' )then
        --Write(output_line, now, left, 15);
	Write(output_line, to_integer(in_data));
	WriteLine(OutFile, output_line);
        samples <= samples + 1;
      end if;
    end if;

  end process Output;


  assert not end_of_file report "End of Simulation: THIS IS NOT AN ERROR! "&
    "THIS IS JUST A WAY TO STOP THE SIMULATION." severity failure;


end behavior;
