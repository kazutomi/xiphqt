-------------------------------------------------------------------------------
-- Title      : Interface VGA
-- Project    : theora-fpga
-------------------------------------------------------------------------------
-- File       : interface_vga.vhd
-- Author     : Leonardo de Paula Rosa Piga
-- Company    : LSC - IC - UNICAMP
-- Last update: 2007/08/24
-- Platform   : 
-------------------------------------------------------------------------------
-- Description: Send the pixels from the RGB memory to the video controller
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity interface_vga is
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
end interface_vga;


architecture a_lancelot of interface_vga is
  constant HORIZ_RES           : natural := 640;  -- Horizontal Resolution
  constant HSYNC_BACK_PORCH_W  : natural := 40;
  constant HSYNC_FRONT_PORCH_W : natural := 25;
  constant HSYNC_PULSE_WIDTH   : natural := 95;
  constant VERT_RES            : natural := 480;  -- Vertical Resolution
  constant VSYNC_BACK_PORCH_W  : natural := 22;
  constant VSYNC_FRONT_PORCH_W : natural := 10;
  constant VSYNC_PULSE_W       : natural := 2;



--   constant HORIZ_RES           : natural := 160;  -- Horizontal Resolution
--   constant HSYNC_BACK_PORCH_W  : natural := 40;
--   constant HSYNC_FRONT_PORCH_W : natural := 25;
--   constant HSYNC_PULSE_WIDTH   : natural := 95;
--   constant VERT_RES            : natural := 120;  -- Vertical Resolution
--   constant VSYNC_BACK_PORCH_W  : natural := 22;
--   constant VSYNC_FRONT_PORCH_W : natural := 10;
--   constant VSYNC_PULSE_W       : natural := 2;

  
-- DAC State Machine
  type dac_mode_state_type is 
    (
      ds0, ds1, ds2, ds3, ds4
      );
  signal dac_mode_current_state, dac_mode_next_state : dac_mode_state_type;

  type sync_mem_state_t is (stt_sync_mem1, stt_sync_mem2);
  signal sync_mem_state : sync_mem_state_t;
  
  signal count_h, count_v : natural RANGE 0 TO 2043;
  signal video_on_v, video_on_h : std_logic;
  signal s_rgb_rd_addr   : unsigned(ADDR_WIDTH-1 downto 0);
  signal repeat_line : natural range 0 to 7;
  signal repeat_col : natural range 0 to 7;

  signal count_disp_c : natural RANGE 0 TO 1023;


  signal scale_x : natural range 0 to 16383;
  signal scale_y : natural range 0 to 16383;    
  signal max_count : unsigned(20 downto 0);
begin  
  -- fixed signals
  video_clk <= '1';
  
  blank_n <= video_on_h and video_on_v;

  rgb_rd_addr <= s_rgb_rd_addr;
----------------------------------
-- Video DAC Mode state machine --
----------------------------------
-- Set DAC in RGB mode and bias on all three channels
  
  process(video_clock, reset_n)
  begin
    if reset_n = '0' then
      dac_mode_current_state <= ds0;
    elsif (video_clock'event and video_clock = '1') then
      dac_mode_current_state <= dac_mode_next_state after 5 ns;
    end if;
  end process;

  process(dac_mode_current_state)
  begin

    sync_n <= '0';
    m2     <= '0';
    m1     <= '0';
    sync_t <= '0';
    case dac_mode_current_state is 

      when ds0 =>
        dac_mode_next_state <= ds1;
        
      when ds1 =>
        sync_n <= '1';
        m2     <= '0';
        m1     <= '0';
        sync_t <= '0';
        dac_mode_next_state <= ds2;

        -- bias on all three channels
      when ds2 =>
        sync_n <= '0';
        m2     <= '1';
        m1     <= '0';
        sync_t <= '0';
        dac_mode_next_state <= ds3;

      when ds3 =>
        sync_n <= '0';
        m2     <= '1';
        m1     <= '0';
        sync_t <= '0';
        dac_mode_next_state <= ds4;

        -- RGB mode
      when ds4 =>
        sync_n <= '1';
        m2     <= '0';
        m1     <= '0';
        sync_t <= '0';
        dac_mode_next_state <= ds4;
  
      when others =>
        sync_n <= '1';
        m2     <= '0';
        m1     <= '0';
        sync_t <= '0';
        dac_mode_next_state <= ds0;

    end case;

  end process;

  CalcLimits: process (video_clock, reset_n)
  begin  -- process CalcLimits
    if reset_n = '0' then               -- asynchronous reset (active low)
      scale_x <= 16383;
      scale_y <= 16383;
      max_count <= (others => '1');
      
    elsif video_clock'event and video_clock = '1' then  -- rising clock edge
      scale_x <= to_integer(video_width * ZOOM);
      scale_y <= to_integer(video_height * ZOOM);
      max_count <= resize(video_height * video_width, 21);
    end if;
  end process CalcLimits;
  
  timing: process (video_clock, reset_n)
  begin  -- process timing
    if reset_n = '0' then               -- asynchronous reset (active low)
      count_v <= 0;
      count_h <= 0;
      vga_hs <= '1';
      vga_vs <= '1';
      video_on_h <= '1';
      video_on_v <= '1';

      red   <= x"00";
      green <= x"00";
      blue  <= x"00";
      sync_mem_state <= stt_sync_mem1;
      s_rgb_rd_addr <= (others => '0');
      line_pixel <= (others => '1');
      column_pixel <= (others => '1');

      repeat_line <= 0;
      repeat_col <= 0;
      count_disp_c <= 0;

    elsif video_clock'event and video_clock = '1' then  -- rising clock edge

      if (sync_mem_state = stt_sync_mem2) then
        if (count_h = (HORIZ_RES +
                       HSYNC_BACK_PORCH_W +
                       HSYNC_FRONT_PORCH_W +
                       HSYNC_PULSE_WIDTH) - 1
            ) then
          count_h <= 0;

          if (count_v = (VERT_RES +
                         VSYNC_BACK_PORCH_W  +
                         VSYNC_FRONT_PORCH_W +
                         VSYNC_PULSE_W) - 1
              ) then
            count_v <= 0;
          else
            count_v <= count_v + 1;
          end if;
          
        else
          count_h <= count_h + 1;
        end if;

        if ((count_h >= HORIZ_RES + HSYNC_FRONT_PORCH_W) and
            (count_h < HORIZ_RES + HSYNC_FRONT_PORCH_W + HSYNC_PULSE_WIDTH)) then
          vga_hs <= '0';
        else
          vga_hs <= '1';
        end if;

        if ((count_v >= VERT_RES + VSYNC_FRONT_PORCH_W) and
            (count_v < VERT_RES + VSYNC_FRONT_PORCH_W + VSYNC_PULSE_W)) then
          vga_vs <= '0';
        else
          vga_vs <= '1';
        end if;

        -- blank_n signal 
        if count_h < HORIZ_RES then
          video_on_h <= '1';
          column_pixel <= std_logic_vector(to_unsigned(count_h,10));
        else
          video_on_h <= '0';
        end if;

        if count_v <  VERT_RES then
          video_on_v <= '1';
          line_pixel <= std_logic_vector(to_unsigned(count_v, 10));
        else
          video_on_v <= '0'; 
        end if;
      end if;

      if (can_read_mem = '1') then
        
        case sync_mem_state is
          when stt_sync_mem1 =>
            s_rgb_rd_addr <= (others => '0');
            sync_mem_state <= stt_sync_mem2;

--           when stt_sync_mem2 =>
--             s_rgb_rd_addr <= s_rgb_rd_addr + 1;
--             sync_mem_state <= stt_sync_mem3;
            
          when others =>
            red <= x"00";
            green <= x"00";
            blue <= x"00";
            if (count_h < scale_x) then
              if (count_v < scale_y) then
                red   <= std_logic_vector(rgb_rd_data(23 downto 16));
                green <= std_logic_vector(rgb_rd_data(15 downto 8));
                blue  <= std_logic_vector(rgb_rd_data(7 downto 0));

               
                repeat_col <= repeat_col + 1;
                if (repeat_col = ZOOM - 1) then
                  repeat_col <= 0;
                  s_rgb_rd_addr <= s_rgb_rd_addr + 1;
                  count_disp_c <= count_disp_c + 1;

                  if (count_disp_c = video_width - 1) then
                    s_rgb_rd_addr <= s_rgb_rd_addr - (video_width - 1);
                    count_disp_c <= 0;
                    repeat_line <= repeat_line + 1;
                    if (repeat_line = ZOOM - 1) then
                      repeat_line <= 0;

                      s_rgb_rd_addr <= s_rgb_rd_addr + 1;
                      if (s_rgb_rd_addr = max_count - 1) then
                        s_rgb_rd_addr <= (others => '0');
                      end if;
                    end if;
                  end if;
                end if;
              end if;
            end if;
        end case;
      end if;
    end if;
  end process timing;

end a_lancelot;
