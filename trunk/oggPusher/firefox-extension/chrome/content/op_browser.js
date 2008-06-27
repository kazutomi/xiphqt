function openDialog()
{
   //alert("hello"); 
   var object = window.open("chrome://oggPusher/content/op_Window.xul",
                      "oggPusher", "chrome,centerscreen,resizable=yes,toolbar=no,menubar=no");
	if(object==null)
	{ alert("inside the if");	}
}

