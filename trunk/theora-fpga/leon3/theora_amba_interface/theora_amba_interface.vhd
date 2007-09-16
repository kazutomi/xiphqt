
-- Theora AMBA Interface
-- Tested on LEON3 and linux-2.6.21.1
-- André Costa - andre.lnc@gmail.com

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

library grlib;
use grlib.amba.all;
use grlib.stdlib.all;
use grlib.devices.all;

library work;
use work.all;

library opencores;
use opencores.theora_apb.all;

entity theora_amba_interface is
  generic(
    pindex      : integer := 0; 
    paddr       : integer := 0;
    pmask       : integer := 16#fff#
    );
  port (
    rst   : in  std_ulogic;
    clk   : in  std_ulogic;
    apbi   : in  apb_slv_in_type;
    apbo   : out apb_slv_out_type;

    clk_25Mhz : in std_logic;
    red	         : out std_logic_vector(7 downto 0);  -- red component
    green	 : out std_logic_vector(7 downto 0);  -- green component
    blue	 : out std_logic_vector(7 downto 0);  -- blue component
    --line_pixel   : out std_logic_vector(9 downto 0);  -- compute line
    --column_pixel : out std_logic_vector(9 downto 0);  -- compute column
    m1, m2       : out std_logic;                     -- select dac mode
    blank_n      : out std_logic;                     -- dac command
    sync_n       : out std_logic;                     -- dac command
    sync_t       : out std_logic;                     -- dac command
    video_clk    : out std_logic;                     -- dac command
    vga_vs       : out std_logic;                     -- vertical sync
    vga_hs       : out std_logic                      -- horizontal sync
    
--    irq   : out std_logic
  );
end entity theora_amba_interface;

architecture rtl of theora_amba_interface is

  component theora_hardware
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

  signal out_requested : std_logic;
  signal out_valid : std_logic;
  signal out_data : signed(31 downto 0);
  signal vga_in_valid : std_logic;
  
  signal line_pixel   : std_logic_vector(9 downto 0);  -- compute line
  signal column_pixel : std_logic_vector(9 downto 0);  -- compute column

--constant REVISION       : amba_version_type := 0; 
constant pconfig        : apb_config_type := (
                        0 => ahb_device_reg ( VENDOR_OPENCORES, OPENCORES_THEORA_HARDWARE, 0, 0, 0),
                        1 => apb_iobar(paddr, pmask));

  --signal r, rin : std_logic;
   signal rdata : std_logic_vector(31 downto 0);
   signal s_reset_n : std_logic;
   signal soft_reset_n : std_logic;
   
begin

  --recon1 : ReconRefFrames
  --port map ( clk, rst, in_request, in_valid, in_data, out_requested, out_valid, out_data);
  theora_hardware10: theora_hardware
     port map (
      clk          => clk,
      clk_25Mhz  => clk_25Mhz,
      reset_n      => s_reset_n,

      in_request    => in_request,
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
    
  s_reset_n <= rst and soft_reset_n;
  
  reg : process(rst, clk)
  begin 
    if (rst = '0') then
    rdata <= (others => '0');
    out_requested <= '0';
    in_valid <= '0';
    in_data <= (others => '0');
    soft_reset_n <= '0';
    
  elsif rising_edge(clk) then 

    rdata <= (others => '0');             -- init
    out_requested <= '0';
    in_valid <= '0';
    in_data <= (others => '0');
    
-- APB BUS want Read?
 if (apbi.psel(pindex)) = '1' then
  case apbi.paddr(4 downto 2) is
    when "000" =>  -- Can software write data to Theora Hardware?
      rdata(0) <= in_request; -- rdata=a
    when "010" => -- Can software read data from Theora Hardware?
      rdata(0) <= out_valid;
    when others => -- ead data from Theora Hardware ?
      out_requested <= '1';
      rdata <= std_logic_vector(out_data);
  end case;
 end if;
 
  soft_reset_n <= '1';

-- APB BUS want Write?
  if (apbi.psel(pindex) and apbi.penable and apbi.pwrite) = '1' then
    case apbi.paddr(4 downto 2) is
      when "100" =>  
        soft_reset_n <= '0';
      when others =>
        in_valid <= '1';
        in_data <= signed(apbi.pwdata(31 downto 0)); -- a:=pwdata
      end case;    
  end if;
  end if;  
  end process;

    apbo.prdata <= rdata;
    apbo.pindex <= pindex;
    apbo.pirq <= (others => '0');
    apbo.pconfig <= pconfig;
    
   -- pragma translate_off   
   bootmsg : report_version 
   generic map ("apbvgreport_versiona" & tost(pindex) & ": APB THEORA HARDWARE module rev 0");
   -- pragma translate_on
   
end rtl;
