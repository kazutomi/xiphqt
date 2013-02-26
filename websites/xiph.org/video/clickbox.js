// piggybacks off subtitles.js

var cbTimers = new Array();

// 00:00:00.000 --> 00:00:00.000
// left% right% top% bottom% #border #background
// link

function playClickboxes(boxElement,icon,vid) {
    var cb = boxElement.text();
    boxElement.text('');
    cb = cb.replace(/\r\n|\r|\n/g, '\n');
    var clickboxes = {};
    cb = strip(cb);
    var cb_ = cb.split('\n\n');

    for(s in cb_) {
        st = cb_[s].split('\n');
        if(st.length >=2) {
          n = st[0];

          if(!(st[1].split(' --> ')[0]) ||
             !(st[1].split(' --> ')[1]))
              alert("Clickbox format error: "+st[1]);

          if(!(st[2].split(' ')[0]) ||
	     !(st[2].split(' ')[1]) ||
	     !(st[2].split(' ')[2]) ||
	     !(st[2].split(' ')[3]) ||
	     !(st[2].split(' ')[4]))
              alert("Clickbox format error: "+st[2]);

	  if(!(st[3]))
              alert("Clickbox format error: "+st[3]);

          i = strip(st[1].split(' --> ')[0]);
          o = strip(st[1].split(' --> ')[1]);
	  left   = strip(st[2].split(' ')[0]);
	  right  = strip(st[2].split(' ')[1]);
	  topp   = strip(st[2].split(' ')[2]);
	  bottom = strip(st[2].split(' ')[3]);
	  cl     = strip(st[2].split(' ')[4]);
	  link   = strip(st[3]);

          is = toMilliSeconds(i);
          os = toMilliSeconds(o);
          clickboxes[is] = {o: os, l: left, r: right, t: topp, b: bottom, h: link, c: cl};
        }
    }
    var current_clickbox = -1;
    var current_opacity = 0;

    cbTimers[boxElement] = setInterval(function() {
            if(vid){
                var currentTime = vid.currentTime;
                var clickbox = -1;
                for(s in clickboxes) {
                    if(s > currentTime*1000)
                        break;
                    clickbox = s;
                }
                if(clickbox > 0 && clickboxes[clickbox].o > currentTime*1000) {
                    if(current_clickbox != clickbox){
                        current_opacity = 0;
			boxElement.height(clickboxes[clickbox].b - clickboxes[clickbox].t + "%");
			boxElement.width(clickboxes[clickbox].r - clickboxes[clickbox].l + "%");
			boxElement.css({"top": clickboxes[clickbox].t+"%", "left": clickboxes[clickbox].t+"%"});
                        boxElement.attr("href",clickboxes[clickbox].h);
			boxElement.css({"top": clickboxes[clickbox].t+"%",
					"left": clickboxes[clickbox].l+"%",
					"display" : "block",
			                "opacity" : 0});
			boxElement.removeClass().addClass("cb "+clickboxes[clickbox].c);
			if(icon){
			    icon.css({"display" : "block",
			              "top": clickboxes[clickbox].b+"%",
				      "left": clickboxes[clickbox].r+"%"});
			}
                        current_clickbox = clickbox;
                    }else{
			if (current_opacity<1){
			    current_opacity+=.2;
			    boxElement.css({"opacity" : current_opacity});
			}
		    }
                }else{
                    if(current_clickbox != -1){
			boxElement.css({"display" : "none"});
                        current_clickbox = -1;
                        current_opacity = 0;
			if(icon){
			    icon.css({"display" : "none"});
			}
                    }
                }
            }
        }, 100);
}

function loadClickboxes(clickboxElement, url){
    var el = $(clickboxElement);
    var vid = findUnder(clickboxElement,'video');
    var icon = findUnder(clickboxElement,'.cbicon');
    if(cbTimers[el]){
        clearInterval(cbTimers[el]);
        cbTimers[el]=null;
        el.html("");
    }
    if(url) {
        el.load(url, function (responseText, textStatus, req) {playClickboxes(el,$(icon),vid)});
    } else {
        el.html("");
    }
}

function SetClickboxes(el)
{
    if(el){
        var file = $(el).attr("file");
        if(file===undefined){
            loadClickboxes(el);
        }else{
            loadClickboxes(el,file);
        }
    }
}

