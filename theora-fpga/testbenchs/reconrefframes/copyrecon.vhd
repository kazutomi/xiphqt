-------------------------------------------------------------------------------
--  Description: This file implements the CopyRecon.
--               It copies the visible fragments of the source pointer
--               to the equivalent pointed by destination pointer.
-------------------------------------------------------------------------------

library std;
library ieee;
library work;

use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use work.all;

entity CopyRecon is
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
end entity CopyRecon;


architecture a_copyrecon of CopyRecon is
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

  
  -- We are using 1024 as the maximum width and height size
  -- = ceil(log2(Maximum Size))
  constant LG_MAX_SIZE    : natural := 10;
  constant MEM_ADDR_WIDTH : natural := 20;
-------------------------------------------------------------------------------
-- Signals that must be read at the beginning
-------------------------------------------------------------------------------
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

-------------------------------------------------------------------------------
-- Signal that must be read for all frames
-------------------------------------------------------------------------------
  signal OffSetSrcPtr : unsigned(MEM_ADDR_WIDTH-1 downto 0);
  signal OffSetDestPtr : unsigned(MEM_ADDR_WIDTH-1 downto 0);
  
-------------------------------------------------------------------------------
-- Internal Signals
-------------------------------------------------------------------------------
  signal count : integer range 0 to 2097151;

  signal BlockCount : unsigned(2 downto 0);
  signal SlotCount : unsigned(2 downto 0);
  
  signal ValueCount : unsigned(LG_MAX_SIZE*2 downto 0);
  signal EndValue : unsigned(LG_MAX_SIZE*2 downto 0);

  signal PlaneLineStep  : unsigned(LG_MAX_SIZE+1 downto 0);

  signal s_in_request : std_logic;

  signal mem_rd_data  : signed(31 downto 0);
  signal MaxDPFCount : unsigned(LG_MAX_SIZE*2 downto 0);

  signal SrcPtr : unsigned(MEM_ADDR_WIDTH-1 downto 0);
  signal DestPtr : unsigned(MEM_ADDR_WIDTH-1 downto 0);

-------------------------------------------------------------------------------
-- ReconPixelIndex signals and constants
-------------------------------------------------------------------------------
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

  

-------------------------------------------------------------------------------
-- display_fragment signals and constants
-------------------------------------------------------------------------------
  constant DPF_DEPTH : positive := 57;
  constant DPF_DATA_WIDTH : positive := 32;
  constant DPF_ADDR_WIDTH : positive := 6;

  signal dpf_wr_e    : std_logic;
  signal dpf_wr_addr : unsigned(DPF_ADDR_WIDTH-1 downto 0);
  signal dpf_wr_data : signed(DPF_DATA_WIDTH-1 downto 0);
  signal dpf_rd_addr : unsigned(DPF_ADDR_WIDTH-1 downto 0);
  signal dpf_rd_data : signed(DPF_DATA_WIDTH-1 downto 0);

-------------------------------------------------------------------------------
-- States and sub-states
-------------------------------------------------------------------------------
  type state_t is (stt_ReadIn, stt_Proc, stt_CopyBlock, stt_ReadMemory,
                   stt_WriteMemory, stt_EndProc);
  signal state : state_t;

  type read_state_t is (stt_32bitsData, stt_DispFrag,
                        stt_ReadOffset);
  signal read_state : read_state_t;

  type proc_state_t is (stt_proc1, stt_proc2, stt_proc3,
                        stt_proc4, stt_proc5, stt_proc6,
                        stt_proc7, stt_proc8, stt_proc9,
                        stt_proc10);
  signal proc_state : proc_state_t;

  type copy_block_state_t is (stt_CopyBlk1, stt_CopyBlk2, stt_CopyBlk3);
  signal copy_block_state : copy_block_state_t;


-------------------------------------------------------------------------------
-- HandShake
-------------------------------------------------------------------------------
  signal s_in_sem_request : std_logic;
  signal s_out_sem_valid : std_logic;

begin  -- a_copyrecon

  in_request <= s_in_request;
  in_sem_request <= s_in_sem_request;
  out_sem_valid <= s_out_sem_valid;

  
  mem_disp_frag: syncram
    generic map (DPF_DEPTH, DPF_DATA_WIDTH, DPF_ADDR_WIDTH)
    port map (clk, dpf_wr_e, dpf_wr_addr, dpf_wr_data, dpf_rd_addr, dpf_rd_data);

  
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
                          rpi_position, s_in_request)
  begin  -- process RPI_HandShake
    s_rpi_out_data <= x"00000000";
    s_rpi_out_valid <= '0';
    if (s_in_request = '1') then
      if (read_state = stt_32bitsData) then
        if (count >=0 and count <=8) then
          s_rpi_out_data <= in_data;
          s_rpi_out_valid <= in_valid;
        end if;
      end if;
    else
      if (state = stt_Proc and
          proc_state = stt_proc7) then
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
        UnitFragmets <= unsigned(in_data(LG_MAX_SIZE*2 downto 0));

        MaxDPFCount <= SHIFT_RIGHT(
          unsigned(in_data(LG_MAX_SIZE*2 downto 0)), 5) + 1;
        if (in_data(4 downto 0) = "00000") then
        MaxDPFCount <= SHIFT_RIGHT(
          unsigned(in_data(LG_MAX_SIZE*2 downto 0)), 5);
        end if;
        read_state <= stt_DispFrag;
        count <= 0;
      end if;
    end procedure Read32bitsData;

    procedure ReadDispFrag is
    begin
--      assert false report "DispFrag Count = "&integer'image(count) severity note;
      dpf_wr_e <= '1';
      dpf_wr_data <= in_data;
      dpf_wr_addr <= dpf_wr_addr + 1;
      if (count = 0) then
        dpf_wr_addr <= "000000";
        count <= 1;
      elsif (count = MaxDPFCount - 1) then
        read_state <= stt_ReadOffset;
        count <= 0;
      else
        count <= count + 1;
      end if;
    end procedure ReadDispFrag;

   
    procedure ReadOffsets is
    begin
      dpf_wr_e <= '0';
      if (count = 0) then
        OffSetSrcPtr <= unsigned(in_data(MEM_ADDR_WIDTH-1 downto 0));
        count <= count + 1;
      else
        s_in_request <= '0';
        OffSetDestPtr <= unsigned(in_data(MEM_ADDR_WIDTH-1 downto 0));
        count <= 0;
        state <= stt_Proc;
        read_state <= stt_DispFrag;
      end if;
    end procedure ReadOffsets;

    procedure ReadIn is
    begin
      s_in_request <= '1';
      if (s_in_request = '1' and in_valid = '1') then
        case read_state is
          when stt_32bitsData => Read32bitsData;
          when stt_DispFrag => ReadDispFrag;
          when others => ReadOffsets;
        end case;
      end if;
    end procedure ReadIn;


-------------------------------------------------------------------------------
-- Preparing the copy
-------------------------------------------------------------------------------
    procedure Proc is
    begin
--      assert false report "proc_state = "&proc_state_t'image(proc_state) severity note;
      case proc_state is
        when stt_proc1 =>
          PlaneLineStep <= YStride;
          ValueCount <= to_unsigned(0, LG_MAX_SIZE*2+1);
          EndValue <= YPlaneFragments;
          proc_state <= stt_proc3;


        when stt_proc2 =>
          PlaneLineStep <= '0' & UVStride;
          ValueCount <= YPlaneFragments;
          EndValue <= UnitFragmets;
          proc_state <= stt_proc3;


        when stt_proc3 =>
          dpf_rd_addr <= resize(SHIFT_RIGHT(ValueCount, 5), DPF_ADDR_WIDTH);
          proc_state <= stt_proc4;

        when stt_proc4 =>
          -- Wait for the memory access
          proc_state <= stt_proc5;

        when stt_proc5 =>
          proc_state <= stt_proc10;
          -- Now we have 32 display_fragments values
          if (dpf_rd_data(31 - to_integer(ValueCount(4 downto 0))) = '1') then
            proc_state <= stt_proc6;
          end if;
          
        when stt_proc6 =>
          -- Requet the recon_pixel_index position
          rpi_position <= resize(ValueCount, RPI_POS_WIDTH);
          -- Wait for the memory access
          proc_state <= stt_proc7;

        when stt_proc7 =>
          -- Wait until ReconPixelIndex can receive the data
          if (s_rpi_out_requested = '1') then
            proc_state <= stt_proc8;
          end if;

        when stt_proc8 =>
          -- Wait until ReconPixelIndex returns the value
          s_rpi_in_request <= '1';
          if (s_rpi_in_request = '1' and s_rpi_in_valid = '1') then
            rpi_value <= s_rpi_in_data;
            s_rpi_in_request <= '0';
            proc_state <= stt_proc9;
          end if;

          
        when stt_proc9 =>
          -- Copying the blocks
          SrcPtr <= OffSetSrcPtr + to_integer(rpi_value);
          DestPtr <= OffSetDestPtr + to_integer(rpi_value);
          state <= stt_CopyBlock;
          proc_state <= stt_proc10;


        when stt_proc10 =>
          proc_state <= stt_proc5;
--          assert false report "ValueCount = "&integer'image(to_integer(ValueCount)) severity note;
          if (ValueCount = EndValue - 1) then
            proc_state <= stt_proc2;
            if (ValueCount = UnitFragmets - 1) then
              -- All done
              out_done <= '1';
              state <= stt_EndProc;
              proc_state <= stt_proc1;
            end if;
          elsif (ValueCount(4 downto 0) = "11111") then
            -- If we have checked all display_fragments bits
            -- then we need to read the next display_fragment's
            -- memory slot
            proc_state <= stt_proc3;
          end if;
          ValueCount <= ValueCount + 1;
         
      end case;
      
    end procedure Proc;


-------------------------------------------------------------------------------
-- This procedure do the main job
-------------------------------------------------------------------------------
    procedure CopyBlock is
    begin
      case copy_block_state is
        when stt_CopyBlk1 =>
          s_in_sem_request <= '1';
          in_sem_addr <= SHIFT_RIGHT(SrcPtr + SlotCount, 2);
          state <= stt_ReadMemory;
          copy_block_state <= stt_CopyBlk2;

          
        when stt_CopyBlk2 =>
          out_sem_addr <= SHIFT_RIGHT(DestPtr + SlotCount, 2);
          out_sem_data <= mem_rd_data;
          s_out_sem_valid <= '1';
          state <= stt_WriteMemory;

          SlotCount <= SlotCount + 4;
          copy_block_state <= stt_CopyBlk1;
          if (SlotCount = 4) then
            copy_block_state <= stt_CopyBlk3;
            SlotCount <= "000";
          end if;

          
        when stt_CopyBlk3 =>
          SrcPtr <= SrcPtr + PlaneLineStep;
          DestPtr <= DestPtr + PlaneLineStep;
          BlockCount <= BlockCount + 1;
          copy_block_state <= stt_CopyBlk1;
          if (BlockCount = 7) then
            BlockCount <= "000";
            state <= stt_Proc;
          end if;
          
      end case;
    end procedure CopyBlock;
    
    procedure ReadMemory is
    begin
      s_in_sem_request <= '1';
      if (s_in_sem_request = '1' and in_sem_valid = '1') then
        mem_rd_data <= in_sem_data;
        s_in_sem_request <= '0';
        state <= stt_CopyBlock;
      end if;
    end procedure ReadMemory;

    
    procedure WriteMemory is
    begin
      if (out_sem_requested = '1') then
        state <= stt_CopyBlock;
        s_out_sem_valid <= '0';
      end if;
    end procedure WriteMemory;

-------------------------------------------------------------------------------
-- Procedures called when state is stt_EndProc
-------------------------------------------------------------------------------
    procedure EndProc is
    begin
--      assert false report "Writing Data" severity note;
      count <= 0;
      out_done <= '0';
      state <= stt_readIn;
    end procedure EndProc;

    
  begin  -- process
    if (clk'event and clk = '1') then
      if (Reset_n = '0') then
        state <= stt_ReadIn;
        read_state <= stt_32bitsData;
        proc_state <= stt_proc1;
        copy_block_state <= stt_CopyBlk1;
        BlockCount <= "000";
        count <= 0;
        SlotCount <= "000";

        s_in_request <= '0';
        s_in_request <= '0';
        s_out_sem_valid <= '0';
        out_done <= '0';

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

        s_rpi_in_request <= '0';
        dpf_wr_e  <= '0';
        dpf_wr_addr <= to_unsigned(0, DPF_ADDR_WIDTH);
        dpf_wr_data <= x"00000000";
        dpf_rd_addr <= to_unsigned(0, DPF_ADDR_WIDTH);
      else
        s_in_request <= '0';
        if (Enable = '1') then
          case state is
            when stt_ReadIn => ReadIn;
            when stt_Proc => Proc;
            when stt_CopyBlock => CopyBlock;
            when stt_ReadMemory => ReadMemory;
            when stt_WriteMemory => WriteMemory;
            when others => EndProc;
          end case;
        end if;
      end if;
    end if;
  end process;

end a_copyrecon;
