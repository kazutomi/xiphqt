-------------------------------------------------------------------------------
--  Description: This file implements the loopfilter. A filter that do
--               a deblocking on the fragments.
-------------------------------------------------------------------------------
library std;
library ieee;
library work;
  
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use work.all;


entity LoopFilter is

  port (Clk,
        Reset_n       :       in std_logic;
        Enable        :       in std_logic;
        
        in_request    :       out std_logic;
        in_valid      :       in std_logic;
        in_data       :       in signed(31 downto 0);

        in_sem_request    :   out std_logic;
        in_sem_valid      :   in  std_logic;
        in_sem_addr       :   out unsigned(19 downto 0);
        in_sem_data       :   in  signed(31 downto 0);

        out_sem_requested :   in  std_logic;
        out_sem_valid     :   out std_logic;
        out_sem_addr      :   out unsigned(19 downto 0);
        out_sem_data      :   out signed(31 downto 0);

        out_done          :   out std_logic
        );
end LoopFilter;

architecture a_LoopFilter of LoopFilter is
  component syncram
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
  end component;

  component ReconPixelIndex
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

  component LFLimits
    port (
      parameter : in  unsigned(8 downto 0);
      FLimit    : in  signed(8 downto 0);
      fbv_value : out signed(9 downto 0));
  end component;

  
  -- We are using 1024 as the maximum width and height size
  -- = ceil(log2(Maximum Size))
  constant LG_MAX_SIZE    : natural := 10;
  constant MEM_ADDR_WIDTH : natural := 20;
  constant ZERO_ADDR_MEM  : unsigned(LG_MAX_SIZE*2 downto 0) := "000000000000000000000";
  
  -- This values must not be changed.
  constant MEM_DATA_WIDTH : natural := 32;
  
  subtype ogg_int32_t is signed(31 downto 0);
  subtype uchar_t is unsigned (7 downto 0);
  
  type mem_64_8bits_t is array (0 to 63) of uchar_t;

-- Fragment Parameters
  signal HFragments : unsigned(LG_MAX_SIZE-3 downto 0);
  signal VFragments : unsigned(LG_MAX_SIZE-3 downto 0);
  signal YStride    : unsigned(LG_MAX_SIZE+1 downto 0);
  signal UVStride   : unsigned(LG_MAX_SIZE   downto 0);
  signal YPlaneFragments : unsigned(LG_MAX_SIZE*2 downto 0);
  signal UVPlaneFragments : unsigned(LG_MAX_SIZE*2-2 downto 0);
  signal ReconYDataOffset : unsigned(MEM_ADDR_WIDTH-1 downto 0);
  signal ReconUDataOffset : unsigned(MEM_ADDR_WIDTH-1 downto 0);
  signal ReconVDataOffset : unsigned(MEM_ADDR_WIDTH-1 downto 0);
  signal UnitFragmets : unsigned(LG_MAX_SIZE*2 downto 0);

-- FLimits signals
  signal FLimit        : signed(8 downto 0);
  signal fbv_position  : unsigned(8 downto 0);
  signal fbv_value     : signed(9 downto 0);

-- ReconPixelIndex signals and constants
  constant RPI_DATA_WIDTH : positive := 32;
  constant RPI_POS_WIDTH : positive := 17;
  signal rpi_position : unsigned(RPI_POS_WIDTH-1 downto 0);
  signal rpi_value    : signed(RPI_DATA_WIDTH-1 downto 0);

  signal s_rpi_in_request    : std_logic;
  signal s_rpi_in_valid      : std_logic;
  signal s_rpi_in_data       : signed(31 downto 0);
        
  signal s_rpi_out_requested : std_logic;
  signal s_rpi_out_valid     : std_logic;
  signal s_rpi_out_data      : signed(31 downto 0);

  
-- Memories
  signal LoopFilterLimits : mem_64_8bits_t;

-- Process Signals  
  signal ThisFrameQualityValue : signed(31 downto 0);

  signal QIndex : unsigned(5 downto 0);
  signal pli : unsigned(1 downto 0);

  signal FragsAcross   : unsigned(LG_MAX_SIZE-3 downto 0);
  signal FragsDown     : unsigned(LG_MAX_SIZE-3 downto 0); 
  signal LineLength    : unsigned(LG_MAX_SIZE+1 downto 0);
  signal LineFragments : unsigned(LG_MAX_SIZE-3 downto 0);
  signal Fragment      : unsigned(LG_MAX_SIZE*2 downto 0);
  signal dpf_position  : unsigned(LG_MAX_SIZE*2 downto 0);

  signal MaxDPFCount : unsigned(LG_MAX_SIZE*2 downto 0);

  signal pixelPtr         : unsigned(MEM_ADDR_WIDTH-1 downto 0);
  signal DeltaHorizFilter : signed(3 downto 0);

  signal CountFilter  : unsigned(2 downto 0);
  signal CountColumns : unsigned(2 downto 0);

  signal disp_frag_value : std_logic;

  signal CountMiddles : unsigned(LG_MAX_SIZE*2 downto 0);
  signal CountMidCols : unsigned(LG_MAX_SIZE*2 downto 0);

-- Memories Signals
  signal mem_rd_data  : signed(31 downto 0);
  signal mem_rd_valid : std_logic;
  signal mem_wr_ready : std_logic;
  
-- FSMs
  type state_t is (readIn, proc);
  signal state : state_t;

  type read_state_t is (stt_qTT, stt_lfLim,
                        stt_dispFrag, stt_Others,
                        stt_32bitsData);
  signal read_state : read_state_t;

  
  type proc_state_t is (stt_ReadMemory, stt_WriteMemory,
                        stt_FindQIndex, stt_CalcFLimit,
                        stt_SelectColor, stt_ApplyFilter,
                        stt_CalcDispFragPos,
                        stt_CallFilterHoriz, stt_CalcFilterHoriz,
                        stt_CallFilterVert, stt_CalcFilterVert,
                        stt_Calc_RPI_Value);
  signal proc_state : proc_state_t;
  signal back_proc_state : proc_state_t;
  signal next_proc_state : proc_state_t;

  type calc_rpi_state_t is (stt_calc_rpi1, stt_calc_rpi2);
  signal calc_rpi_state : calc_rpi_state_t;

  type set_bound_val_state_t is (stt_SetBVal1, stt_SetBVal2, stt_SetBVal3, stt_SetBVal4);
  signal set_bound_val_state : set_bound_val_state_t;

  type calc_filter_state_t is (stt_CalcFilter1, stt_CalcFilter2,
                               stt_CalcFilter3);
  signal calc_filter_state : calc_filter_state_t;


  type apply_filter_state_t is (stt_ApplyFilter_1, stt_ApplyFilter_2,
                                stt_ApplyFilter_3, stt_ApplyFilter_4,
                                stt_ApplyFilter_5, stt_ApplyFilter_6,
                                stt_ApplyFilter_7, stt_ApplyFilter_8,
                                stt_ApplyFilter_9, stt_ApplyFilter_10,
                                stt_ApplyFilter_11, stt_ApplyFilter_12,
                                stt_ApplyFilter_13, stt_ApplyFilter_14,
                                stt_ApplyFilter_15, stt_ApplyFilter_16,
                                stt_ApplyFilter_17, stt_ApplyFilter_18,
                                stt_ApplyFilter_19, stt_ApplyFilter_20,
                                stt_ApplyFilter_21, stt_ApplyFilter_22,
                                stt_ApplyFilter_23, stt_ApplyFilter_24,
                                stt_ApplyFilter_25, stt_ApplyFilter_26,
                                stt_ApplyFilter_27, stt_ApplyFilter_28,
                                stt_ApplyFilter_29, stt_ApplyFilter_30,
                                stt_ApplyFilter_31, stt_ApplyFilter_32,
                                stt_ApplyFilter_33, stt_ApplyFilter_34);
  signal apply_filter_state : apply_filter_state_t;
  signal next_apply_filter_state : apply_filter_state_t;


  type  disp_frag_state_t is (stt_DispFrag1, stt_DispFrag2,
                              stt_DispFrag3, stt_DispFrag4,
                              stt_DispFrag5, stt_DispFrag6,
                              stt_DispFrag7, stt_DispFrag8,
                              stt_DispFrag9, stt_DispFrag10,
                              stt_DispFrag11, stt_DispFrag12,
                              stt_DispFrag13, stt_DispFrag14,
                              stt_DispFrag15, stt_DispFrag16,
                              stt_DispFrag17, stt_DispFrag18,
                              stt_DispFrag19, stt_DispFrag20,
                              stt_DispFrag21, stt_DispFrag22);
                              
  signal disp_frag_state : disp_frag_state_t;
  signal next_disp_frag_state : disp_frag_state_t;

  type  calc_disp_frag_state_t is (stt_CalcDispFrag1,
                                   stt_CalcDispFrag2,
                                   stt_CalcDispFrag3);
  signal calc_disp_frag_state : calc_disp_frag_state_t;
  
-- Handshake
  signal count : integer range 0 to 2097151;
  
  signal s_in_request : std_logic;

  signal s_in_sem_request : std_logic;
  signal s_out_sem_valid : std_logic;
  signal s_out_done : std_logic;

  signal lfr_OffSet  : unsigned(MEM_ADDR_WIDTH-1 downto 0);
  
  constant NULL_24bits : signed(23 downto 0) := "000000000000000000000000";
  constant NULL_32bits : signed(31 downto 0) := x"00000000";
  constant MAX_32bits  : signed(31 downto 0) := x"11111111";
  
  constant QTT_DEPTH : positive := 64;
  constant QTT_DATA_WIDTH : positive := 32;
  constant QTT_ADDR_WIDTH : positive := 6;

  constant DPF_DEPTH : positive := 57;
  constant DPF_DATA_WIDTH : positive := 32;
  constant DPF_ADDR_WIDTH : positive := 6;

-- Memories
  signal qtt_wr_e     : std_logic;
  signal qtt_wr_addr  : unsigned(QTT_ADDR_WIDTH-1 downto 0);
  signal qtt_wr_data  : signed(QTT_DATA_WIDTH-1 downto 0); 
  signal qtt_rd_addr  : unsigned(QTT_ADDR_WIDTH-1 downto 0);
  signal qtt_rd_data  : signed(QTT_DATA_WIDTH-1 downto 0);

  type mem4bytes_t is array (0 to 3) of uchar_t;
  signal Pixel : mem4bytes_t;
  type lfr_array_2_t is array (0 to 1) of ogg_int32_t;
  signal lfr_datas : lfr_array_2_t;
  type lfr_pos_pixels_t is array (0 to 1) of unsigned(1 downto 0);
  signal lfr_pos_pixels : lfr_pos_pixels_t;

  signal dpf_wr_e    : std_logic;
  signal dpf_wr_addr : unsigned(DPF_ADDR_WIDTH-1 downto 0);
  signal dpf_wr_data : signed(DPF_DATA_WIDTH-1 downto 0);
  signal dpf_rd_addr : unsigned(DPF_ADDR_WIDTH-1 downto 0);
  signal dpf_rd_data : signed(DPF_DATA_WIDTH-1 downto 0);

  
--   signal T_Pixel1 : ogg_int32_t;
--   signal T_Pixel2 : ogg_int32_t;

--   signal applyfilter_states : integer;
--   signal dispfragstates_states : integer;

  
begin  -- a_LoopFilter

  in_request <= s_in_request;
  in_sem_request <= s_in_sem_request;
  out_sem_valid <= s_out_sem_valid;
  out_done <= s_out_done;

  mem_64_int32: syncram
    generic map (QTT_DEPTH, QTT_DATA_WIDTH, QTT_ADDR_WIDTH)
    port map (clk, qtt_wr_e, qtt_wr_addr, qtt_wr_data, qtt_rd_addr, qtt_rd_data);

  mem_512_int32_1: syncram
    generic map (DPF_DEPTH, DPF_DATA_WIDTH, DPF_ADDR_WIDTH)
    port map (clk, dpf_wr_e, dpf_wr_addr, dpf_wr_data, dpf_rd_addr, dpf_rd_data);

  lflimits0: lflimits
    port map (fbv_position, FLimit, fbv_value);

  rpi0: reconpixelindex
    port map (Clk => Clk,
              Reset_n => Reset_n,
              in_request => s_rpi_out_requested,
              in_valid => s_rpi_out_valid,
              in_data => s_rpi_out_data,

              out_requested => s_rpi_in_request,
              out_valid => s_rpi_in_valid,
              out_data => s_rpi_in_data);


  RPI_HandShake: process (count, in_data, in_valid,
                          state, read_state, proc_state,
                          calc_rpi_state, rpi_position,
                          s_in_request)
  begin  -- process RPI_HandShake
    s_rpi_out_data <= x"00000000";
    s_rpi_out_valid <= '0';
    if (s_in_request = '1') then
      if (state = readIn and read_state = stt_32bitsData) then
        if (count >=0 and count <=8) then
          s_rpi_out_data <= in_data;
          s_rpi_out_valid <= in_valid;
        end if;
      end if;
    else
      if (state = proc and
          proc_state = stt_Calc_RPI_Value and
          calc_rpi_state = stt_calc_rpi1) then
        s_rpi_out_data <= resize(signed('0'&rpi_position), 32);
        s_rpi_out_valid <= '1';
      end if;
    end if;
  end process RPI_HandShake;

  
  process (clk)
-------------------------------------------------------------------------------
-- Procedures called when state is readIn
-------------------------------------------------------------------------------
    procedure Read32bitsData is
    begin
--      assert false report "in_data = "&integer'image(to_integer(in_data)) severity note;
      if (count = 0) then
        HFragments <= unsigned(in_data(LG_MAX_SIZE-3 downto 0));
        count <= count + 1;
      elsif (count = 1) then
        YPlaneFragments <= unsigned(in_data(LG_MAX_SIZE*2 downto 0));
        count <= count + 1;
      elsif (count = 2) then
        YStride <= unsigned(in_data(LG_MAX_SIZE+1 downto 0));
        count <= count + 1;
      elsif (count = 3) then
        UVPlaneFragments <= unsigned(in_data(LG_MAX_SIZE*2-2 downto 0));
        count <= count + 1;
      elsif (count = 4) then
        UVStride <= unsigned(in_data(LG_MAX_SIZE downto 0));
        count <= count + 1;
      elsif (count = 5) then
        VFragments <= unsigned(in_data(LG_MAX_SIZE-3 downto 0));
        count <= count + 1;
      elsif (count = 6) then
        ReconYDataOffset <= unsigned(in_data(MEM_ADDR_WIDTH-1 downto 0));
        count <= count + 1;
      elsif (count = 7) then
        ReconUDataOffset <= unsigned(in_data(MEM_ADDR_WIDTH-1 downto 0));
        count <= count + 1;
      elsif (count = 8) then
        ReconVDataOffset <= unsigned(in_data(MEM_ADDR_WIDTH-1 downto 0));
        count <= count + 1;
      else
        assert false report "UnitFragments = "&integer'image(to_integer(in_data)) severity note;
        UnitFragmets <= unsigned(in_data(LG_MAX_SIZE*2 downto 0));

        MaxDPFCount <= SHIFT_RIGHT(
          unsigned(in_data(LG_MAX_SIZE*2 downto 0)), 5) + 1;
        if (in_data(4 downto 0) = "00000") then
        MaxDPFCount <= SHIFT_RIGHT(
          unsigned(in_data(LG_MAX_SIZE*2 downto 0)), 5);
        end if;
        read_state <= stt_qTT;
        count <= 0;
      end if;
    end procedure Read32bitsData;

-------------------------------------------------------------------------------
-- Procedure that receives the QThreashTable matrice and keep the data
-- in a SRAM memory
-------------------------------------------------------------------------------    
    procedure QThreTab is
    begin
      qtt_wr_e <= '1';
      qtt_wr_data <= in_data;
      qtt_wr_addr <= qtt_wr_addr + 1;

      if (count = 0) then
        qtt_wr_addr <= "000000";
        count <= count + 1;
      elsif (count = 63) then
        read_state <= stt_lfLim;
        count <= 0;
        -- on next state must set qtt_wr_e to 0
      else
        count <= count + 1;
      end if;
    end procedure QThreTab;

-------------------------------------------------------------------------------
-- Procedure that receives the loop filter limits values and keep the data
-- in an internal memory
-------------------------------------------------------------------------------    
    procedure LfLim is
    begin
      qtt_wr_e <= '0';

      LoopFilterLimits(count + 3) <= unsigned(in_data(7 downto 0));
      LoopFilterLimits(count + 2) <= unsigned(in_data(15 downto 8));
      LoopFilterLimits(count + 1) <= unsigned(in_data(23 downto 16));
      LoopFilterLimits(count) <= unsigned(in_data(31 downto 24));
      if(count = 60)then
        read_state <= stt_dispFrag;
        count <= 0;
      else
        count <= count + 4;
      end if;
    end procedure LfLim;

-------------------------------------------------------------------------------
-- Procedure that receives the display fragments matrice and keep the data
-- in a SRAM memory
-------------------------------------------------------------------------------    
    procedure DispFrag is
    begin
      dpf_wr_e <= '1';
      dpf_wr_data <= in_data;
      dpf_wr_addr <= dpf_wr_addr + 1;
      if (count = 0) then
        dpf_wr_addr <= "000000";
        count <= 1;
      elsif (count = MaxDPFCount - 1) then
        read_state <= stt_Others;
        count <= 0;
      else
        count <= count + 1;
      end if;
    end procedure DispFrag;

-------------------------------------------------------------------------------
-- Procedure that receives the ThisFrameQualityValue and the Last Reconstructed
-- Frame offset
-------------------------------------------------------------------------------    
    procedure ReadOthers is
    begin
      if (count = 0) then
        ThisFrameQualityValue <= in_data;
        count <= count + 1;
      else
        lfr_OffSet <= SHIFT_RIGHT(unsigned(in_data(MEM_ADDR_WIDTH-1 downto 0)), 2);
        state <= proc;
        count <= 0;
        read_state <= stt_dispFrag;
        proc_state <= stt_FindQIndex;
        s_in_request <= '0';
        QIndex <= "111111";
      end if;
    end procedure ReadOthers;
    
-------------------------------------------------------------------------------
-- Procedure that controls the read state machine
-------------------------------------------------------------------------------    
    procedure ReadIn is
    begin
      s_out_done <= '0';
      s_in_request <= '1';

      s_out_sem_valid <= '0';
      s_in_sem_request <= '0';
      if (s_in_request = '1' and in_valid = '1') then

        case read_state is
          when stt_qTT => QThreTab;
          when stt_lfLim => LfLim;
          when stt_dispFrag => DispFrag;
          when stt_Others => ReadOthers;
          when others => Read32bitsData;
        end case;  
      end if;
    end procedure ReadIn;
    
-- *****************************************************
-- Procedures called when state is proc
-- *****************************************************

    procedure ReadMemory is
    begin
      -- After use the data mem_rd_valid must
      -- be set to '0'
      mem_rd_valid <= in_sem_valid;
      s_in_sem_request <= '1';
      if (s_in_sem_request = '1' and in_sem_valid = '1') then
        mem_rd_data <= in_sem_data;
        s_in_sem_request <= '0';
        proc_state <= back_proc_state;
      end if;
    end procedure ReadMemory;

    procedure WriteMemory is
    begin
      if (out_sem_requested = '1') then
        proc_state <= back_proc_state;
        mem_wr_ready <= '1';
        s_out_sem_valid <= '0';
      end if;
    end procedure WriteMemory;

    procedure CalcRPIValue is
    begin
      case calc_rpi_state is
        when stt_calc_rpi1 =>
          -- Wait until ReconPixelIndex can receive the data
          if (s_rpi_out_requested = '1') then
            calc_rpi_state <= stt_calc_rpi2;
          end if;


        when others =>
          -- Wait until ReconPixelIndex returns the value
          s_rpi_in_request <= '1';
          if (s_rpi_in_request = '1' and s_rpi_in_valid = '1') then
            rpi_value <= s_rpi_in_data;
            s_rpi_in_request <= '0';
            proc_state <= next_proc_state;
          end if;
      end case;
    end procedure CalcRPIValue;
    
-------------------------------------------------------------------------------
-- Initialize QIndex with 63.
-- For each element i of qtt in decreasing order, if i >= ThisFrameQualityValue
-- then stop, else subtract one of the QIndex and read the i-1 element of qtt,
-- until QIndex = 0
-------------------------------------------------------------------------------    
    procedure FindQIndex is
    begin
      if (count = 0) then
        qtt_rd_addr <= QIndex;
        count <= 1;
      elsif (count = 1) then
        qtt_rd_addr <= QIndex - 1;
        count <= 2;
      else
        if ((QIndex = "000000") or
            (qtt_rd_data >= ThisFrameQualityValue)) then
          proc_state <= stt_CalcFLimit;
          count <= 0;
        else
          qtt_rd_addr <= QIndex - 2;
          QIndex <= QIndex - 1;
          count <= 2;
        end if;
      end if;
    end procedure FindQIndex;

-------------------------------------------------------------------------------
-- If LoopFilterLimits[QIndex] is not zero do the loopfiltering in the frame.
-- The next procedure is SelectColor
-------------------------------------------------------------------------------
    procedure CalcFLimit is
    begin
      if (LoopFilterLimits(to_integer(QIndex)) /= "00000000000000000000000000000000") then
        proc_state <= stt_SelectColor;
        FLimit <= '0' & signed(LoopFilterLimits(to_integer(QIndex)));
      else
        pli <= "00";
        count <= 0;
        s_out_done <= '1';

        state <= readIn;
        read_state <= stt_dispFrag;
        proc_state <= stt_FindQIndex;
        apply_filter_state <= stt_ApplyFilter_1;
        calc_filter_state <= stt_CalcFilter1;
      end if;      
    end procedure CalcFLimit;

-------------------------------------------------------------------------------
-- Adjust some parameters according the plane of color
-------------------------------------------------------------------------------    
    procedure SelectColor is
    begin
      if (pli = "00") then
        FragsAcross <= HFragments;
        LineLength <= YStride;
        LineFragments <= HFragments;
        FragsDown <= VFragments;
        Fragment <= "000000000000000000000";
        proc_state <= stt_ApplyFilter;
        disp_frag_state <= stt_DispFrag1;
        pli <= pli + 1;

      elsif (pli = "01") then

        FragsAcross <= SHIFT_RIGHT(HFragments, 1);
        LineLength <= '0' & UVStride;
        LineFragments <= SHIFT_RIGHT(HFragments, 1);
        FragsDown <= SHIFT_RIGHT(VFragments, 1);
        Fragment <= YPlaneFragments;
        proc_state <= stt_ApplyFilter;
        disp_frag_state <= stt_DispFrag1;
        pli <= pli + 1;
        
      elsif (pli = "10") then

        FragsAcross <= SHIFT_RIGHT(HFragments, 1);
        LineLength <= '0' & UVStride;
        LineFragments <= SHIFT_RIGHT(HFragments, 1);
        FragsDown <= SHIFT_RIGHT(VFragments, 1);
        Fragment <= YPlaneFragments + UVPlaneFragments;
        proc_state <= stt_ApplyFilter;
        disp_frag_state <= stt_DispFrag1;
        pli <= pli + 1;
      else
        assert false report "SelectColor 4" severity note;
        pli <= "00";
        count <= 0;
        s_out_done <= '1';

        state <= readIn;
        read_state <= stt_dispFrag;
        proc_state <= stt_FindQIndex;
        apply_filter_state <= stt_ApplyFilter_1;
        calc_filter_state <= stt_CalcFilter1;
      end if;
    end procedure SelectColor;

    
    procedure CallFilterHoriz is
      variable fourPixels : signed(MEM_DATA_WIDTH-1 downto 0);
      variable memPosPixel : unsigned(1 downto 0);
      variable numPixel : signed(RPI_DATA_WIDTH-1 downto 0);

    begin
      numPixel := rpi_value +
                  DeltaHorizFilter +
                  ('0' & signed(pixelPtr) + count);
      memPosPixel := unsigned(numPixel(1 downto 0));

      -- When use the data mem_rd_valid must
      -- be set to '0'
      if (mem_rd_valid = '0') then
        s_in_sem_request <= '1';
        in_sem_addr <= lfr_OffSet +
                       resize(
                         SHIFT_RIGHT('0' & unsigned(numPixel), 2)
                         , MEM_ADDR_WIDTH
                         );
        back_proc_state <= stt_CallFilterHoriz;
        proc_state <= stt_ReadMemory;
      else
        mem_rd_valid <= '0';
        fourPixels := (SHIFT_RIGHT(
          mem_rd_data,
          24 -
          to_integer(memPosPixel) * 8));
        Pixel(count) <= unsigned(fourPixels(7 downto 0));

        if (count = 1 or count = 2) then
          -- Saves the second or third pixel data slot and
          -- their positions in the slot
          lfr_datas(count - 1) <= mem_rd_data;
          lfr_pos_pixels(count - 1) <= memPosPixel;
        end if;
        if (count = 3) then
          fbv_position <= resize(unsigned(
            256 +
            SHIFT_RIGHT(
              ("0000" & 
               signed(Pixel(0))) -
              (("0000" &
                signed(Pixel(1))) * 3) +
              (("0000" &
                signed(Pixel(2))) * 3) -
              ("0000" &
               signed(fourPixels(7 downto 0))) + 4
              , 3)), 9);

          count <= 0;
          proc_state <= stt_CalcFilterHoriz;
        else
          count <= count + 1;
        end if;
      end if;
    end procedure CallFilterHoriz;

    procedure CalcFilterHoriz is
      variable Pixel1 : ogg_int32_t;
      variable Pixel2 : ogg_int32_t;
      variable newPixel : uchar_t;
      
    begin
      if (calc_filter_state = stt_CalcFilter1) then
        
        Pixel1 := NULL_24bits &
                  signed(Pixel(1)) + fbv_value;
--        T_Pixel1 <= Pixel1;
        
        out_sem_addr <= lfr_OffSet +
                        resize(
                          SHIFT_RIGHT(
                            '0' &
                            unsigned(
                              rpi_value +
                              DeltaHorizFilter +
                              ('0' & signed(pixelPtr)) + 1)
                            ,2
                            ),  MEM_ADDR_WIDTH
                          );

        if (Pixel1 < "00000000000000000000000000000000") then
          newPixel := "00000000";
        elsif (Pixel1 > "00000000000000000000000011111111") then
          newPixel := "11111111";
        else
          newPixel := unsigned(Pixel1(7 downto 0));
        end if;

        case lfr_pos_pixels(0) is
          when "00" =>
            out_sem_data <= signed(newPixel) &
                            lfr_datas(0)(23 downto 0);
          when "01" =>
            out_sem_data <= lfr_datas(0)(31 downto 24) &
                            signed(newPixel) &
                            lfr_datas(0)(15 downto 0);
          when "10" =>
            out_sem_data <= lfr_datas(0)(31 downto 16) &
                            signed(newPixel) &
                            lfr_datas(0)(7 downto 0);
          when others =>
            out_sem_data <= lfr_datas(0)(31 downto 8) &
                            signed(newPixel);
        end case;  

        s_out_sem_valid <= '1';
        calc_filter_state <= stt_CalcFilter2;
        back_proc_state <= stt_CalcFilterHoriz;
        proc_state <= stt_WriteMemory;

      elsif (calc_filter_state = stt_CalcFilter2) then
        Pixel2 := NULL_24bits &
                  signed(Pixel(2)) - fbv_value;

--        T_Pixel2 <= Pixel2;
        
        out_sem_addr <= lfr_OffSet +
                        resize(
                          SHIFT_RIGHT(
                            '0' &
                            unsigned(
                              rpi_value +
                              DeltaHorizFilter +
                              ('0' & signed(pixelPtr)) + 2)
                            ,2
                            ),  MEM_ADDR_WIDTH
                          );


        if (Pixel2 < "00000000000000000000000000000000") then
          newPixel := "00000000";
        elsif (Pixel2 > "00000000000000000000000011111111") then
          newPixel := "11111111";
        else
          newPixel := unsigned(Pixel2(7 downto 0));
        end if;

        case lfr_pos_pixels(1) is
          when "00" =>
            out_sem_data <= signed(newPixel) &
                            lfr_datas(1)(23 downto 0);
          when "01" =>
            out_sem_data <= lfr_datas(1)(31 downto 24) &
                            signed(newPixel) &
                            lfr_datas(1)(15 downto 0);
          when "10" =>
            out_sem_data <= lfr_datas(1)(31 downto 16) &
                            signed(newPixel) &
                            lfr_datas(1)(7 downto 0);
          when others =>
            out_sem_data <= lfr_datas(1)(31 downto 8) &
                            signed(newPixel);
        end case;  

        s_out_sem_valid <= '1';
        calc_filter_state <= stt_CalcFilter3;
        back_proc_state <= stt_CalcFilterHoriz;
        proc_state <= stt_WriteMemory;

      else
        
        if (CountFilter = "111") then
          proc_state <= stt_ApplyFilter;
          apply_filter_state <= next_apply_filter_state;  -- Next state
          pixelPtr <= "00000000000000000000";
          CountFilter <= "000";
        else
          pixelPtr <= pixelPtr + LineLength;  -- Next Row
          proc_state <= stt_CallFilterHoriz;
          CountFilter <= CountFilter + 1;
        end if;
        calc_filter_state <= stt_CalcFilter1;
      end if;
    end procedure CalcFilterHoriz;

   
    procedure CallFilterVert is
      variable fourPixels : signed(MEM_DATA_WIDTH-1 downto 0);
      variable memPosPixel : unsigned(1 downto 0);
      variable numPixel : signed(RPI_DATA_WIDTH-1 downto 0);
      variable posLine : signed(2 downto 0);
    begin
      if (count = 0) then
        posLine := "110";
      elsif (count = 1) then
        posLine := "111";
      elsif (count = 2) then
        posLine := "000";
      else
        posLine := "001";
      end if;

      numPixel := rpi_value +
                  (signed('0' & LineLength)*posLine) +
                  ('0' & signed(CountColumns));
      memPosPixel := unsigned(numPixel(1 downto 0));

      -- When use the data mem_rd_valid must
      -- be set to '0'

      if (mem_rd_valid = '0') then
        s_in_sem_request <= '1';
        in_sem_addr <= lfr_OffSet +
                       resize(
                         SHIFT_RIGHT('0' & unsigned(numPixel), 2)
                         , MEM_ADDR_WIDTH
                         );

        if (to_integer(lfr_OffSet +
                       resize(
                         SHIFT_RIGHT('0' & unsigned(numPixel), 2)
                         , MEM_ADDR_WIDTH
                         )) = 1530) then
        end if;

        
        back_proc_state <= stt_CallFilterVert;
        proc_state <= stt_ReadMemory;

      else
        mem_rd_valid <= '0';

        fourPixels := (SHIFT_RIGHT(
          mem_rd_data,
          24 -
          to_integer(memPosPixel) * 8));

        Pixel(count) <= unsigned(fourPixels(7 downto 0));

        if (count = 1 or count = 2) then
          -- Saves the second or third pixel data slot and
          -- their positions in the slot
          lfr_datas(count - 1) <= mem_rd_data;
          lfr_pos_pixels(count - 1) <= memPosPixel;
        end if;
        if (count = 3) then
          fbv_position <= resize(unsigned(
            256 +
            SHIFT_RIGHT(
              ("0000" & 
               signed(Pixel(0))) -
              (("0000" &
                signed(Pixel(1))) * 3) +
              (("0000" &
                signed(Pixel(2))) * 3) -
              ("0000" &
               signed(fourPixels(7 downto 0))) + 4
              , 3)), 9);
          count <= 0;
          proc_state <= stt_CalcFilterVert;
        else
          count <= count + 1;
        end if;
      end if;
    end procedure CallFilterVert;

    procedure CalcFilterVert is
      variable Pixel1 : ogg_int32_t;
      variable Pixel2 : ogg_int32_t;
      variable newPixel : uchar_t;
      
    begin

      if (calc_filter_state = stt_CalcFilter1) then

        Pixel1 := (NULL_24bits &
                   signed(Pixel(1))) + fbv_value;
--        T_Pixel1 <= Pixel1;
        out_sem_addr <= lfr_OffSet +
                        resize(
                          SHIFT_RIGHT(
                            '0' &
                            unsigned(
                              rpi_value -
                              ('0' & signed(LineLength)) +
                              ('0' & signed(CountColumns)))
                            ,2
                            ),  MEM_ADDR_WIDTH
                          );

        if (Pixel1 < "00000000000000000000000000000000") then
          newPixel := "00000000";
        elsif (Pixel1 > "00000000000000000000000011111111") then
          newPixel := "11111111";
        else
          newPixel := unsigned(Pixel1(7 downto 0));
        end if;

        case lfr_pos_pixels(0) is
          when "00" =>
            out_sem_data <= signed(newPixel) &
                           lfr_datas(0)(23 downto 0);
          when "01" =>
            out_sem_data <= lfr_datas(0)(31 downto 24) &
                           signed(newPixel) &
                           lfr_datas(0)(15 downto 0);
          when "10" =>
            out_sem_data <= lfr_datas(0)(31 downto 16) &
                           signed(newPixel) &
                           lfr_datas(0)(7 downto 0);
          when others =>
            out_sem_data <= lfr_datas(0)(31 downto 8) &
                           signed(newPixel);
        end case;  

        s_out_sem_valid <= '1';
        calc_filter_state <= stt_CalcFilter2;
        back_proc_state <= stt_CalcFilterVert;
        proc_state <= stt_WriteMemory;

      elsif (calc_filter_state = stt_CalcFilter2) then

        Pixel2 := (NULL_24bits &
                   signed(Pixel(2))) - fbv_value;
--        T_Pixel2 <= Pixel2;
        
        out_sem_addr <= lfr_OffSet +
                        resize(
                          SHIFT_RIGHT(
                            '0' &
                            unsigned(
                              rpi_value +
                              ('0' & signed(CountColumns)))
                            ,2
                            ),  MEM_ADDR_WIDTH
                          );

        if (Pixel2 < "00000000000000000000000000000000") then
          newPixel := "00000000";
        elsif (Pixel2 > "00000000000000000000000011111111") then
          newPixel := "11111111";
        else
          newPixel := unsigned(Pixel2(7 downto 0));
        end if;

        case lfr_pos_pixels(1) is
          when "00" =>
            out_sem_data <= signed(newPixel) &
                           lfr_datas(1)(23 downto 0);
          when "01" =>
            out_sem_data <= lfr_datas(1)(31 downto 24) &
                           signed(newPixel) &
                            lfr_datas(1)(15 downto 0);
          when "10" =>
            out_sem_data <= lfr_datas(1)(31 downto 16) &
                            signed(newPixel) &
                            lfr_datas(1)(7 downto 0);
          when others =>
            out_sem_data <= lfr_datas(1)(31 downto 8) &
                            signed(newPixel);
        end case;  

        s_out_sem_valid <= '1';
        calc_filter_state <= stt_CalcFilter3;
        back_proc_state <= stt_CalcFilterVert;
        proc_state <= stt_WriteMemory;

      else

        if (CountFilter = "111") then
          proc_state <= stt_ApplyFilter;
          apply_filter_state <= next_apply_filter_state;  -- Next state
          CountFilter <= "000";
          CountColumns <= "000";
        else
          CountColumns <= CountColumns + 1;
          proc_state <= stt_CallFilterVert;
          CountFilter <= CountFilter + 1;
        end if;
        calc_filter_state <= stt_CalcFilter1;
      end if;
    end procedure CalcFilterVert;


    procedure CalcDispFragPos is
    begin
      if (calc_disp_frag_state = stt_CalcDispFrag1) then
        -- Wait display_fragments memory
        calc_disp_frag_state <= stt_CalcDispFrag2;

      else
        disp_frag_value <= dpf_rd_data(31 - to_integer(dpf_position(4 downto 0)));
        calc_disp_frag_state <= stt_CalcDispFrag1;
        proc_state <= stt_ApplyFilter;
        apply_filter_state <= next_apply_filter_state;  -- Next state
        disp_frag_state <= next_disp_frag_state;
      end if;
    end procedure CalcDispFragPos;    

    
    procedure ApplyFilter is
      variable NextFragment : unsigned(LG_MAX_SIZE*2 downto 0);
    begin

      if (apply_filter_state = stt_ApplyFilter_1) then
--        applyfilter_states <= 1;
        -- ******************************************************
        -- First Row
        -- ******************************************************
        -- First column coditions
        -- only do 2 prediction if fragment coded and on non intra
        -- or if all fragments are intra
        if (disp_frag_state = stt_DispFrag1) then
--          dispfragstates_states <= 1;
          -- dpf_rd_addr <= resize(SHIFT_RIGHT(Fragment,5),  DPF_ADDR_WIDTH);
          dpf_rd_addr <= Fragment((4+DPF_ADDR_WIDTH) downto 5);
          dpf_position <= Fragment;
          proc_state <= stt_CalcDispFragPos;
          next_apply_filter_state <= stt_ApplyFilter_1;
          next_disp_frag_state <= stt_DispFrag2;
        else
          if (disp_frag_value = '1') then
            apply_filter_state <= stt_ApplyFilter_34;
          else
            apply_filter_state <= stt_ApplyFilter_3;
          end if;
        end if;
        
      elsif (apply_filter_state = stt_ApplyFilter_34) then
--        applyfilter_states <= 34;
        -- Filter right hand border only if the block to the right
        -- is not coded
        if (disp_frag_state = stt_DispFrag2) then
--          dispfragstates_states <= 2;
          NextFragment := Fragment + 1;
          dpf_rd_addr <= NextFragment((4+DPF_ADDR_WIDTH) downto 5);
          dpf_position <= Fragment + 1;
          proc_state <= stt_CalcDispFragPos;
          next_apply_filter_state <= stt_ApplyFilter_34;
          next_disp_frag_state <= stt_DispFrag3;
        else
          if (disp_frag_value = '0') then
            rpi_position <= resize(Fragment, RPI_POS_WIDTH);
            -- Horizontal Filter Parameter
            DeltaHorizFilter <= x"6";

            -- Calculate RPI_Value before continue
            proc_state <= stt_Calc_RPI_Value;
            calc_rpi_state <= stt_calc_rpi1;
            -- Next state after RPI_Value calculation
            next_proc_state <= stt_CallFilterHoriz;
            next_apply_filter_state <= stt_ApplyFilter_2;
          else
            apply_filter_state <= stt_ApplyFilter_2;
          end if;
        end if;
  
      elsif (apply_filter_state = stt_ApplyFilter_2) then
--        applyfilter_states <= 2;
        -- Bottom done if next row set
        if (disp_frag_state = stt_DispFrag3) then
          NextFragment := Fragment + LineFragments;
          dpf_rd_addr <= NextFragment((4+DPF_ADDR_WIDTH) downto 5);
          dpf_position <= Fragment + LineFragments;
          proc_state <= stt_CalcDispFragPos;
          next_apply_filter_state <= stt_ApplyFilter_2;
          next_disp_frag_state <= stt_DispFrag4;
        else
          if (disp_frag_value = '0') then
            rpi_position <= resize(Fragment +  LineFragments, RPI_POS_WIDTH);

            -- Calculate RPI_Value before continue
            proc_state <= stt_Calc_RPI_Value;
            calc_rpi_state <= stt_calc_rpi1;
            -- Next state after RPI_Value calculation
            next_proc_state <= stt_CallFilterVert;
            next_apply_filter_state <= stt_ApplyFilter_3;
          else
            apply_filter_state <= stt_ApplyFilter_3;
          end if;
        end if;
        
      elsif (apply_filter_state = stt_ApplyFilter_3) then
--        applyfilter_states <= 3;
        Fragment <= Fragment + 1;
        CountMiddles <= '0' & x"00001";
        apply_filter_state <= stt_ApplyFilter_4;
        disp_frag_state <= stt_DispFrag4;
        
      elsif (apply_filter_state = stt_ApplyFilter_4) then

        if (CountMiddles < FragsAcross - 1) then

          -- Middle Columns

          if (disp_frag_state = stt_DispFrag4) then
            dpf_rd_addr <= Fragment((4+DPF_ADDR_WIDTH) downto 5);
            dpf_position <= Fragment;
            proc_state <= stt_CalcDispFragPos;
            next_apply_filter_state <= stt_ApplyFilter_4;
            next_disp_frag_state <= stt_DispFrag5;

          else

            if (disp_frag_value = '1') then
              -- Filter Left edge always
              rpi_position <= resize(Fragment, RPI_POS_WIDTH);
              -- Horizontal Filter Parameter
              DeltaHorizFilter <= x"E";
              
              -- Calculate RPI_Value before continue
              proc_state <= stt_Calc_RPI_Value;
              calc_rpi_state <= stt_calc_rpi1;
              -- Next state after RPI_Value calculation
              next_proc_state <= stt_CallFilterHoriz;
              
              next_apply_filter_state <= stt_ApplyFilter_5;
            else
              apply_filter_state <= stt_ApplyFilter_7;  -- Increment CountMiddles
            end if;
          end if;
        else
          apply_filter_state <= stt_ApplyFilter_8;
          disp_frag_state <= stt_DispFrag7;
        end if;


        
      elsif (apply_filter_state = stt_ApplyFilter_5) then

        -- Enter here only if (CountMiddles < FragsAcross - 1) is true
        -- and display_fragments(Fragment) is not zero
        if (disp_frag_state = stt_DispFrag5) then
          NextFragment := Fragment + 1;
          dpf_rd_addr <= NextFragment((4+DPF_ADDR_WIDTH) downto 5);

          dpf_position <= Fragment + 1;
          proc_state <= stt_CalcDispFragPos;
          next_apply_filter_state <= stt_ApplyFilter_5;
          next_disp_frag_state <= stt_DispFrag6;

        else

          if (disp_frag_value = '0') then

            -- Filter right hand border only if the block to the right is
            -- not coded
            rpi_position <= resize(Fragment, RPI_POS_WIDTH);
            -- Horizontal Filter Parameter
            DeltaHorizFilter <= x"6";

            -- Calculate RPI_Value before continue
            proc_state <= stt_Calc_RPI_Value;
            calc_rpi_state <= stt_calc_rpi1;
            -- Next state after RPI_Value calculation
            next_proc_state <= stt_CallFilterHoriz;
            next_apply_filter_state <= stt_ApplyFilter_6;

          else
            apply_filter_state <= stt_ApplyFilter_6;
          end if;
        end if;

      elsif (apply_filter_state = stt_ApplyFilter_6) then 

        -- Enter here only if (CountMiddles < FragsAcross - 1) is true
        -- and display_fragments(Fragment) is not zero
        if (disp_frag_state = stt_DispFrag6) then
          NextFragment := Fragment + LineFragments;
          dpf_rd_addr <= NextFragment((4+DPF_ADDR_WIDTH) downto 5);

          dpf_position <= Fragment + LineFragments;
          proc_state <= stt_CalcDispFragPos;
          next_apply_filter_state <= stt_ApplyFilter_6;
          next_disp_frag_state <= stt_DispFrag7;

        else

          if (disp_frag_value = '0') then

            -- Bottom done if next row set
            rpi_position <= resize(Fragment + LineFragments, RPI_POS_WIDTH);

            -- Calculate RPI_Value before continue
            proc_state <= stt_Calc_RPI_Value;
            calc_rpi_state <= stt_calc_rpi1;
            -- Next state after RPI_Value calculation
            next_proc_state <= stt_CallFilterVert;
            next_apply_filter_state <= stt_ApplyFilter_7;
          else
            apply_filter_state <= stt_ApplyFilter_7;
          end if;
        end if;

      elsif (apply_filter_state = stt_ApplyFilter_7) then

        CountMiddles <= CountMiddles + 1;
        Fragment <= Fragment + 1;
        apply_filter_state <= stt_ApplyFilter_4;
        disp_frag_state <= stt_DispFrag4;


      elsif (apply_filter_state = stt_ApplyFilter_8) then

        -- ******************************************************
        -- Last Column
        -- ******************************************************
        if (disp_frag_state = stt_DispFrag7) then
          dpf_rd_addr <= Fragment((4+DPF_ADDR_WIDTH) downto 5);

          dpf_position <= Fragment;
          proc_state <= stt_CalcDispFragPos;
          next_apply_filter_state <= stt_ApplyFilter_8;
          next_disp_frag_state <= stt_DispFrag8;

        else

          if (disp_frag_value = '1') then
            -- Filter Left edge always
            rpi_position <= resize(Fragment, RPI_POS_WIDTH);
            -- Horizontal Filter Parameter
            DeltaHorizFilter <= x"E";

            -- Calculate RPI_Value before continue
            proc_state <= stt_Calc_RPI_Value;
            calc_rpi_state <= stt_calc_rpi1;
            -- Next state after RPI_Value calculation
            next_proc_state <= stt_CallFilterHoriz;
            next_apply_filter_state <= stt_ApplyFilter_9;
          else
            apply_filter_state <= stt_ApplyFilter_10;
          end if;
        end if;
        
      elsif (apply_filter_state = stt_ApplyFilter_9) then

        if (disp_frag_state = stt_DispFrag8) then
          NextFragment := Fragment + LineFragments;
          dpf_rd_addr <= NextFragment((4+DPF_ADDR_WIDTH) downto 5);

          dpf_position <= Fragment + LineFragments;
          proc_state <= stt_CalcDispFragPos;
          next_apply_filter_state <= stt_ApplyFilter_9;
          next_disp_frag_state <= stt_DispFrag9;

        else
          if (disp_frag_value = '0') then
            -- Bottom done if next row set
            rpi_position <= resize(Fragment + LineFragments, RPI_POS_WIDTH);

            -- Calculate RPI_Value before continue
            proc_state <= stt_Calc_RPI_Value;
            calc_rpi_state <= stt_calc_rpi1;
            -- Next state after RPI_Value calculation
            next_proc_state <= stt_CallFilterVert;
            next_apply_filter_state <= stt_ApplyFilter_10;
          else
            apply_filter_state <= stt_ApplyFilter_10;
          end if;
        end if;
        
      elsif (apply_filter_state = stt_ApplyFilter_10) then

        Fragment <= Fragment + 1;
        CountMiddles <= '0' & x"00001";
        apply_filter_state <= stt_ApplyFilter_11;
        disp_frag_state <= stt_DispFrag9;

      elsif (apply_filter_state = stt_ApplyFilter_11) then


        -- ******************************************************
        -- Middle Rows
        -- ******************************************************
        if (CountMiddles < FragsDown - 1) then
          -- first column conditions
          -- only do 2 prediction if fragment coded and on non intra or if
          -- all fragments are intra */
          if (disp_frag_state = stt_DispFrag9) then
            dpf_rd_addr <= Fragment((4+DPF_ADDR_WIDTH) downto 5);

            dpf_position <= Fragment;
            proc_state <= stt_CalcDispFragPos;
            next_apply_filter_state <= stt_ApplyFilter_11;
            next_disp_frag_state <= stt_DispFrag10;
          else
            if (disp_frag_value = '1') then
              -- TopRow is always done
              rpi_position <= resize(Fragment, RPI_POS_WIDTH);

              -- Calculate RPI_Value before continue
              proc_state <= stt_Calc_RPI_Value;
              calc_rpi_state <= stt_calc_rpi1;
              -- Next state after RPI_Value calculation
              next_proc_state <= stt_CallFilterVert;
              next_apply_filter_state <= stt_ApplyFilter_12;
            else
              apply_filter_state <= stt_ApplyFilter_14;  -- Do middle columns
            end if;
          end if;
        else
          apply_filter_state <= stt_ApplyFilter_24;    -- End "Loop"
          disp_frag_state <= stt_DispFrag17;
        end if;

        
      elsif (apply_filter_state = stt_ApplyFilter_12) then

        -- Enter here only if (CountMiddles < FragsDown - 1) is true
        -- and display_fragments(Fragment) is not zero
        if (disp_frag_state = stt_DispFrag10) then
          NextFragment := Fragment + 1;
          dpf_rd_addr <= NextFragment((4+DPF_ADDR_WIDTH) downto 5);

          dpf_position <= Fragment + 1;
          proc_state <= stt_CalcDispFragPos;
          next_apply_filter_state <= stt_ApplyFilter_12;
          next_disp_frag_state <= stt_DispFrag11;
        else

          if (disp_frag_value = '0') then
            -- Filter right hand border only if the block to the right is
            -- not coded
            rpi_position <= resize(Fragment, RPI_POS_WIDTH);
            -- Horizontal Filter Parameter
            DeltaHorizFilter <= x"6";

            -- Calculate RPI_Value before continue
            proc_state <= stt_Calc_RPI_Value;
            calc_rpi_state <= stt_calc_rpi1;
            -- Next state after RPI_Value calculation
            next_proc_state <= stt_CallFilterHoriz;
            next_apply_filter_state <= stt_ApplyFilter_13;
          else
            apply_filter_state <= stt_ApplyFilter_13;
          end if;
        end if;

      elsif (apply_filter_state = stt_ApplyFilter_13) then

        -- Enter here only if (CountMiddles < FragsDown - 1) is true
        -- and display_fragments(Fragment) is not zero
        if (disp_frag_state = stt_DispFrag11) then
          NextFragment := Fragment + LineFragments;
          dpf_rd_addr <= NextFragment((4+DPF_ADDR_WIDTH) downto 5);

          dpf_position <= Fragment + LineFragments;
          proc_state <= stt_CalcDispFragPos;
          next_apply_filter_state <= stt_ApplyFilter_13;
          next_disp_frag_state <= stt_DispFrag12;
        else

          if (disp_frag_value = '0') then
            -- Bottom done if next row set
            rpi_position <= resize(Fragment + LineFragments, RPI_POS_WIDTH);

            -- Calculate RPI_Value before continue
            proc_state <= stt_Calc_RPI_Value;
            calc_rpi_state <= stt_calc_rpi1;
            -- Next state after RPI_Value calculation
            next_proc_state <= stt_CallFilterVert;
            next_apply_filter_state <= stt_ApplyFilter_14;
          else
            apply_filter_state <= stt_ApplyFilter_14;
          end if;
        end if;


      elsif (apply_filter_state = stt_ApplyFilter_14) then

        Fragment <= Fragment + 1;       -- Increment position
        CountMidCols <= '0' & x"00001"; -- Initialize Counter
        apply_filter_state <= stt_ApplyFilter_15;
        disp_frag_state <= stt_DispFrag12;
        
      elsif (apply_filter_state = stt_ApplyFilter_15) then

        -- ******************************************************
        -- Middle Columns inside Middle Rows
        -- ******************************************************
        if (CountMidCols < FragsAcross - 1) then

          if (disp_frag_state = stt_DispFrag12) then
            dpf_rd_addr <= Fragment((4+DPF_ADDR_WIDTH) downto 5);

            dpf_position <= Fragment;
            proc_state <= stt_CalcDispFragPos;
            next_apply_filter_state <= stt_ApplyFilter_15;
            next_disp_frag_state <= stt_DispFrag13;
          else
            if (disp_frag_value = '1') then
              -- Filter Left edge always
              rpi_position <= resize(Fragment, RPI_POS_WIDTH);
              -- Horizontal Filter Parameter
              DeltaHorizFilter <= x"E";

              -- Calculate RPI_Value before continue
              proc_state <= stt_Calc_RPI_Value;
              calc_rpi_state <= stt_calc_rpi1;
              -- Next state after RPI_Value calculation
              next_proc_state <= stt_CallFilterHoriz;
              next_apply_filter_state <= stt_ApplyFilter_16;
            else
              apply_filter_state <= stt_ApplyFilter_19;  -- Increment CountMidCols
            end if;
          end if;
        else

          apply_filter_state <= stt_ApplyFilter_20;    -- End "Loop" and
                                                       -- do Last Column
          disp_frag_state <= stt_DispFrag15;
        end if;

        
      elsif (apply_filter_state = stt_ApplyFilter_16) then

        -- Enter here only if (CountMidCols < FragsAcross - 1) is true
        -- and display_fragments(Fragment) is not zero
        
        -- TopRow is always done
        rpi_position <= resize(Fragment, RPI_POS_WIDTH);

        -- Calculate RPI_Value before continue
        proc_state <= stt_Calc_RPI_Value;
        calc_rpi_state <= stt_calc_rpi1;
        -- Next state after RPI_Value calculation
        next_proc_state <= stt_CallFilterVert;
        next_apply_filter_state <= stt_ApplyFilter_17;


      elsif (apply_filter_state = stt_ApplyFilter_17) then

        -- Enter here only if (CountMidCols < FragsAcross - 1) is true
        -- and display_fragments(Fragment) is not zero

        -- Filter right hand border only if the block to the right
        -- is not coded
        if (disp_frag_state = stt_DispFrag13) then
          NextFragment := Fragment + 1;
          dpf_rd_addr <= NextFragment((4+DPF_ADDR_WIDTH) downto 5);

          dpf_position <= Fragment + 1;
          proc_state <= stt_CalcDispFragPos;
          next_apply_filter_state <= stt_ApplyFilter_17;
          next_disp_frag_state <= stt_DispFrag14;
        else
          if (disp_frag_value = '0') then
            rpi_position <= resize(Fragment, RPI_POS_WIDTH);
            -- Horizontal Filter Parameter
            DeltaHorizFilter <= x"6";

            -- Calculate RPI_Value before continue
            proc_state <= stt_Calc_RPI_Value;
            calc_rpi_state <= stt_calc_rpi1;
            -- Next state after RPI_Value calculation
            next_proc_state <= stt_CallFilterHoriz;
            next_apply_filter_state <= stt_ApplyFilter_18;
          else
            apply_filter_state <= stt_ApplyFilter_18;
          end if;
        end if;

      elsif (apply_filter_state = stt_ApplyFilter_18) then

        -- Enter here only if (CountMidCols < FragsAcross - 1) is true
        -- and display_fragments(Fragment) is not zero

        -- Bottom done if next row set
        if (disp_frag_state = stt_DispFrag14) then
          NextFragment := Fragment + LineFragments;
          dpf_rd_addr <= NextFragment((4+DPF_ADDR_WIDTH) downto 5);

          dpf_position <= Fragment + LineFragments;
          proc_state <= stt_CalcDispFragPos;
          next_apply_filter_state <= stt_ApplyFilter_18;
          next_disp_frag_state <= stt_DispFrag15;
        else
          if (disp_frag_value = '0') then
            rpi_position <= resize(Fragment + LineFragments, RPI_POS_WIDTH);

            -- Calculate RPI_Value before continue
            proc_state <= stt_Calc_RPI_Value;
            calc_rpi_state <= stt_calc_rpi1;
            -- Next state after RPI_Value calculation
            next_proc_state <= stt_CallFilterVert;
            next_apply_filter_state <= stt_ApplyFilter_19;
          else
            apply_filter_state <= stt_ApplyFilter_19;
          end if;
        end if;

      elsif (apply_filter_state = stt_ApplyFilter_19) then

        CountMidCols <= CountMidCols + 1;
        Fragment <= Fragment + 1;
        apply_filter_state <= stt_ApplyFilter_15;
        disp_frag_state <= stt_DispFrag12;
        
      elsif (apply_filter_state = stt_ApplyFilter_20) then

        -- Last Column

        if (disp_frag_state = stt_DispFrag15) then
          dpf_rd_addr <= Fragment((4+DPF_ADDR_WIDTH) downto 5);
          
          dpf_position <= Fragment;
          proc_state <= stt_CalcDispFragPos;
          next_apply_filter_state <= stt_ApplyFilter_20;
          next_disp_frag_state <= stt_DispFrag16;
        else
          if (disp_frag_value = '1') then
            -- Filter Left edge always
            rpi_position <= resize(Fragment, RPI_POS_WIDTH);
            -- Horizontal Filter Parameter
            DeltaHorizFilter <= x"E";

            -- Calculate RPI_Value before continue
            proc_state <= stt_Calc_RPI_Value;
            calc_rpi_state <= stt_calc_rpi1;
            -- Next state after RPI_Value calculation
            next_proc_state <= stt_CallFilterHoriz;
            next_apply_filter_state <= stt_ApplyFilter_21;
          else
            apply_filter_state <= stt_ApplyFilter_23;
          end if;
        end if;

      elsif (apply_filter_state = stt_ApplyFilter_21) then

        -- Enter here only if display_fragments(Fragment) not zero

        -- TopRow is always done
        rpi_position <= resize(Fragment, RPI_POS_WIDTH);

        -- Calculate RPI_Value before continue
        proc_state <= stt_Calc_RPI_Value;
        calc_rpi_state <= stt_calc_rpi1;
        -- Next state after RPI_Value calculation
        next_proc_state <= stt_CallFilterVert;
        next_apply_filter_state <= stt_ApplyFilter_22;

        
      elsif (apply_filter_state = stt_ApplyFilter_22) then
        -- Enter here only if display_fragments(Fragment) not zero


        -- Bottom done if next row set
        if (disp_frag_state = stt_DispFrag16) then
          NextFragment := Fragment + LineFragments;
          dpf_rd_addr <= NextFragment((4+DPF_ADDR_WIDTH) downto 5);

          dpf_position <= Fragment + LineFragments;
          proc_state <= stt_CalcDispFragPos;
          next_apply_filter_state <= stt_ApplyFilter_22;
          next_disp_frag_state <= stt_DispFrag17;
        else

          if (disp_frag_value = '0') then
            rpi_position <= resize(Fragment + LineFragments, RPI_POS_WIDTH);

            -- Calculate RPI_Value before continue
            proc_state <= stt_Calc_RPI_Value;
            calc_rpi_state <= stt_calc_rpi1;
            -- Next state after RPI_Value calculation
            next_proc_state <= stt_CallFilterVert;
            next_apply_filter_state <= stt_ApplyFilter_23;
          else
            apply_filter_state <= stt_ApplyFilter_23;
          end if;
        end if;
        
      elsif (apply_filter_state = stt_ApplyFilter_23) then

        Fragment <= Fragment + 1;
        CountMiddles <= CountMiddles + 1;
        apply_filter_state <= stt_ApplyFilter_11;
        disp_frag_state <= stt_DispFrag9;

      elsif (apply_filter_state = stt_ApplyFilter_24) then

        -- ******************************************************
        -- Last Row
        -- ******************************************************

        -- First column conditions
        -- Only do 2 prediction if fragment coded and on non intra or if
        -- all fragments are intra */
        if (disp_frag_state = stt_DispFrag17) then
          dpf_rd_addr <= Fragment((4+DPF_ADDR_WIDTH) downto 5);
          dpf_position <= Fragment;
          proc_state <= stt_CalcDispFragPos;
          next_apply_filter_state <= stt_ApplyFilter_24;
          next_disp_frag_state <= stt_DispFrag18;
        else
          
          if (disp_frag_value = '1') then
            -- TopRow is always done
            rpi_position <= resize(Fragment, RPI_POS_WIDTH);

            -- Calculate RPI_Value before continue
            proc_state <= stt_Calc_RPI_Value;
            calc_rpi_state <= stt_calc_rpi1;
            -- Next state after RPI_Value calculation
            next_proc_state <= stt_CallFilterVert;
            next_apply_filter_state <= stt_ApplyFilter_25;
          else
            apply_filter_state <= stt_ApplyFilter_26;
          end if;
        end if;
      elsif (apply_filter_state = stt_ApplyFilter_25) then

        -- Enter here only if display_fragments(Fragment) is not zero

        -- Filter right hand border only if the block to the right
        -- is not coded
        if (disp_frag_state = stt_DispFrag18) then
          NextFragment := Fragment + 1;
          dpf_rd_addr <= NextFragment((4+DPF_ADDR_WIDTH) downto 5);

          dpf_position <= Fragment + 1;
          proc_state <= stt_CalcDispFragPos;
          next_apply_filter_state <= stt_ApplyFilter_25;
          next_disp_frag_state <= stt_DispFrag19;
        else

          if (disp_frag_value = '0') then
            rpi_position <= resize(Fragment, RPI_POS_WIDTH);
            -- Horizontal Filter Parameter
            DeltaHorizFilter <= x"6";

            -- Calculate RPI_Value before continue
            proc_state <= stt_Calc_RPI_Value;
            calc_rpi_state <= stt_calc_rpi1;
            -- Next state after RPI_Value calculation
            next_proc_state <= stt_CallFilterHoriz;
            next_apply_filter_state <= stt_ApplyFilter_26;
          else
            apply_filter_state <= stt_ApplyFilter_26;
          end if;
        end if;

      elsif (apply_filter_state = stt_ApplyFilter_26) then

        Fragment <= Fragment + 1;
        CountMiddles <= '0' & x"00001";
        apply_filter_state <= stt_ApplyFilter_27;
        disp_frag_state <= stt_DispFrag19;
        
      elsif (apply_filter_state = stt_ApplyFilter_27) then

        if (CountMiddles < FragsAcross - 1) then
          -- Middle Columns
          if (disp_frag_state = stt_DispFrag19) then
            dpf_rd_addr <= Fragment((4+DPF_ADDR_WIDTH) downto 5);

            dpf_position <= Fragment;
            proc_state <= stt_CalcDispFragPos;
            next_apply_filter_state <= stt_ApplyFilter_27;
            next_disp_frag_state <= stt_DispFrag20;
          else
            if (disp_frag_value = '1') then
              -- Filter Left edge always
              rpi_position <= resize(Fragment, RPI_POS_WIDTH);
              -- Horizontal Filter Parameter
              DeltaHorizFilter <= x"E";

              -- Calculate RPI_Value before continue
              proc_state <= stt_Calc_RPI_Value;
              calc_rpi_state <= stt_calc_rpi1;
              -- Next state after RPI_Value calculation
              next_proc_state <= stt_CallFilterHoriz;
              next_apply_filter_state <= stt_ApplyFilter_28;
            else
              apply_filter_state <= stt_ApplyFilter_30;  -- Increment CountMiddles
            end if;
          end if;
        else
          apply_filter_state <= stt_ApplyFilter_31;    -- End "Loop"
          disp_frag_state <= stt_DispFrag21;
        end if;

      elsif (apply_filter_state = stt_ApplyFilter_28) then

        -- Enter here only if display_fragments(Fragment) is not zero

        -- TopRow is always done
        rpi_position <= resize(Fragment, RPI_POS_WIDTH);

        -- Calculate RPI_Value before continue
        proc_state <= stt_Calc_RPI_Value;
        calc_rpi_state <= stt_calc_rpi1;
        -- Next state after RPI_Value calculation
        next_proc_state <= stt_CallFilterVert;
        next_apply_filter_state <= stt_ApplyFilter_29;
        
      elsif (apply_filter_state = stt_ApplyFilter_29) then

        -- Enter here only if (CountMidCols < FragsAcross - 1) is true
        -- and display_fragments(Fragment) is not zero
        
        -- Filter right hand border only if the block to the right
        -- is not coded
        if (disp_frag_state = stt_DispFrag20) then
          NextFragment := Fragment + 1;
          dpf_rd_addr <= NextFragment((4+DPF_ADDR_WIDTH) downto 5);

          dpf_position <= Fragment + 1;
          proc_state <= stt_CalcDispFragPos;
          next_apply_filter_state <= stt_ApplyFilter_29;
          next_disp_frag_state <= stt_DispFrag21;
        else
          if (disp_frag_value = '0') then
            rpi_position <= resize(Fragment, RPI_POS_WIDTH);
            --Horizontal Filter Parameter
            DeltaHorizFilter <= x"6";

            -- Calculate RPI_Value before continue
            proc_state <= stt_Calc_RPI_Value;
            calc_rpi_state <= stt_calc_rpi1;
            -- Next state after RPI_Value calculation
            next_proc_state <= stt_CallFilterHoriz;
            next_apply_filter_state <= stt_ApplyFilter_30;
          else
            apply_filter_state <= stt_ApplyFilter_30;
          end if;
        end if;


      elsif (apply_filter_state = stt_ApplyFilter_30) then

        CountMiddles <= CountMiddles +1;
        Fragment <= Fragment + 1;
        apply_filter_state <= stt_ApplyFilter_27;
        disp_frag_state <= stt_DispFrag19;
        
      elsif (apply_filter_state = stt_ApplyFilter_31) then

        -- ******************************************************
        -- Last Column
        -- ******************************************************
        if (disp_frag_state = stt_DispFrag21) then
          dpf_rd_addr <= Fragment((4+DPF_ADDR_WIDTH) downto 5);

          dpf_position <= Fragment;
          proc_state <= stt_CalcDispFragPos;
          next_apply_filter_state <= stt_ApplyFilter_31;
          next_disp_frag_state <= stt_DispFrag22;
        else

          if (disp_frag_value = '1') then
            -- Filter Left edge always
            rpi_position <= resize(Fragment, RPI_POS_WIDTH);
            -- Horizontal Filter Parameter
            DeltaHorizFilter <= x"E";

            -- Calculate RPI_Value before continue
            proc_state <= stt_Calc_RPI_Value;
            calc_rpi_state <= stt_calc_rpi1;
            -- Next state after RPI_Value calculation
            next_proc_state <= stt_CallFilterHoriz;
            next_apply_filter_state <= stt_ApplyFilter_32;
          else
            apply_filter_state <= stt_ApplyFilter_33;
          end if;
        end if;

      elsif (apply_filter_state = stt_ApplyFilter_32) then

        -- Enter here only if display_fragments(Fragment) is not zero

        -- TopRow is always done
        rpi_position <= resize(Fragment, RPI_POS_WIDTH);

        -- Calculate RPI_Value before continue
        proc_state <= stt_Calc_RPI_Value;
        calc_rpi_state <= stt_calc_rpi1;
        -- Next state after RPI_Value calculation
        next_proc_state <= stt_CallFilterVert;
        next_apply_filter_state <= stt_ApplyFilter_33;


        --elsif (apply_filter_state = stt_ApplyFilter_33) then
      else

        proc_state <= stt_SelectColor;
        apply_filter_state <= stt_ApplyFilter_1;
        next_disp_frag_state <= stt_DispFrag1;
      end if;
    end procedure ApplyFilter;

    
    procedure Proc is
    begin
      case proc_state is
        when stt_ReadMemory => ReadMemory;
        when stt_WriteMemory => WriteMemory;
        when stt_Calc_RPI_Value => CalcRPIValue;
        when stt_FindQIndex => FindQIndex;
        when stt_CalcFLimit => CalcFLimit;
        when stt_SelectColor => SelectColor;
        when stt_ApplyFilter => ApplyFilter;
        when stt_CalcDispFragPos => CalcDispFragPos;
        when stt_CallFilterHoriz => CallFilterHoriz;
        when stt_CalcFilterHoriz  => CalcFilterHoriz;
        when stt_CallFilterVert  => CallFilterVert;
                                    -- when stt_CalcFilterVert = other
        when others => CalcFilterVert;
      end case;  
    end procedure Proc;
    
  begin  -- process

    
    if (clk'event and clk = '1') then
      if (Reset_n = '0') then
        state <= readIn;
        read_state <= stt_32bitsData;
        proc_state <= stt_FindQIndex;
        apply_filter_state <= stt_ApplyFilter_1;
        calc_filter_state <= stt_CalcFilter1;
        calc_rpi_state <= stt_calc_rpi1;
        
        s_in_request <= '0';
        s_in_sem_request <= '0';
        count <= 0;
        pli <=  "00";
        s_out_sem_valid <= '0';
        s_out_done <= '0';

        mem_rd_valid <= '0';
        
        CountFilter <= "000";
        CountColumns <= "000";
        pixelPtr <= "00000000000000000000";

        rpi_position <= '0' & x"0000";
        HFragments <= x"11";
        VFragments <= x"00";
        YStride <= x"000";
        UVStride <= "000" & x"00";
        YPlaneFragments <= '0' & x"00000";
        UVPlaneFragments <= "000" & x"0000";
        ReconYDataOffset <= x"00000";
        ReconUDataOffset <= x"00000";
        ReconVDataOffset <= x"00000";

-- FLimits signals initialiation
        fbv_position <= "000000000";
        FLimit <= "000000000";

        
-- QThreshTable signal memories
        qtt_wr_e <= '0';
        qtt_wr_addr <= "000000";
        qtt_wr_data <= "00000000000000000000000000000000";
        qtt_rd_addr <= "000000";


--display_fragments signal memories
        dpf_wr_e <= '0';
        dpf_wr_addr <= to_unsigned(0, DPF_ADDR_WIDTH);
        dpf_wr_data <= "00000000000000000000000000000000";
        dpf_rd_addr <= to_unsigned(0, DPF_ADDR_WIDTH);
      else
        if (Enable = '1') then
          case state is
            when readIn => ReadIn;
            when proc => Proc;
            when others => ReadIn; state <= readIn;
          end case;
        end if;
      end if;
    end if;
  end process;

  
end a_LoopFilter;
