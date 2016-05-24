function clog(msg) {
    window.external.Debug(msg);
}

var currentPercent = -1;
var progressWidth = 0;
var containerWidth = 0;

function setProgress(percent) {
    if (containerWidth == 0) {
        var container = document.getElementById("ProgressBarContainer");
        containerWidth = container.clientWidth / 100;
    }

    if (currentPercent != percent) {
        currentPercent = percent;
        var bar = document.getElementById("ProgressBar");
        progressWidth = containerWidth * percent;
//        clog("Setting " + progressWidth);
        bar.style.width = Math.ceil(progressWidth) + "px";
    }
}

function enableButton(id, enable) {
	addClassName(id, enable, "ButtonDisabled");
}

function enableElement(id, enable) {
	var element = document.getElementById(id);
	if(element == null) {
		clog("F(enableElement) No element found with id " + id);
	} else{
		element.disabled = !enable;
	}
}

function enableDownload(enable) {
    enableElement("OperatingSystemTypeOnline", enable);
    enableElement("OnlineImagesSelect", enable);

	showElement("NoInternetConnection", !enable);
	
	enableElement("DownloadLanguageSelect", enable);
//	enableButton("DownloadLightButtonC", enable); //disabled by C++
	enableButton("DownloadFullButtonC", enable);

	enableButton("SelectFileNextButtonC", enable);
}

function addClassName(id, remove, classname) {
    var elem = document.getElementById(id);
    if (elem == null) {
        clog("f(addClassName) No element found with id " + id);
        return;
    }

    if (remove) {		
        elem.className = elem.className.replace(classname, "");
    } else if (elem.className.indexOf(classname) == -1) {	
        elem.className += " " + classname;
    }
}

function showElement(id, show) {
    var classname = "hidden";
	addClassName(id, show, classname);    
}

function triggerEvent(elem, eventName) {
	if(document.fireEvent) {	
		elem.fireEvent("on" + eventName);
	} else {
		var event;
		if(document.createEvent) {
			event = document.createEvent("HTMLEvents");
			event.initEvent(eventName, true, true);
		} else if(document.createEventObject){ // IE < 9
			event = document.createEventObject();
            event.eventType = eventName;
		}
		event.eventName = eventName;
		clog(elem.dispatchEvent(event));
	}
}
	
function remoteSelectionChanged() {	
	var element = document.getElementById("DownloadLanguageSelect");
	clog("remoteSelectionChanged to " +  element.value);
	var remoteSelect = document.getElementById("OnlineImagesSelect");
	remoteSelect.selectedIndex = element.value;
	triggerEvent(remoteSelect, "change"); // for c++
}

function selectImageClicked(inputId) {
	var elem = document.getElementById(inputId);
	if(elem ==null) {
		clog("Element is null " + inputId);
		return;
	}
	elem.checked = true;
	triggerEvent(elem, "change"); // for c++
}