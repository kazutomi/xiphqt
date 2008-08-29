var opwindowCommon={leafName:null, timer:null,isOgg:null,targetPathToUpload:null,
	openFileWindowDialog:function()
    {
		//alert("Inside openFileWindowDialog");
		try{
           	const nsIFilePicker = Components.interfaces.nsIFilePicker;
			var fp = Components.classes["@mozilla.org/filepicker;1"].createInstance(nsIFilePicker);
        	fp.init(window, "Choose a File", nsIFilePicker.modeOpen);
           	fp.appendFilters(nsIFilePicker.filterAll );
            fp.appendFilter("Video Files","*.mpeg;*.mpg;*.avi;*.asf;*.flv;*.mov;*.mkv;*.wmv");
            fp.appendFilter("Audio Files","*.mp3;*.wma");
            fp.appendFilter("Free Format","*.ogg;*.ogv;*.oga;*.ogx")
            //fp.appendFilter("mpg","*.mpg");
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
            this.isOggFormat();
            this.appendActionButton();    
            
        }
        
    },
    isOggFormat:function(){
        var file_name = document.getElementById("file-path").value;
        var _1 = file_name.lastIndexOf('.');
        var file_extension = file_name.substr(_1 + 1);
        if( file_extension == "ogg" || file_extension == "ogx" || file_extension == "ogv" || file_extension == "oga")
        {
            this.isOgg = true;
            this.targetPathToUpload = file_name;
        }else{
            this.isOgg = false;        
        }
        
    },
    appendActionButton:function(){
        
        var hboxParent = document.getElementById("op-input");
        
        var actionButton = document.getElementById("action-button");
        if(actionButton == null){
            if(this.isOgg){        
                actionButton = document.createElement("button");
                actionButton.setAttribute("id","action-button");
                actionButton.setAttribute("oncommand","opwindowCommon.actionButtonHandler();");
                actionButton.setAttribute("image","chrome://oggPusher/content/images/xiph.ico");
                actionButton.setAttribute("orient","vertical");
                actionButton.setAttribute("label","Upload Audio/Video");
                hboxParent.appendChild(actionButton);
            }
            else
            {
                actionButton = document.createElement("button");
                actionButton.setAttribute("id","action-button");
                actionButton.setAttribute("oncommand","opwindowCommon.actionButtonHandler();");
                actionButton.setAttribute("image","chrome://oggPusher/content/images/xiph.ico");
                actionButton.setAttribute("orient","vertical");
                actionButton.setAttribute("label","Transcode & Upload");
                hboxParent.appendChild(actionButton);
                
            }
        }
        else
        {
            hboxParent.removeChild(actionButton);
            if(this.isOgg){        
                actionButton = document.createElement("button");
                actionButton.setAttribute("id","action-button");
                actionButton.setAttribute("oncommand","opwindowCommon.actionButtonHandler();");
                actionButton.setAttribute("image","chrome://oggPusher/content/images/xiph.ico");
                actionButton.setAttribute("orient","vertical");
                actionButton.setAttribute("label","Upload Audio/Video");
                hboxParent.appendChild(actionButton);
            }
            else
            {
                actionButton = document.createElement("button");
                actionButton.setAttribute("id","action-button");
                actionButton.setAttribute("oncommand","opwindowCommon.actionButtonHandler();");
                actionButton.setAttribute("image","chrome://oggPusher/content/images/xiph.ico");
                actionButton.setAttribute("orient","vertical");
                actionButton.setAttribute("label","Transcode & Upload");
                hboxParent.appendChild(actionButton);
                
            }
                
        }
    },
    actionButtonHandler:function(){
        if(this.isOgg){
            this.uploadFile();
        }
        else{
            this.convertToTheora();
            
                   
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
                if(process.exitValue!= -1){
                        alert(process.exitValue+"Inside the processHandler");
                        this.timer = setTimeout( function(el) { return function(){el.processHandler(process);}}(this), 10000 );
                }else{
                        alert("done with transcoding");
                        this.uploadFile();
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
			var file = em.getInstallLocation(MY_ID).getItemFile(MY_ID, "chrome/content/sample");
			// returns nsIFile for the extension's install.rdf
			var process = Components.classes["@mozilla.org/process/util;1"].createInstance(Components.interfaces.nsIProcess);
			process.init(file);
			file.permissions = 0755;
			var args_list = new Array(3);
           	args_list[0] = " -o ";
            var _1 = this.leafName.lastIndexOf('.');
            var _2 = this.leafName.substr(0,_1);
            //alert("_2"+ _2);
            var _3 = _2.replace(/\s/g,'\\ ');
            //alert("_3"+_3);
			args_list[1] = "/tmp/"+_3+".ogv " ;
            //alert(args_list[1]);
            this.targetPathToUpload = "/tmp/"+_3 +".ogv";
			args_list[2] = document.getElementById("file-path").value.replace(/\s/g,'\\ ');
			//alert(args_list[2]);
	         //alert("Before process.run");
        	 alert(process.run(false,args_list, args_list.length));
			//alert("Pid "+process.pid+" exit value "+process.exitValue+" Location "+process.location+" processName "+process.processName);
			//alert("After the process.run");
			//this.timer = setTimeout( function(){this.processHandler(process);}, 10000 );
			this.timer = setTimeout( function(el) { return function(){el.processHandler(process);}}(this), 10000 );
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
		/*var filePath;
        if(this.isOgg){
            filePath = document.getElementById("file-path").value;        
        }
        else{
                
        }*/
		
		alert(this.targetPathToUpload);
        
		var file = Components.classes['@mozilla.org/file/local;1'].createInstance(Components.interfaces.nsILocalFile);
		file.initWithPath(this.targetPathToUpload);
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
		+ 'Content-Disposition: form-data; name="myfile"; filename="' + this.targetPathToUpload+ '"'+'\n'
		+'Content-Type: application/octet-stream'+'\n'
		+ '\n'
		+ escape(binary.readBytes(binary.available()))
		+ '\n'
		+ boundary;
		
		
		//alert(requestbody);

	    xhr.onreadystatechange = function() { if(this.readyState == 4) {alert("Uploading Done Successfully.");}}
		xhr.open("POST", "http://www.it.iitb.ac.in/~nithind/post.php", true);
		xhr.setRequestHeader("Content-Type", "multipart/form-data; boundary=\""+ boundaryString + "\"");
		xhr.setRequestHeader("Connection", "close");
		xhr.setRequestHeader("Content-length", requestbody.length);
		xhr.send(requestbody);
		

	}
	

}

