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
-- We are using 1024 as the maximum width and height size
  -- = ceil(log2(Maximum Size))
  constant LG_MAX_SIZE    : natural := 10;
  constant MEM_ADDR_WIDTH : natural := 20;

-------------------------------------------------------------------------------
-- Signals that must be read at the beginning
-------------------------------------------------------------------------------

  signal HFragments              : unsigned(31 downto 0);
  signal VFragments              : unsigned(31 downto 0);
  signal YStride                 : unsigned(31 downto 0);
  signal UVStride                : unsigned(31 downto 0);
  signal YPlaneFragments         : unsigned(31 downto 0);
  signal UVPlaneFragments        : unsigned(31 downto 0);
  signal ReconYDataOffset        : unsigned(31 downto 0);
  signal ReconUDataOffset        : unsigned(31 downto 0);
  signal ReconVDataOffset        : unsigned(31 downto 0);
  signal info_height             : unsigned(31 downto 0);


-------------------------------------------------------------------------------
-- Signal that must be read for all frames
-------------------------------------------------------------------------------
  signal FrameOfs                : unsigned(31 downto 0);

-------------------------------------------------------------------------------
-- ReconPixelIndex signal
-------------------------------------------------------------------------------
  signal rpi_position     : unsigned(31 downto 0);
  signal rpi_value        : signed(31 downto 0);

-------------------------------------------------------------------------------
-- Internal Signals
-------------------------------------------------------------------------------
  signal s_in_request            : std_logic;

  signal count                   : integer;
  signal count2                  : integer;

  -- VERIFICAR OS TIPOS
  signal PlaneStride             : unsigned(31 downto 0);
  signal PlaneBorderWidth        : integer;
  signal LineFragments           : unsigned(31 downto 0);
  signal PlaneHeight             : integer;
  signal BlockVStep              : integer;
  signal PlaneFragments          : integer;
  signal position                : unsigned(31 downto 0);
  signal SrcPtr1                 : integer;
  signal DestPtr1                : integer;
  signal DestPtr1_i              : integer;
  signal SrcPtr2                 : integer;
  signal DestPtr2                : integer;
  signal DestPtr2_i              : integer;

  signal copy1                   : signed(31 downto 0);
  signal copy2                   : signed(31 downto 0);

  
-------------------------------------------------------------------------------
-- States and sub-states
-------------------------------------------------------------------------------
  
  type layer_t is (stt_Y, stt_U, stt_V);
  signal layer : layer_t;

  type state_t is (stt_readin, stt_Ver, stt_Hor, stt_done, stt_ReadMem, stt_WriteMem);
  signal state : state_t;
  signal save_state : state_t;

  type read_state_t is (stt_onetime, stt_offset );
  signal read_state : read_state_t;

  type update_state_t is (stt_1, stt_2, stt_3, stt_4, stt_5);
  signal update_state : update_state_t;

  type update_int_state_t is (stt_read1, stt_read2, stt_wait, stt_write1, stt_write2);
  signal update_int_state : update_int_state_t;

-------------------------------------------------------------------------------
-- Constants
-------------------------------------------------------------------------------

  constant UMV_BORDER  : integer := 16;
  constant HFRAGPIXELS : integer := 8;
  constant VFRAGPIXELS : integer := 8;



  signal s_out_done : std_logic;
  
  signal s_in_sem_request : std_logic;
  signal s_out_sem_valid : std_logic;

-- Memories Signals
  signal mem_rd_data  : signed(31 downto 0);

  
begin


  out_done <= s_out_done;
  in_request <= s_in_request;


  rpi0: entity work.reconpixelindex
    generic map(32,32,32,32,32,32,32,32)
    port map (rpi_position, HFragments, VFragments, YStride, UVStride,
              YPlaneFragments, UVPlaneFragments, ReconYDataOffset,
              ReconUDataOffset, ReconVDataOffset, rpi_value);


  in_sem_request <= s_in_sem_request;
  out_sem_valid <= s_out_sem_valid;

  
  process (clk)

    procedure read_onetime is
    begin
      if (count = 0) then
        HFragments <= unsigned(in_data);
        count <= count + 1;
      elsif (count = 1) then
        YPlaneFragments <= unsigned(in_data);
        count <= count + 1;
      elsif (count = 2) then
        YStride <= unsigned(in_data);
        count <= count + 1;
      elsif (count = 3) then
        UVPlaneFragments <= unsigned(in_data);
        count <= count + 1;
      elsif (count = 4) then
        UVStride <= unsigned(in_data);
        count <= count + 1;
      elsif (count = 5) then
        VFragments <= unsigned(in_data);
        count <= count + 1;
      elsif (count = 6) then
        ReconYDataOffset <= unsigned(in_data);
        count <= count + 1;
      elsif (count = 7) then
        ReconUDataOffset <= unsigned(in_data);
        count <= count + 1;
      elsif (count = 8) then
        ReconVDataOffset <= unsigned(in_data);
        count <= count + 1;
      else
--        assert false report "uu.height = "&integer'image(to_integer(in_data)) severity note;
        info_height <= unsigned(in_data);
        read_state <= stt_offset;
        count <= 0;
      end if;
    end procedure read_onetime;

    procedure ReadIn is
    begin
      s_in_request <= '1';
      s_out_sem_valid <= '0';
      s_in_sem_request <= '0';
      if (s_in_request = '1' and in_valid = '1') then
        case read_state is
          when stt_onetime => read_onetime;
          when others =>                  -- when stt_offset
--            assert false report "uu.FrameOfs = "&integer'image(to_integer(in_data)) severity note;
            state <= stt_Ver;
            s_in_request <= '0';
            FrameOfs <= unsigned(in_data);
        end case;  
      end if;
    end procedure ReadIn;

    procedure Vert is
    begin
      case update_state is
        when stt_1 =>
          update_state <= stt_2;
          case layer is
            when stt_Y =>
              PlaneStride <= YStride; 
              PlaneBorderWidth <= UMV_BORDER; 
              LineFragments <= HFragments; 
              PlaneHeight <= to_integer(info_height); 
              rpi_position <= x"00000000";
              position <= x"00000000";
            when stt_U =>
              PlaneStride <= UVStride; 
              PlaneBorderWidth <= UMV_BORDER / 2; 
              LineFragments <= HFragments / 2; 
              PlaneHeight <= to_integer(info_height) / 2; 
              rpi_position <= YPlaneFragments;
              position <= YPlaneFragments;
            when stt_V =>
              PlaneStride <= UVStride;
              PlaneBorderWidth <= UMV_BORDER / 2;
              LineFragments <= HFragments / 2;
              PlaneHeight <= to_integer(info_height) / 2;
              rpi_position <= YPlaneFragments + UVPlaneFragments;
              position <= YPlaneFragments + UVPlaneFragments;
          end case;
        when stt_2 =>
          update_state <= stt_3;
          SrcPtr1 <= to_integer(FrameOfs) + to_integer(rpi_value);
          DestPtr1 <= to_integer(FrameOfs) + to_integer(rpi_value) - PlaneBorderWidth;
        when stt_3 => 
          update_state <= stt_4;
          rpi_position <= position + LineFragments - 1;  -- Por seguranca eu nao
                                                         -- faco isso no estado
                                                         -- anterior e espero o
                                                         -- proximo estado pela resposta
        when stt_4 => 
          update_state <= stt_5;
          SrcPtr2 <= to_integer(FrameOfs) + to_integer(rpi_value) + (HFRAGPIXELS) - 1;
          DestPtr2 <= to_integer(FrameOfs) + to_integer(rpi_value) + HFRAGPIXELS;
        when stt_5 => 
          if (count = PlaneHeight) then
            count <= 0;
            count2 <= 0;
            update_state <= stt_1;
            case layer is
              when stt_Y =>
                layer <= stt_U;
              when stt_U =>
                layer <= stt_V;
              when stt_V =>
                layer <= stt_Y;
                state <= stt_Hor;
            end case;
          else
            save_state <= state;
            case update_int_state is
              when stt_read1 =>
                update_int_state <= stt_read2;
                state <= stt_ReadMem;
                in_sem_addr <= SHIFT_RIGHT(to_unsigned(SrcPtr1,20),2);
                s_in_sem_request <= '1';

              when stt_read2 =>
                update_int_state <= stt_wait;
                state <= stt_ReadMem;
                in_sem_addr <= SHIFT_RIGHT(to_unsigned(SrcPtr2,20), 2);
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
                  count2 <= 0;
                  count <= count + 1;
                  update_int_state <= stt_read1;
                  SrcPtr1 <= SrcPtr1 + to_integer(PlaneStride);
                  SrcPtr2 <= SrcPtr2 + to_integer(PlaneStride);
                  DestPtr1 <= DestPtr1 + to_integer(PlaneStride);
                  DestPtr2 <= DestPtr2 + to_integer(PlaneStride);
                else
                  update_int_state <= stt_write2;
                  out_sem_addr <= SHIFT_RIGHT(to_unsigned(DestPtr1 + count2,20), 2);
                  out_sem_data <= copy1;
                  s_out_sem_valid <= '1';
                  state <= stt_WriteMem;
                end if;

              when stt_write2 =>
                update_int_state <= stt_write1;
                out_sem_addr <= SHIFT_RIGHT(to_unsigned(DestPtr2 + count2,20), 2);
                count2 <= count2 + 4;
                out_sem_data <= copy2;
                s_out_sem_valid <= '1';
                state <= stt_WriteMem;
            end case;
          end if;
        when others => null;
      end case;
    end procedure Vert;

    procedure Horz is
    begin
      case update_state is
        when stt_1 =>
          update_state <= stt_2;
          case layer is
            when stt_Y =>
              BlockVStep <= to_integer(YStride) * (VFRAGPIXELS - 1);
              PlaneStride <= YStride; 
              PlaneBorderWidth <= UMV_BORDER;
              PlaneFragments <= to_integer(YPlaneFragments);
              LineFragments <= HFragments; 
              rpi_position <= x"00000000";
              position <= x"00000000";
            when stt_U =>
              BlockVStep <= to_integer(UVStride) * (VFRAGPIXELS - 1);
              PlaneStride <= UVStride; 
              PlaneBorderWidth <= UMV_BORDER / 2 ;
              PlaneFragments <= to_integer(UVPlaneFragments);
              LineFragments <= HFragments / 2; 
              rpi_position <= YPlaneFragments;
              position <= YPlaneFragments;
            when stt_V =>
              BlockVStep <= to_integer(UVStride) * (VFRAGPIXELS - 1);
              PlaneStride <= UVStride; 
              PlaneBorderWidth <= UMV_BORDER / 2 ;
              PlaneFragments <= to_integer(UVPlaneFragments);
              LineFragments <= HFragments / 2; 
              rpi_position <= YPlaneFragments + UVPlaneFragments;
              position <= YPlaneFragments + UVPlaneFragments;
          end case;
        when stt_2 =>
          update_state <= stt_3;
          SrcPtr1 <= to_integer(FrameOfs) + to_integer(rpi_value) - PlaneBorderWidth;
          DestPtr1 <= to_integer(FrameOfs) + to_integer(rpi_value) - PlaneBorderWidth*(to_integer(PlaneStride) + 1); 
        when stt_3 => 
          update_state <= stt_4;
          rpi_position <= position + PlaneFragments - LineFragments;  -- Por seguranca eu nao
                                                         -- faco isso no estado
                                                         -- anterior e espero o
                                                         -- proximo estado pela resposta
        when stt_4 => 
          update_state <= stt_5;
          SrcPtr2 <= to_integer(FrameOfs) + to_integer(rpi_value) + BlockVStep - PlaneBorderWidth;
          DestPtr2 <= to_integer(FrameOfs) + to_integer(rpi_value) + BlockVStep - PlaneBorderWidth + to_integer(PlaneStride);
        when stt_5 => 
          if (count = PlaneStride) then
            count <= 0;
            count2 <= 0;
            update_state <= stt_1;
            case layer is
              when stt_Y =>
                layer <= stt_U;
              when stt_U =>
                layer <= stt_V;
              when stt_V =>
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
                in_sem_addr <= SHIFT_RIGHT(to_unsigned(SrcPtr1,20),2);
                s_in_sem_request <= '1';
                
              when stt_read2 =>
                update_int_state <= stt_wait;
                state <= stt_ReadMem;
                in_sem_addr <= SHIFT_RIGHT(to_unsigned(SrcPtr2,20), 2);
                s_in_sem_request <= '1';
                copy1 <= mem_rd_data;
                
              when stt_wait =>
                update_int_state <= stt_write1;
                copy2 <= mem_rd_data;

              when stt_write1 =>
                if (count2 = PlaneBorderWidth) then
                  count2 <= 0;
                  count <= count + 4;
                  update_int_state <= stt_read1;
                  SrcPtr1 <= SrcPtr1 + 4;
                  SrcPtr2 <= SrcPtr2 + 4;
                else
                  update_int_state <= stt_write2;
                  out_sem_addr <= SHIFT_RIGHT(to_unsigned(DestPtr1_i,20), 2);
                  out_sem_data <= copy1;
                  s_out_sem_valid <= '1';
                  state <= stt_WriteMem;
                end if;

              when stt_write2 =>
                update_int_state <= stt_write1;
                out_sem_addr <= SHIFT_RIGHT(to_unsigned(DestPtr2_i,20), 2);
                count2 <= count2 + 1;
                DestPtr1_i <= DestPtr1_i + to_integer(PlaneStride);
                DestPtr2_i <= DestPtr2_i + to_integer(PlaneStride);
                out_sem_data <= copy2;
                s_out_sem_valid <= '1';
                state <= stt_WriteMem;
            end case;
          end if;
        when others => null;
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
      	state <= stt_readin;
      	read_state <= stt_offset;
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
    if (Reset_n = '0') then
      s_out_done <= '0';
      s_in_request <= '0';
      layer <= stt_Y;
      read_state <= stt_onetime;
      state <= stt_readin;
      update_int_state <= stt_read1;
      update_state <= stt_1;
      count <= 0;
      count2 <= 0;
      s_in_sem_request <= '0';
      s_out_sem_valid <= '0';

      rpi_position <= "00000000000000000000000000000000";
      HFragments <="11111111111111111111111111111111";
      VFragments <="00000000000000000000000000000000";
      YStride <= "00000000000000000000000000000000";
      UVStride <= "00000000000000000000000000000000";
      YPlaneFragments <= "00000000000000000000000000000000";
      UVPlaneFragments <= "00000000000000000000000000000000";
      ReconYDataOffset <= "00000000000000000000000000000000";
      ReconUDataOffset <= "00000000000000000000000000000000";
      ReconVDataOffset <= "00000000000000000000000000000000";
      info_height	<= "00000000000000000000000000000000";
    elsif (clk'event and clk = '1') then
      if (Enable = '1') then
        case state is
          when stt_readin => ReadIn;
          when stt_Ver => Vert;
          when stt_Hor => Horz;
          when stt_Done => Done;
          when stt_WriteMem => WriteMemory;
          when stt_ReadMem => ReadMemory;
          when others => ReadIn; state <= stt_readin;
        end case;
      end if;
    end if;
  end process;
  
end a_UpdateUMV;
