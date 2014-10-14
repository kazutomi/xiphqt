/*
 * jQuery srt
 *
 * version 0.1 (November 28, 2008)
 *
 * Dual licensed under the MIT and GPL licenses:
 *   http://www.opensource.org/licenses/mit-license.php
 *   http://www.gnu.org/licenses/gpl.html
 */
/*
  usage:
    <video src="example.ogg" id="examplevideo" />
    <div class="srt" data-video="examplevideo" data-srt="example.srt"></div>

  jquery.srt.js will try to load subtitles in all elements with 'srt' class.
  'data-video' atribute is used to link to the related video,
  if no data-srt is provided, the contents of the div is parsed as srt.
*/

/* Modified for site-specific use (namely many bugfixes and dynamic
 * replacement / restart of subtitles on the fly -- Monty 20100917 */

var srtTimers = new Array();

// this needed to be refactored somewhat to use milliseconds (and thus
// return integers to make Opera's array handling happy) as well as
// not feed octal format [leading zeroes] into javascript's numeric
// parser.
function toMilliSeconds(t) {
    var ms = 0;
    if(t) {
        var p = t.split(':');
        for(i=0;i<p.length;i++)
            ms = ms * 60 + parseInt('1'+p[i].replace(/,|\./g, ' ')) - 100;
        ms*=1000;
        if(i){
            var m = p[i-1].split(/,|\./);
            if(m[1]) ms+= parseInt(m[1]);
        }
    }
    return ms;
}

function strip(s) {
    return s.replace(/^\s+|\s+$/g,"");
}

function playSubtitles(subtitleElement,vid) {
    var srt = subtitleElement.text();
    subtitleElement.text('');
    srt = srt.replace(/\r\n|\r|\n/g, '\n');
    var subtitles = {};
    srt = strip(srt);
    var srt_ = srt.split('\n\n');

    for(s in srt_) {
        st = srt_[s].split('\n');
        if(st.length >=2) {
          n = st[0];

          if(!(st[1].split(' --> ')[0]) ||
             !(st[1].split(' --> ')[1]))
              alert("Subtitle format error: "+st[1]);

          i = strip(st[1].split(' --> ')[0]);
          o = strip(st[1].split(' --> ')[1]);
          t = st[2];
	    if(n==442){
		alert(t);
}
          if(st.length > 2) {
            for(j=3; j<st.length;j++)
              t += '\n'+st[j];
          }
          is = toMilliSeconds(i);
          os = toMilliSeconds(o);
          subtitles[is] = {o: os, t: t};
        }
    }
    var current_subtitle = -1;

    srtTimers[subtitleElement] = setInterval(function() {
            if(vid){
                var currentTime = vid.currentTime;
                var subtitle = -1;
                for(s in subtitles) {
                    if(s > currentTime*1000)
                        break;
                    subtitle = s;
                }
                if(subtitle > 0 && subtitles[subtitle].o > currentTime*1000) {
                    if(current_subtitle != subtitle){
                        var text = subtitles[subtitle].t;
                        subtitleElement.html(text);
			var hgt = subtitleElement.innerHeight();
			var baseline = $(document.querySelector('.baseline')).outerHeight();
			if(hgt > baseline*1.5 && hgt < baseline*2.5) {
			    var split = text.split(''),
			    len = text.length,
			    liners = [], breaks = [],
			    res = 0, rnum = 0, top, btm, tlen, blen, t, tt, splim = 1000000;

			    for(var j = 0; j < len; j++) {
				if(split[j] === ' ') {
				    breaks.push(j);
				}
			    }

			    for(var k = 0, kk = breaks.length; k < kk; k++) {
				top = text.substr(0,breaks[k]).replace(/\ /g,'^').split('');
				btm = text.substr(breaks[k]+1,len).replace(/\ /g,'^').split('');
				liners.push(Math.abs(top.length - btm.length));
			    }
			    for(var c = 0, cc = liners.length; c < cc; c++) {
				if(liners[c] < splim) {
				    res = c;
				    splim = liners[c];
				}
			    }
			    res = breaks[res];
			    subtitleElement.html(text.substr(0,res+1)+'<br>'+text.substr(res+1,len));
			}
			
                        current_subtitle = subtitle;
                    }
                }else{
                    if(current_subtitle != -1){
                        subtitleElement.html('');
                        current_subtitle = -1;
                    }
                }
            }
        }, 100);
}

function loadSubtitles(subtitleElement, url){
    var el = $(subtitleElement);
    var vid = findUnder(subtitleElement,'video');
    if(srtTimers[el]){
        clearInterval(srtTimers[el]);
        srtTimers[el]=null;
        el.html("");
    }
    if(url) {
        el.load(url, function (responseText, textStatus, req) {playSubtitles(el,vid)});
    } else {
        el.html("");
    }
}

function SetSubtitles(dropdown)
{
    var srt = findUnder(dropdown,'.srt');
    if(srt){
        var file = $(dropdown.options[dropdown.selectedIndex]).attr("file");
        if(file===undefined){
            loadSubtitles(srt);
        }else{
            loadSubtitles(srt,file);
        }
    }
}

