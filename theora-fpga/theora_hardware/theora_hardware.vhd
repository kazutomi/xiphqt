-------------------------------------------------------------------------------
-- Title      : Theora Hardware
-- Project    : theora-fpga
-------------------------------------------------------------------------------
-- File       : theora_hardware.vhd
-- Author     : Leonardo de Paula Rosa Piga
-- Company    : LSC - IC - UNICAMP
-- Last update: 2007/08/23
-- Platform   : 
-------------------------------------------------------------------------------
-- Description: Wrapper to receive data from NIOS processor
-------------------------------------------------------------------------------

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
      DEPTH      : natural := 8192;              -- RGB MEMORY DEPTH
      ADDR_WIDTH : natural := 13;                -- RGB MEMORY ADDRESS WIDTH
      DATA_WIDTH : natural := 24;                -- RGB MEMORY DATA WIDTH
      ZOOM       : natural range 0 to 7 := 4     -- Image will be scaled to ZOOM
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
      DEPTH      : natural := 8192;       -- RGB MEMORY DEPTH
      ADDR_WIDTH : natural := 13;         -- RGB MEMORY ADDRESS WIDTH
      DATA_WIDTH : natural := 24;         -- RGB MEMORY DATA WIDTH
      ZOOM       : natural range 0 to 7 := 4     -- Image will be scaled to ZOOM
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

  component ReconRefFrames
    port (Clk,
          Reset_n       : in  std_logic;
          
          in_request    : out std_logic;
          in_valid      : in  std_logic;
          in_data       : in  signed(31 downto 0);
          
          out_requested : in  std_logic;
          out_valid     : out std_logic;
          out_data      : out signed(31 downto 0)
          );

  end component;

  constant ZOOM : natural := 2;
  
  signal s_line_pixel   : std_logic_vector(9 downto 0);  -- compute line
  signal s_column_pixel : std_logic_vector(9 downto 0);  -- compute column
    
  signal out_conv_requested : std_logic;
  signal out_conv_valid     : std_logic;
  signal out_conv_data      : signed(31 downto 0);

  signal in_rr_request    : std_logic;
  signal in_rr_valid      : std_logic;
  signal in_rr_data       : signed(31 downto 0);
  
  signal out_rr_requested : std_logic;
  signal out_rr_valid     : std_logic;
  signal out_rr_data      : signed(31 downto 0);

  type switch_state_t is (stt_switch1, stt_switch2, stt_switch3,
                          stt_switch4, stt_switch5);
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

  reconrefframes0: ReconRefFrames
    port map (
      Clk => clk,
      Reset_n       => reset_n,
        
      in_request    => out_rr_requested,
      in_valid      => out_rr_valid,
      in_data       => out_rr_data,
        
      out_requested => in_rr_request,
      out_valid     => in_rr_valid,
      out_data      => in_rr_data
      );

  in_request <= s_in_request;

  out_rr_valid <= in_valid;

  out_rr_data <= in_data;

  Mux_out_conv_valid: with switch_state select
    out_conv_valid <= in_valid when stt_switch3 | stt_switch4,
                      in_rr_valid when stt_switch5,
                      '0'      when others;

  
  Mux_out_conv_data: with switch_state select
    out_conv_data <= in_data when stt_switch3 | stt_switch4,
                     in_rr_data when stt_switch5,
                     (others => '0') when others;

  out_valid <= clk_25Mhz when (unsigned(s_line_pixel) < video_height * ZOOM and
                               unsigned(s_column_pixel) < video_width * ZOOM) else
               '0';

  line_pixel <= s_line_pixel;
  column_pixel <= s_column_pixel;

  
  SwitchSignalsIn: process (reset_n,
                            out_conv_requested, out_rr_requested,
                            switch_state)
  begin  -- process SwitchSignals

    s_in_request <= '0';
    in_rr_request <= '0';
    
    if (switch_state = stt_switch3 or  switch_state = stt_switch4) then
      in_rr_request <= '0';
      s_in_request <= out_rr_requested and out_conv_requested;

    else
      in_rr_request <= out_conv_requested;
      s_in_request <= out_rr_requested;

    end if;

  end process SwitchSignalsIn;


                        
  -- purpose: Control the switch the interface_vga and ReconRefFrames ports' signals
  -- type   : sequential
  -- inputs : clk, reset_n
  SwitchSignalsControl: process (clk, reset_n)
  begin  -- process SwitchEntries
    if reset_n = '0' then               -- asynchronous reset (active low)
      switch_state <= stt_switch1;
      
    elsif clk'event and clk = '1' then  -- rising clock edge
      if (s_in_request = '1' and in_valid = '1') then
        case switch_state is
          -- Ignore first data
          when stt_switch1 =>
            switch_state <= stt_switch2;

          -- FrameSize
          when stt_switch2 =>
            switch_state <= stt_switch3;

          -- Video Height
          when stt_switch3 =>
            switch_state <= stt_switch4;
            video_height <= unsigned(in_data(11 downto 0));

          -- Video Width
          when stt_switch4 =>
            switch_state <= stt_switch5;
            video_width <= unsigned(in_data(11 downto 0));

          -- Reconrefframes parameters
          when stt_switch5 =>
            switch_state <= stt_switch5;
        end case;
      end if;
    end if;
  end process SwitchSignalsControl;

end a_theora_hardware;
