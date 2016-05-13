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
        clog("Setting " + progressWidth);
        bar.style.width = Math.ceil(progressWidth) + "px";
    }
}

function enableDownload(enable) {
    document.getElementById("OperatingSystemTypeOnline").disabled = !enable;
    document.getElementById("OnlineImagesSelect").disabled = !enable;
}

function showElement(id, show) {
    var classname = "hidden";
    var elem = document.getElementById(id);
    if (elem == null) {
        clog("No element found with id " + id);
        return;
    }
    if (show) {
        elem.className = elem.className.replace(classname, "");
    } else if (elem.className.indexOf(classname) == -1) {
        elem.className += " " + classname;
    }
}