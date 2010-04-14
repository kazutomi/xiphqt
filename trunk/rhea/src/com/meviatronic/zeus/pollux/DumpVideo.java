/* Copyright (C) <2009> Maik Merten <maikmerten@googlemail.com>
 * Copyright (C) <2004> Wim Taymans <wim@fluendo.com> (TheoraDec.java parts)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
package com.meviatronic.zeus.pollux;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.Date;
import java.util.HashMap;
import java.util.HashSet;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.logging.Level;
import java.util.logging.Logger;
import org.xiph.ogg.Packet;
import org.xiph.ogg.Page;
import org.xiph.ogg.StreamState;
import org.xiph.ogg.SyncState;

/**
 * This class borrows code from TheoraDec.java
 */
public class DumpVideo {

    public static final Integer OK = new Integer(0);
    public static final Integer ERROR = new Integer(-5);
    private static final byte[] signature = {-128, 0x74, 0x68, 0x65, 0x6f, 0x72, 0x61};
    Logger log = Logger.getLogger(this.getClass().getName());

    private class TheoraDecoderWrapper {

        private VideoReader info = new VideoReader();
        private TheoraDecoder dec;
        private short[][] ycbcrdata;
        int packet = 0;
        int frame = 0;

        public TheoraDecoderWrapper() {
            super();
        }

        public boolean takePacket(Packet p) {

            if (packet < 3) {
                try {
                    info.readMediaInformation(p);
                    packet++;

                    if (packet == 3) {
                        dec = new TheoraDecoder(info);
                    }

                } catch (Exception ex) {
                    Logger.getLogger(DumpVideo.class.getName()).log(Level.SEVERE, null, ex);
                }
            } else {
                try {
                    dec.synthesis(p);
                    ycbcrdata = dec.getYCbCrData();

                    System.out.println("Decoded frame: " + ++frame);

                } catch (Exception ex) {
                    Logger.getLogger(DumpVideo.class.getName()).log(Level.SEVERE, null, ex);
                }
            }
            return true;
        }
    }

    private class YUVWriter {

        private OutputStream os;
        private boolean wroteHeader = false;
        private byte[] ybytes;
        private byte[] uvbytes;
        private boolean raw;

        public YUVWriter(File outfile, boolean raw) {
            this.raw = raw;
            try {
                os = new FileOutputStream(outfile);
            } catch (FileNotFoundException ex) {
                ex.printStackTrace();
            }
        }

        public void writeYUVFrame(VideoReader ti, short[][] yuv) {

            int width = ti.codedPictureWidth;
            int height = ti.codedPictureHeight;

            // TODO: Support other modes than 4:2:0!!!
            int uvwidth = width / 2;
            int uvheight = height / 2;

            int fps_numerator = (int) ti.frameRate * 100000;
            int fps_denominator = 100000;
            int aspect_numerator = ti.aspectRatio.width;
            int aspect_denominator = ti.aspectRatio.height;

            // TODO: crop image according to offset

            try {
                if (!raw) {
                    if (!wroteHeader) {
                        String headerstring = "YUV4MPEG2 W" + width + " H" + height + " F" + fps_numerator + ":" + fps_denominator + " Ip A" + aspect_numerator + ":" + aspect_denominator + "\n";
                        os.write(headerstring.getBytes());
                        wroteHeader = true;
                    }
                    os.write("FRAME\n".getBytes());
                }

                if (ybytes == null || ybytes.length != yuv[0].length) {
                    ybytes = new byte[yuv[0].length];
                }


                // image from decoder is upside-down, so mirror it vertically
                for (int line = 0; line < height; ++line) {
                    int offset = line * width;
                    int offset2 = ((height - 1) - line) * width;
                    for (int x = 0; x < width; ++x) {
                        ybytes[offset2 + x] = (byte) yuv[0][offset + x];
                    }
                }

                os.write(ybytes);

                if (uvbytes == null || uvbytes.length != yuv[1].length) {
                    uvbytes = new byte[yuv[1].length];
                }

                for (int line = 0; line < uvheight; ++line) {
                    int offset = line * uvwidth;
                    int offset2 = ((uvheight - 1) - line) * uvwidth;
                    for (int x = 0; x < uvwidth; ++x) {
                        uvbytes[offset2 + x] = (byte) yuv[1][offset + x];
                    }
                }
                os.write(uvbytes);

                if (uvbytes == null || uvbytes.length != yuv[2].length) {
                    uvbytes = new byte[yuv[2].length];
                }

                for (int line = 0; line < uvheight; ++line) {
                    int offset = line * uvwidth;
                    int offset2 = ((uvheight - 1) - line) * uvwidth;
                    for (int x = 0; x < uvwidth; ++x) {
                        uvbytes[offset2 + x] = (byte) yuv[2][offset + x];
                    }
                }
                os.write(uvbytes);

            } catch (IOException ex) {
                ex.printStackTrace();
            }
        }
    }

    public boolean isTheora(Packet op) {
        return typeFind(op.packetBase, op.packet, op.bytes) > 0;
    }

    private boolean startsWith(byte[] data, int offset, int length, byte[] signature) {
        if (length < signature.length) {
            return false;
        }

        for (int i = 0; i < signature.length; ++i) {
            if (data[offset + i] != signature[i]) {
                return false;
            }
        }

        return true;
    }

    public int typeFind(byte[] data, int offset, int length) {
        if (startsWith(data, offset, length, signature)) {
            return 10;
        }
        return -1;
    }

    public void dumpVideo(File videofile, List outfiles, boolean raw) throws IOException {
        InputStream is = new FileInputStream(videofile);

        boolean onlytime = outfiles.size() == 0;

        SyncState oy = new SyncState();
        Page og = new Page();
        Packet op = new Packet();
        byte[] buf = new byte[512];

        Map streamstates = new HashMap();
        Map theoradecoders = new HashMap();
        Map yuvwriters = new HashMap();
        Set hasdecoder = new HashSet();

        
        int read = is.read(buf);
        while (read > 0) {
            int offset = oy.buffer(read);
            java.lang.System.arraycopy(buf, 0, oy.data, offset, read);
            oy.wrote(read);

            while (oy.pageout(og) == 1) {

                Integer serialno = new Integer(og.serialno());

                StreamState state = (StreamState) streamstates.get(serialno);
                if (state == null) {
                    state = new StreamState();
                    state.init(serialno.intValue());
                    streamstates.put(serialno, state);
                    log.info("created StreamState for stream no. " + og.serialno());
                }

                state.pagein(og);

                while (state.packetout(op) == 1) {

                    if (!(hasdecoder.contains(serialno)) && isTheora(op)) {

                        TheoraDecoderWrapper theoradec = (TheoraDecoderWrapper) theoradecoders.get(serialno);
                        if (theoradec == null) {
                            theoradec = new TheoraDecoderWrapper();
                            theoradecoders.put(serialno, theoradec);
                            hasdecoder.add(serialno);
                        }

                        log.info("is Theora: " + serialno);
                    }

                    TheoraDecoderWrapper theoradec = (TheoraDecoderWrapper) theoradecoders.get(serialno);

                    if (theoradec != null) {
                        theoradec.takePacket(op);

                        if (!onlytime && theoradec.ycbcrdata != null) {
                            YUVWriter yuvwriter = (YUVWriter) yuvwriters.get(serialno);
                            if (yuvwriter == null && !outfiles.isEmpty()) {
                                yuvwriter = new YUVWriter((File) outfiles.get(0), raw);
                                yuvwriters.put(serialno, yuvwriter);
                                outfiles.remove(0);
                            }

                            if (yuvwriter != null) {
                                yuvwriter.writeYUVFrame(theoradec.info, theoradec.ycbcrdata);
                            }
                        }
                    }
                }
            }
            read = is.read(buf);
        }

    }

    public static void main(String[] args) throws Exception {

        if (args.length < 1) {
            System.err.println("usage: DumpVideo <videofile> [<outfile_1> ... <outfile_n>] [--raw>]");
            System.exit(1);
        }

        boolean raw = false;
        File infile = new File(args[0]);

        List outfiles = new LinkedList();


        for (int i = 1; i < args.length; ++i) {
            if (args[i].equals("--raw")) {
                raw = true;
                break;
            }
            outfiles.add(new File(args[i]));
        }

        if(outfiles.size() == 0) {
            System.out.println("no output files given, will only time decode");
        }

        DumpVideo dv = new DumpVideo();

        Date start = new Date();
        dv.dumpVideo(infile, outfiles, raw);
        Date end = new Date();

        System.out.println("time: " + (end.getTime() - start.getTime()) + " milliseconds");

    }
}

