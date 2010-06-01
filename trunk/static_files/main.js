function refreshOldPage(pageId, timestamp){
	// Session Storage is available in FF3 and IE8
	var prevTimestamp = sessionStorage.getItem(pageId);
	if (prevTimestamp && timestamp <= parseInt(prevTimestamp)) {
		location.reload();
	}
	else {
		sessionStorage.setItem(pageId, timestamp.toString());
	}
}

function reloadScrollPos(pageId, refElement){
	var scrollPos = sessionStorage.getItem(pageId);
	if (scrollPos){
		scrollPos = parseInt(scrollPos);
		if (refElement){
			scrollPos = refElement.offset().top - scrollPos;
		}
		$(window).scrollTop(scrollPos);
		sessionStorage.removeItem(pageId);
	}
}

function saveScrollPos(pageId, refElement){
	var scrollPos = $(window).scrollTop();
	if (scrollPos > 0){
		if (refElement){
			scrollPos = refElement.offset().top - scrollPos;
		}
		sessionStorage.setItem(pageId, scrollPos);
	}
}

function showHideWidgets(){
	$(".widget").each(function() {
		$(".show_hide_widget", this).click(function(){
			showHideWidget($(this).parents(".widget"), true);
			return false;
		});
		showHideWidget(this, false);
	});
}

function showHideWidget(widget, change){
	var widget_id = $(widget).attr("id");	
	var show = localStorage.getItem("SH_" + widget_id) == "1";
	if (change){
		show = !show;
	}
	localStorage.setItem("SH_" + widget_id, show ? "1" : "0");

	if (show){
		$(".show_hide_widget > a", widget).text("Hide");
		$(".widget_body", widget).show();
	}
	else{
		$(".show_hide_widget > a", widget).text("Show");
		$(".widget_body", widget).hide();
	}
}
