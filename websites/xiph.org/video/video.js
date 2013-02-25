function fixupVideo(video)
{
    // the problem is that at least Safari will see the video element
    // and refuse to fallback to non-source elements within (like the
    // Cortado applet).  In this case, strip the video tag and let the
    // now-naked applet run.

    var videoElement = false;

    if ((video.canPlayType &&
	 (video.canPlayType("video/ogg; codecs=\"theora, vorbis\"") == "probably" ||
          video.canPlayType("video/webm; codecs=\"vp8, vorbis\"") == "probably"))) {
        videoElement = true;
    }

    if (!videoElement) {

        // replace <video> with its contents
        while (video.firstChild) {
            video.parentNode.insertBefore(video.firstChild, video);
        }
        video.parentNode.removeChild(video);

        // Also kill off the HTML5-specific vid controls

        var controls = $(".vidcontrols");
	while (controls.length > 0) {
	    var control = controls[0];
            control.parentNode.removeChild(control);
            controls = $(".vidcontrols");
	}
    }

}

// If the URL has a time offset query, set the video offset
var uri_time = -1;
function check_set_time(video, query)
{
    var queries = query.split("?");
    for(q in queries) {
        var f = queries[q].split("=");
        var key = f[0];
        var value = f[1];

        if(key == "time"){
            if(video && !(value===undefined))
                uri_time = parseFloat(value);
        }
    }
}


// Search up from one of the control elements until we hit the upper video wrapper
function findWrapper(el){
    var wrapper = el;
    while (wrapper && (!wrapper.className || wrapper.className.toLowerCase() != "vid_wrapper")){
        wrapper = wrapper.parentNode;
    }
    return wrapper;
}

// Search up to video wrapper, then back down to sibling target
function findUnder(el,target){
    var wrapper = el;
    while (wrapper && (!wrapper.className || wrapper.className.toLowerCase() != "vid_wrapper")){
        wrapper = wrapper.parentNode;
    }
    if(!wrapper) return;
    return $(wrapper).find(target)[0];
}

// Navigate to a specific time offset as selected by a dropdown.  Reset the dropdown
// after the seek completes
function SetTime(dropdown)
{
    var video = findUnder(dropdown,'video');
    var time = $(dropdown.options[dropdown.selectedIndex]).attr('timecode');
    if(!video || time===undefined) return;

    video.addEventListener("seeked",
                           function() { dropdown.selectedIndex=0;
                               this.removeEventListener("seeked",arguments.callee,true);
                           }, true);
    video.currentTime = time;
}

// Set the video playing to the current dropdown choice; this is meant
// to select from several different encodings of the same video, and
// updates the Xiph page window layout to properly size/center it.
function SetVideo(dropdown)
{
    var video = findUnder(dropdown,'video');
    var wrapper = findWrapper(dropdown);
    var vid_width = $(dropdown.options[dropdown.selectedIndex]).attr('video-width');
    var vid_height = $(dropdown.options[dropdown.selectedIndex]).attr('video-height');
    var fontsize = $(dropdown.options[dropdown.selectedIndex]).attr('subtitle-font-size');
    var bottom = $(dropdown.options[dropdown.selectedIndex]).attr('subtitle-bottom');
    var poster = $(dropdown.options[dropdown.selectedIndex]).attr('poster');
    var webm = $(dropdown.options[dropdown.selectedIndex]).attr('src-webm');
    var ogg = $(dropdown.options[dropdown.selectedIndex]).attr('src-ogg');
    if(!video) return;
    if(!wrapper)return;

    // get our other wrappers
    var srt_wrapper = $(wrapper).find(".srtwrapper")[0];
    var vc_wrapper = $(wrapper).find(".vcwrapper")[0];
    var vidtype = $(wrapper).find(".vidtype")[0];
    var sizers = $(wrapper).find(".vid_sizer");
    var sizers2 = $(wrapper).find(".vid_caption");

    // reloading the vid will cause it to start over; save play status and playtime
    var playing = !(video.paused);
    var position = video.currentTime;
    if(uri_time > 0) {
        position = uri_time;
        uri_time = -1;
    }

    // update the video, video wrapper, and control wrapper size
    video.style.width = vid_width;
    video.style.height = vid_height;
    sizers.each(function(){
            this.style.width = vid_width;
            this.style.height = vid_height;
        });
    sizers2.each(function(){
            this.style.width = vid_width;
        });
    if(vc_wrapper)
        vc_wrapper.style.width = vid_width;

    // update video sources, but first figure out if we want Ogg or WebM
    if (video.canPlayType &&
        video.canPlayType('video/webm;codecs="vp8,vorbis"') == "probably"){
        video.src=webm;
        if(vidtype)$(vidtype).html("WebM");
    }else{
        video.src=ogg;
        if(vidtype)$(vidtype).html("Ogg");
    }

    // Remove the poster once playback is in progress, as some
    // browsers keep popping it back up at annoying times, like when
    // playback stutters or during seeking.
    if(position>0)
        $(video).removeAttr("poster");
    else{
        $(video).attr("poster",poster);
        video.addEventListener("playing",
                               function() {
                                   // remove the poster to prevent
                                   // browsers like chrome from contantly
                                   // flickering it over the frame
                                   $(video).removeAttr("poster");
                                   this.removeEventListener("playing",arguments.callee,true);
                               }, true);
    }

    // We can't set the playback time on the new video to match where
    // the old vid left off until metadata has successfully loaded;
    // push the rest of the playback resume process into a callback
    video.addEventListener("loadedmetadata",
                           function() {
                               // don't set if zero; that will drop the poster and cause preload
                               if(position>0)
                                   video.currentTime = position;
                               if(playing)
                                   video.play();
                               this.removeEventListener("loadedmetadata",arguments.callee,true);
                           }, true);

    // Update outer page sizing
    document.getElementById("thepage").style.width =
        (video.offsetWidth + document.getElementById("navbar").offsetWidth*2) + "px";
    document.getElementById("xiphbar_outer").style.minWidth =
        (video.offsetWidth + document.getElementById("navbar").offsetWidth*2) + "px";

    // update the subtitle alignment within video pane
    if(srt_wrapper){
        srt_wrapper.style.fontSize = fontsize;
        srt_wrapper.style.width = vid_width;
        srt_wrapper.style.bottom = bottom;
    }
    video.load();

}

// Show/hide our upper control toolbar according to mouseover
var stick=0; // only one input queue, so a single global stick will do
var mousein=0;
var mousetimeout;
var showtimeout;
var playonce=0;

function showControls(el,delay){
    var wrapper = findUnder(el,'.vcwrapper');
    var video = findUnder(el,'video');
    var playing = !(video.paused);
    var position = video.currentTime;
    if(!wrapper)return;
    if(!mousein){
	clearTimeout(showtimeout);
	showtimeout=setTimeout(function(){
	    $(wrapper).animate({opacity: 1}, {duration: 160, queue: false});},
			       delay);
	mousein=1;
    }
    clearTimeout(mousetimeout);
    if(playonce)
	mousetimeout=setTimeout(function(){mousein=0;hideControls(el);},2000);
    if(position)playonce=1;
    stick=0;
}

var focused_el = 0;
function hideControls(el){
    var video = findUnder(el,'video');
    var playing = !(video.paused);
    var position = video.currentTime;
    if(position)playonce=1;
    clearTimeout(showtimeout);
    if(!stick && !mousein && playonce){
        var wrapper = findUnder(el,'.vcwrapper');
        if(!wrapper)return;
        $(wrapper).animate({opacity: 0.}, {duration: 160, queue: false});
        el.blur();
        focused_el=0;
    }
}

// This is a Chrome hack; the Chrome browser mistakenly fires mouseout
// when a select box is clicked; we simply force-stick it open on
// mousedown.  Sadly, Chrome does not seem to fire any events at all
// when a Select box is activated by keyboard, only if an option is
// selected.

function stickControls(el){
    showControls(el);
    stick=1;
}

function unstickControls(el){
    stick=0;
}

function focusControl(el){
    if(!focused_el || el != focused_el){
        focused_el = el;
        stickControls(el);
    }
}

$(document).ready(function() {

        // Run the Safari conditional applet fix
        $('video').each(function() {
                fixupVideo(this);
            });

        // the URI might have an anchor requesting a specific time
        // offset
        $('video').each(function() {
                check_set_time(this, window.location.search);
            });

        $('.vid_control').each(function() {
                this.onmouseout=function(){mousein=0;hideControls(this)};
                this.onmouseover=function(){showControls(this,0)};
                this.onmousemove=function(){showControls(this,500)};
            });

        // subtitles and video settings aren't statically set in the
        // HTML; pull the selections out of our dropdowns.
        $('.srt-select').each(function() {
                this.onmousedown=function(){stickControls(this);};
                this.onkeydown=function(){stickControls(this);};
                this.onblur=function(){unstickControls(this);hideControls(this);};
                this.onselect=function(){unstickControls(this);};
                this.onfocus=function(){focusControl(this);};
                this.onchange=function(){SetSubtitles(this);};
                SetSubtitles(this);
            });
        $('.video-select').each(function() {
                this.onmousedown=function(){stickControls(this);};
                this.onkeydown=function(){stickControls(this);};
                this.onfocus=function(){focusControl(this);};
                this.onblur=function(){unstickControls(this);hideControls(this);};
                this.onselect=function(){unstickControls(this);};
                this.onchange=function(){SetVideo(this);};
                SetVideo(this);
            });
        $('.chapter-select').each(function() {
                this.onmousedown=function(){stickControls(this);};
                this.onkeydown=function(){stickControls(this);};
                this.onblur=function(){unstickControls(this);hideControls(this);};
                this.onselect=function(){unstickControls(this);};
                this.onfocus=function(){focusControl(this);};
                this.onchange=function(){SetTime(this);};
            });

    });

