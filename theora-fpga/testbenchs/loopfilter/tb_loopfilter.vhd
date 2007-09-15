library std;
library ieee;

use ieee.std_logic_1164.all;
--use ieee.std_logic_arith.all;
--use ieee.std_logic_unsigned.all;
--use ieee.std_logic_signed.all;
use ieee.numeric_std.all;
--use ieee.std_logic_textio.all;
use std.textio.all;


entity tb_LoopFilter is
  
end tb_LoopFilter;


architecture behavior of tb_LoopFilter is

  constant clk_period : time := 100 ns;  -- Clock period
  constant delta : time := clk_period / 4;

  file DataInFile1   : text open read_mode is "golden/in.tb";  -- Input file
  file DataInFile2   : text open read_mode is "golden/in2.tb"; -- Input file
  file OutFile       : text open write_mode is "OUT.DUV";  -- Output file

  file stdOUTPUT: TEXT open WRITE_MODE is "STD_OUTPUT";
  
--  file FullFile      : text open write_mode is "quadratura.full.output";  -- Full output


  signal end_of_file : boolean;  	-- End of File indicator
  signal end_of_file2 : boolean;  	-- End of File indicator
  signal stop_simulation : boolean;  	-- End of File indicator
  
  signal clk	     : std_logic := '0';
  signal resetn      : std_logic;

  signal out_lf_request : std_logic;
  signal out_lf_valid : std_logic := '0';
  signal out_lf_data : signed(31 downto 0);

  signal out_sem_request : std_logic;
  signal out_sem_valid : std_logic := '0';
  signal out_sem_data : signed(31 downto 0);

  signal in_sem_requested : std_logic := '0';
  signal in_sem_valid : std_logic;
  signal in_sem_data : signed(31 downto 0);

  signal buffer_wr_e : std_logic;

  signal in_lf_sem_requested  : std_logic;
  signal in_lf_sem_valid      : std_logic;
  signal in_lf_sem_addr       : unsigned(19 downto 0);
  signal in_lf_sem_data       : signed(31 downto 0);

  signal out_lf_sem_request   : std_logic;
  signal out_lf_sem_valid     : std_logic;
  signal out_lf_sem_addr      : unsigned(19 downto 0);
  signal out_lf_sem_data      : signed(31 downto 0);

  signal count : integer;

  signal lf_enable : std_logic;
  signal sem_enable : std_logic;

  constant OFFSET1 : unsigned(19 downto 0) := "0000" & x"0000";
begin  -- behavior

  semaphore0: entity work.semaphore
    port map(clk, resetn, sem_enable,
             out_sem_request, out_sem_valid, out_sem_data,
             in_sem_requested, in_sem_valid, in_sem_data,
             buffer_wr_e,

             out_lf_sem_request, out_lf_sem_valid, out_lf_sem_addr, out_lf_sem_data,

             in_lf_sem_requested, in_lf_sem_valid, in_lf_sem_addr, in_lf_sem_data
             );

  loopfilter0: entity work.loopfilter
    port map(clk, resetn, lf_enable,
             out_lf_request, out_lf_valid, out_lf_data,

             in_lf_sem_requested, in_lf_sem_valid, in_lf_sem_addr, in_lf_sem_data,

             out_lf_sem_request, out_lf_sem_valid, out_lf_sem_addr, out_lf_sem_data,

             buffer_wr_e
             );
  
  clk <= not clk after clk_period / 2;
  resetn <= '0', '1' after 7 * clk_period;

    
  Input : process(clk, resetn)

    variable input_line	  : line;
    variable aux : integer;

  begin  -- process ReadInput
    
    if (resetn = '0') then
      stop_simulation <= false;
      end_of_file <= false;
      end_of_file2 <= false;
      lf_enable <= '0';
      sem_enable <= '0';
      
    elsif clk'EVENT and clk = '1' then

      if (out_lf_sem_request = '0') then
        lf_enable <= '0';
        sem_enable <= '1';
        if ( EndFile(DataInFile2)) then
          end_of_file2 <= true;
        else
          if (out_sem_request = '1') then
            ReadLine( DataInFile2, input_line);
            Read( input_line, aux );
            out_sem_data <= to_signed(aux,32) after delta;
            out_sem_valid <= '1' after delta;
          end if;
        end if;
      else
        lf_enable <= '1';
        sem_enable <= '0';
        if ( EndFile(DataInFile1)) then
          end_of_file <= true;
        else
          if (out_lf_request = '1') then
            ReadLine( DataInFile1, input_line);
            Read( input_line, aux );
            out_lf_data <= to_signed(aux,32) after delta;
            out_lf_valid <= '1' after delta;
          end if;          
        end if;
      end if;
    end if;
  end process Input;


  Output : process(clk, resetn)
    variable output_line	  : line;

  begin  -- process ReadInput
    if (resetn = '0') then
    elsif clk'EVENT and clk = '1' then
      in_sem_requested <= '1' after delta;
      if( in_sem_requested = '1' and in_sem_valid = '1' )then
        --Write(output_line, now, left, 15);
        Write(output_line, to_integer(in_sem_data));
        WriteLine(OutFile, output_line);
      end if;
    end if;

  end process Output;


  assert not end_of_file report "End of Simulation: THIS IS NOT AN ERROR! "&
    "THIS IS JUST A WAY TO STOP THE SIMULATION." severity failure;


end behavior;
