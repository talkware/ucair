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

function showRating(ratingElem, sendRating) {
	var y = ratingElem.nextAll(".search_result_rating_y");
	var n = ratingElem.nextAll(".search_result_rating_n");
	var c = ratingElem.nextAll(".search_result_rating_c");
	var val = ratingElem.text();
	if (val == "Y") {
		y.addClass("rated").show();
		n.removeClass("rated").hide();
		c.show();
	}
	else if (val == "N") {
		y.removeClass("rated").hide();
		n.addClass("rated").show();
		c.show();
	}
	else {
		y.removeClass("rated").show();
		n.removeClass("rated").show();
		c.hide();
	}
	if (sendRating) {
		var url = ratingElem.nextAll(".search_result_rating_url").text();
		$.get(url + encodeURI(val));
	}
}

function prepareRatings() {
	$(".search_result_rating").each(function() {
		var ratingElem = $(this);
		showRating(ratingElem, false);
		var y = ratingElem.nextAll(".search_result_rating_y");
		var n = ratingElem.nextAll(".search_result_rating_n");
		var c = ratingElem.nextAll(".search_result_rating_c");
		y.click(function() {
			ratingElem.text("Y");
			showRating(ratingElem, true);
		});
		n.click(function() {
			ratingElem.text("N");
			showRating(ratingElem, true);
		});
		c.click(function() {
			ratingElem.text("");
			showRating(ratingElem, true);
		});
	});
}
