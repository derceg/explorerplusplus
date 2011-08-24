/*	rollover.js - for "rollover" images
Usage:
	o  attachdiv() must be called after page load (in BODY tag)
	   Eg. <body onload="javascript:attachdiv()">
	o  script must be loaded (of course!)
	o  main image must have id="MAINimg"
	o  rollovers do NOT work if image is contained in a table, however, the code can
	   be modified to do this (correction for offset)
Note that the rollover image has a border; THIS IS NECESSARY.  Without the border, the
hideroll() works intermittently!  Border tends to visually define the rollover
area/control better, anyway.

Multiple images per page CAN be done, but each image must:
	o  have its own unique ID
	o  make calls to its own unique function - eg. rollovr1(), rollovr2() - which
	   performs the same actions as rollovr(), but using the new image ID.

A peculiar (?) bug may be present in higher speed machines: if a link is clicked which
loads a page, and the mouse position is such that it is over a NEW link after the page is
loaded, then an error may result.  This is due to the new link being activated immediately,
BEFORE "attachdiv()" is completed on page load.  Solution is a simple YES/NO variable test
using the "pageloaded" variable.
*/

// GLOBAL DHTML objects
var Odiv,Oanchor,Oimg;
var pageloaded = false; // initially false, set true by attachdiv()
var url_scut=new Array
	("../images/roll",		// 0, for expand_url()
	"../images/roll/mnu_file",	// 1
	"../mnu_file",			// 2
	"../images/roll/mnu_edit",	// 3
	"../mnu_edit",			// 4
	"../images/roll/mnu_selection",	// 5
	"../mnu_selection",		// 6
	"../images/roll/redbox",	// 7
	"../images/roll/mnu_view",	// 8
	"../mnu_view",			// 9
	"../toolbars",			// 10
	"../images/roll/mnu_actions",	// 11
	"../mnu_actions",		// 12
	"../images/roll/mnu_go",	// 13
	"../mnu_go",			// 14
	"../images/roll/mnu_bookmarks",	// 15
	"../mnu_bookmarks",		// 16
	"../images/roll/mnu_tools",	// 17
	"../mnu_tools",			// 18
	"../images/roll/mnu_help",	// 19
	"../mnu_help",			// 20
	"../images/roll/radio",		// 21
	"../images/roll/checkbox",	// 22
	"../images/roll/button",	// 23
	"../pane_folders",		// 24
	"../pane_files",		// 25
	"../pane_dispwin",		// 26
	"../pane_tabs",			// 27
	"../images/roll/tb_bookmarks",	// 28
	"../images/roll/ctxt-bm_organize",	// 29
	"../images/roll/tb_application",// 30
	"../images/roll/tab_context",	// 31
	"../mnu_tools/general",		// 32
	"../mnu_tools/files_folders",	// 33
	"../mnu_tools/window",		// 34
	"../mnu_tools/tabs",		// 35
	"../mnu_tools/default");	// 36


var tip_scut=new Array
	(" - Click for more...",	// 0, for expand_tip()
	"Click for more...",		// 1
	"'",				// 2 single quote
	'"',				// 3 double quote
	"Apply changes (close dialog)",		// 4 for dialog buttons
	"Discard changes (close dialog)",	// 5
	"Apply changes (leave dialog open)",	// 6
	"Toggles visibility of the ",	// 7
	"Close dialog",			// 8
	"Window resize grip");		// 9

//------------------------------------------------

function attachdiv() // to be done ONLY after document has loaded
{ Odiv=document.createElement("DIV"); // houses anchor
  Oanchor=document.createElement("A"); // houses image
  Oimg=document.createElement("IMG");  // rollover image

  Odiv.appendChild(Oanchor);
  with (Odiv.style)
  { display="none";
    position="absolute";
    overflow="hidden";
    marginBottom=-15
  }
  with (Oanchor)
  { appendChild(Oimg);
    attachEvent("onmouseout",hideroll)
  }
  Oimg.border="1px";
  document.body.insertBefore(Odiv,document.body.firstChild); 
// attach for display, as first element (for speed?), eliminates scroll bar effect
/* Once page is abandoned, javascript garbage collection should clean up objects, since
all references are void */
  pageloaded=true // ok to activate rollover functions - see bug note above
}

//------------------------------------------------

function expand_url(s) // expand urls by utilizing location shortcuts, returns modifed url
/* Sometimes a few locations are used a lot - you can put them in the "loc" array, and
use a special syntax to refer to them.   For example, the relative url to a rollover image
might be "../images/roll/edit.gif", but the special syntax is "{0}/edit.gif".  This may not
save a lot of space (if any), but is a convenience. Only a single expansion per href. */
// NO ERROR CHECKS!
{ var bracepos1=s.indexOf("{");
  var bracepos2=s.indexOf("}");
  if ((bracepos1>-1)&&(bracepos2>-1)) // both found, shorcut exists
    { var shcut=s.substring(bracepos1,bracepos2+1);
      s=s.replace(shcut,url_scut[s.substring(bracepos1+1,bracepos2)])
    }
  return(s)
}

//------------------------------------------------

function expand_tip(s) // similar to expand_urls, but for tooltip text - multi use allowed
// NO ERROR CHECKS!
{ while(s.indexOf("{") > -1)
    { var bracepos1=s.indexOf("{");
      var bracepos2=s.indexOf("}",bracepos1);
      var shcut=s.substring(bracepos1,bracepos2+1);
      s=s.replace(shcut,tip_scut[s.substring(bracepos1+1,bracepos2)])
    }
  return(s)
}

//------------------------------------------------

function doroll(image,area,rollimg,rollhref,rolltip)
// image:	image object which houses image map
// rollimg:	url to rollover image, see expand_url()
// rollhref:	link for image area, see expand_url(),
//		use "#x" for empty (never used)
//			- but, causes page to re-display at top if clicked
// rolltip:	text tip, see expand_tip()
// sample action: onmouseover="javascript:rollovr(this,'{0}/file.gif','mnu-file.htm','File menu{0}')"
{ if (!pageloaded) // if attachdiv() not completed, then do nothing!
    return;
  var crds=area.coords.split(","); // array of "values"
  with (Oimg)
   { src=expand_url(rollimg)
   }
   with (Oanchor)
   { title=expand_tip(rolltip);
     href=expand_url(rollhref);
     style.backgroundColor="transparent" // affects rollover image HREFs, no CSS colors!
     if (href.substr(0,7).toLowerCase()=="http://") // this is an Internet URL!
       target="_blank" // new window
     else
       target="_self"; // same window
   }

   var leftval=parseInt(crds[0])+image.offsetLeft-1; //-1 to allow for border
   var topval=parseInt(crds[1])+image.offsetTop-1; //-1 to allow for border
// image might be housed in a table or ?, which would give a false absolute left/top value!
   var parent=image.offsetParent;
   if (parent.tagName != 'BODY') // if parent is not BODY...
       { do
         { leftval += parent.offsetLeft; // add parent's left
           topval += parent.offsetTop; // add parent's top
           parent=parent.offsetParent // and get IT'S parent
         }
         while (parent.tagName != 'BODY')
       }

   with (Odiv.style)
   { left=leftval; //-1 to allow for border
     top=topval; //-1 to allow for border
     display="block"
   }
}

//------------------------------------------------

function rollovr(area,rollimg,rollhref,rolltip) // from main image
{ doroll(MAINimg,area,rollimg,rollhref,rolltip)
}

//------------------------------------------------

function rollovr2(area,rollimg,rollhref,rolltip) // from secondary image
{ doroll(IMG02,area,rollimg,rollhref,rolltip)
}

//------------------------------------------------

function rollovr3(area,rollimg,rollhref,rolltip) // from tertiary image
{ doroll(IMG03,area,rollimg,rollhref,rolltip)
}

//------------------------------------------------

function hideroll()
{ Odiv.style.display="none"
}
