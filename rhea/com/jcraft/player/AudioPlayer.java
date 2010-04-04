/* Audio player, created by JCraft.
 *
 * Audio player (c) 2000 ymnk, JCraft,Inc. <ymnk@jcraft.com>
 *
 * Many thanks to
 *   Monty <monty@xiph.org> and
 *   The XIPHOPHORUS Company http://www.xiph.org/ .
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License
 * as published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
   
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */


package com.jcraft.player;

import com.meviatronic.zeus.castor.*;
import com.meviatronic.zeus.helen.*;
import org.xiph.ogg.*;

import java.util.*;
import java.net.*;
import java.io.*;
import java.awt.*;
import java.awt.event.*;
import java.applet.*;
import javax.swing.*;
import javax.sound.sampled.*;

/**
 * @author Xiph, Michael Scheerer (Adaptions, Fixes)
 */
public class AudioPlayer extends JApplet implements ActionListener, Runnable{

  private static final long serialVersionUID=1L;

  boolean running_as_applet=true;

  Thread player=null;
  InputStream bitStream=null;

  int udp_port=-1;
  String udp_baddress=null;

  static AppletContext acontext=null;

  static final int BUFSIZE=4096*2;
  
  static byte[] convbuffer;

  private int RETRY=3;
  int retry=RETRY;

  String playlistfile="playlist";

  boolean icestats=false;

  SyncState oy;
  StreamState os;
  Page og;
  Packet op;
  AudioReader vi;
  Output vo;

  byte[] buffer=null;
  int bytes=0;

  int format;
  int sampleRate=0;
  int channels=0;
  int left_vol_scale=100;
  int right_vol_scale=100;
  SourceDataLine outputLine=null;
  String current_source=null;

  int frameSizeInBytes;
  int bufferLengthInBytes;

  boolean playonstartup=false;

  public void init(){
    running_as_applet=true;

    acontext=getAppletContext();

    String s=getParameter("jorbis.player.playlist");
    playlistfile=s;

    s=getParameter("jorbis.player.icestats");
    if(s!=null&&s.equals("yes")){
      icestats=true;
    }

    loadPlaylist();
    initUI();

    if(playlist.size()>0){
      s=getParameter("jorbis.player.playonstartup");
      if(s!=null&&s.equals("yes")){
        playonstartup=true;
      }
    }

    setBackground(Color.lightGray);
    //  setBackground(Color.white);
    getContentPane().setLayout(new BorderLayout());
    getContentPane().add(panel);
  }

  public void start(){
    super.start();
    if(playonstartup){
      play_sound();
    }
  }

	void init_jorbis(){
    oy=new SyncState();
    os=new StreamState();
    og=new Page();
    op=new Packet();
	vi=new AudioReader();
    
    buffer=null;
    bytes=0;
	oy.init();
  }

  SourceDataLine getOutputLine(int channels, int sampleRate){
    if(outputLine==null||this.sampleRate!=sampleRate||this.channels!=channels){
      if(outputLine!=null){
        outputLine.drain();
        outputLine.stop();
        outputLine.close();
      }
      init_audio(channels, sampleRate);
      outputLine.start();
    }
    return outputLine;
  }

  void init_audio(int channels, int sampleRate){
    try{
      //ClassLoader originalClassLoader=null;
      //try{
      //  originalClassLoader=Thread.currentThread().getContextClassLoader();
      //  Thread.currentThread().setContextClassLoader(ClassLoader.getSystemClassLoader());
      //}
      //catch(Exception ee){
      //  System.out.println(ee);
      //}
      AudioFormat audioFormat=new AudioFormat((float)sampleRate, 16, channels, true, // PCM_Signed
          false // littleEndian
      );
      DataLine.Info info=new DataLine.Info(SourceDataLine.class, audioFormat, AudioSystem.NOT_SPECIFIED);
      if(!AudioSystem.isLineSupported(info)){
        //System.out.println("Line " + info + " not supported.");
        return;
      }

      try{
        outputLine=(SourceDataLine)AudioSystem.getLine(info);
        //outputLine.addLineListener(this);
        outputLine.open(audioFormat);
      }
      catch(LineUnavailableException ex){
        System.out.println("Unable to open the sourceDataLine: "+ex);
        return;
      }
      catch(IllegalArgumentException ex){
        System.out.println("Illegal Argument: "+ex);
        return;
      }

      frameSizeInBytes=audioFormat.getFrameSize();
      int bufferLengthInFrames=outputLine.getBufferSize()/frameSizeInBytes/2;
      bufferLengthInBytes=bufferLengthInFrames*frameSizeInBytes;

      //if(originalClassLoader!=null)
      //  Thread.currentThread().setContextClassLoader(originalClassLoader);

      this.sampleRate=sampleRate;
      this.channels=channels;
    }
    catch(Exception ee){
      System.out.println(ee);
    }
  }

  private int item2index(String item){
    for(int i=0; i<cb.getItemCount(); i++){
      String foo=(String)(cb.getItemAt(i));
      if(item.equals(foo))
        return i;
    }
    cb.addItem(item);
    return cb.getItemCount()-1;
  }

	public synchronized void run(){
    Thread me=Thread.currentThread();
    String item=(String)(cb.getSelectedItem());
    int current_index=item2index(item);
	 while(true){
      item=(String)(cb.getItemAt(current_index));
		cb.setSelectedIndex(current_index);
      bitStream=selectSource(item);
      if(bitStream!=null){
        if(udp_port!=-1){
          play_udp_stream(me);
        }
		  else{
			  play_stream(me,item);
        }
      }
      else if(cb.getItemCount()==1){
        break;
      }
      if(player!=me){
        break;
      }
      
      current_index++;
      if(current_index>=cb.getItemCount()){
        current_index=0;
      }
      if(cb.getItemCount()<=0)
			break;
    }
     
	  start_button.setText("start");
	  stop();
  }

  private void play_stream(Thread me,String item){

    boolean chained=false;

    init_jorbis();

    retry=RETRY;

    //System.out.println("play_stream>");

    loop: while(true){
      int eos=0;

      int index=oy.buffer(BUFSIZE);
      buffer=oy.data;
      try{
        bytes=bitStream.read(buffer, index, BUFSIZE);
      }
      catch(Exception e){
        System.err.println(e+" "+buffer+" "+bitStream);
        return;
      }
      oy.wrote(bytes);

      if(chained){ //
        chained=false; //
      } //
      else{ //
        if(oy.pageout(og)!=1){
          if(bytes<BUFSIZE)
            break;
          System.err.println("Input does not appear to be an Ogg bitstream.");
          return;
        }
      } //
      os.init(og.serialno());
      os.reset();

      if(os.pagein(og)<0){
        // error; stream version mismatch perhaps
        System.err.println("Error reading first page of Ogg bitstream data.");
        return;
      }

      retry=RETRY;

      if(os.packetout(op)!=1){
        // no page? must not be vorbis
        System.out.println("Error reading initial header packet.");
        break;
        //      return;
      }
		
		try {
			vi.readMediaInformation(op);
		} catch (Exception e) {
			System.out.println("This Ogg bitstream does not contain Vorbis audio data.");
			stop();
			return;
		}
      

      int i=0;

      while(i<2){
        while(i<2){
          int result=oy.pageout(og);
          if(result==0)
            break; // Need more data
          if(result==1){
            os.pagein(og);
            while(i<2){
              result=os.packetout(op);
              if(result==0)
                break;
              if(result==-1){
                System.out.println("Corrupt secondary header.  Exiting.");
                //return;
                break loop;
			  }
				try {
					vi.readMediaInformation(op);
				} catch (Exception e) {
					stop();
				}
              i++;
            }
          }
        }

        index=oy.buffer(BUFSIZE);
        buffer=oy.data;
        try{
          bytes=bitStream.read(buffer, index, BUFSIZE);
        }
        catch(Exception e){
          System.err.println(e);
          return;
        }
        if(bytes==0&&i<2){
          System.err.println("End of file before finding all Vorbis headers!");
          return;
        }
        oy.wrote(bytes);
      }
		
		
		System.out.println(vi.getOggCommentContent());
		if (acontext != null) {
			acontext.showStatus(vi.getOggCommentContent());
		}
		   
		if (vo == null) {
			vo = new VorbisDecoder(vi);
		}
     
      float[][][] pcmf = new float [1][][];
      int[] _index=new int[vi.channels];

      getOutputLine(vi.channels, vi.sampleRate);

      while(eos==0){
        while(eos==0){

			if(player!=me){
				stop();
            	return;
          }

          int result=oy.pageout(og);
          if(result==0)
            break; // need more data
          if(result==-1){ // missing or corrupt data at this page position
          //	    System.err.println("Corrupt or missing data in bitstream; continuing...");
          }
          else{
            os.pagein(og);

            if(og.granulepos()==0){ //
              chained=true; //
              eos=1; //
              break; //
            } //

            while(true){
              result=os.packetout(op);
              if(result==0)
                break; // need more data
              if(result==-1){ // missing or corrupt data at this page position
                // no reason to complain; already complained above

                //System.err.println("no reason to complain; already complained above");
              }
              else{
                // we have a packet.  Decode it
				  
				  try {
					convbuffer = vo.synthesis(op);
				  } catch (Exception e) {
					  
				  }
				  outputLine.write(convbuffer, 2 * vo.getSampleOffset(), 2 * vo.getNumberOfSamples());
              }
            }
            if(og.eos()!=0)
              eos=1;
          }
        }

		  if(eos==0){
          index=oy.buffer(BUFSIZE);
          buffer=oy.data;
          try{
            bytes=bitStream.read(buffer, index, BUFSIZE);
          }
          catch(Exception e){
            System.err.println(e);
            return;
          }
          if(bytes==-1){
            break;
          }
          oy.wrote(bytes);
          if(bytes==0)
            eos=1;
        }
      }
    }
	stop();
  }

  private void play_udp_stream(Thread me){
    init_jorbis();

    try{
      loop: while(true){
        int index=oy.buffer(BUFSIZE);
        buffer=oy.data;
        try{
          bytes=bitStream.read(buffer, index, BUFSIZE);
        }
        catch(Exception e){
          System.err.println(e);
          return;
        }

        oy.wrote(bytes);
        if(oy.pageout(og)!=1){
          //        if(bytes<BUFSIZE)break;
          System.err.println("Input does not appear to be an Ogg bitstream.");
          return;
        }

        os.init(og.serialno());
        os.reset();

        if(os.pagein(og)<0){
          // error; stream version mismatch perhaps
          System.err.println("Error reading first page of Ogg bitstream data.");
          return;
        }

        if(os.packetout(op)!=1){
          // no page? must not be vorbis
          System.err.println("Error reading initial header packet.");
          //        break;
          return;
        }
		  
		  try {
			  vi.readMediaInformation(op);
		  } catch (Exception e) {
			  stop();
		  }
        

        int i=0;
        while(i<2){
          while(i<2){
            int result=oy.pageout(og);
            if(result==0)
              break; // Need more data
            if(result==1){
              os.pagein(og);
              while(i<2){
                result=os.packetout(op);
                if(result==0)
                  break;
                if(result==-1){
                  System.err.println("Corrupt secondary header.  Exiting.");
                  //return;
                  break loop;
				}
				  
				  try {
					  vi.readMediaInformation(op);
				  } catch (Exception e) {
					  stop();
				  }
                i++;
              }
            }
          }

          if(i==2)
            break;

          index=oy.buffer(BUFSIZE);
          buffer=oy.data;
          try{
            bytes=bitStream.read(buffer, index, BUFSIZE);
          }
			catch(Exception e){
				stop();
            System.err.println(e);
            return;
          }
			if(bytes==0&&i<2){
				stop();
            System.err
                .println("End of file before finding all Vorbis headers!");
            return;
          }
          oy.wrote(bytes);
        }
        break;
      }
    }
    catch(Exception e){
    }
	  
    stop();

    UDPIO io=null;
    io=new UDPIO(udp_port);
    

    bitStream=io;
    play_stream(me,null);
  }

	public synchronized void stop(){
    	if (os != null) {
			os.close();
		}
		if (vo != null) {
			vo.close();
		}
		if (vi != null) {
			vi.close();
		}
		if (oy != null) {
			oy.close();
		}
		if (og != null) {
			og.close();
		}
		if (outputLine != null) {
			outputLine.drain();
        	outputLine.stop();
        	outputLine.close();
			outputLine = null;
		}
		try {
			if(bitStream != null) {
				bitStream.close();
			}
			bitStream = null;
		} catch (Exception e) {
			
		}
		
		os = null;
      	vo = null;
      	vi = null;
		oy = null;
		og = null;
		buffer = null;
  	}

  Vector playlist=new Vector();

  public void actionPerformed(ActionEvent e){

    if(e.getSource()==stats_button){
      String item=(String)(cb.getSelectedItem());
      if(!item.startsWith("http://"))
        return;
      if(item.endsWith(".pls")){
        item=fetch_pls(item);
        if(item==null)
          return;
      }
      else if(item.endsWith(".m3u")){
        item=fetch_m3u(item);
        if(item==null)
          return;
      }
      byte[] foo=item.getBytes();
      for(int i=foo.length-1; i>=0; i--){
        if(foo[i]=='/'){
          item=item.substring(0, i+1)+"stats.xml";
          break;
        }
      }
      System.out.println(item);
      try{
        URL url=null;
        if(running_as_applet)
          url=new URL(getCodeBase(), item);
        else
          url=new URL(item);
        BufferedReader stats=new BufferedReader(new InputStreamReader(url
            .openConnection().getInputStream()));
        while(true){
          String bar=stats.readLine();
          if(bar==null)
            break;
          System.out.println(bar);
        }
      }
      catch(Exception ee){
        System.err.println(ee);
      }
      return;
    }

    String command=((JButton)(e.getSource())).getText();
    if(command.equals("start")&&player==null){
      play_sound();
    }
    else if(player!=null){
      stop_sound();
    }
  }

  public String getTitle(){
    return (String)(cb.getSelectedItem());
  }

  public void play_sound(){
    if(player!=null)
      return;
    player=new Thread(this);
    start_button.setText("stop");
    player.start();
  }

	public void stop_sound(){
    if(player==null)
      return;
	  player=null;
		start_button.setText("start");
  }

  InputStream selectSource(String item){
    if(item.endsWith(".pls")){
      item=fetch_pls(item);
      if(item==null)
        return null;
      //System.out.println("fetch: "+item);
    }
    else if(item.endsWith(".m3u")){
      item=fetch_m3u(item);
      if(item==null)
        return null;
      //System.out.println("fetch: "+item);
    }

    if(!item.endsWith(".ogg")){
      return null;
    }

    InputStream is=null;
    URLConnection urlc=null;
    try{
      URL url=null;
      if(running_as_applet)
        url=new URL(getCodeBase(), item);
      else
        url=new URL(item);
      urlc=url.openConnection();
      is=urlc.getInputStream();
      current_source=url.getProtocol()+"://"+url.getHost()+":"+url.getPort()
          +url.getFile();
    }
    catch(Exception ee){
      //System.err.println(ee);
    }

    if(is==null&&!running_as_applet){
      try{
        is=new FileInputStream(System.getProperty("user.dir")
            +System.getProperty("file.separator")+item);
        current_source=null;
      }
      catch(Exception ee){
        //System.err.println(ee);
      }
    }

    if(is==null)
      return null;

    //System.out.println("Select: "+item);

    {
      boolean find=false;
      for(int i=0; i<cb.getItemCount(); i++){
        String foo=(String)(cb.getItemAt(i));
        if(item.equals(foo)){
          find=true;
          break;
        }
      }
      if(!find){
        cb.addItem(item);
      }
    }

    int i=0;
    String s=null;
    String t=null;
    udp_port=-1;
    udp_baddress=null;
    while(urlc!=null&&true){
      s=urlc.getHeaderField(i);
      t=urlc.getHeaderFieldKey(i);
      if(s==null)
        break;
      i++;
      if(t!=null&&t.equals("udp-port")){
        try{
          udp_port=Integer.parseInt(s);
        }
        catch(Exception ee){
          System.err.println(ee);
        }
      }
      else if(t!=null&&t.equals("udp-broadcast-address")){
        udp_baddress=s;
      }
    }
    return is;
  }

  String fetch_pls(String pls){
    InputStream pstream=null;
    if(pls.startsWith("http://")){
      try{
        URL url=null;
        if(running_as_applet)
          url=new URL(getCodeBase(), pls);
        else
          url=new URL(pls);
        URLConnection urlc=url.openConnection();
        pstream=urlc.getInputStream();
      }
      catch(Exception ee){
        System.err.println(ee);
        return null;
      }
    }
    if(pstream==null&&!running_as_applet){
      try{
        pstream=new FileInputStream(System.getProperty("user.dir")
            +System.getProperty("file.separator")+pls);
      }
      catch(Exception ee){
        System.err.println(ee);
        return null;
      }
    }

    String line=null;
    while(true){
      try{
        line=readline(pstream);
      }
      catch(Exception e){
      }
      if(line==null)
        break;
      if(line.startsWith("File1=")){
        byte[] foo=line.getBytes();
        int i=6;
        for(; i<foo.length; i++){
          if(foo[i]==0x0d)
            break;
        }
        return line.substring(6, i);
      }
    }
    return null;
  }

  String fetch_m3u(String m3u){
    InputStream pstream=null;
    if(m3u.startsWith("http://")){
      try{
        URL url=null;
        if(running_as_applet)
          url=new URL(getCodeBase(), m3u);
        else
          url=new URL(m3u);
        URLConnection urlc=url.openConnection();
        pstream=urlc.getInputStream();
      }
      catch(Exception ee){
        System.err.println(ee);
        return null;
      }
    }
    if(pstream==null&&!running_as_applet){
      try{
        pstream=new FileInputStream(System.getProperty("user.dir")
            +System.getProperty("file.separator")+m3u);
      }
      catch(Exception ee){
        System.err.println(ee);
        return null;
      }
    }

    String line=null;
    while(true){
      try{
        line=readline(pstream);
      }
      catch(Exception e){
      }
      if(line==null)
        break;
      return line;
    }
    return null;
  }

  void loadPlaylist(){

    if(running_as_applet){
      String s=null;
      for(int i=0; i<10; i++){
        s=getParameter("jorbis.player.play."+i);
        if(s==null)
          break;
        playlist.addElement(s);
      }
    }

    if(playlistfile==null){
      return;
    }

    try{
      InputStream is=null;
      try{
        URL url=null;
        if(running_as_applet)
          url=new URL(getCodeBase(), playlistfile);
        else
          url=new URL(playlistfile);
        URLConnection urlc=url.openConnection();
        is=urlc.getInputStream();
      }
      catch(Exception ee){
      }
      if(is==null&&!running_as_applet){
        try{
          is=new FileInputStream(System.getProperty("user.dir")
              +System.getProperty("file.separator")+playlistfile);
        }
        catch(Exception ee){
        }
      }

      if(is==null)
        return;

      while(true){
        String line=readline(is);
        if(line==null)
          break;
        byte[] foo=line.getBytes();
        for(int i=0; i<foo.length; i++){
          if(foo[i]==0x0d){
            line=new String(foo, 0, i);
            break;
          }
        }
        playlist.addElement(line);
      }
    }
    catch(Exception e){
      System.out.println(e);
    }
  }

  private String readline(InputStream is){
    StringBuffer rtn=new StringBuffer();
    int temp;
    do{
      try{
        temp=is.read();
      }
      catch(Exception e){
        return (null);
      }
      if(temp==-1){
        String str=rtn.toString();
        if(str.length()==0)
          return (null);
        return str;
      }
      if(temp!=0&&temp!='\n'&&temp!='\r')
        rtn.append((char)temp);
    }
    while(temp!='\n'&&temp!='\r');
    return (rtn.toString());
  }

  JPanel panel;
  JComboBox cb;
  JButton start_button;
  JButton stats_button;

  void initUI(){
    panel=new JPanel();

    cb=new JComboBox(playlist);
    cb.setEditable(true);
    panel.add(cb);

    start_button=new JButton("start");
    start_button.addActionListener(this);
    panel.add(start_button);

    if(icestats){
      stats_button=new JButton("IceStats");
      stats_button.addActionListener(this);
      panel.add(stats_button);
    }
  }

  class UDPIO extends InputStream {
    InetAddress address;
    DatagramSocket socket=null;
    DatagramPacket sndpacket;
    DatagramPacket recpacket;
    byte[] buf=new byte[1024];
    //String host;
    int port;
    byte[] inbuffer=new byte[2048];
    byte[] outbuffer=new byte[1024];
    int instart=0, inend=0, outindex=0;

    UDPIO(int port){
      this.port=port;
      try{
        socket=new DatagramSocket(port);
      }
      catch(Exception e){
        System.err.println(e);
      }
      recpacket=new DatagramPacket(buf, 1024);
    }

    void setTimeout(int i){
      try{
        socket.setSoTimeout(i);
      }
      catch(Exception e){
        System.out.println(e);
      }
    }

    int getByte() throws java.io.IOException{
      if((inend-instart)<1){
        read(1);
      }
      return inbuffer[instart++]&0xff;
    }

    int getByte(byte[] array) throws java.io.IOException{
      return getByte(array, 0, array.length);
    }

    int getByte(byte[] array, int begin, int length) throws java.io.IOException{
      int i=0;
      int foo=begin;
      while(true){
        if((i=(inend-instart))<length){
          if(i!=0){
            System.arraycopy(inbuffer, instart, array, begin, i);
            begin+=i;
            length-=i;
            instart+=i;
          }
          read(length);
          continue;
        }
        System.arraycopy(inbuffer, instart, array, begin, length);
        instart+=length;
        break;
      }
      return begin+length-foo;
    }

    int getShort() throws java.io.IOException{
      if((inend-instart)<2){
        read(2);
      }
      int s=0;
      s=inbuffer[instart++]&0xff;
      s=((s<<8)&0xffff)|(inbuffer[instart++]&0xff);
      return s;
    }

    int getInt() throws java.io.IOException{
      if((inend-instart)<4){
        read(4);
      }
      int i=0;
      i=inbuffer[instart++]&0xff;
      i=((i<<8)&0xffff)|(inbuffer[instart++]&0xff);
      i=((i<<8)&0xffffff)|(inbuffer[instart++]&0xff);
      i=(i<<8)|(inbuffer[instart++]&0xff);
      return i;
    }

    void getPad(int n) throws java.io.IOException{
      int i;
      while(n>0){
        if((i=inend-instart)<n){
          n-=i;
          instart+=i;
          read(n);
          continue;
        }
        instart+=n;
        break;
      }
    }

    void read(int n) throws java.io.IOException{
      if(n>inbuffer.length){
        n=inbuffer.length;
      }
      instart=inend=0;
      int i;
      while(true){
        recpacket.setData(buf, 0, 1024);
        socket.receive(recpacket);

        i=recpacket.getLength();
        System.arraycopy(recpacket.getData(), 0, inbuffer, inend, i);
        if(i==-1){
          throw new java.io.IOException();
        }
        inend+=i;
        break;
      }
    }

	  
	public void close() throws java.io.IOException{
      socket.close();
    }

    public int read() throws java.io.IOException{
      return 0;
    }

    public int read(byte[] array, int begin, int length)
        throws java.io.IOException{
      return getByte(array, begin, length);
    }
  }

  public static void main(String[] arg){

    JFrame frame=new JFrame("AudioPlayer");
    frame.setBackground(Color.lightGray);
    frame.setBackground(Color.white);
    frame.getContentPane().setLayout(new BorderLayout());

    frame.addWindowListener(new WindowAdapter(){
      public void windowClosing(WindowEvent e){
        System.exit(0);
      }
    });

    AudioPlayer player=new AudioPlayer();
    player.running_as_applet=false;

    if(arg.length>0){
      for(int i=0; i<arg.length; i++){
        player.playlist.addElement(arg[i]);
      }
    }

    player.loadPlaylist();
    player.initUI();

    frame.getContentPane().add(player.panel);
    frame.pack();
    frame.setVisible(true);
  }
}
