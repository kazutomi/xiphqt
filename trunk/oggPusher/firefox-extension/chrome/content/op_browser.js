var fileName, filePath;

var oggPusherCommon={
	openDialog:function()
	{
   		 window.open("chrome://oggPusher/content/op_Window.xul",
                      "oggPusher", "chrome,centerscreen,resizable=yes,toolbar=no,menubar=no");
	}
	/*openFileWindowDialog:function()
	{
		const nsIFilePicker = Components.interfaces.nsIFilePicker;
		var fp = Components.classes["@mozilla.org/filepicker;1"].createInstance(nsIFilePicker);
		fp.init(window, "Dialog Title", nsIFilePicker.modeOpen);
		fp.appendFilters(nsIFilePicker.filterAll | nsIFilePicker.filterText);

		var rv = fp.show();
		if (rv == nsIFilePicker.returnOK || rv == nsIFilePicker.returnReplace) {
  		var file = fp.file;
  		// Get the path as string. Note that you usually won't 
		//   // need to work with the string paths.
		var path = fp.file.path;
		// work with returned nsILocalFile...
		
		//var textboxElement = document.getElementById("file-path");
			
  		//textboxElement.value =

	       	}	
  	}*/
}



function getMainWindow() {
  return window.QueryInterface(Components.interfaces.nsIInterfaceRequestor)
                   .getInterface(Components.interfaces.nsIWebNavigation)
                   .QueryInterface(Components.interfaces.nsIDocShellTreeItem)
                   .rootTreeItem
                   .QueryInterface(Components.interfaces.nsIInterfaceRequestor)
                   .getInterface(Components.interfaces.nsIDOMWindow);
}

