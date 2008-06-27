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
         }
}

