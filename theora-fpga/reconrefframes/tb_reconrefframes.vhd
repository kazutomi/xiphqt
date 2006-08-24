library std;
library ieee;

use ieee.std_logic_1164.all;
--use ieee.std_logic_arith.all;
--use ieee.std_logic_unsigned.all;
--use ieee.std_logic_signed.all;
use ieee.numeric_std.all;
--use ieee.std_logic_textio.all;
use std.textio.all;


entity tb_ReconRefFrames is
  
end tb_ReconRefFrames;


architecture behavior of tb_ReconRefFrames is

  constant clk_period : time := 100 ns;  -- Clock period
  constant delta : time := clk_period / 4;

  
  file DataInFile     : text open read_mode is "golden/in.tb";  -- Input file
  file OutFile       : text open write_mode is "OUT.DUV";  -- Output file
--  file FullFile      : text open write_mode is "quadratura.full.output";  -- Full output

  signal end_of_file : boolean;  	-- End of File indicator

  signal clk	     : std_logic := '0';
  signal resetn      : std_logic;

  signal in_requested : std_logic;
  signal in_valid : std_logic := '0';
  signal in_data : signed(31 downto 0);

  signal out_request : std_logic := '0';
  signal out_valid : std_logic;
  signal out_data : signed(31 downto 0);

begin  -- behavior

  ReconRefFrame0: entity work.reconrefframes
    port map(clk, resetn, in_requested, in_valid, in_data,
             out_request, out_valid, out_data);


  clk <= not clk after clk_period / 2;
  resetn <= '0', '1' after 7 * clk_period;

  
  Input : process(clk, resetn)

    variable input_line	  : line;
    variable aux : integer;
  begin  -- process ReadInput
  
    if (resetn = '0') then
      end_of_file <= false;
    elsif clk'EVENT and clk = '1' then
      if ( EndFile(DataInFile) ) then
	end_of_file <= true;
      else

        if( in_requested = '1' )then

          
          ReadLine( DataInFile, input_line);
          Read( input_line, aux );
--          assert false report "testbench = "&integer'image(aux) severity note;
          in_data <= to_signed(aux,32) after delta;
          in_valid <= '1' after delta;
        end if;
      end if;
    end if;

  end process Input;





  Output : process(clk, resetn)

    variable output_line	  : line;
  begin  -- process ReadInput
    if (resetn = '0') then
    elsif clk'EVENT and clk = '1' then
      out_request <= '1' after delta;
      if( out_request = '1' and out_valid = '1' )then
        --Write(output_line, now, left, 15);
	Write(output_line, to_integer(out_data));
	WriteLine(OutFile, output_line);
      end if;
    end if;

  end process Output;


  assert not end_of_file report "End of Simulation: THIS IS NOT AN ERROR! "&
    "THIS IS JUST A WAY TO STOP THE SIMULATION." severity failure;


end behavior;
