/*
 * ------------------------------------------------------------------------------------
 *  File: main.cpp
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Original code:
 *      Copyright (c) 2020  Gavrilovici Eduard
 *
 *  Fork, refactoring, and ongoing development:
 *      Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Hosts the Qt application entry point wiring the core controller and GUI window together.
 *
 *  License:
 *      This file is part of a derived work based on the Asclepios DICOM Viewer,
 *      licensed under the MIT License.
 *
 *      Permission is hereby granted, free of charge, to any person obtaining a copy
 *      of this software and associated documentation files (the "Software"), to deal
 *      in the Software without restriction, including without limitation the rights
 *      to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *      copies of the Software, and to permit persons to whom the Software is
 *      furnished to do so, subject to the following conditions:
 *
 *      The above copyright notice and this permission notice shall be included in
 *      all copies or substantial portions of the Software.
 *
 *      THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *      IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *      FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *      AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *      LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *      OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *      SOFTWARE.
 * ------------------------------------------------------------------------------------
 */

//#include <vld.h>
#include <vtkAutoInit.h>
#include <algorithm>
#include <QDateTime>
#include <cstdlib>
#include <memory>
#include <string>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QSurfaceFormat>
#include <QVTKOpenGLNativeWidget.h>
#include <QStandardPaths>
#include <QMutex>
#include <QMutexLocker>
#include <QTextStream>
#include <QString>

#include <vtkDataArray.h>
#include <vtkFileOutputWindow.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkImageActor.h>
#include <vtkNew.h>
#include <vtkPNGWriter.h>
#include <vtkPointData.h>
#include <vtkSmartPointer.h>
#include <vtkOutputWindow.h>
#include <vtkWindowToImageFilter.h>

#include "gui.h"
#include "guiframe.h"
#include "dicomvolume.h"
#include "image.h"
#include "vtkwidgetdicom.h"

VTK_MODULE_INIT(vtkRenderingOpenGL2);
VTK_MODULE_INIT(vtkInteractionStyle);
VTK_MODULE_INIT(vtkRenderingFreeType);
VTK_MODULE_INIT(vtkRenderingVolumeOpenGL2);

namespace
{
QFile g_logFile;
QMutex g_logMutex;

QString logTypeToString(QtMsgType type)
{
	switch (type)
	{
	case QtDebugMsg: return "DEBUG";
	case QtInfoMsg: return "INFO";
	case QtWarningMsg: return "WARN";
	case QtCriticalMsg: return "CRITICAL";
	case QtFatalMsg: return "FATAL";
	default: return "UNKNOWN";
	}
}

void messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& message)
{
	QMutexLocker locker(&g_logMutex);

	if (!g_logFile.isOpen())
	{
		return;
	}

	QTextStream stream(&g_logFile);
	stream << QDateTime::currentDateTime().toString(Qt::ISODateWithMs) << " ["
	       << logTypeToString(type) << "] ";

	if (context.function && *context.function)
	{
		stream << context.function << " - ";
	}

	stream << message << '\n';
	stream.flush();

	if (type == QtFatalMsg)
	{
		abort();
	}
}

void setupLogging()
{
	const auto logDirPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
	qDebug() << "[Logging] AppDataLocation resolved to" << logDirPath;
	if (logDirPath.isEmpty())
	{
		qWarning() << "[Logging] AppDataLocation is empty; logging disabled.";
		return;
	}

	QDir logDir(logDirPath);
	if (!logDir.exists() && !logDir.mkpath("."))
	{
		qWarning() << "[Logging] Failed to create log directory" << logDirPath;
		return;
	}

	g_logFile.setFileName(logDir.filePath("viewer.log"));
	if (!g_logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
	{
		qWarning() << "[Logging] Could not open log file at" << g_logFile.fileName();
		return;
	}

	qInstallMessageHandler(messageHandler);
	qInfo() << "[Logging] Initialized. File:" << g_logFile.fileName();
}
} // namespace

	int main(int argc, char* argv[])
	{
		QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
		QApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
		QSurfaceFormat::setDefaultFormat(QVTKOpenGLNativeWidget::defaultFormat());
	QApplication application(argc, argv);
	QCoreApplication::setOrganizationName("Isis");
	QCoreApplication::setOrganizationDomain("isis.local");
	QCoreApplication::setApplicationName("Isis-DICOM-Viewer");
	setupLogging();

	QCommandLineParser parser;
	parser.setApplicationDescription(QStringLiteral("Isis DICOM Viewer"));
	parser.addHelpOption();
	QCommandLineOption headlessOption(
		QStringLiteral("headless-dicom-test"),
		QCoreApplication::translate("main", "Run a headless DICOM viewport test against <path>."),
		QCoreApplication::translate("main", "path"));
	parser.addOption(headlessOption);
	parser.process(application);

	if (parser.isSet(headlessOption))
	{
		const QString testPath = parser.value(headlessOption);
		QTextStream standardOut(stdout);
		QTextStream standardErr(stderr);

		auto fileOutput = vtkSmartPointer<vtkFileOutputWindow>::New();
		fileOutput->SetFileName("vtk-headless.log");
		fileOutput->SetAppend(0);
		vtkOutputWindow::SetInstance(fileOutput);

		standardOut << "[HeadlessTest] Starting viewport verification for "
		            << testPath << Qt::endl;
		qInfo() << "[HeadlessTest] Starting viewport verification for" << testPath;

		if (testPath.isEmpty())
		{
			standardErr << "[HeadlessTest] Provided path is empty." << Qt::endl;
			qCritical() << "[HeadlessTest] Provided path is empty.";
			return EXIT_FAILURE;
		}

		QDir directory(testPath);
		if (!directory.exists())
		{
			standardErr << "[HeadlessTest] Directory does not exist: "
			            << testPath << Qt::endl;
			qCritical() << "[HeadlessTest] Directory does not exist:" << testPath;
			return EXIT_FAILURE;
		}

		const QStringList filters = {QStringLiteral("*.dcm"), QStringLiteral("*.DCM")};
		const QFileInfoList entries =
			directory.entryInfoList(filters, QDir::Files | QDir::Readable, QDir::Name | QDir::IgnoreCase);
		if (entries.isEmpty())
		{
			standardErr << "[HeadlessTest] No DICOM files were found in "
			            << testPath << Qt::endl;
			qCritical() << "[HeadlessTest] No DICOM files were found in" << testPath;
			return EXIT_FAILURE;
		}

		const QFileInfo& referenceEntry = entries.front();
		auto headlessImage = std::make_unique<isis::core::Image>();
		headlessImage->setImagePath(referenceEntry.absoluteFilePath().toStdString());

		QString failureReason;
		const std::shared_ptr<isis::core::DicomVolume> volume =
			headlessImage->getDicomVolume(&failureReason);

		if (!volume || !volume->ImageData)
		{
			standardErr << "[HeadlessTest] Loaded volume is invalid. "
			            << failureReason << Qt::endl;
			qCritical() << "[HeadlessTest] Loaded volume is invalid." << failureReason;
			return EXIT_FAILURE;
		}

		int dims[3] = {0, 0, 0};
		volume->ImageData->GetDimensions(dims);
		if (dims[0] <= 0 || dims[1] <= 0 || dims[2] <= 0)
		{
			standardErr << "[HeadlessTest] Volume dimensions are invalid: "
			            << dims[0] << ' ' << dims[1] << ' ' << dims[2] << Qt::endl;
			qCritical() << "[HeadlessTest] Volume dimensions are invalid:" << dims[0] << dims[1] << dims[2];
			return EXIT_FAILURE;
		}

		const double* scalarRange = nullptr;
		if (vtkPointData* pointData = volume->ImageData->GetPointData())
		{
			if (auto* scalars = pointData->GetScalars())
			{
				static double range[2] = {0.0, 0.0};
				scalars->GetRange(range);
				scalarRange = range;
				standardOut << "[HeadlessTest] Scalar range: "
				            << range[0] << ' ' << range[1] << Qt::endl;
			}
			else
			{
				standardErr << "[HeadlessTest] Volume is missing scalar data." << Qt::endl;
			}
		}
		else
		{
			standardErr << "[HeadlessTest] Volume point data unavailable." << Qt::endl;
		}

		vtkNew<vtkGenericOpenGLRenderWindow> renderWindow;
		renderWindow->SetOffScreenRendering(1);

		auto widget = vtkSmartPointer<isis::gui::vtkWidgetDICOM>::New();
		widget->SetRenderWindow(renderWindow.GetPointer());
		widget->setVolume(volume);
		const int sliceMin = widget->GetSliceMin();
		const int sliceMax = widget->GetSliceMax();
		const int lowerSlice = std::min(sliceMin, sliceMax);
		const int upperSlice = std::max(sliceMin, sliceMax);
		const int safeSlice = std::clamp(0, lowerSlice, upperSlice);
		widget->SetSlice(safeSlice);

		widget->Render();
		renderWindow->Render();

		auto* const imageActor = widget->GetImageActor();
		if (!imageActor)
		{
			standardErr << "[HeadlessTest] vtkWidgetDICOM does not expose an image actor."
			            << Qt::endl;
			qCritical() << "[HeadlessTest] vtkWidgetDICOM does not expose an image actor.";
			return EXIT_FAILURE;
		}

		int extent[6] = {0, -1, 0, -1, 0, -1};
		imageActor->GetDisplayExtent(extent);
		const bool extentValid =
			extent[0] <= extent[1] &&
			extent[2] <= extent[3] &&
			extent[4] <= extent[5];

		if (!extentValid)
		{
			standardErr
				<< "[HeadlessTest] Image actor display extent is invalid: "
				<< extent[0] << ' ' << extent[1] << ' '
				<< extent[2] << ' ' << extent[3] << ' '
				<< extent[4] << ' ' << extent[5] << Qt::endl;
			qCritical()
				<< "[HeadlessTest] Image actor display extent is invalid:"
				<< extent[0] << extent[1]
				<< extent[2] << extent[3]
				<< extent[4] << extent[5];
			return EXIT_FAILURE;
		}

		const QString capturePath =
			QDir::current().absoluteFilePath(QStringLiteral("headless-render.png"));
		vtkNew<vtkWindowToImageFilter> capture;
		capture->SetInput(renderWindow);
		capture->SetScale(1);
		capture->SetInputBufferTypeToRGB();
		capture->ReadFrontBufferOff();
		capture->Update();

		vtkNew<vtkPNGWriter> writer;
		writer->SetFileName(capturePath.toUtf8().constData());
		writer->SetInputConnection(capture->GetOutputPort());
		writer->Write();

		standardOut
			<< "[HeadlessTest] Viewport verification succeeded. Extent: "
			<< extent[0] << ' ' << extent[1] << ' '
			<< extent[2] << ' ' << extent[3] << ' '
			<< extent[4] << ' ' << extent[5]
			<< " Dimensions: "
			<< dims[0] << ' ' << dims[1] << ' ' << dims[2];
		if (scalarRange)
		{
			standardOut << " ScalarRange: " << scalarRange[0] << ' ' << scalarRange[1];
		}
		standardOut << " Output: " << capturePath << Qt::endl;

		qInfo()
			<< "[HeadlessTest] Viewport verification succeeded. Extent:"
			<< extent[0] << extent[1]
			<< extent[2] << extent[3]
			<< extent[4] << extent[5]
			<< "Dimensions:" << dims[0] << dims[1] << dims[2];
		if (scalarRange)
		{
			qInfo() << "[HeadlessTest] Scalar range:" << scalarRange[0] << scalarRange[1];
		}
		qInfo() << "[HeadlessTest] Output saved to" << capturePath;

		return EXIT_SUCCESS;
	}

	vtkObject::GlobalWarningDisplayOff();
	application.setWindowIcon(QIcon(iconapp));
	isis::gui::GUIFrame guiFrame;
	isis::gui::GUI gui;
	guiFrame.setContent(&gui);
	guiFrame.showMaximized();
	return application.exec();
}

