
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

function tmpChangePage(from, to) {
    document.getElementById(from).className = "WizardPage hidden";
    document.getElementById(to).className = "WizardPage";
}