Codec Comparison
================

<table id='comparison'>
	<thead>
		<tr>
			<th>Codec</th>
			<th>Rate (kHz)</th>
			<th>bitrate (kbps)</th>
			<th>delay (ms)</th>
			<th><a href="#multiRate">multi-rate</a></th>
			<th><a href="#embedded">embedded</a></th>
			<th><a href="#vbr"><abbr title="variable bitrate">VBR</abbr></a></th>
			<th><a href="#plc"><abbr title="packet loss concealment">PLC</abbr></a></th>
			<th><a href="#bitRobust">bit-robust</a></th>
			<th>license</th>
		</tr>
	</thead>
	<tbody>
		<tr>
			<td>Speex</td>
			<td>8, 16, 32</td>
			<td>
				2.15-24.6 (<abbr title="narrowband">NB</abbr>)<br>
				4-44.2    (<abbr title="wideband">WB</abbr>)
			</td>
			<td>
				30 (<abbr title="narrowband">NB</abbr>)<br>
				34 (<abbr title="wideband">WB</abbr>)
			</td>

			<td>yes</td>
			<td>yes</td>
			<td>yes</td>
			<td>yes</td>
			<td></td>
			<td>open-source/<br>free software</td>

		</tr>
		<tr>
			<td><a href="http://www.ilbcfreeware.org/">iLBC</a></td>
			<td>8</td>
			<td>13.3</td>
			<td>30</td>
			<td>

			<td></td>
			<td></td>
			<td>yes</td>
			<td></td>
			<td>no charge, but not open-source</td>
		</tr>
		<tr>
			<td>AMR-NB</td>

			<td>8</td>
			<td>4.75-12.2</td>
			<td>20</td>
			<td>yes</td>
			<td></td>
			<td></td>
			<td>yes</td>

			<td>yes</td>
			<td>proprietary</td>
		</tr>
		<tr>
			<td>AMR-<abbr title="wideband">WB</abbr><br/>(G.722.2)</td>
			<td>16</td>

			<td>6.6-23.85</td>
			<td>20</td>
			<td>yes</td>
			<td></td>
			<td></td>
			<td>yes</td>
			<td>yes</td>

			<td>proprietary</td>
		</tr>
		<tr>
			<td>G.729</td>
			<td>8</td>
			<td>8</td>
			<td>15</td>

			<td></td>
			<td></td>
			<td></td>
			<td>yes</td>
			<td>yes</td>
			<td>proprietary</td>
		</tr>

		<tr>
			<td><a href="http://kbs.cs.tu-berlin.de/~jutta/toast.html">GSM-FR</a></td>
			<td>8</td>
			<td>13</td>
			<td>20</td>
			<td></td>
			<td></td>
			<td></td>
			<td>?</td>
			<td>?</td>
			<td>patented?</td>
		</tr>
		<tr>
			<td>GSM-EFR</td>

			<td>8</td>
			<td>12.2</td>
			<td>20</td>
			<td></td>
			<td></td>
			<td></td>
			<td>yes</td>

			<td>yes</td>
			<td>proprietary</td>
		</tr>
		<tr>
			<td>G.723.1</td>
			<td>8</td>
			<td>5.3 6.3</td>

			<td>37.5</td>
			<td></td>
			<td></td>
			<td></td>
			<td>yes</td>
			<td>?</td>
			<td>proprietary</td>

		</tr>
		<tr>
			<td>G.728</td>
			<td>8</td>
			<td>16</td>
			<td>0.625</td>
			<td></td>
			<td></td>
			<td></td>
			<td></td>
			<td></td>
			<td>proprietary</td>
		</tr>
		<tr>
			<td>G.722</td>

			<td>16</td>
			<td>48 56 64</td>
			<td>?</td>
			<td></td>
			<td>yes</td>
			<td></td>
			<td></td>
			<td>?</td>
			<td>?</td>
		</tr>
	</tbody>
</table>

## Definitions

<dl>
	<dt><dfn id="multiRate">multi-rate</dfn></dt>
	<dd>
		Allows the codec to change bitrate
		dynamically, at any moment
	</dd>

	<dt><dfn id="embedded">embedded</dfn></dt>
	<dd>
		A codec that embeds narrowband bitstreams in wideband bitstreams
	</dd>

	<dt><dfn id="vbr"><abbr title="variable bitrate">VBR</abbr></dfn></dt>
	<dd>
		Variable bitrate
	</dd>
	
	<dt><dfn id="plc"><abbr title="packet loss concealment">PLC</abbr></dfn></dt>
	<dd>
		Packet loss concealment
	</dd>

	<dt><dfn id="bitRobust">bit-robust</dfn></dt>
	<dd>
		Robust to corruption at the bit level. Wireless networks often have noticeable
		amounts of bit-level corruption.
	</dd>
</dl>

## Special Features

<dl>
	<dt>Speex</dt>
	<dd>
		Speex supports intensity stereo encoding and 32
		<abbr title='kilohertz'>kHz</abbr> sampling
	</dd>

	<dt>iLBC</dt>
	<dd>
		iLBC frames are independent of each other; while this provides better
		quality when 10% (or more) of the packets are dropped, this makes the codec
		suboptimal for clean line conditions.
	</dd>
</dl>

