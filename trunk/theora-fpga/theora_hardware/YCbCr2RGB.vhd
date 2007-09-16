-------------------------------------------------------------------------------
-- Title      : YCbCr to RGB converter
-- Project    : theora-fpga
-------------------------------------------------------------------------------
-- File       : YCbCr2RGB.vhd
-- Author     : Leonardo de Paula Rosa Piga
-- Company    : LSC - IC - UNICAMP
-- Last update: 2007/08/23
-- Platform   : 
-------------------------------------------------------------------------------
-- Description: A YCbCr to RGB converter
-------------------------------------------------------------------------------


library std;
library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;


entity YCbCr2RGB is
  generic (
    DEPTH      : natural := 8192;         -- RGB MEMORY DEPTH
    ADDR_WIDTH : natural := 13;           -- RGB MEMORY ADDRESS WIDTH
    DATA_WIDTH : natural := 24;           -- RGB MEMORY DATA WIDTH
    ZOOM       : natural range 0 to 7 := 4  -- Image will be scaled to ZOOM
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

    
end YCbCr2RGB;

architecture a_YCbCr2RGB of YCbCr2RGB is

  component tsyncram
    generic (
      DEPTH : positive := 64;             -- How many slots
      DATA_WIDTH : positive := 16;        -- How many bits per slot
      ADDR_WIDTH : positive := 6          -- = ceil(log2(DEPTH))
      );
    
    port (
      clk : in std_logic;
      wr_e  : in std_logic;
      wr_addr : in unsigned(ADDR_WIDTH-1 downto 0);
      wr_data : in signed(DATA_WIDTH-1 downto 0);
      rd_addr : in unsigned(ADDR_WIDTH-1 downto 0);
      rd_data : out signed(DATA_WIDTH-1 downto 0)
      );
  end component tsyncram;

  -----------------------------------------------------------------------------
  -- Signals of Master State Machine
  -----------------------------------------------------------------------------
  type state_t is (stt_clean_buffers,
                   stt_read,
                   stt_conv_readmem,
                   stt_conv_writemem
                   );
  signal state : state_t;



  -----------------------------------------------------------------------------
  -- Signals of ReadIn procedure
  -----------------------------------------------------------------------------
  type read_state_t is (stt_read_height,
                        stt_read_width,
                        stt_read_Y,
                        stt_read_Cb,
                        stt_read_Cr
                        );
  signal read_state : read_state_t;

  -----------------------------------------------------------------------------
  -- Signals of ReadMemConvert procedure
  -----------------------------------------------------------------------------
  type conv_readmem_state_t is (stt_conv_readmem1,
                        stt_conv_readmem2,
                        stt_conv_readmem3,
                        stt_conv_readmem4,
                        stt_conv_readmem5,
                        stt_conv_readmem6,
                        stt_conv_readmem7,
                        stt_conv_readmem8
                        );
  signal conv_readmem_state : conv_readmem_state_t;

  signal val_Y, val_Cb, val_Cr : unsigned(7 downto 0);
  
  -----------------------------------------------------------------------------
  -- Signals of WriteMemConvert procedure
  -----------------------------------------------------------------------------
  type conv_writemem_state_t is (stt_conv_writemem1,
                                 stt_conv_writemem2,
                                 stt_conv_writemem3,
                                 stt_conv_writemem4,
                                 stt_conv_writemem5,
                                 stt_conv_writemem6
                                 );
  signal conv_writemem_state : conv_writemem_state_t;

  signal val_red, val_green, val_blue : signed(31 downto 0);  

  -----------------------------------------------------------------------------
  -- Signals of BufferAccessControl process
  -----------------------------------------------------------------------------
  type buf_access_state_t is (stt_buf_access1,
                              stt_buf_access2
                              );
  signal buf_access_state : buf_access_state_t;
  signal repeat_line : natural range 0 to 7;

  -----------------------------------------------------------------------------
  -- Buffer memories parameters
  -----------------------------------------------------------------------------
  constant MEM_Y_DEPTH      : natural := 2048;
  constant MEM_Y_DATA_WIDTH : natural := 32;
  constant MEM_Y_ADDR_WIDTH : natural := 11;


  constant MEM_DEPTH      : natural := 512;
  constant MEM_DATA_WIDTH : natural := 32;
  constant MEM_ADDR_WIDTH : natural := 9;


  constant MEM_RGB_DEPTH      : natural := DEPTH;
  constant MEM_RGB_DATA_WIDTH : natural := DATA_WIDTH;
  constant MEM_RGB_ADDR_WIDTH : natural := ADDR_WIDTH;

  constant MAX_SIZE           : natural := 1023;
  constant LG2_MAX_SIZE       : natural := 10;
  -----------------------------------------------------------------------------
  -- signals of Y memory
  -----------------------------------------------------------------------------
  signal mem_Y_wr_e     : std_logic;
  signal mem_Y_wr_addr  : unsigned (MEM_Y_ADDR_WIDTH-1 downto 0);
  signal mem_Y_wr_data  : signed   (MEM_Y_DATA_WIDTH-1 downto 0);
  signal mem_Y_rd_addr  : unsigned (MEM_Y_ADDR_WIDTH-1 downto 0);
  signal mem_Y_rd_data  : signed   (MEM_Y_DATA_WIDTH-1 downto 0);

  -----------------------------------------------------------------------------
  -- signals of Cb memory
  -----------------------------------------------------------------------------
  signal mem_Cb_wr_e     : std_logic;
  signal mem_Cb_wr_addr  : unsigned (MEM_ADDR_WIDTH-1 downto 0);
  signal mem_Cb_wr_data  : signed   (MEM_DATA_WIDTH-1 downto 0);
  signal mem_Cb_rd_addr  : unsigned (MEM_ADDR_WIDTH-1 downto 0);
  signal mem_Cb_rd_data  : signed   (MEM_DATA_WIDTH-1 downto 0);

  -----------------------------------------------------------------------------
  -- signals of Cr memory
  -----------------------------------------------------------------------------
  signal mem_Cr_wr_e     : std_logic;
  signal mem_Cr_wr_addr  : unsigned (MEM_ADDR_WIDTH-1 downto 0);
  signal mem_Cr_wr_data  : signed   (MEM_DATA_WIDTH-1 downto 0);
  signal mem_Cr_rd_addr  : unsigned (MEM_ADDR_WIDTH-1 downto 0);
  signal mem_Cr_rd_data  : signed   (MEM_DATA_WIDTH-1 downto 0);

  -----------------------------------------------------------------------------
  -- signals of RGB memory
  -----------------------------------------------------------------------------
  signal mem_RGB_wr_e     : std_logic;
  signal mem_RGB_wr_addr  : unsigned (MEM_RGB_ADDR_WIDTH-1 downto 0);
  signal mem_RGB_wr_data  : signed   (MEM_RGB_DATA_WIDTH-1 downto 0);
  signal mem_RGB_rd_addr  : unsigned (MEM_RGB_ADDR_WIDTH-1 downto 0);
  signal mem_RGB_rd_data  : signed   (MEM_RGB_DATA_WIDTH-1 downto 0);

  signal mem_Y_cleaned   : std_logic;
  signal mem_Cr_cleaned  : std_logic;
  signal mem_Cb_cleaned  : std_logic;
  signal mem_RGB_cleaned : std_logic;

  
  signal first_clean     : std_logic;
  signal first_write     : std_logic;
  signal half_left_half_right : std_logic;  -- '0' if we will choose the left
                                            -- half of Cb and Cr pixels of one
                                            -- read

  signal s_in_request : std_logic;
  
  signal can_write_mem :  std_logic;
  signal new_frame : std_logic;

  signal mem_read_line : integer range 0 to 1023;
  signal mem_read_col  : integer range 0 to 1023;
  signal counter_cols, counter_lines : integer range 0 to 1023;

  -----------------------------------------------------------------------------
  -- Video parameter signals
  -----------------------------------------------------------------------------
  signal video_width, video_height : integer range 0 to 1023;

begin  -- a_YCbCr2RGB
  mem_RGB_rd_addr <= rgb_rd_addr;
  rgb_rd_data <= mem_RGB_rd_data;

  can_read_mem <= '0' when state = stt_clean_buffers else
                  '0' when read_state = stt_read_width else
                  '0' when read_state = stt_read_height else
                  '1';
  
  mem_Y_plane: tsyncram
    generic map (
      DEPTH      => MEM_Y_DEPTH,
      DATA_WIDTH => MEM_Y_DATA_WIDTH,
      ADDR_WIDTH => MEM_Y_ADDR_WIDTH)
    port map (
      clk     => Clk,
      wr_e    => mem_Y_wr_e,
      wr_addr => mem_Y_wr_addr,
      wr_data => mem_Y_wr_data,
      rd_addr => mem_Y_rd_addr,
      rd_data => mem_Y_rd_data);

  mem_Cb_plane: tsyncram
    generic map (
      DEPTH      => MEM_DEPTH,
      DATA_WIDTH => MEM_DATA_WIDTH,
      ADDR_WIDTH => MEM_ADDR_WIDTH)
    port map (
      clk     => Clk,
      wr_e    => mem_Cb_wr_e,
      wr_addr => mem_Cb_wr_addr,
      wr_data => mem_Cb_wr_data,
      rd_addr => mem_Cb_rd_addr,
      rd_data => mem_Cb_rd_data);

  mem_Cr_plane: tsyncram
    generic map (
      DEPTH      => MEM_DEPTH,
      DATA_WIDTH => MEM_DATA_WIDTH,
      ADDR_WIDTH => MEM_ADDR_WIDTH)
    port map (
      clk     => Clk,
      wr_e    => mem_Cr_wr_e,
      wr_addr => mem_Cr_wr_addr,
      wr_data => mem_Cr_wr_data,
      rd_addr => mem_Cr_rd_addr,
      rd_data => mem_Cr_rd_data);

  mem_RGB: tsyncram
    generic map (
      DEPTH      => MEM_RGB_DEPTH,
      DATA_WIDTH => MEM_RGB_DATA_WIDTH,
      ADDR_WIDTH => MEM_RGB_ADDR_WIDTH)
    port map (
      clk     => Clk,
      wr_e    => mem_RGB_wr_e,
      wr_addr => mem_RGB_wr_addr,
      wr_data => mem_RGB_wr_data,
      rd_addr => mem_RGB_rd_addr,
      rd_data => mem_RGB_rd_data);


  in_request <= s_in_request;

  -- purpose: Verify if the RGB buffer was written before overwrite it
  -- type   : sequential
  -- inputs : clk, reset_n
  -- outputs: can_write_mem
  BufferAccessControl: process (clk, reset_n)
  begin  -- process BufferAccessControl
    if (reset_n) = '0' then              -- asynchronous reset (active low)
      can_write_mem <= '1';
      buf_access_state <= stt_buf_access1;
      repeat_line <= 0;
      
    elsif clk'event and clk = '1' then  -- rising clock edge

      case buf_access_state is
        when stt_buf_access1 =>
          if (new_frame = '1') then
            buf_access_state <= stt_buf_access2;
            can_write_mem <= '0';
          end if;
        when stt_buf_access2 =>
          if (rgb_rd_addr = video_height * video_width - 1) then
            repeat_line <= repeat_line + 1;
            if (repeat_line = ZOOM - 1) then
              repeat_line <= 0;
              buf_access_state <= stt_buf_access1;
              can_write_mem <= '1';
            end if;
          end if;
      end case;
      
    end if;
  end process BufferAccessControl;


  
  -- purpose: Clean buffers, read width and height once and the frame in the YUV
  -- format and convert the Buffer to the RGB representation
  -- type   : sequential
  -- inputs : Clk, reset_n
  Converter: process (Clk, reset_n)

    ---------------------------------------------------------------------------
    -- Procedure that writes zero in all positions of all memories
    ---------------------------------------------------------------------------
    procedure CleanBuffers is
    begin
      mem_Y_wr_e <= '1';
      mem_Y_wr_data <= (others => '0');
      
      mem_Cb_wr_e <= '1';
      mem_Cb_wr_data <= (others => '0');

      mem_Cr_wr_e <= '1';
      mem_Cr_wr_data <= (others => '0');

      mem_RGB_wr_e <= '1';
      mem_RGB_wr_data <= (others => '0');

      if (first_clean = '1') then
        first_clean <= '0';
        mem_Y_wr_addr   <= (others => '0');
        mem_Cb_wr_addr  <= (others => '0');
        mem_Cr_wr_addr  <= (others => '0');
        mem_RGB_wr_addr <= (others => '0');
      else
        mem_Y_wr_addr   <= mem_Y_wr_addr + 1;
        mem_Cb_wr_addr  <= mem_Cb_wr_addr + 1;
        mem_Cr_wr_addr  <= mem_Cr_wr_addr + 1;
        mem_RGB_wr_addr <= mem_RGB_wr_addr + 1;
      end if;

      if ((mem_Y_wr_addr = MEM_Y_DEPTH - 1) or (mem_Y_cleaned = '1')) then
        mem_Y_wr_e    <= '0';
        mem_Y_cleaned <= '1';
        mem_Y_wr_addr <= (others => '0');
      end if;

      if ((mem_Cb_wr_addr = MEM_DEPTH - 1) or (mem_Cb_cleaned = '1')) then
        mem_Cb_wr_e    <= '0';
        mem_Cb_cleaned <= '1';
        mem_Cb_wr_addr <= (others => '0');
      end if;

      if ((mem_Cr_wr_addr = MEM_DEPTH - 1) or (mem_Cr_cleaned = '1')) then
        mem_Cr_wr_e    <= '0';
        mem_Cr_cleaned <= '1';
        mem_Cr_wr_addr <= (others => '0');
      end if;

      if ((mem_RGB_wr_addr = MEM_RGB_DEPTH - 1) or (mem_RGB_cleaned = '1')) then
        mem_RGB_wr_e    <= '0';
        mem_RGB_cleaned <= '1';
        mem_RGB_wr_addr <= (others => '0');
      end if;

      if (mem_Y_cleaned = '1' and mem_Cb_cleaned = '1' and
          mem_Cr_cleaned = '1' and mem_RGB_cleaned = '1') then
        mem_Y_cleaned <= '0';
        mem_Cb_cleaned <= '0';
        mem_Cr_cleaned <= '0';
        mem_RGB_cleaned <= '0';
        read_state <= stt_read_Y;
        state <= stt_read;
      end if;
    end procedure CleanBuffers;

    procedure ReadIn is
    begin
      
      s_in_request <= '1';
      if (s_in_request = '1' and in_valid = '1') then
        case read_state is
          when stt_read_height =>
            video_height <= to_integer(in_data);
            read_state <= stt_read_width;
            
          when stt_read_width =>
            video_width <= to_integer(in_data);
            read_state <= stt_read_Y;
            state <= stt_clean_buffers;
            s_in_request <= '0';

          when stt_read_Y =>
            s_in_request <= '0';
            mem_Y_wr_e    <= '1';
            mem_Y_wr_data <= in_data;
            mem_Y_wr_addr <= resize
                             (SHIFT_RIGHT
                              (to_unsigned(video_width*counter_lines + counter_cols, MEM_Y_ADDR_WIDTH+4)
                               , 2), MEM_Y_ADDR_WIDTH);
            
            counter_cols <= counter_cols + 4;
            if (counter_cols = video_width - 4) then  -- if it is the last four pixels
              counter_cols <= 0;
              counter_lines <= counter_lines + 1;
              if (counter_lines = video_height - 1) then
                counter_lines <= 0;
                read_state <= stt_read_Cb;
              end if;
            end if;
            
          when stt_read_Cb =>
            s_in_request <= '0';
            mem_Cb_wr_e    <= '1';
            mem_Cb_wr_data <= in_data;
            mem_Cb_wr_addr <= resize
                              (SHIFT_RIGHT
                               (to_unsigned((video_width/2)*counter_lines + counter_cols, MEM_ADDR_WIDTH+4)
                                , 2), MEM_ADDR_WIDTH);

            
            counter_cols <= counter_cols + 4;
            if (counter_cols = to_integer(SHIFT_RIGHT(to_unsigned(video_width, LG2_MAX_SIZE), 1)) - 4) then  -- if it is the last four pixels
              counter_cols <= 0;
              counter_lines <= counter_lines + 1;
              if (counter_lines = (to_integer(SHIFT_RIGHT(to_unsigned(video_height, LG2_MAX_SIZE), 1))- 1)) then
                counter_lines <= 0;
                read_state <= stt_read_Cr;
              end if;
            end if;
            
            
          when stt_read_Cr =>
            s_in_request <= '0';
            mem_Cr_wr_e    <= '1';
            mem_Cr_wr_data <= in_data;
            mem_Cr_wr_addr <= resize
                              (SHIFT_RIGHT
                               (to_unsigned((video_width/2)*counter_lines + counter_cols, MEM_ADDR_WIDTH+4)
                                , 2), MEM_ADDR_WIDTH);

            
            counter_cols <= counter_cols + 4;
            if (counter_cols = to_integer(SHIFT_RIGHT(to_unsigned(video_width, LG2_MAX_SIZE), 1)) - 4) then  -- if it is the last four pixels
              counter_cols <= 0;
              counter_lines <= counter_lines + 1;
              if (counter_lines = (to_integer(SHIFT_RIGHT(to_unsigned(video_height, LG2_MAX_SIZE), 1))- 1)) then

                
                counter_lines <= 0;
                read_state <= stt_read_Y;
                s_in_request <= '0';
                state <= stt_conv_readmem;
                conv_readmem_state <= stt_conv_readmem1;
              end if;

            end if;
        end case;
      end if;
    end procedure ReadIn;

    -- purpose: Read the Y, Cb and Cr memory and move to stt_conv_writedmem
    procedure ReadMemConvert is
      variable v_addr_Y, v_addr_CbCr : unsigned(19 downto 0);
    begin  -- ReadMemConvert
      if (can_write_mem = '1') then     -- If the frame has benn already displayed
        case conv_readmem_state is
          when stt_conv_readmem1 =>
            v_addr_Y := to_unsigned(mem_read_line * video_width + mem_read_col, 20);
            v_addr_CbCr := resize(SHIFT_RIGHT(to_unsigned(mem_read_line, 10), 1) *
                                  SHIFT_RIGHT(to_unsigned(video_width, 10), 1) +
                                  SHIFT_RIGHT(to_unsigned(mem_read_col, 10), 1), 20);
            
            mem_Y_rd_addr  <= resize(SHIFT_RIGHT(v_addr_Y, 2), MEM_Y_ADDR_WIDTH);
            mem_Cb_rd_addr <= resize(SHIFT_RIGHT(v_addr_CbCr, 2), MEM_ADDR_WIDTH);
            mem_Cr_rd_addr <= resize(SHIFT_RIGHT(v_addr_CbCr, 2), MEM_ADDR_WIDTH);
            conv_readmem_state <= stt_conv_readmem2;
            half_left_half_right <= '0';
            first_write <= '1';
            
          when stt_conv_readmem2 =>
            -- Just wait for the memory delay
            conv_readmem_state <= stt_conv_readmem3;

          when stt_conv_readmem3 =>
            val_Y  <= unsigned(mem_Y_rd_data(31 downto 24));
            if (half_left_half_right = '0') then
              val_Cb <= unsigned(mem_Cb_rd_data(31 downto 24));
              val_Cr <= unsigned(mem_Cr_rd_data(31 downto 24));
            else
              val_Cb <= unsigned(mem_Cb_rd_data(15 downto 8));
              val_Cr <= unsigned(mem_Cr_rd_data(15 downto 8));
            end if;
            -- Move to stt_conv_writemem to convert to RGB and write the memory
            state <= stt_conv_writemem;
            conv_writemem_state <= stt_conv_writemem1;

            -- When return from stt_conv_writemem mo to the next conv_readmem_state
            conv_readmem_state <= stt_conv_readmem4;

          when stt_conv_readmem4 =>
            val_Y  <= unsigned(mem_Y_rd_data(23 downto 16));
            if (half_left_half_right = '0') then
              val_Cb <= unsigned(mem_Cb_rd_data(31 downto 24));
              val_Cr <= unsigned(mem_Cr_rd_data(31 downto 24));
            else
              val_Cb <= unsigned(mem_Cb_rd_data(15 downto 8));
              val_Cr <= unsigned(mem_Cr_rd_data(15 downto 8));
            end if;
            -- Move to stt_conv_writemem to convert to RGB and write the memory
            first_write <= '0';
            state <= stt_conv_writemem;
            conv_writemem_state <= stt_conv_writemem1;

            -- When return from stt_conv_writemem mo to the next conv_readmem_state
            conv_readmem_state <= stt_conv_readmem5;

            
          when stt_conv_readmem5 =>
            val_Y  <= unsigned(mem_Y_rd_data(15 downto 8));
            if (half_left_half_right = '0') then
              val_Cb <= unsigned(mem_Cb_rd_data(23 downto 16));
              val_Cr <= unsigned(mem_Cr_rd_data(23 downto 16));
            else
              val_Cb <= unsigned(mem_Cb_rd_data(7 downto 0));
              val_Cr <= unsigned(mem_Cr_rd_data(7 downto 0));
            end if;
            -- Move to stt_conv_writemem to convert to RGB and write the memory
            first_write <= '0';
            state <= stt_conv_writemem;
            conv_writemem_state <= stt_conv_writemem1;

            -- When return from stt_conv_writemem mo to the next conv_readmem_state
            conv_readmem_state <= stt_conv_readmem6;

          when stt_conv_readmem6 =>
            val_Y  <= unsigned(mem_Y_rd_data(7 downto 0));
            if (half_left_half_right = '0') then
              val_Cb <= unsigned(mem_Cb_rd_data(23 downto 16));
              val_Cr <= unsigned(mem_Cr_rd_data(23 downto 16));
            else
              val_Cb <= unsigned(mem_Cb_rd_data(7 downto 0));
              val_Cr <= unsigned(mem_Cr_rd_data(7 downto 0));
            end if;
            -- Move to stt_conv_writemem to convert to RGB and write the memory
            first_write <= '0';
            state <= stt_conv_writemem;
            conv_writemem_state <= stt_conv_writemem1;

            -- When return from stt_conv_writemem mo to the next conv_readmem_state
            conv_readmem_state <= stt_conv_readmem7;


          when stt_conv_readmem7 =>
            half_left_half_right <= not half_left_half_right;

            -- Prepare to request another address
            conv_readmem_state <= stt_conv_readmem8;
            
            mem_read_col <= mem_read_col + 4;
            if (mem_read_col = video_width - 4) then  -- if it is the last pixel
              mem_read_col <= 0;
              mem_read_line <= mem_read_line + 1;

              if (mem_read_line = video_height - 1) then
                mem_read_line <= 0;

                -- Convertion done
                assert false report "YCbCr2RGB done" severity NOTE;
                new_frame <= '1';
                -- Can read another frame then move to read state
                conv_readmem_state <= stt_conv_readmem1;
                state <= stt_read;
                read_state <= stt_read_Y;
              end if;
            end if;

          when stt_conv_readmem8 =>
            -- Request another address
            v_addr_Y := to_unsigned(mem_read_line * video_width + mem_read_col, 20);
            v_addr_CbCr := resize(SHIFT_RIGHT(to_unsigned(mem_read_line, 10), 1) *
                                  SHIFT_RIGHT(to_unsigned(video_width, 10), 1) +
                                  SHIFT_RIGHT(to_unsigned(mem_read_col, 10), 1), 20);
                
            mem_Y_rd_addr  <= resize(SHIFT_RIGHT(v_addr_Y, 2), MEM_Y_ADDR_WIDTH);
            mem_Cb_rd_addr <= resize(SHIFT_RIGHT(v_addr_CbCr, 2), MEM_ADDR_WIDTH);
            mem_Cr_rd_addr <= resize(SHIFT_RIGHT(v_addr_CbCr, 2), MEM_ADDR_WIDTH);
            conv_readmem_state <= stt_conv_readmem2;
        end case;
      end if;  
    end ReadMemConvert;

    -- purpose: Do YCbCr convertion and write data to the memory
    procedure WriteMemConvert is
    begin  -- WriteMemConvert
      case conv_writemem_state is
        when stt_conv_writemem1 =>
          val_red   <= resize(to_signed(1220542, 22) * (signed('0' & val_Y) - 16), 32);
          val_green <= resize(to_signed(1220542, 22) * (signed('0' &val_Y) - 16), 32);
          val_blue  <= resize(to_signed(1220542, 22) * (signed('0' &val_Y) - 16), 32);
          conv_writemem_state <= stt_conv_writemem2;

        when stt_conv_writemem2 =>
          val_red <= val_red + to_signed(1673527, 22) * (signed('0' & val_Cr) - 128);
          val_green <= val_green - to_signed(852492, 22) * (signed('0' &val_Cr) - 128);
          val_blue <= val_blue + to_signed(2114978, 23) * (signed('0' &val_Cb) - 128);
          conv_writemem_state <= stt_conv_writemem3;

        when stt_conv_writemem3 =>
          val_green <= val_green - to_signed(411042, 22) *(signed('0' &val_Cb) - 128);
          conv_writemem_state <= stt_conv_writemem4;


        when stt_conv_writemem4 =>
          val_red <= SHIFT_RIGHT(val_red , 20);
          val_green <= SHIFT_RIGHT(val_green , 20);
          val_blue <= SHIFT_RIGHT(val_blue , 20);
          conv_writemem_state <= stt_conv_writemem5;

          
        when stt_conv_writemem5 =>
          -- Clamp values
          if (val_red < 0) then
            val_red(7 downto 0) <= x"00";
          elsif (val_red > 255) then
            val_red(7 downto 0) <= x"ff";
          end if;

          if (val_green < 0) then
            val_green(7 downto 0) <= x"00";
          elsif (val_green > 255) then
            val_green(7 downto 0) <= x"ff";
          end if;

          if (val_blue < 0) then
            val_blue(7 downto 0) <= x"00";
          elsif (val_blue > 255) then
            val_blue(7 downto 0) <= x"ff";
          end if;
          conv_writemem_state <= stt_conv_writemem6;

        when stt_conv_writemem6 =>
          -- Write data
          mem_RGB_wr_e    <= '1';
          mem_RGB_wr_data <= val_red(7 downto 0) & val_green(7 downto 0) & val_blue(7 downto 0);

          if (first_write = '1') then
            mem_RGB_wr_addr <= (others => '0');
          else
            mem_RGB_wr_addr <= mem_RGB_wr_addr + 1;
            if (mem_RGB_wr_addr = video_height * video_width - 1) then
              mem_RGB_wr_addr <= (others => '0');
            end if;
          end if;

          conv_writemem_state <= stt_conv_writemem1;
          state <= stt_conv_readmem;
      end case;
    end WriteMemConvert;
    
  begin  -- process Converter
    if reset_n = '0' then               -- asynchronous reset (active low)
      state <= stt_read;
      conv_readmem_state <= stt_conv_readmem1;
      read_state <= stt_read_height;
      
      mem_Y_wr_e      <= '0';
      mem_Y_wr_addr   <= (others => '0');
      mem_Y_wr_data   <= (others => '0');
      mem_Y_rd_addr   <= (others => '0');
      mem_Y_cleaned   <= '0';
      
      mem_Cb_wr_e     <= '0';
      mem_Cb_wr_addr  <= (others => '0');
      mem_Cb_wr_data  <= (others => '0');
      mem_Cb_rd_addr   <= (others => '0');
      mem_Cb_cleaned  <= '0';
      
      mem_Cr_wr_e     <= '0';
      mem_Cr_wr_addr  <= (others => '0');
      mem_Cr_wr_data  <= (others => '0');
      mem_Cr_rd_addr   <= (others => '0');
      mem_Cr_cleaned  <= '0';
      
      mem_RGB_wr_e    <= '0';
      mem_RGB_wr_addr <= (others => '0');
      mem_RGB_wr_data <= (others => '0');
      mem_RGB_cleaned <= '0';
      
      first_clean <= '1';
      half_left_half_right <= '0';
      
      s_in_request <= '0';
      
    elsif Clk'event and Clk = '1' then  -- rising clock edge
      s_in_request <= '0';
      new_frame <= '0';
      case state is
        when stt_clean_buffers => CleanBuffers;
        when stt_read => ReadIn;
        when stt_conv_readmem => ReadMemConvert;
        when stt_conv_writemem => WriteMemConvert;
        when others =>
          ReadIn;
          state <= stt_read;
      end case;
    end if;
  end process Converter;
  
end a_YCbCr2RGB;
