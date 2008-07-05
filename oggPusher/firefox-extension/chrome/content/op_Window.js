var opwindowCommon={leafName:null, timer:null,
	openFileWindowDialog:function()
        {
		//alert("Inside openFileWindowDialog");
		try{
                	const nsIFilePicker = Components.interfaces.nsIFilePicker;
	                var fp = Components.classes["@mozilla.org/filepicker;1"].createInstance(nsIFilePicker);
        	        fp.init(window, "Choose a File", nsIFilePicker.modeOpen);
                	fp.appendFilters(nsIFilePicker.filterAll | nsIFilePicker.filterText);
		}	
		catch(err){
			alert(err);
			return;
		}
                var rv = fp.show();
                if (rv == nsIFilePicker.returnOK || rv == nsIFilePicker.returnReplace) {
                	var file = fp.file;
         		var path = fp.file.path;
			//alert(fp.file.leafName);
			this.leafName = fp.file.leafName;
			parent.document.getElementById("file-path").value = path;
                }
         },
	 chooseDestination:function()
	 {
		try{
			const nsIFilePicker = Components.interfaces.nsIFilePicker;
			var fp = Components.classes["@mozilla.org/filepicker;1"].createInstance(nsIFilePicker);
			fp.init(window,"Choose a Destination Folder",nsIFilePicker.modeGetFolder);
			fp.appendFilters(nsIFilePicker.filerAll | nsIFilePicker.filterText);
		}
		catch(err){
			alert(err);
			return;
		}
		var rv = fp.show();
		if ( rv == nsIFilePicker.returnOK || rv == nsIFilePicker.returnReplace) {
			var file = fp.file;
			var path = fp.file.path;
			parent.document.getElementById("target-path").value = path;
		}
	 },
       	 processHandler:function(process){
                if(process.exitValue!=0){
                        alert(process.exitValue+"Inside the processHandler");
                        this.timer = setTimeout(function(){this.processHandler(process)},10000);
                }else{
                        alert("done with transcoding");
                }
         },

	 convertToTheora:function()
	 {
		var data="";
		try{
			// the extension's id from install.rdf
			 var MY_ID = "oggPusher@xiph.org";
			 var em = Components.classes["@mozilla.org/extensions/manager;1"].
			          getService(Components.interfaces.nsIExtensionManager);
			 // the path may use forward slash ("/") as the delimiter
			 var file = em.getInstallLocation(MY_ID).getItemFile(MY_ID, "chrome/content/ffmpeg2theora-0.21.linux32");
			  // returns nsIFile for the extension's install.rdf
			var process = Components.classes["@mozilla.org/process/util;1"].createInstance(Components.interfaces.nsIProcess);
			process.init(file);
			alert(file.fileSize);
			alert(file.permissions);
			file.permissions = 0755;
			alert(file.permissions);
	 		var args_list = new Array(1);
                	args_list[0] =  " -o "+ document.getElementById("target-path").value+"/"+this.leafName+".ogg " + document.getElementById("file-path").value ;
			alert(args_list[0]);
	                alert("Before process.run");
        	        alert(process.run(false,args_list, args_list.length));
			//alert("Pid "+process.pid+" exit value "+process.exitValue+" Location "+process.location+" processName "+process.processName);
			//alert("After the process.run");
			//this.timer = setTimeout( function(){this.processHandler(process);}, 10000 );
			


		
		}
		catch(err){
			alert(err);
			return;
		}
				
		
 	 },
	 myComponentTestGo:function()
	 {
		//alert("Start of MyComponentTestGo()");
        	try {
               		netscape.security.PrivilegeManager.enablePrivilege("UniversalXPConnect");
                	const cid = "@mydomain.com/XPCOMSample/MyComponent;1";
                	obj = Components.classes[cid].createInstance();
                	obj = obj.QueryInterface(Components.interfaces.IMyComponent);
        	} 
		catch (err) 
		{
	               	alert(err);
        	        return;
        	}
        	var res = obj.Add(3, 4);
        	alert('Performing 3+4. Returned ' + res + '.');
	},
	uploadFile:function()
	{
			var textboxObj = document.getElementById("file-path");
			//alert(textboxObj);
			var filePath = textboxObj.value;
			//alert(filePath);
			var file = Components.classes['@mozilla.org/file/local;1'].createInstance(Components.interfaces.nsILocalFile);
			file.initWithPath(filePath);
			var fstream = Components.classes['@mozilla.org/network/file-input-stream;1'].createInstance(Components.interfaces.nsIFileInputStream);
			fstream.init(file, 1, 1, Components.interfaces.nsIFileInputStream.CLOSE_ON_EOF);
			var bstream = Components.classes['@mozilla.org/network/buffered-input-stream;1'].createInstance(Components.interfaces.nsIBufferedInputStream);
			bstream.init(fstream, 4096);
			var binary = Components.classes["@mozilla.org/binaryinputstream;1"].createInstance(Components.interfaces.nsIBinaryInputStream);
			binary.setInputStream(fstream);
			xhr = new XMLHttpRequest();
		
		

		var boundaryString = 'capitano';
		var boundary = '--' + boundaryString;
		var requestbody = boundary + '\n' 
		+ 'Content-Disposition: form-data; name="myfile"; filename="' + this.leafName+ '"'+'\n'
		+'Content-Type: application/octet-stream'+'\n'
		+ '\n'
		+ escape(binary.readBytes(binary.available()))
		+ '\n'
		+ boundary;
		
		
		//alert(requestbody);

	        xhr.onreadystatechange = function() { if(this.readyState == 4) {}}
		xhr.open("POST", "http://www.it.iitb.ac.in/~nithind/post.php", true);
		xhr.setRequestHeader("Content-Type", "multipart/form-data; boundary=\""+ boundaryString + "\"");
		xhr.setRequestHeader("Connection", "close");
		xhr.setRequestHeader("Content-length", requestbody.length);
		//alert(requestbody.length);
		xhr.send(requestbody);
		

	}
	

}

