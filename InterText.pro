# InterText qmake project file
QT += widgets xml network svg
QTPLUGIN += qico
TEMPLATE = app
TARGET = InterText
DEPENDPATH += . src
INCLUDEPATH += . src
RESOURCES = InterText.qrc
CODECFORTR      = UTF-8
CODECFORSRC     = UTF-8
RC_FILE = InterText.rc
ICON = InterText.icns

# Input
HEADERS += \
    src/ItWindow.h \
		src/ItAlignment.h \
		src/ItDocument.h \
		src/ItElement.h \
		src/ItAlignmentModel.h \
		src/ItAlignmentView.h \
		src/ItAlignmentDelegate.h \
		src/ItSegmentView.h \
    src/ItPlainTextEdit.h \
    src/ItCommands.h \
    src/numberingdialog.h \
    src/ItSearchBar.h \
    src/ItAbstractDelegate.h \
    src/AutoAlignDialog.h \
    src/AlignerView.h \
    src/ImportXmlDialog.h \
    src/ItSentenceSplitter.h \
    src/ImportTxtDialog.h \
    src/AlignmentManager.h \
    src/SettingsDialog.h \
    src/AlignerProfileDialog.h \
    src/ServerDialog.h \
    src/simplecrypt.h \
    src/ChangeDialog.h \
    src/RemoteAttrDialog.h \
    src/AlignmentAttrDialog.h \
    src/NewAlignmentDialog.h \
    src/ItCustomVarsDialog.h \
    src/ItSegmentDelegate.h \
    src/ItCommon.h \
    src/ItQuestionDialog.h \
    src/ItFloatControls.h \
    src/CustomizeDialog.h \
    src/ItServerConn.h \
    src/itdommodel.h \
    src/itdomitem.h \
    src/xmltreedialog.h
SOURCES += \
    src/ItWindow.cpp \
		src/ItAlignment.cpp \
		src/ItDocument.cpp \
		src/main.cpp \
		src/ItElement.cpp \
		src/ItAlignmentModel.cpp \
		src/ItAlignmentView.cpp \
		src/ItAlignmentDelegate.cpp \
		src/ItSegmentView.cpp \
    src/ItPlainTextEdit.cpp \
    src/ItCommands.cpp \
    src/numberingdialog.cpp \
    src/ItSearchBar.cpp \
    src/ItAbstractDelegate.cpp \
    src/AutoAlignDialog.cpp \
    src/AlignerView.cpp \
    src/ImportXmlDialog.cpp \
    src/ItSentenceSplitter.cpp \
    src/ImportTxtDialog.cpp \
    src/AlignmentManager.cpp \
    src/SettingsDialog.cpp \
    src/AlignerProfileDialog.cpp \
    src/ServerDialog.cpp \
    src/simplecrypt.cpp \
    src/ChangeDialog.cpp \
    src/RemoteAttrDialog.cpp \
    src/AlignmentAttrDialog.cpp \
    src/NewAlignmentDialog.cpp \
    src/ItCustomVarsDialog.cpp \
    src/ItSegmentDelegate.cpp \
    src/ItQuestionDialog.cpp \
    src/ItFloatControls.cpp \
    src/CustomizeDialog.cpp \
    src/ItServerConn.cpp \
    src/itdommodel.cpp \
    src/itdomitem.cpp \
    src/xmltreedialog.cpp

FORMS += \
    src/numberingdialog.ui \
    src/AutoAlignDialog.ui \
    src/AlignerView.ui \
    src/ImportXmlDialog.ui \
    src/ImportTxtDialog.ui \
    src/AlignmentManager.ui \
    src/SettingsDialog.ui \
    src/AlignerProfileDialog.ui \
    src/ServerDialog.ui \
    src/ChangeDialog.ui \
    src/RemoteAttrDialog.ui \
    src/AlignmentAttrDialog.ui \
    src/NewAlignmentDialog.ui \
    src/ItCustomVarsDialog.ui \
    src/ItQuestionDialog.ui \
    src/CustomizeDialog.ui \
    src/xmltreedialog.ui

OTHER_FILES += \
    Changelog.txt

DISTFILES += \
    CHANGELOG

