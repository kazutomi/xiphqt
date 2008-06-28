var opwindowCommon={
	openFileWindowDialog:function()
        {
                const nsIFilePicker = Components.interfaces.nsIFilePicker;
                var fp = Components.classes["@mozilla.org/filepicker;1"].createInstance(nsIFilePicker);
                fp.init(window, "Choose a File", nsIFilePicker.modeOpen);
                fp.appendFilters(nsIFilePicker.filterAll | nsIFilePicker.filterText);

                var rv = fp.show();
                if (rv == nsIFilePicker.returnOK || rv == nsIFilePicker.returnReplace) {
                	var file = fp.file;
         		var path = fp.file.path;
			parent.document.getElementById("file-path").value = path;
                }
         },
	 myComponentTestGo:function()
	 {
		alert("Start of MyComponentTestGo()");
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
	}

}

