library std;
library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
--ordem this last gold
entity UpdateUMV is

  port (Clk,
        Reset_n           :   in std_logic;
        Enable            :   in std_logic;
        
        in_request        :   out std_logic;
        in_valid          :   in std_logic;
        in_data           :   in signed(31 downto 0);

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
end UpdateUMV;


architecture a_UpdateUMV of UpdateUMV is
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
  signal HFragments       : unsigned(LG_MAX_SIZE-3 downto 0);
  signal VFragments       : unsigned(LG_MAX_SIZE-3 downto 0);
  signal YStride          : unsigned(LG_MAX_SIZE+1 downto 0);
  signal UVStride         : unsigned(LG_MAX_SIZE   downto 0);
  signal YPlaneFragments  : unsigned(LG_MAX_SIZE*2 downto 0);
  signal UVPlaneFragments : unsigned(LG_MAX_SIZE*2-2 downto 0);
  signal ReconYDataOffset : unsigned(MEM_ADDR_WIDTH-1 downto 0);
  signal ReconUDataOffset : unsigned(MEM_ADDR_WIDTH-1 downto 0);
  signal ReconVDataOffset : unsigned(MEM_ADDR_WIDTH-1 downto 0);
  signal info_height      : unsigned(LG_MAX_SIZE-1 downto 0);


-------------------------------------------------------------------------------
-- Signal that must be read for all frames
-------------------------------------------------------------------------------
  signal FrameOfs         : unsigned(MEM_ADDR_WIDTH-1 downto 0);

-------------------------------------------------------------------------------
-- ReconPixelIndex signal
-------------------------------------------------------------------------------
  constant RPI_DATA_WIDTH : positive := 32;
  constant RPI_POS_WIDTH  : positive := 17;
  signal rpi_position     : unsigned(RPI_POS_WIDTH-1 downto 0);
  signal rpi_value        : signed(RPI_DATA_WIDTH-1 downto 0);

  signal s_rpi_in_request    : std_logic;
  signal s_rpi_in_valid      : std_logic;
  signal s_rpi_in_data       : signed(31 downto 0);
        
  signal s_rpi_out_requested : std_logic;
  signal s_rpi_out_valid     : std_logic;
  signal s_rpi_out_data      : signed(31 downto 0);

-------------------------------------------------------------------------------
-- Internal Signals
-------------------------------------------------------------------------------
  signal count                   : integer range 0 to 4095;
  signal count2                  : unsigned(4 downto 0);

  -- VERIFICAR OS TIPOS
  signal PlaneStride             : unsigned(LG_MAX_SIZE+1 downto 0);
  signal PlaneBorderWidth        : unsigned(4 downto 0);
  signal LineFragments           : unsigned(LG_MAX_SIZE-3 downto 0);
  signal PlaneHeight             : unsigned(LG_MAX_SIZE-1 downto 0);
  signal BlockVStep              : unsigned(LG_MAX_SIZE+5 downto 0);
  signal PlaneFragments          : unsigned(LG_MAX_SIZE*2 downto 0);
  signal position                : unsigned(LG_MAX_SIZE*2 downto 0);
  signal SrcPtr1                 : unsigned(MEM_ADDR_WIDTH-1 downto 0);
  signal DestPtr1                : unsigned(MEM_ADDR_WIDTH-1 downto 0);
  signal DestPtr1_i              : unsigned(MEM_ADDR_WIDTH-1 downto 0);
  signal SrcPtr2                 : unsigned(MEM_ADDR_WIDTH-1 downto 0);
  signal DestPtr2                : unsigned(MEM_ADDR_WIDTH-1 downto 0);
  signal DestPtr2_i              : unsigned(MEM_ADDR_WIDTH-1 downto 0);

  signal copy1                   : signed(31 downto 0);
  signal copy2                   : signed(31 downto 0);

  
-------------------------------------------------------------------------------
-- States and sub-states
-------------------------------------------------------------------------------
  
  type layer_t is (stt_Y, stt_U, stt_V);
  signal layer : layer_t;

  type state_t is (stt_readin, stt_Calc_RPI_Value,
                   stt_Ver, stt_Hor,
                   stt_done, stt_ReadMem,
                   stt_WriteMem);
  signal state : state_t;
  signal save_state : state_t;

  type read_state_t is (stt_read_HFragments,
                        stt_read_YPlaneFragments,
                        stt_read_YStride,
                        stt_read_UVPlaneFragments,
                        stt_read_UVStride,
                        stt_read_VFragments,
                        stt_read_ReconYDataOffset,
                        stt_read_ReconUDataOffset,
                        stt_read_ReconVDataOffset,
                        stt_read_info,
                        stt_read_offset);
  signal read_state : read_state_t;

  type calc_rpi_state_t is (stt_calc_rpi1, stt_calc_rpi2);
  signal calc_rpi_state : calc_rpi_state_t;

  type update_state_t is (stt_1, stt_2, stt_3, stt_4, stt_5);
  signal update_state : update_state_t;

  type update_int_state_t is (stt_read1, stt_read2, stt_wait, stt_write1, stt_write2);
  signal update_int_state : update_int_state_t;

-------------------------------------------------------------------------------
-- Constants
-------------------------------------------------------------------------------

  constant UMV_BORDER  : unsigned(4 downto 0) := "10000";
  constant HFRAGPIXELS : unsigned(3 downto 0) := "1000";
  constant VFRAGPIXELS : unsigned(3 downto 0) := "1000";


  signal s_in_request            : std_logic;
  
  signal s_out_done : std_logic;
  
  signal s_in_sem_request : std_logic;
  signal s_out_sem_valid : std_logic;

-- Memories Signals
  signal mem_rd_data  : signed(31 downto 0);

  
begin

  in_request <= s_in_request;
  in_sem_request <= s_in_sem_request;  
  out_sem_valid <= s_out_sem_valid;
  out_done <= s_out_done;



  rpi0: reconpixelindex
    port map (Clk => Clk,
              Reset_n => Reset_n,
              in_request => s_rpi_out_requested,
              in_valid => s_rpi_out_valid,
              in_data => s_rpi_out_data,

              out_requested => s_rpi_in_request,
              out_valid => s_rpi_in_valid,
              out_data => s_rpi_in_data);

  RPI_HandShake: process (in_data, in_valid,
                          state, read_state,
                          calc_rpi_state,
                          rpi_position,
                          s_in_request)
  begin  -- process RPI_HandShake
    s_rpi_out_data <= x"00000000";
    s_rpi_out_valid <= '0';
    if (s_in_request = '1') then
      if (state = stt_readIn and
          read_state /= stt_read_info and
          read_state /= stt_read_offset) then
        s_rpi_out_data <= in_data;
        s_rpi_out_valid <= in_valid;
      end if;
    else
      if (state = stt_Calc_RPI_Value and
          calc_rpi_state = stt_calc_rpi1) then
        s_rpi_out_data <= resize(signed('0'&rpi_position), 32);
        s_rpi_out_valid <= '1';
      end if;
    end if;
  end process RPI_HandShake;

  
  process (clk)

    procedure ReadIn is
    begin
      s_in_request <= '1';
      s_out_sem_valid <= '0';
      s_in_sem_request <= '0';
      if (s_in_request = '1' and in_valid = '1') then
        case read_state is
          when stt_read_HFragments =>
            HFragments <= unsigned(in_data(LG_MAX_SIZE-3 downto 0));
            read_state <= stt_read_YPlaneFragments;


          when stt_read_YPlaneFragments =>
            YPlaneFragments <= unsigned(in_data(LG_MAX_SIZE*2 downto 0));
            read_state <= stt_read_YStride;


          when stt_read_YStride =>
            YStride <= unsigned(in_data(LG_MAX_SIZE+1 downto 0));
            read_state <= stt_read_UVPlaneFragments;


          when stt_read_UVPlaneFragments =>
            UVPlaneFragments <= unsigned(in_data(LG_MAX_SIZE*2-2 downto 0));
            read_state <= stt_read_UVStride;


          when stt_read_UVStride =>
            UVStride <= unsigned(in_data(LG_MAX_SIZE downto 0));
            read_state <= stt_read_VFragments;


          when stt_read_VFragments =>
            VFragments <= unsigned(in_data(LG_MAX_SIZE-3 downto 0));
            read_state <= stt_read_ReconYDataOffset;


          when stt_read_ReconYDataOffset =>
            ReconYDataOffset <= unsigned(in_data(MEM_ADDR_WIDTH-1 downto 0));
            read_state <= stt_read_ReconUDataOffset;


          when stt_read_ReconUDataOffset =>
            ReconUDataOffset <= unsigned(in_data(MEM_ADDR_WIDTH-1 downto 0));
            read_state <= stt_read_ReconVDataOffset;


          when stt_read_ReconVDataOffset =>
            ReconVDataOffset <= unsigned(in_data(MEM_ADDR_WIDTH-1 downto 0));
            read_state <= stt_read_info;


          when stt_read_info =>
            info_height <= unsigned(in_data(LG_MAX_SIZE-1 downto 0));
            read_state <= stt_read_offset;


          when others =>                  -- when stt_offset
            state <= stt_Ver;
            s_in_request <= '0';
            FrameOfs <= unsigned(in_data(MEM_ADDR_WIDTH-1 downto 0));
            count2 <= "00000";
            count <= 0;
        end case;
      end if;
    end procedure ReadIn;

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
            state <= save_state;
          end if;
      end case;
    end procedure CalcRPIValue;
    
    
    procedure Vert is
    begin
      case update_state is
        when stt_1 =>
          update_state <= stt_2;
          case layer is
            when stt_Y =>
              PlaneStride <= YStride; 
--               assert FrameOfs = 15360 report "LastFrame";
--               assert FrameOfs = 7680 report "--------------------GoldFrame";
              PlaneBorderWidth <= UMV_BORDER; 
              LineFragments <= HFragments; 
              PlaneHeight <= info_height; 

              rpi_position <= resize("00", RPI_POS_WIDTH);
              state <= stt_Calc_RPI_Value;
              calc_rpi_state <= stt_calc_rpi1;
              save_state <= stt_Ver;
              
              position <= resize("00", LG_MAX_SIZE*2+1);
            when stt_U =>
              PlaneStride <= '0' & UVStride; 
              PlaneBorderWidth <= SHIFT_RIGHT(UMV_BORDER, 1); 
              LineFragments <= SHIFT_RIGHT(HFragments, 1); 
              PlaneHeight <= SHIFT_RIGHT(info_height, 1); 

              rpi_position <= resize(YPlaneFragments, RPI_POS_WIDTH);
              state <= stt_Calc_RPI_Value;
              calc_rpi_state <= stt_calc_rpi1;
              save_state <= stt_Ver;

              position <= YPlaneFragments;
            when others =>              -- when stt_V =>
              PlaneStride <= '0' & UVStride;
              PlaneBorderWidth <= SHIFT_RIGHT(UMV_BORDER, 1);
              LineFragments <= SHIFT_RIGHT(HFragments, 1);
              PlaneHeight <= SHIFT_RIGHT(info_height, 1);

              rpi_position <= resize(YPlaneFragments + UVPlaneFragments, RPI_POS_WIDTH);
              state <= stt_Calc_RPI_Value;
              calc_rpi_state <= stt_calc_rpi1;
              save_state <= stt_Ver;
              
              position <= YPlaneFragments + UVPlaneFragments;
          end case;

        when stt_2 =>
          update_state <= stt_3;
          SrcPtr1 <= resize(FrameOfs + ('0' & unsigned(rpi_value)), MEM_ADDR_WIDTH);
          DestPtr1 <= resize(FrameOfs + ('0' & unsigned(rpi_value)) - PlaneBorderWidth, MEM_ADDR_WIDTH);

        when stt_3 => 
          update_state <= stt_4;

          rpi_position <= resize(position + LineFragments - 1, RPI_POS_WIDTH);
          state <= stt_Calc_RPI_Value;
          calc_rpi_state <= stt_calc_rpi1;
          save_state <= stt_Ver;
          
        when stt_4 => 
          update_state <= stt_5;
          SrcPtr2 <= resize(FrameOfs + ('0' & unsigned(rpi_value)) + HFRAGPIXELS - 1, MEM_ADDR_WIDTH);
          DestPtr2 <= resize(FrameOfs + ('0' & unsigned(rpi_value)) + HFRAGPIXELS, MEM_ADDR_WIDTH);
        when others =>                  -- when stt_5 =>
          if (count = PlaneHeight) then
            count <= 0;
            count2 <= "00000";
            update_state <= stt_1;
            case layer is
              when stt_Y =>
                layer <= stt_U;
              when stt_U =>
                layer <= stt_V;
              when others =>    -- when stt_V =>
                layer <= stt_Y;
                state <= stt_Hor;
            end case;
          else
            save_state <= state;
            case update_int_state is
              when stt_read1 =>
                update_int_state <= stt_read2;
                state <= stt_ReadMem;
                in_sem_addr <= SHIFT_RIGHT(SrcPtr1, 2);
                s_in_sem_request <= '1';

              when stt_read2 =>
                update_int_state <= stt_wait;
                state <= stt_ReadMem;
                in_sem_addr <= SHIFT_RIGHT(SrcPtr2, 2);
                s_in_sem_request <= '1';
                copy1 <= mem_rd_data(31 downto 24) &
                         mem_rd_data(31 downto 24) &
                         mem_rd_data(31 downto 24) &
                         mem_rd_data(31 downto 24);

              when stt_wait =>
                update_int_state <= stt_write1;
                copy2 <= mem_rd_data(7 downto 0) &
                         mem_rd_data(7 downto 0) &
                         mem_rd_data(7 downto 0) &
                         mem_rd_data(7 downto 0);              

              when stt_write1 =>
                if (count2 = PlaneBorderWidth) then
                  count2 <= "00000";
                  count <= count + 1;
                  update_int_state <= stt_read1;
                  SrcPtr1 <= SrcPtr1 + PlaneStride;
                  SrcPtr2 <= SrcPtr2 + PlaneStride;
                  DestPtr1 <= DestPtr1 + PlaneStride;
                  DestPtr2 <= DestPtr2 + PlaneStride;
                else
                  update_int_state <= stt_write2;
                  out_sem_addr <= SHIFT_RIGHT(DestPtr1 + count2, 2);
                  out_sem_data <= copy1;
                  s_out_sem_valid <= '1';
                  state <= stt_WriteMem;
                end if;

              when others => -- when stt_write2 =>
                update_int_state <= stt_write1;
                out_sem_addr <= SHIFT_RIGHT(DestPtr2 + count2, 2);
                count2 <= count2 + 4;
                out_sem_data <= copy2;
                s_out_sem_valid <= '1';
                state <= stt_WriteMem;
            end case;
          end if;
      end case;
    end procedure Vert;

    procedure Horz is
    begin
      case update_state is
        when stt_1 =>
          update_state <= stt_2;
          case layer is
            when stt_Y =>
              BlockVStep <= YStride * (VFRAGPIXELS - 1);
              PlaneStride <= YStride; 
              PlaneBorderWidth <= UMV_BORDER;
              PlaneFragments <= YPlaneFragments;
              LineFragments <= HFragments; 

              rpi_position <= resize("00", RPI_POS_WIDTH);
              state <= stt_Calc_RPI_Value;
              calc_rpi_state <= stt_calc_rpi1;
              save_state <= stt_Hor;

              
              position <= resize("00", LG_MAX_SIZE*2+1);
            when stt_U =>
              BlockVStep <= ('0' & UVStride) * (VFRAGPIXELS - 1);
              PlaneStride <= '0' & UVStride; 
              PlaneBorderWidth <= SHIFT_RIGHT(UMV_BORDER, 1) ;
              PlaneFragments <= "00" & UVPlaneFragments;
              LineFragments <= SHIFT_RIGHT(HFragments, 1); 

              rpi_position <= resize(YPlaneFragments, RPI_POS_WIDTH);
              state <= stt_Calc_RPI_Value;
              calc_rpi_state <= stt_calc_rpi1;
              save_state <= stt_Hor;

              position <= YPlaneFragments;
            when others =>      -- when stt_V =>
              BlockVStep <= ('0' & UVStride) * (VFRAGPIXELS - 1);
              PlaneStride <= '0' & UVStride; 
              PlaneBorderWidth <= SHIFT_RIGHT(UMV_BORDER,1) ;
              PlaneFragments <= "00" & UVPlaneFragments;
              LineFragments <= SHIFT_RIGHT(HFragments,1); 

              rpi_position <= resize(YPlaneFragments + UVPlaneFragments, RPI_POS_WIDTH);
              state <= stt_Calc_RPI_Value;
              calc_rpi_state <= stt_calc_rpi1;
              save_state <= stt_Hor;
              
              position <= YPlaneFragments + UVPlaneFragments;
          end case;
        when stt_2 =>
          update_state <= stt_3;
          SrcPtr1 <= resize(FrameOfs + ('0' & unsigned(rpi_value)) - PlaneBorderWidth, MEM_ADDR_WIDTH);
          DestPtr1 <= resize(FrameOfs + ('0' & unsigned(rpi_value)) - PlaneBorderWidth*(PlaneStride + 1), MEM_ADDR_WIDTH); 
        when stt_3 => 
          update_state <= stt_4;

          rpi_position <= resize(position + PlaneFragments - LineFragments, RPI_POS_WIDTH);
          state <= stt_Calc_RPI_Value;
          calc_rpi_state <= stt_calc_rpi1;
          save_state <= stt_Hor;

        when stt_4 => 
          update_state <= stt_5;
          SrcPtr2 <= resize(FrameOfs + ('0' & unsigned(rpi_value)) + BlockVStep - PlaneBorderWidth, MEM_ADDR_WIDTH);
          DestPtr2 <= resize(FrameOfs + ('0' & unsigned(rpi_value)) + BlockVStep - PlaneBorderWidth + PlaneStride, MEM_ADDR_WIDTH);
        when others =>    -- when stt_5 => 
          if (count = PlaneStride) then
            count <= 0;
            count2 <= "00000";
            update_state <= stt_1;
            case layer is
              when stt_Y =>
                layer <= stt_U;
              when stt_U =>
                layer <= stt_V;
              when others =>    -- when stt_V =>
                layer <= stt_Y;
		state <= stt_Done;
            end case;
          else
            save_state <= state;
            case update_int_state is
              when stt_read1 =>
                DestPtr1_i <= DestPtr1 + count;
                DestPtr2_i <= DestPtr2 + count;

                update_int_state <= stt_read2;
                state <= stt_ReadMem;
                in_sem_addr <= SHIFT_RIGHT(SrcPtr1, 2);
                s_in_sem_request <= '1';
                
              when stt_read2 =>
                update_int_state <= stt_wait;
                state <= stt_ReadMem;
                in_sem_addr <= SHIFT_RIGHT(SrcPtr2, 2);
                s_in_sem_request <= '1';
                copy1 <= mem_rd_data;
                
              when stt_wait =>
                update_int_state <= stt_write1;
                copy2 <= mem_rd_data;

              when stt_write1 =>
                if (count2 = PlaneBorderWidth) then
                  count2 <= "00000";
                  count <= count + 4;
                  update_int_state <= stt_read1;
                  SrcPtr1 <= SrcPtr1 + 4;
                  SrcPtr2 <= SrcPtr2 + 4;
                else
                  update_int_state <= stt_write2;
                  out_sem_addr <= SHIFT_RIGHT(DestPtr1_i, 2);
                  out_sem_data <= copy1;
                  s_out_sem_valid <= '1';
                  state <= stt_WriteMem;
                end if;

              when others =>    -- when stt_write2 =>
                update_int_state <= stt_write1;
                out_sem_addr <= SHIFT_RIGHT(DestPtr2_i, 2);
                count2 <= count2 + 1;
                DestPtr1_i <= DestPtr1_i + PlaneStride;
                DestPtr2_i <= DestPtr2_i + PlaneStride;
                out_sem_data <= copy2;
                s_out_sem_valid <= '1';
                state <= stt_WriteMem;
            end case;
          end if;
      end case;
    end procedure Horz;

    procedure Done is
      begin
      if (count = 0) then
      	s_out_done <= '1';
      	count <= count+1;
      elsif (count = 1) then
	s_out_done <= '0';
        count <= count+1;
      else
        assert false report "UpdateUMV is done" severity note;
      	state <= stt_readin;
      	read_state <= stt_read_offset;
      	count <= 0;
      end if;
    end procedure Done;

    procedure ReadMemory is
    begin
      s_in_sem_request <= '1';
      if (s_in_sem_request = '1' and in_sem_valid = '1') then
        mem_rd_data <= in_sem_data;
        s_in_sem_request <= '0';
        state <= save_state;
      end if;
    end procedure ReadMemory;


    
    procedure WriteMemory is
    begin
      if (out_sem_requested = '1') then
        s_out_sem_valid <= '0';
        state <= save_state;
      end if;
    end procedure WriteMemory;


    
  begin                                 -- process
    if (clk'event and clk = '1') then
      if (Reset_n = '0') then
        s_out_done <= '0';
        s_in_request <= '0';
        layer <= stt_Y;
        read_state <= stt_read_HFragments;
        state <= stt_readin;
        update_int_state <= stt_read1;
        update_state <= stt_1;
        count <= 0;
        count2 <= "00000";
        s_in_sem_request <= '0';
        s_out_sem_valid <= '0';
        rpi_position <= '0' & x"0000";
        s_rpi_in_request <= '0';
        calc_rpi_state <= stt_calc_rpi1;
        
        HFragments <= x"11";
        VFragments <= x"00";
        YStride <= x"000";
        UVStride <= "000" & x"00";
        YPlaneFragments <= '0' & x"00000";
        UVPlaneFragments <= "000" & x"0000";
        ReconYDataOffset <= x"00000";
        ReconUDataOffset <= x"00000";
        ReconVDataOffset <= x"00000";
        info_height	 <= resize("00", LG_MAX_SIZE);
      else
        if (Enable = '1') then
          case state is
            when stt_readin => ReadIn;
            when stt_Calc_RPI_Value => CalcRPIValue;
            when stt_Ver => Vert;
            when stt_Hor => Horz;
            when stt_Done => Done;
            when stt_WriteMem => WriteMemory;
            when stt_ReadMem => ReadMemory;
            when others => ReadIn; state <= stt_readin;
          end case;
        end if;
      end if;
    end if;
  end process;
  
end a_UpdateUMV;
