// Basic functions
function is_chrome() {
	return ((navigator.userAgent + "").indexOf(" Chrome/") >= 0);
}
function is_firefox() {
	return ((navigator.userAgent + "").indexOf("Mozilla/") >= 0);
}

function text_to_html(str) {
	return str.replace(/&/g, "&amp;")
		.replace(/>/g, "&gt;")
		.replace(/</g, "&lt;")
		.replace(/"/g, "&quot;");
}

function change_style_display(class_names, display_prefix, on) {
	on = on ? 1 : 0;
	$("." + class_names[on]).removeClass(display_prefix + "DisplayOff").addClass(display_prefix + "DisplayOn");
	$("." + class_names[1 - on]).removeClass(display_prefix + "DisplayOn").addClass(display_prefix + "DisplayOff");
}
function change_browser_display(show_all) {
	if (!show_all && is_chrome()) {
		$(".SpecificBrowser").removeClass("BrowserDisplayOff").addClass("BrowserDisplayOn");
		$(".Firefox").removeClass("BrowserDisplayOn").addClass("BrowserDisplayOff");
		$(".UniversalBrowser").removeClass("BrowserDisplayOn").addClass("BrowserDisplayOff");
		$(".Chrome").removeClass("BrowserDisplayOff").addClass("BrowserDisplayOn");
	}
	else if (!show_all && is_firefox()) {
		$(".SpecificBrowser").removeClass("BrowserDisplayOff").addClass("BrowserDisplayOn");
		$(".Chrome").removeClass("BrowserDisplayOn").addClass("BrowserDisplayOff");
		$(".UniversalBrowser").removeClass("BrowserDisplayOn").addClass("BrowserDisplayOff");
		$(".Firefox").removeClass("BrowserDisplayOff").addClass("BrowserDisplayOn");
	}
	else {
		$(".Firefox").removeClass("BrowserDisplayOff").addClass("BrowserDisplayOn");
		$(".Chrome").removeClass("BrowserDisplayOff").addClass("BrowserDisplayOn");
		$(".SpecificBrowser").removeClass("BrowserDisplayOn").addClass("BrowserDisplayOff");
		$(".UniversalBrowser").removeClass("BrowserDisplayOff").addClass("BrowserDisplayOn");
	}
}

// Window url hash management
function WindowHash() {
	this.hash = "";
	this.page = "";
	this.vars = {};
	this.history_mode = 0;

	this.history = [];
	this.history_index = -1;
};
WindowHash.prototype = {
	constructor: WindowHash,
	on_change: function (event) {
		if (this.hash == window.location.hash) return;

		// Get the new hash
		this.hash = window.location.hash;
		if (this.hash.length > 0) this.hash = this.hash.substr(1);

		// Get the page
		var h = this.hash.split("?");
		this.page = h[0];

		// Get any variables
		this.vars = {};
		h = h.splice(1, h.length - 1).join("?").split("&");
		for (var i = 0; i < h.length; ++i) {
			if (h[i].length == 0) continue;
			var p = h[i].split("=");
			this.vars[p[0]] = (p.length == 1) ? null : p.splice(1, p.length - 1).join("=");
		}

		// History update
		if (this.history_mode == 0) {
			if (this.history_index < this.history.length - 1) {
				this.history.splice(this.history_index, this.history.length - 1 - this.history_index);
			}
			this.history.push([this.hash , this.page , this.vars]);
			++this.history_index;
		}
		else {
			this.history_index += this.history_mode;
			alert(this.history[this.history_index][0] + "\n" + this.hash);
		}
	},
	goto_page: function (page, vars) {
		page = page || "";

		var i = 0;
		for (var a = 1; a < arguments.length; ++a) {
			for (var v in arguments[a]) {
				page += (i == 0 ? "?" : "&") + v + (arguments[a][v] === null ? "" : "=" + arguments[a][v]);
				++i;
			}
		}

		window.location.hash = page;
	},
	has_previous: function () {
		return (this.history_index > 0);
	},
	goto_previous: function () {
		if (this.history_index > 0) {
			this.history_mode = -1;
			window.location.hash = this.history[this.history_index - 1][0];
			this.on_change();
			this.history_mode = 0;

			return true;
		}
		return false;
	},
	has_next: function () {
		return (this.history_index < this.history.length - 1);
	},
	goto_next: function () {
		if (this.history_index < this.history.length - 1) {
			this.history_mode = 1;
			window.location.hash = this.history[this.history_index + 1][0];
			this.on_change();
			this.history_mode = 0;

			return true;
		}
		return false;
	}
};
var window_hash = new WindowHash();

// Pages
var page_list = {
	"about": {
		"userscript": null,
		"development": null,
		"codecs": {
			"embed.exe": null,
			"extract.exe": null,
			"batch-*.exe": null
		},
		"acknowledgements": null
	},
	"issues": null,
	"source": null,
	"wiki": null,
	"changes": null
};
function PageBrowser() {

}
PageBrowser.prototype = {
	constructor: PageBrowser,
	open: function (page, vars, refresh) {
		// Which page
		var title = "";
		var p = page.split("/");
		var s = page_list;
		var nav_page = page;
		for (var i = 0; i < p.length; ++i) {
			if (s !== null && p[i] in s) {
				s = s[p[i]];
				title += (title.length == 0 ? "" : " / ") + p[i];
				if (i == 0) nav_page = p[i];
			}
			else {
				title = "";
				nav_page = page = "install";
			}
		}

		$(".Content").removeClass("ContentActive");
		$(".NavigationLink").removeClass("NavigationLinkCurrent");
		$("#content_" + page.replace(/\W/g, "_")).addClass("ContentActive");
		$("#navigation_" + nav_page).addClass("NavigationLinkCurrent");

		$("title").html(default_title + (title.length == 0 ? "" : " / " + title));
		change_style_display(["NonDeveloper","Developer"], "Developer", ("dev" in vars));
		change_style_display(["NoHelp","Help"], "Help", ("help" in vars));
		change_browser_display(("all" in vars));

		$(".PageVariableDisplay").each(function () {
			$(this)
			.removeClass("PageVariableDisplayOn PageVariableDisplayOff")
			.addClass("PageVariableDisplay" + (($(this).attr("pvar") in vars) ? "Off" : "On"));
		});
		$(".PageVariableDisplayInv").each(function () {
			$(this)
			.removeClass("PageVariableDisplayOn PageVariableDisplayOff")
			.addClass("PageVariableDisplay" + (($(this).attr("pvar") in vars) ? "On" : "Off"));
		});
	}
};
var page_browser = new PageBrowser();

// Change log
var change_log_version = null;
function get_change_log() {
	var log_url = "changelog.txt";

	$.ajax({
		type: "GET",
		url: log_url,
		dataType: "text",
		success: function (data, status, jqXHR) {
			display_change_log(parse_change_log(data));
		},
		error: function (jqXHR, status, error) {
			$("#change_log").css("display", "");
		}
	});
}
function parse_change_log(data) {
	// Parse change log
	data = data.replace(/\r\n/g, "\n").split("\n\n");
	var log = [];
	for (var i = 0; i < data.length; ++i) {
		data[i] = data[i].trim();
		if (data[i].length == 0) continue;

		log.push([]);
		data[i] = data[i].split("\n");
		for (var j = 0; j < data[i].length; ++j) {
			if (j == 0) {
				log[log.length - 1].push(data[i][j]);
			}
			else {
				if (data[i][j][0] == "-") {
					log[log.length - 1].push(data[i][j].substr(1).trim());
				}
				else {
					log[log.length - 1][log[log.length - 1].length - 1] += "\n" + (data[i][j].substr(1).trim());
				}
			}
		}
	}

	return log;
}
function display_change_log(log) {
	// Output version
	change_log_version = log[0][0];
	$(".Version").html(text_to_html(change_log_version));

	// Output changelog
	var cl = $("#change_log");
	cl.css("display", "");
	cl.html("");
	for (var i = 0; i < log.length; ++i) {
		var list;
		cl.append(
			$(document.createElement("div"))
			.addClass("ChangeLogLabel")
			.html(text_to_html(log[i][0]) + (i == 0 ? "<span class=\"ChangeLogLabelCurrent\"> (current)</span>" : ""))
		);
		cl.append(
			(list = $(document.createElement("ul")))
			.addClass("ChangeLogList")
		);
		for (var j = 1; j < log[i].length; ++j) {
			list.append(
				$(document.createElement("li"))
				.html(text_to_html(log[i][j]).replace("\n", "<br />"))
			);
		}
	}

	// Compare
	version_compare();
}

// Title management
var default_title = "";

// Image previewing
function image_preview(obj) {
	var descr = (obj.next().length > 0 ? (obj.next().hasClass && obj.next().hasClass("ImageDescription") ? obj.next().html() : "") : "");
	var descr_container, img_append, offset, offset2, img;

	// Remove any previous
	$("body").find(".ImagePreviewBoxInner2").remove();

	// Create new
	$("body").append(
		(offset = $(document.createElement("div")))
		.addClass("ImagePreviewBoxInner2")
		.append(
			(offset2 = $(document.createElement("div")))
			.append(
				(img_append = $(document.createElement("a")))
				.addClass("ImagePreviewImageContainer")
				.attr("href", obj.attr("href"))
				.attr("target", "_blank")
				.on("click", function () { return true; })
			)
			.append(
				(descr_container = $(document.createElement("div")))
				.addClass("ImagePreviewDescriptionContainer")
				.html(descr)
			)
		)
		.on("click", {}, function (event) {
			if (event.which == 1) {
				event.preventDefault();
				event.stopPropagation();
				return false;
			}
			return true;
		})
		.css({"left": "0", "top": "0", "opacity": "0"})
	);

	// Click to close
	$(".ImagePreviewOverlay")
	.on("click", {}, function (event) {
		if (event.which == 1) {
			image_preview_close(img);
		}
		return true;
	});

	// Image
	img_append.append(
		(img = $(document.createElement("img")))
		.attr("src", obj.attr("href"))
		.on("load", {}, function (event) {
			// Image loaded; open
			descr_container.css({
				"width": descr_container.outerWidth(),
			});
			offset.css({
				"left": (-offset.outerWidth() / 2) + "px",
				"top": (-offset.outerHeight() / 2) + "px",
			});
			$(".ImagePreviewOverlayInner").html(
				$(document.createElement("div"))
				.addClass("ImagePreviewBox")
				.append(
					$(document.createElement("div"))
					.addClass("ImagePreviewBoxInner1")
					.append(
						offset
						.css("opacity", "")
					)
				)
			);
			$(".ImagePreviewOverlay").css("display", "block");
		})
	);
}
function image_preview_close(img) {
	img.off("load").removeAttr("src");
	$(".ImagePreviewOverlay")
	.off("click")
	.css("display", "");
}

// Version check
var current_version = null;
function version_check(version) {
	if (typeof(version) == typeof("")) {
		current_version = version;

		version_compare();
	}
}
function version_compare() {
	// Only compare if both versions are available
	if (current_version === null || current_version === null) return;

	// Compare
	var current_version_split = current_version.toString().split(".");
	var new_version_split = change_log_version.split(".");
	var len = (new_version_split.length > current_version_split.length ? new_version_split.length : current_version_split.length);
	for (var i = 0; i < len; ++i) {
		if (
			(i < new_version_split.length ? (parseInt(new_version_split[i]) || 0) : 0) >
			(i < current_version_split.length ? (parseInt(current_version_split[i]) || 0) : 0)
		) {
			// Behind
			$(".Version")
			.addClass("VersionBehind")
			.attr("title", "Your version is behind (at " + current_version + ")")
			.html(text_to_html(change_log_version));
			break;
		}
		else if (
			(i < new_version_split.length ? (parseInt(new_version_split[i]) || 0) : 0) <
			(i < current_version_split.length ? (parseInt(current_version_split[i]) || 0) : 0)
		) {
			// Ahead
			$(".Version")
			.addClass("VersionAhead")
			.attr("title", "Your version is ahead (of " + change_log_version + ")")
			.html("*" + text_to_html(current_version));
			break;
		}
	}
	if (i == len) { // Same
		$(".Version")
		.addClass("VersionSame")
		.attr("title", "Your version is up to date")
		.html(text_to_html(change_log_version));
	}
}

// Entry
$(document).ready(function () {
	// Events
	$("#show_all_browser").on("click", {}, function (event) {
		if (event.which == 1) {
			change_browser_display(true);
			return false;
		}
		return true;
	});
	$("#developer_change").on("click", {}, function (event) {
		if (event.which == 1) {
			change_style_display(["NonDeveloper","Developer"], "Developer", true);
			return false;
		}
		return true;
	});
	$("#help_change").on("click", {}, function (event) {
		if (event.which == 1) {
			change_style_display(["NoHelp","Help"], "Help", true);
			return false;
		}
		return true;
	});
	$(".Hardlink").on("click", {}, function (event) {
		if (event.which == 1) {
			var ex = {};
			var v = $(this).attr("id").substr("hardlink_enable_".length);
			if (!(v in window_hash.vars)) ex[v] = null;
			window_hash.goto_page(
				window_hash.page,
				window_hash.vars,
				ex
			);
			return false;
		}
		return true;
	});
	$(".NavigationLink").on("click", {}, function (event) {
		if (event.which == 1) {
			window_hash.goto_page(
				$(this).attr("href")[0] == "#" ? $(this).attr("href").substr(1) : $(this).attr("id").substr("navigation_".length),
				window_hash.vars
			);
			return false;
		}
		return true;
	});
	$(".ImageLink").on("click", {}, function (event) {
		if (event.which == 1) {
			image_preview($(this));
			return false;
		}
		return true;
	});


	// Change log
	get_change_log();

	// Title
	default_title = $("title").html();

	// Page display
	var hashchange = function (event) {
		window_hash.on_change(event);
		page_browser.open(window_hash.page, window_hash.vars, event===null);
	};
	$(window).on("hashchange", {}, hashchange);
	hashchange(null);
});

