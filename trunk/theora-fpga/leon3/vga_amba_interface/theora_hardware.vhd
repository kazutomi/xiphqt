library std;
library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity theora_hardware is
  
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

end theora_hardware;

architecture a_theora_hardware of theora_hardware is
  constant DEPTH      : natural := 8192;       -- RGB MEMORY DEPTH
  constant ADDR_WIDTH : natural := 13;         -- RGB MEMORY ADDRESS WIDTH
  constant DATA_WIDTH : natural := 24;         -- RGB MEMORY DATA WIDTH

  component interface_vga
    generic (
      DEPTH      : natural := 8192;               -- RGB MEMORY DEPTH
      ADDR_WIDTH : natural := 13;                 -- RGB MEMORY ADDRESS WIDTH
      DATA_WIDTH : natural := 24;                 -- RGB MEMORY DATA WIDTH
      ZOOM       : natural range 0 to 7 := 4      -- Image will be scaled to ZOOM
      );        

    port(
      video_clock  : in  std_logic;
      reset_n      : in  std_logic;                     -- reset


      ---------------------------------------------------------------------------
      -- Ports of RGB frame memory
      ---------------------------------------------------------------------------
      rgb_rd_addr   : out unsigned(ADDR_WIDTH-1 downto 0);
      rgb_rd_data   : in  signed(DATA_WIDTH-1 downto 0);

      ---------------------------------------------------------------------------
      -- Port of RGB frame memory control access
      ---------------------------------------------------------------------------
      can_read_mem  : in  std_logic;

      video_width   : in  unsigned(11 downto 0);
      video_height  : in  unsigned(11 downto 0);

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

  component YCbCr2RGB
    generic (
      DEPTH      : natural := 8192;               -- RGB MEMORY DEPTH
      ADDR_WIDTH : natural := 13;                 -- RGB MEMORY ADDRESS WIDTH
      DATA_WIDTH : natural := 24;                 -- RGB MEMORY DATA WIDTH
      ZOOM       : natural range 0 to 7 := 4      -- Image will be scaled to ZOOM
      );        
    
    port (
      Clk           : in std_logic;
      reset_n       : in std_logic;

      ---------------------------------------------------------------------------
      -- Ports of Handshake
      ---------------------------------------------------------------------------
      in_request    : out std_logic;
      in_valid      : in  std_logic;
      in_data       : in  signed(31 downto 0);

      ---------------------------------------------------------------------------
      -- Ports of RGB frame memory
      ---------------------------------------------------------------------------
      rgb_rd_addr   : in unsigned(ADDR_WIDTH-1 downto 0);
      rgb_rd_data   : out signed(DATA_WIDTH-1 downto 0);

      ---------------------------------------------------------------------------
      -- Port of RGB frame memory control access
      ---------------------------------------------------------------------------
      can_read_mem  : out std_logic
      );
  end component;

  constant ZOOM : natural := 2;
  
  signal s_line_pixel   : std_logic_vector(9 downto 0);  -- compute line
  signal s_column_pixel : std_logic_vector(9 downto 0);  -- compute column
    
  signal out_conv_requested : std_logic;
  signal out_conv_valid     : std_logic;
  signal out_conv_data      : signed(31 downto 0);

  type switch_state_t is (stt_switch1, stt_switch2,
                          stt_switch3, stt_switch4);
  signal switch_state : switch_state_t;
  signal s_in_request : std_logic;

  signal rgb_rd_addr   : unsigned(ADDR_WIDTH-1 downto 0);
  signal rgb_rd_data   : signed(DATA_WIDTH-1 downto 0);

  signal video_width   : unsigned(11 downto 0);
  signal video_height  : unsigned(11 downto 0);

  signal can_read_mem : std_logic;

begin  -- a_theora_hardware

  interface_vga0: interface_vga
    generic map (
      DEPTH      => DEPTH,
      ADDR_WIDTH => ADDR_WIDTH,
      DATA_WIDTH => DATA_WIDTH,
      ZOOM       => ZOOM)

    port map (
      video_clock  => clk_25Mhz,
      reset_n      => reset_n,

      ---------------------------------------------------------------------------
      -- Ports of RGB frame memory
      ---------------------------------------------------------------------------
      rgb_rd_addr   => rgb_rd_addr,
      rgb_rd_data   => rgb_rd_data,

      ---------------------------------------------------------------------------
      -- Port of RGB frame memory control access
      ---------------------------------------------------------------------------
      can_read_mem  => can_read_mem,

      video_width   => video_width,
      video_height  => video_height,

      ---------------------------------------------------------------------------
      -- Ports of video controller
      ---------------------------------------------------------------------------
      red	   => red,
      green	   => green,
      blue	   => blue,
      line_pixel   => s_line_pixel,
      column_pixel => s_column_pixel,
      m1           => m1,
      m2           => m2,
      blank_n      => blank_n,
      sync_n       => sync_n,
      sync_t       => sync_t,
      video_clk    => video_clk,
      vga_vs       => vga_vs,
      vga_hs       => vga_hs
      );

  YCbCr2RGB_0: YCbCr2RGB
    generic map (
      DEPTH      => DEPTH,
      ADDR_WIDTH => ADDR_WIDTH,
      DATA_WIDTH => DATA_WIDTH,
      ZOOM       => ZOOM)

    port map (
      Clk           => Clk,
      reset_n       => reset_n,

      ---------------------------------------------------------------------------
      -- Ports of Handshake
      ---------------------------------------------------------------------------
      in_request    => out_conv_requested,
      in_valid      => out_conv_valid,
      in_data       => out_conv_data,

      ---------------------------------------------------------------------------
      -- Ports of RGB frame memory
      ---------------------------------------------------------------------------
      rgb_rd_addr   => rgb_rd_addr,
      rgb_rd_data   => rgb_rd_data,

      ---------------------------------------------------------------------------
      -- Port of RGB frame memory control access
      ---------------------------------------------------------------------------
      can_read_mem  => can_read_mem
      );
  

  in_request <= s_in_request;

  s_in_request <= '1' when switch_state = stt_switch1 else
                  out_conv_requested;
                  
  line_pixel <= s_line_pixel;
  column_pixel <= s_column_pixel;

  out_valid <= clk_25Mhz when (unsigned(s_line_pixel) < video_height*ZOOM and
                         unsigned(s_column_pixel) < video_width*ZOOM) else
               '0';

  out_conv_valid <= '0' when switch_state = stt_switch1 else
                    in_valid;
  out_conv_data <= (others => '0') when switch_state = stt_switch1 else
                   in_data;

  
  ReadIn: process (clk, reset_n)
  begin  -- process ReadIn
    if reset_n = '0' then               -- asynchronous reset (active low)
      switch_state <= stt_switch1;

    elsif clk'event and clk = '1' then  -- rising clock edge

      if (s_in_request = '1' and in_valid = '1') then
        case switch_state is
          when stt_switch1 =>
            -- Ignore the first data
            switch_state <= stt_switch2;

          when stt_switch2 =>
            video_height <= unsigned(in_data(11 downto 0));
            switch_state <= stt_switch3;

          when stt_switch3 =>
            video_width <= unsigned(in_data(11 downto 0));
            switch_state <= stt_switch4;

          when others =>
            switch_state <= stt_switch4;
        end case;
      end if;
    end if;
  end process ReadIn;
end a_theora_hardware;
