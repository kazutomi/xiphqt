library std;
library ieee;
library work;

use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
--use ieee.std_logic_arith.all;
--use ieee.std_logic_unsigned.all;
--use ieee.std_logic_signed.all;
use work.all;



entity casca_avalon is
  port (clk,
        clock_25Mhz  : in  std_logic;

        reset_n : in std_logic;
        address : in std_logic_vector(1 downto 0);

        read : in std_logic;
        write : in std_logic;

        writedata : in std_logic_vector(31 downto 0);
        readdata : out std_logic_vector(31 downto 0);
        
        chipselect : in std_logic;

        ---------------------------------------------------------------------------
        -- Ports of video controller
        ---------------------------------------------------------------------------

        red      : out std_logic_vector(7 downto 0);  -- red component
        green	 : out std_logic_vector(7 downto 0);  -- green component
        blue	 : out std_logic_vector(7 downto 0);  -- blue component
        line_pixel   : out std_logic_vector(9 downto 0);  -- compute line
        column_pixel : out std_logic_vector(9 downto 0);  -- compute column
        m1, m2       : out std_logic;                     -- select dac mode
        blank_n      : out std_logic;                     -- dac command
        sync_n       : out std_logic;                     -- dac command
        sync_t       : out std_logic;                     -- dac command
        video_clk    : out std_logic;                     -- dac command
        vga_vs       : out std_logic;                     -- vertical sync
        vga_hs       : out std_logic                      -- horizontal sync

        
        );

end entity casca_avalon;




architecture rtl of casca_avalon is
  component theora_hardware is
  
  port (
    clk : in std_logic;
    clk_25Mhz : in std_logic;
    reset_n      : in  std_logic;                     -- reset

    ---------------------------------------------------------------------------
    -- Ports of Handshake
    ---------------------------------------------------------------------------
    in_request    : out std_logic;
    in_valid      : in  std_logic;      -- in_data
    in_data       : in  signed(31 downto 0);

    -- Remover depois
    out_valid     : out std_logic;
    ---------------------------------------------------------------------------
    -- Ports of video controller
    ---------------------------------------------------------------------------
    red	         : out std_logic_vector(7 downto 0);  -- red component
    green	 : out std_logic_vector(7 downto 0);  -- green component
    blue	 : out std_logic_vector(7 downto 0);  -- blue component
    line_pixel   : out std_logic_vector(9 downto 0);  -- compute line
    column_pixel : out std_logic_vector(9 downto 0);  -- compute column
    m1, m2       : out std_logic;                     -- select dac mode
    blank_n      : out std_logic;                     -- dac command
    sync_n       : out std_logic;                     -- dac command
    sync_t       : out std_logic;                     -- dac command
    video_clk    : out std_logic;                     -- dac command
    vga_vs       : out std_logic;                     -- vertical sync
    vga_hs       : out std_logic                      -- horizontal sync
    );
  end component;
  
  signal in_request : std_logic;
  signal in_valid : std_logic;
  signal in_data : signed(31 downto 0);

  signal out_valid : std_logic;

begin

  theora_hardware0: theora_hardware
    port map (
      clk => clk,
      clk_25Mhz => clock_25Mhz,
      reset_n      => reset_n,

      ---------------------------------------------------------------------------
      -- Ports of Handshake
      ---------------------------------------------------------------------------
      in_request    => in_request,
      in_valid      => in_valid,
      in_data       => in_data,

      -- Remover depois
      out_valid     => out_valid,
      ---------------------------------------------------------------------------
      -- Ports of video controller
      ---------------------------------------------------------------------------
      red	 => red,
      green	 => green,
      blue	 => blue,
      line_pixel   => line_pixel,
      column_pixel => column_pixel,
      m1           => m1,
      m2           => m2,
      blank_n      => blank_n,
      sync_n       => sync_n,
      sync_t       => sync_t,
      video_clk    => video_clk,
      vga_vs       => vga_vs,
      vga_hs       => vga_hs
      );

   process(chipselect, read, address, in_request, Reset_n)
   begin
     readdata <= "00000000000000000000000000000000";


     if (chipselect = '1') then
       if (read = '1') then
         case address is
           when "00" => -- Can software write data to Theora Hardware ?
             readdata <= "0000000000000000000000000000000"&in_request;
                 
           when others => -- Nothing
             readdata <= (others => '0');
         end case;
       end if;
     end if;

     if (Reset_n = '0') then
       readdata <= (others => '0');
     end if;
   end process;


  process(Reset_n, chipselect, write, writedata)
   begin

     in_valid <= '0';
     in_data <= (others => '0');
     if (chipselect = '1') then
       if (write = '1') then
         in_data <= signed(writedata(31 downto 0));
         in_valid <= '1';
       end if;
     end if;
     if (Reset_n = '0') then
       in_valid <= '0';
       in_data <= "00000000000000000000000000000000";
     end if;
   end process;
end rtl;
