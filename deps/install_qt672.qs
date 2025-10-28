var PageEnum = QInstaller.ComponentSelection;

function Controller()
{
    installer.autoAcceptMessageBoxes();
    installer.setMessageBoxAutomaticAnswer(QInstaller.ConfirmCancelDialog, QMessageBox.Yes);
}

Controller.prototype.IntroductionPageCallback = function()
{
    gui.clickButton(buttons.NextButton);
};

Controller.prototype.ComponentSelectionPageCallback = function()
{
    var widget = gui.pageWidgetByObjectName("ComponentSelectionPage");
    if (widget) {
        widget.deselectAll();
        widget.selectComponent("qt.qt6.672");
        widget.selectComponent("qt.qt6.672.win64_msvc2019_64");
        widget.selectComponent("qt.qt6.672.win64_msvc2019_64.qt6compat");
        widget.selectComponent("qt.qt6.672.win64_msvc2019_64.qtdeclarative");
        widget.selectComponent("qt.qt6.672.win64_msvc2019_64.qttools");
        widget.selectComponent("qt.qt6.672.win64_msvc2019_64.qtshadertools");
        widget.selectComponent("qt.qt6.672.win64_msvc2019_64.qtimageformats");
    }
    gui.clickButton(buttons.NextButton);
};

Controller.prototype.LicenseAgreementPageCallback = function()
{
    var widget = gui.pageWidgetByObjectName("LicenseAgreementPage");
    if (widget) {
        widget.acceptLicenseAgreement();
    }
    gui.clickButton(buttons.NextButton);
};

Controller.prototype.StartMenuDirectoryPageCallback = function()
{
    gui.clickButton(buttons.NextButton);
};

Controller.prototype.ReadyForInstallationPageCallback = function()
{
    gui.clickButton(buttons.NextButton);
};

Controller.prototype.PerformInstallationPageCallback = function()
{
    gui.clickButton(buttons.NextButton);
};

Controller.prototype.FinishedPageCallback = function()
{
    gui.clickButton(buttons.FinishButton);
}
