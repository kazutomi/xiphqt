library std;
library ieee;

use ieee.std_logic_1164.all;
--use ieee.std_logic_arith.all;
--use ieee.std_logic_unsigned.all;
--use ieee.std_logic_signed.all;
use ieee.numeric_std.all;
--use ieee.std_logic_textio.all;
use std.textio.all;


entity tb_toplevel is
  
end tb_toplevel;


architecture behavior of tb_toplevel is

  constant clk_period : time := 100 ns;  -- Clock period
  constant delta : time := clk_period / 4;

  
  file DataInFile     : text open read_mode is "golden/in.tb";  -- Input file
  file OutFile       : text open write_mode is "OUT.DUV";  -- Output file
--  file FullFile      : text open write_mode is "quadratura.full.output";  -- Full output

  signal end_of_file : boolean;  	-- End of File indicator

  signal clk	     : std_logic := '1';
  signal clk2        : std_logic := '1';
  signal resetn      : std_logic;

  signal in_requested : std_logic;
  signal in_valid : std_logic := '0';
  signal in_data : signed(31 downto 0);

  signal count_entrada : integer := 0;
  signal count_saida : integer := 0;
  signal frames : integer := 0;
  shared variable flag : std_logic := '0';

  signal vga_in_valid : std_logic;
  signal count_frames : signed(31 downto 0) := x"11111111";
  signal red	         : std_logic_vector(7 downto 0);  -- red component
  signal green	 : std_logic_vector(7 downto 0);  -- green component
  signal blue	 : std_logic_vector(7 downto 0);  -- blue component
  signal line_pixel   : std_logic_vector(9 downto 0);  -- compute line
  signal column_pixel : std_logic_vector(9 downto 0);  -- compute column
  signal m1, m2       : std_logic;                     -- select dac mode
  signal blank_n      : std_logic;                     -- dac command
  signal sync_n       : std_logic;                     -- dac command
  signal sync_t       : std_logic;                     -- dac command
  signal video_clk    : std_logic;                     -- dac command
  signal vga_vs       : std_logic;                     -- vertical sync
  signal vga_hs       : std_logic;                      -- horizontal sync

begin  -- behavior


  theora_hardware0: entity work.theora_hardware
    port map (
      clk          => clk,
      clk_25Mhz  => clk2,
      reset_n      => resetn,

      in_request    => in_requested,
      in_valid      => in_valid, 
      in_data       => in_data,

      -- Remover depois
      out_valid     => vga_in_valid,

      red          => red,
      green	 => green,
      blue	 => blue,
      line_pixel   => line_pixel,
      column_pixel => column_pixel,
      m1 => m1,
      m2 => m2,
      blank_n      => blank_n,
      sync_n       => sync_n,
      sync_t       => sync_t,
      video_clk    => video_clk,
      vga_vs       => vga_vs,
      vga_hs       => vga_hs
      );
    


  clk  <= not clk  after clk_period / 2;
  clk2 <= not clk2 after clk_period;
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
          flag := '0';
--          assert frames < 2 report "2 frames: count_entrada="&integer'image(count_entrada) severity failure;
          count_entrada <= count_entrada + 1;
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
      if(vga_in_valid = '1' )then
        --Write(output_line, now, left, 15);
        Write(output_line, integer'image(to_integer(unsigned(line_pixel)))&" "&integer'image(to_integer(unsigned(column_pixel)))&": "&integer'image(to_integer(unsigned(red)))&", "&integer'image(to_integer(unsigned(green)))&", "&integer'image(to_integer(unsigned(blue))));
        WriteLine(OutFile, output_line);
--        assert false report "count_entrada = "&integer'image(count_entrada) severity failure;
      end if;
    end if;

  end process Output;

  --assert (not (to_integer(unsigned(line_pixel)) = 167)) report "Fim" severity FAILURE;

  assert not end_of_file report "End of Simulation: THIS IS NOT AN ERROR! "&
    "THIS IS JUST A WAY TO STOP THE SIMULATION." severity failure;


end behavior;
