var vlcActiveX = false;
var vlcPlugin = false;
var videoElement = false;
var oggPlugin = false;
var javaEnabled = false;
var cortadourl ='';

function initVideo() {

	var video = document.createElement("video");

	if(video.canPlayType && video.canPlayType("video/ogg;codecs=\"theora,vorbis\"") == "probably") {
		videoElement = true;
	}

	var searchedMimeTypes = false;
	var foundJavaMimeType = false;

	// search for MIME-types
	if(navigator.mimeTypes && navigator.mimeTypes.length > 0) {

		searchedMimeTypes = true;

		for (var i = 0; i < navigator.mimeTypes.length; i++) {
			if(navigator.mimeTypes[i].type.indexOf("video/ogg") > -1) {
				oggPlugin = true;
    			}
			if(navigator.mimeTypes[i].type.indexOf("application/x-vlc-plugin") > -1) {
				vlcPlugin = true;
			}
			if(navigator.mimeTypes[i].type.indexOf("application/x-java-applet") > -1) {
				foundJavaMimeType = true;
			}
		}
	}

	var nativePlayback = vlcActiveX || vlcPlugin || videoElement || oggPlugin;
 
	if(searchedMimeTypes) {
		javaEnabled = navigator.javaEnabled() && foundJavaMimeType;
	} else {
		javaEnabled = navigator.javaEnabled();
	}

	var htmlvideos = document.getElementsByTagName("video");
	var oggvideos = document.getElementsByTagName("oggvideo");

	var videos = new Array(htmlvideos.length + oggvideos.length);
	for(i = 0; i < videos.length; ++i) {
		if(i < htmlvideos.length) {
			videos[i] = htmlvideos[i];
		} else {
			videos[i] = oggvideos[i - htmlvideos.length];
		}
	}

	// replace all video elements with fallbacks
	for(i = 0; i < videos.length; ++i) {

		var video = videos[i];

		if(videoElement && video.tagName.toLowerCase() == "video") {
			continue;
		}

		var src = video.getAttribute("src");
		var width = video.getAttribute("width");
		var height = video.getAttribute("height");
		var controls = video.getAttribute("controls");
		var autoplay = video.getAttribute("autoplay");

		var parent = video.parentNode;
		if(videoElement) {
			var videoelem = document.createElement("video");
			videoelem.setAttribute("src", src);
			videoelem.setAttribute("width", width);
			videoelem.setAttribute("height", height);
			if(controls) videoelem.setAttribute("controls",  "true");
			if(autoplay) videoelem.setAttribute("autoplay",  "true");
			parent.replaceChild(videoelem, video);
		} else if(javaEnabled) {
			if (cortadourl=='') {
			  if (src.indexOf('/')!=-1)
				cortadourl = src.substr(0,src.lastIndexOf('/'))+'/';
                          cortadourl = cortadourl+'cortado.jar';
			}
			var applet = document.createElement("applet");
			applet.setAttribute("code", "com.fluendo.player.Cortado.class");
			applet.setAttribute("archive", cortadourl);
			applet.setAttribute("width", width);
			applet.setAttribute("height", height);

			var param = document.createElement("param");
			param.setAttribute("name","BufferSize");
			param.setAttribute("value","4096");
			applet.appendChild(param);
			
			param = document.createElement("param");
			param.setAttribute("name","BufferHigh");
			param.setAttribute("value","25");
			applet.appendChild(param);
			
			param = document.createElement("param");
			param.setAttribute("name","BufferLow");
			param.setAttribute("value","5");
			applet.appendChild(param);

			param = document.createElement("param");
			param.setAttribute("name","url");
			param.setAttribute("value", src);
			applet.appendChild(param);

			parent.replaceChild(applet, video);

		} else if(oggPlugin) {
			var object = document.createElement("object");
			object.setAttribute("width", width);
			object.setAttribute("height", height);
			object.setAttribute("data", src);
			object.setAttribute("type", "video/ogg");
			parent.replaceChild(object, video);
		} else if(vlcActiveX) {
			var span = document.createElement("span");
			span.innerHTML = "<object classid='clsid:9BE31822-FDAD-461B-AD51-BE1D1C159921' codebase='http://downloads.videolan.org/pub/videolan/vlc/latest/win32/axvlc.cab' width='" + width + "' height='" + height + "' id='vlc' events='True'><param name='Src' value='"+ src +"'/> <param name='ShowDisplay' value='True'/><param name='AutoLoop' value='False'/><param name='AutoPlay' value='False'/></object>";
			parent.replaceChild(span,video);
		} else if(vlcPlugin) {
			var span = document.createElement("span");
			span.innerHTML = "<object type='application/x-vlc-plugin' id='vlc_element' width='"+width+"' height='"+height+"' data='" + src + "'></object><br>" + "<input type='button' value='play' onClick='document.getElementById(\"vlc_element\").play()'>" + "<input type='button' value='stop' onClick='document.getElementById(\"vlc_element\").stop()'>";
			parent.replaceChild(span,video);
			
		} else {
			var msg = document.createTextNode("No fitting Ogg decoder found. Install a proper browser with Ogg Theora support or a browser plugin fit for Ogg Theora playback.");
			parent.appendChild(msg);
		}
	}

}
