function clog(msg) {
    window.external.Debug(msg);
}


function opSystemTypeSelected() {
    var local = document.getElementById("OperatingSystemTypeLocal");
    if (local.checked) {
        document.getElementById("LocalImagesSelect").className = "selected";
        document.getElementById("OnlineImagesSelect").className = "";
    } else {
        document.getElementById("LocalImagesSelect").className = "";
        document.getElementById("OnlineImagesSelect").className = "selected";
    }
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