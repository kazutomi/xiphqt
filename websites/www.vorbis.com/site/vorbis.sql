--
-- Selected TOC Entries:
--
\connect - jack
--
-- TOC Entry ID 2 (OID 27336)
--
-- Name: type_type_id_seq Type: SEQUENCE Owner: jack
--

CREATE SEQUENCE "type_type_id_seq" start 1 increment 1 maxvalue 2147483647 minvalue 1  cache 1 ;

--
-- TOC Entry ID 16 (OID 27355)
--
-- Name: type Type: TABLE Owner: jack
--

CREATE TABLE "type" (
	"type_id" integer DEFAULT nextval('"type_type_id_seq"'::text) NOT NULL,
	"name" character varying(50) NOT NULL,
	"iconfile" character varying(255) NOT NULL,
	Constraint "type_pkey" Primary Key ("type_id")
);

--
-- TOC Entry ID 4 (OID 27371)
--
-- Name: news_news_id_seq Type: SEQUENCE Owner: jack
--

CREATE SEQUENCE "news_news_id_seq" start 1 increment 1 maxvalue 2147483647 minvalue 1  cache 1 ;

--
-- TOC Entry ID 17 (OID 27390)
--
-- Name: news Type: TABLE Owner: jack
--

CREATE TABLE "news" (
	"news_id" integer DEFAULT nextval('"news_news_id_seq"'::text) NOT NULL,
	"type_id" integer,
	"poster" character varying(25) NOT NULL,
	"title" character varying(100) NOT NULL,
	"date_added" timestamp with time zone NOT NULL,
	"blurb" text NOT NULL,
	Constraint "news_pkey" Primary Key ("news_id")
);

--
-- TOC Entry ID 18 (OID 27430)
--
-- Name: platform Type: TABLE Owner: jack
--

CREATE TABLE "platform" (
	"platform_id" integer NOT NULL,
	"name" character varying(100) NOT NULL,
	Constraint "platform_pkey" Primary Key ("platform_id")
);

--
-- TOC Entry ID 6 (OID 27448)
--
-- Name: software_software_id_seq Type: SEQUENCE Owner: jack
--

CREATE SEQUENCE "software_software_id_seq" start 1 increment 1 maxvalue 2147483647 minvalue 1  cache 1 ;

--
-- TOC Entry ID 19 (OID 27467)
--
-- Name: software Type: TABLE Owner: jack
--

CREATE TABLE "software" (
	"software_id" integer DEFAULT nextval('"software_software_id_seq"'::text) NOT NULL,
	"url" character varying(255) NOT NULL,
	"name" character varying(255) NOT NULL,
	"blurb" text NOT NULL,
	"date_added" timestamp with time zone NOT NULL,
	Constraint "software_pkey" Primary Key ("software_id")
);

--
-- TOC Entry ID 20 (OID 27500)
--
-- Name: software_platform Type: TABLE Owner: jack
--

CREATE TABLE "software_platform" (
	"software_id" integer,
	"platform_id" integer
);

--
-- TOC Entry ID 8 (OID 27526)
--
-- Name: software_news_software_news_seq Type: SEQUENCE Owner: jack
--

CREATE SEQUENCE "software_news_software_news_seq" start 1 increment 1 maxvalue 2147483647 minvalue 1  cache 1 ;

--
-- TOC Entry ID 21 (OID 27545)
--
-- Name: software_news Type: TABLE Owner: jack
--

CREATE TABLE "software_news" (
	"software_news_id" integer DEFAULT nextval('"software_news_software_news_seq"'::text) NOT NULL,
	"software_id" integer,
	"date_added" timestamp with time zone NOT NULL,
	"title" character varying(100) NOT NULL,
	"blurb" text NOT NULL,
	Constraint "software_news_pkey" Primary Key ("software_news_id")
);

--
-- TOC Entry ID 10 (OID 27584)
--
-- Name: musicsite_musicsite_id_seq Type: SEQUENCE Owner: jack
--

CREATE SEQUENCE "musicsite_musicsite_id_seq" start 1 increment 1 maxvalue 2147483647 minvalue 1  cache 1 ;

--
-- TOC Entry ID 22 (OID 27603)
--
-- Name: musicsite Type: TABLE Owner: jack
--

CREATE TABLE "musicsite" (
	"musicsite_id" integer DEFAULT nextval('"musicsite_musicsite_id_seq"'::text) NOT NULL,
	"url" character varying(255) NOT NULL,
	"name" character varying(255) NOT NULL,
	"blurb" text NOT NULL,
	"date_added" timestamp with time zone NOT NULL,
	Constraint "musicsite_pkey" Primary Key ("musicsite_id")
);

--
-- TOC Entry ID 12 (OID 27636)
--
-- Name: category_category_id_seq Type: SEQUENCE Owner: jack
--

CREATE SEQUENCE "category_category_id_seq" start 1 increment 1 maxvalue 2147483647 minvalue 1  cache 1 ;

--
-- TOC Entry ID 23 (OID 27655)
--
-- Name: category Type: TABLE Owner: jack
--

CREATE TABLE "category" (
	"category_id" integer DEFAULT nextval('"category_category_id_seq"'::text) NOT NULL,
	"name" character varying(255) NOT NULL,
	"blurb" text NOT NULL,
	Constraint "category_pkey" Primary Key ("category_id")
);

--
-- TOC Entry ID 14 (OID 27686)
--
-- Name: link_link_id_seq Type: SEQUENCE Owner: jack
--

CREATE SEQUENCE "link_link_id_seq" start 1 increment 1 maxvalue 2147483647 minvalue 1  cache 1 ;

--
-- TOC Entry ID 24 (OID 27705)
--
-- Name: link Type: TABLE Owner: jack
--

CREATE TABLE "link" (
	"link_id" integer DEFAULT nextval('"link_link_id_seq"'::text) NOT NULL,
	"category_id" integer,
	"url" character varying(255) NOT NULL,
	"name" character varying(255) NOT NULL,
	"blurb" text NOT NULL,
	"date_added" timestamp with time zone NOT NULL,
	Constraint "link_pkey" Primary Key ("link_id")
);

--
-- Data for TOC Entry ID 26 (OID 27355)
--
-- Name: type Type: TABLE DATA Owner: jack
--


COPY "type"  FROM stdin;
1	Article	/newsicons/newstuff.gif
2	Xiph.org News	/newsicons/xifish-sm.png
3	Ogg News	/newsicons/ogg-sm.png
4	icecast News	/newsicons/icecast-sm.png
\.
--
-- Data for TOC Entry ID 27 (OID 27390)
--
-- Name: news Type: TABLE DATA Owner: jack
--


COPY "news"  FROM stdin;
1	1	jack	test MY news	2001-06-25 01:13:22-06	this is just a test.  DONT READ THIS.\
\
YOU WILL NO1W DIE FOR VIOLATING MY LAW.
3	3	jack	some ogg news	2001-06-30 19:52:21-06	something about ogg.\
\
isn't ogg great?\
\
COME ON NOW BABY AND LICK MY PANTS!
4	4	jack	new icecast released	2001-06-30 19:58:08-06	NOT!\
\
I MADE YOU LOOK!  GOT YOU ALL EXCITED\
DIDN"T! I!\
\
yep. i did.\

5	2	jack	robots have sued xiph.org	2001-06-30 19:59:00-06	we have violated many robot patents.\
\
we must not become part of the robot colony, and live in outer space. development will continue, but now we will be tuning for robot ears.
\.
--
-- Data for TOC Entry ID 28 (OID 27430)
--
-- Name: platform Type: TABLE DATA Owner: jack
--


COPY "platform"  FROM stdin;
1	Linux
2	Windows
3	Legacy MacOS
4	MacOS X
5	Java
6	Win32 Development
7	Linux Development
8	BeOS
9	OS/2
\.
--
-- Data for TOC Entry ID 29 (OID 27467)
--
-- Name: software Type: TABLE DATA Owner: jack
--


COPY "software"  FROM stdin;
6	http://www.panic.com/audion/	Audion 2	Play, Encode, And Perform with Style.  This player for MacOS and MacOS X work great with Vorbis files.	2001-06-30 23:14:57-06
7	http://www.macdvd.org/DropOgg/	DropOgg	A free drag-n-drop encoder for Vorbis for MacOS.  It's AltiVec enhanced.  Also comes with VorbisComment, to manipulate and view the file comments.	2001-06-30 23:16:40-06
8	http://www.proteron.com/n2mp3pro/	N2MP3 Pro	A high quality Vorbis encoder for MacOS. Free download available.	2001-06-30 23:19:24-06
9	http://www.subband.com/macamp/	Macamp	A MacOS vorbis player that has highly integrated streaming features.	2001-06-30 23:21:24-06
10	http://www.nouturn.com/oggdrop/index.html	Ogg Drop for Mac	 Ogg Drop is a freeware application that will encode AIFF and AIFC (.aif) sound files into an Ogg Vorbis (.ogg) stream.  Just Drag-n-Drop!  Works in MacOS 9 and X.	2001-06-30 23:22:45-06
11	http://www.unsanity.com/echo.html	Unsanity Echo	A power audio player for Vorbis that works on both MacOS and MacOS X.  Optimized for G4, and has a great plugin system.	2001-06-30 23:23:56-06
12	http://xtractor.sourceforge.net/	CD-DA X-Tractor	CD-DA X-Tractor is a ripper/encoder for Windows that support Vorbis, intelligent jitter correction, CDDB support and is Open Source.	2001-06-30 23:26:22-06
13	http://admin.dbpoweramp.com/	dBpowerAMP	An audio convertor and an audio player, both supporting Ogg Vorbis files	2001-06-30 23:32:06-06
14	http://djmixpro.com/djmixlite/djmixlite.html	DJ Mix Lite	DJ Mix Lite is designed to play continuous music with cross fading, beat matching mixes between songs.  Use your Ogg files to be a DJ.	2001-06-30 23:34:40-06
15	http://djmixpro.com/djmixpro/djmixpro.html	DJ Mix Pro	Be a DJ with Vorbis!\
\
 DJ Mix Pro is designed to let you choose the songs while it performs by itself quality cross fading, beat matching mixes between songs.\
\
DJ Mix Pro has built in Beatlock Technology, that makes it beat-aware. This technology does not alter your sound files, it just analyses them to find the BPM.\
	2001-06-30 23:36:12-06
16	http://www.etantrum.com/index.php?section=download	eTantrum PMA	The eTantrum personal media agent is a demonstration of eTantrums media recommendation technologies. Runs on both Windows and Linux.	2001-06-30 23:38:00-06
17	http://www.freeamp.org/	FreeAMP	FreeAMP is an open source media player that support Vorbis and other formats.  It has skins, audio fingerprinting, and streaming support.  Works on Windows and Linux.	2001-06-30 23:39:37-06
18	http://www.mgshareware.com/frmmain.html	FreeRip MP3	FreeRip MP3 is a ripper/encoder that support Ogg Vorbis.  Support Ogg Vorbis comments and CDDB lookups.  It can also install the Winamp Ogg plugin automatically.  Only works in Windows.\
	2001-06-30 23:41:53-06
19	http://www.goldwave.com/	GoldWave	A digital audio editor that can import and export Vorbis as well as convert from other file formats.  Only works in Window.  Trial version available.	2001-06-30 23:43:10-06
20	http://www.logipole.com/	Konvertor	File format convertor that supports 291 file formats, including Vorbis.  	2001-06-30 23:45:25-06
21	http://www.musicex.com/mediajukebox/	Media Jukebox	A music organizer, encoder, player for Windows that supports Winamp skins.	2001-06-30 23:49:03-06
22	http://www.quinnware.com/	Quintessential Player	A nice looking media player that support Vorbis.	2001-06-30 23:51:18-06
23	http://www.sonicfoundry.com/Products/NewShowProduct.asp?PID=426	Sound Forge	Sonic Foundry's famous audio edit/processing package.  Now you can import and export Ogg Vorbis files directly from this popular program.	2001-06-30 23:52:28-06
24	http://www.sonicfoundry.com/products/NewShowProduct.asp?PID=187	SIREN	A jukebox application from the makers of Sound Forge and ACID.  SIREN supports encoding and playing of Ogg Vorbis files.	2001-06-30 23:53:15-06
25	http://www.soundprobe.com/	Soundprobe	Soundprobe is an advanced and powerful sound editor, recorder and effects processing package for Windows.  Ogg Vorbis plugins are available.	2001-06-30 23:54:14-06
26	http://members.ozemail.com.au/~spitsw/VorbisExt/	Vorbis Extension	VorbisExt is a property sheet shell extension that can edit the comments in vorbis (.ogg) files and display information about vorbis files.  This app integrates Vorbis Comments right into your Windows desktop!\
	2001-06-30 23:55:26-06
27	http://www.windac.de/	WinDAC	A CD ripper/encoder with plugins available for Ogg Vorbis.	2001-06-30 23:57:08-06
28	http://www.nostatic.org/grip/	GRIP	A popular ripper/encoder/tagger for Linux that supports Vorbis. Supports CDDB and has a built in CD player.	2001-07-01 00:00:17-06
29	http://chinstrap.cylant.com/~stephen/	grip.e	grip.e is a gtk front-end for ripping / encoding / tagging CD audio, based\
very roughly on Mike Oliphant's "Grip".  I t can encode Ogg files, and convert MP3s to Ogg.	2001-07-01 00:01:13-06
30	http://noatun.kde.org/	Noatun	A KDE media player that supports Ogg Vorbis.	2001-07-01 00:01:50-06
31	http://www.iki.fi/wwwwolf/code/util/vocoditor.html	Vocoditor	A GUI Ogg Vorbis comment editor for the Linux operating system.  Vocoditor is written in Perl and uses GTK.	2001-07-01 00:02:55-06
32	http://www.xmms.org/	XMMS	The most popular media player for Linux.  XMMS plays vorbis files, allows ogg comments, and supports vorbis streams.	2001-07-01 00:03:45-06
33	http://www.jcraft.com/jorbis/	JOrbis	A pure java Ogg Vorbis decoder and player.  Now you can play Ogg files in your web browser.	2001-07-01 00:05:31-06
34	http://www.clanlib.org/	ClanLib Game SDK	A game development suite for Windows and Linux that supports Ogg Vorbis files.	2001-07-01 00:07:08-06
35	http://www.fmod.org/	FMOD	FMOD is the fastest, most powerful and easiest to use sound system on Windows and Linux there is. Supporting 3d sound, midi, mods, mp3, ogg vorbis, wma, recording, geometry processing, cd playback, mmx, internet streaming, dsp effects, user created samples and streams, synchronization support, EAX 1&2, A3D 3.0, C/C++/VB/Delphi/MASM and MORE you cant go wrong!\
	2001-07-01 00:08:39-06
36	http://www.bebits.com/app/1058	Vorbis Tools	Encoder, decoder and Cl-Amp plugin for Ogg Vorbis for BeOS.	2001-07-01 00:10:39-06
37	http://silk.apana.org.au/vorbis/	Ogg Vorbis for OS/2	Player, encoder, plugins for Ogg Vorbis on OS/2 operating systems.	2001-07-01 00:11:44-06
38	http://www.sonique.com/	Sonique	A unique looking media player for Windows that supports shaped skins and of course, Ogg Vorbis.	2001-07-01 00:13:18-06
39	http://www.winamp.com/	WinAmp	One of the original MP3 players has been updated to play Ogg Vorbis files.	2001-07-01 00:14:04-06
40	http://www.blorp.com/~peter/	Peter's Plugins	Peter has several plugins, such as an Ogg Vorbis encoder plugins for Winamp and Cool Edit filters for Vorbis files.	2001-07-01 00:23:01-06
41	http://www.poikosoft.com/cdda/	Easy CD-DA Extractor	A windows ripper/encoder that support Ogg Vorbis.	2001-07-01 00:25:40-06
42	http://www.webarts.bc.ca/jOggPlayer	jOggPlayer	An open source platform independant Graphical Vorbis Ogg Player!\
Developed using Java, so if your system has a Java VM you can use this app.	2001-07-01 00:26:59-06
43	http://zorannt.sourceforge.net/	ZoranNT DirectShow Filters	These filters/plugins will allow you to play Ogg Vorbis files in Windows Media Player, as well as use Vorbis in many other DirectX applications.	2001-07-01 00:29:04-06
\.
--
-- Data for TOC Entry ID 30 (OID 27500)
--
-- Name: software_platform Type: TABLE DATA Owner: jack
--


COPY "software_platform"  FROM stdin;
6	3
6	4
7	3
8	3
9	3
10	3
10	4
11	3
11	4
12	2
13	2
14	1
14	2
15	1
15	2
16	1
16	2
17	1
17	2
18	2
19	2
20	2
21	2
22	2
23	2
24	2
25	2
26	2
27	2
28	1
29	1
30	1
31	1
32	1
33	5
34	7
34	6
35	7
35	6
36	8
37	9
38	2
39	2
40	2
41	2
42	5
43	6
43	2
\.
--
-- Data for TOC Entry ID 31 (OID 27545)
--
-- Name: software_news Type: TABLE DATA Owner: jack
--


COPY "software_news"  FROM stdin;
3	32	2001-07-01 00:15:25-06	XMMS 1.2.5 Released	This version now supports Ogg Vorbis comment editing, and Icecast 2.0 streaming amount other things.
4	10	2001-07-01 00:17:25-06	New Mac Encoder	Ogg Drop for Mac is the easiest to use Mac encoder yet.  Just drag and drog.  For OS 9 and OS X.
5	43	2001-07-01 00:29:40-06	Windows Media Player Plugins!!	Download the DirectShow filters and plugins to allow Windows Media Player to play Ogg files!
\.
--
-- Data for TOC Entry ID 32 (OID 27603)
--
-- Name: musicsite Type: TABLE DATA Owner: jack
--


COPY "musicsite"  FROM stdin;
3	http://www.lawsonmedia.com/	Lawson Media	A Christian music site selling music in Ogg Vorbis format.	2001-06-30 22:58:50-06
4	http://www.oggmusic.de/	OggMusic.de	A german site offering lots of music in Vorbis for download.	2001-06-30 23:00:36-06
5	http://www.nnr.to/	Neuro Net Records Internet Music Archive	Get full albums of Vorbis encoded tracks from Neuro Net Records.	2001-06-30 23:09:47-06
6	http://tokyodawn.org/	Tokyo Dawn Records	Electronic music available for free download in Vorbis.	2001-06-30 23:10:43-06
7	http://www.thelongslide.com/	The Long Slide	The Long Slide is an unsigned band from Manchester in the UK that has their tracks availabe in Ogg format.	2001-07-02 10:41:59-06
\.
--
-- Data for TOC Entry ID 33 (OID 27655)
--
-- Name: category Type: TABLE DATA Owner: jack
--


COPY "category"  FROM stdin;
1	Advocacy and News	Sites that promote Ogg Vorbis.
2	How-To	Sites that demonstrate and teach about Ogg Vorbis.
\.
--
-- Data for TOC Entry ID 34 (OID 27705)
--
-- Name: link Type: TABLE DATA Owner: jack
--


COPY "link"  FROM stdin;
2	1	http://solair.eunet.yu/~aldov/	Vorbix Xtreme	Vorbis Xtreme tracks news and updates related to Vorbis.	2001-06-25 16:08:07-06
3	2	http://www.angrycoffee.com/tutorials/vorbis/	Angry Coffee Vorbis Tutorial	Learn how to create Vorbis files and install player plugins.	2001-06-30 22:51:30-06
4	1	http://www.vorbislinks.com/	VorbisLinks.com	Links to all kinds of Vorbis related sites.	2001-06-30 22:52:17-06
5	1	http://www.vorbismusic.org/	VorbisMusic.org	A French site that is all about Vorbis.	2001-06-30 22:54:31-06
6	1	http://hispamp3.com/ogg/	Hispamp3.com Ogg Vorbis Page	A spanish Vorbis news and links site.	2001-06-30 22:55:56-06
\.
--
-- TOC Entry ID 25 (OID 27500)
--
-- Name: "software_platfo_software_id_key" Type: INDEX Owner: jack
--

CREATE UNIQUE INDEX "software_platfo_software_id_key" on "software_platform" using btree ( "software_id" "int4_ops", "platform_id" "int4_ops" );

--
-- TOC Entry ID 37 (OID 27425)
--
-- Name: "RI_ConstraintTrigger_27424" Type: TRIGGER Owner: jack
--

CREATE CONSTRAINT TRIGGER "<unnamed>" AFTER INSERT OR UPDATE ON "news"  FROM "type" NOT DEFERRABLE INITIALLY IMMEDIATE FOR EACH ROW EXECUTE PROCEDURE "RI_FKey_check_ins" ('<unnamed>', 'news', 'type', 'UNSPECIFIED', 'type_id', 'type_id');

--
-- TOC Entry ID 35 (OID 27427)
--
-- Name: "RI_ConstraintTrigger_27426" Type: TRIGGER Owner: jack
--

CREATE CONSTRAINT TRIGGER "<unnamed>" AFTER DELETE ON "type"  FROM "news" NOT DEFERRABLE INITIALLY IMMEDIATE FOR EACH ROW EXECUTE PROCEDURE "RI_FKey_noaction_del" ('<unnamed>', 'news', 'type', 'UNSPECIFIED', 'type_id', 'type_id');

--
-- TOC Entry ID 36 (OID 27429)
--
-- Name: "RI_ConstraintTrigger_27428" Type: TRIGGER Owner: jack
--

CREATE CONSTRAINT TRIGGER "<unnamed>" AFTER UPDATE ON "type"  FROM "news" NOT DEFERRABLE INITIALLY IMMEDIATE FOR EACH ROW EXECUTE PROCEDURE "RI_FKey_noaction_upd" ('<unnamed>', 'news', 'type', 'UNSPECIFIED', 'type_id', 'type_id');

--
-- TOC Entry ID 44 (OID 27515)
--
-- Name: "RI_ConstraintTrigger_27514" Type: TRIGGER Owner: jack
--

CREATE CONSTRAINT TRIGGER "<unnamed>" AFTER INSERT OR UPDATE ON "software_platform"  FROM "software" NOT DEFERRABLE INITIALLY IMMEDIATE FOR EACH ROW EXECUTE PROCEDURE "RI_FKey_check_ins" ('<unnamed>', 'software_platform', 'software', 'UNSPECIFIED', 'software_id', 'software_id');

--
-- TOC Entry ID 40 (OID 27517)
--
-- Name: "RI_ConstraintTrigger_27516" Type: TRIGGER Owner: jack
--

CREATE CONSTRAINT TRIGGER "<unnamed>" AFTER DELETE ON "software"  FROM "software_platform" NOT DEFERRABLE INITIALLY IMMEDIATE FOR EACH ROW EXECUTE PROCEDURE "RI_FKey_noaction_del" ('<unnamed>', 'software_platform', 'software', 'UNSPECIFIED', 'software_id', 'software_id');

--
-- TOC Entry ID 41 (OID 27519)
--
-- Name: "RI_ConstraintTrigger_27518" Type: TRIGGER Owner: jack
--

CREATE CONSTRAINT TRIGGER "<unnamed>" AFTER UPDATE ON "software"  FROM "software_platform" NOT DEFERRABLE INITIALLY IMMEDIATE FOR EACH ROW EXECUTE PROCEDURE "RI_FKey_noaction_upd" ('<unnamed>', 'software_platform', 'software', 'UNSPECIFIED', 'software_id', 'software_id');

--
-- TOC Entry ID 45 (OID 27521)
--
-- Name: "RI_ConstraintTrigger_27520" Type: TRIGGER Owner: jack
--

CREATE CONSTRAINT TRIGGER "<unnamed>" AFTER INSERT OR UPDATE ON "software_platform"  FROM "platform" NOT DEFERRABLE INITIALLY IMMEDIATE FOR EACH ROW EXECUTE PROCEDURE "RI_FKey_check_ins" ('<unnamed>', 'software_platform', 'platform', 'UNSPECIFIED', 'platform_id', 'platform_id');

--
-- TOC Entry ID 38 (OID 27523)
--
-- Name: "RI_ConstraintTrigger_27522" Type: TRIGGER Owner: jack
--

CREATE CONSTRAINT TRIGGER "<unnamed>" AFTER DELETE ON "platform"  FROM "software_platform" NOT DEFERRABLE INITIALLY IMMEDIATE FOR EACH ROW EXECUTE PROCEDURE "RI_FKey_noaction_del" ('<unnamed>', 'software_platform', 'platform', 'UNSPECIFIED', 'platform_id', 'platform_id');

--
-- TOC Entry ID 39 (OID 27525)
--
-- Name: "RI_ConstraintTrigger_27524" Type: TRIGGER Owner: jack
--

CREATE CONSTRAINT TRIGGER "<unnamed>" AFTER UPDATE ON "platform"  FROM "software_platform" NOT DEFERRABLE INITIALLY IMMEDIATE FOR EACH ROW EXECUTE PROCEDURE "RI_FKey_noaction_upd" ('<unnamed>', 'software_platform', 'platform', 'UNSPECIFIED', 'platform_id', 'platform_id');

--
-- TOC Entry ID 46 (OID 27579)
--
-- Name: "RI_ConstraintTrigger_27578" Type: TRIGGER Owner: jack
--

CREATE CONSTRAINT TRIGGER "<unnamed>" AFTER INSERT OR UPDATE ON "software_news"  FROM "software" NOT DEFERRABLE INITIALLY IMMEDIATE FOR EACH ROW EXECUTE PROCEDURE "RI_FKey_check_ins" ('<unnamed>', 'software_news', 'software', 'UNSPECIFIED', 'software_id', 'software_id');

--
-- TOC Entry ID 42 (OID 27581)
--
-- Name: "RI_ConstraintTrigger_27580" Type: TRIGGER Owner: jack
--

CREATE CONSTRAINT TRIGGER "<unnamed>" AFTER DELETE ON "software"  FROM "software_news" NOT DEFERRABLE INITIALLY IMMEDIATE FOR EACH ROW EXECUTE PROCEDURE "RI_FKey_noaction_del" ('<unnamed>', 'software_news', 'software', 'UNSPECIFIED', 'software_id', 'software_id');

--
-- TOC Entry ID 43 (OID 27583)
--
-- Name: "RI_ConstraintTrigger_27582" Type: TRIGGER Owner: jack
--

CREATE CONSTRAINT TRIGGER "<unnamed>" AFTER UPDATE ON "software"  FROM "software_news" NOT DEFERRABLE INITIALLY IMMEDIATE FOR EACH ROW EXECUTE PROCEDURE "RI_FKey_noaction_upd" ('<unnamed>', 'software_news', 'software', 'UNSPECIFIED', 'software_id', 'software_id');

--
-- TOC Entry ID 49 (OID 27740)
--
-- Name: "RI_ConstraintTrigger_27739" Type: TRIGGER Owner: jack
--

CREATE CONSTRAINT TRIGGER "<unnamed>" AFTER INSERT OR UPDATE ON "link"  FROM "category" NOT DEFERRABLE INITIALLY IMMEDIATE FOR EACH ROW EXECUTE PROCEDURE "RI_FKey_check_ins" ('<unnamed>', 'link', 'category', 'UNSPECIFIED', 'category_id', 'category_id');

--
-- TOC Entry ID 47 (OID 27742)
--
-- Name: "RI_ConstraintTrigger_27741" Type: TRIGGER Owner: jack
--

CREATE CONSTRAINT TRIGGER "<unnamed>" AFTER DELETE ON "category"  FROM "link" NOT DEFERRABLE INITIALLY IMMEDIATE FOR EACH ROW EXECUTE PROCEDURE "RI_FKey_noaction_del" ('<unnamed>', 'link', 'category', 'UNSPECIFIED', 'category_id', 'category_id');

--
-- TOC Entry ID 48 (OID 27744)
--
-- Name: "RI_ConstraintTrigger_27743" Type: TRIGGER Owner: jack
--

CREATE CONSTRAINT TRIGGER "<unnamed>" AFTER UPDATE ON "category"  FROM "link" NOT DEFERRABLE INITIALLY IMMEDIATE FOR EACH ROW EXECUTE PROCEDURE "RI_FKey_noaction_upd" ('<unnamed>', 'link', 'category', 'UNSPECIFIED', 'category_id', 'category_id');

--
-- TOC Entry ID 3 (OID 27336)
--
-- Name: type_type_id_seq Type: SEQUENCE SET Owner: 
--

SELECT setval ('"type_type_id_seq"', 4, 't');

--
-- TOC Entry ID 5 (OID 27371)
--
-- Name: news_news_id_seq Type: SEQUENCE SET Owner: 
--

SELECT setval ('"news_news_id_seq"', 5, 't');

--
-- TOC Entry ID 7 (OID 27448)
--
-- Name: software_software_id_seq Type: SEQUENCE SET Owner: 
--

SELECT setval ('"software_software_id_seq"', 43, 't');

--
-- TOC Entry ID 9 (OID 27526)
--
-- Name: software_news_software_news_seq Type: SEQUENCE SET Owner: 
--

SELECT setval ('"software_news_software_news_seq"', 5, 't');

--
-- TOC Entry ID 11 (OID 27584)
--
-- Name: musicsite_musicsite_id_seq Type: SEQUENCE SET Owner: 
--

SELECT setval ('"musicsite_musicsite_id_seq"', 7, 't');

--
-- TOC Entry ID 13 (OID 27636)
--
-- Name: category_category_id_seq Type: SEQUENCE SET Owner: 
--

SELECT setval ('"category_category_id_seq"', 3, 't');

--
-- TOC Entry ID 15 (OID 27686)
--
-- Name: link_link_id_seq Type: SEQUENCE SET Owner: 
--

SELECT setval ('"link_link_id_seq"', 6, 't');

