/*
 * ------------------------------------------------------------------------------------
 *  File: vtkwidget3d.h
 *  Project: Isis DICOM Viewer (derived from Asclepios DICOM Viewer)
 *
 *  Original code:
 *      Copyright (c) 2020  Gavrilovici Eduard
 *
 *  Fork, refactoring, and ongoing development:
 *      Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Declares the vtkWidget3D class encapsulating render window setup for 3D views.
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

#pragma once

#include <memory>
#include <QString>
#include <string>
#include <unordered_map>

#include <vtkVolume.h>
#include <vtkBoxWidget2.h>
#include <vtkSmartVolumeMapper.h>
#include "transferfunction.h"
#include "vtkboxwidget3dcallback.h"
#include "vtkwidget3dinteractorstyle.h"
#include "vtkwidgetbase.h"
#include "vtkdicomvolumeloader.h"

namespace isis::gui
{
	class Widget3D;

	class vtkWidget3D final : public vtkWidgetBase
	{
	public:
		friend class Widget3D;
		vtkWidget3D() { initWidget(); };
		~vtkWidget3D() = default;
		//getters
		[[nodiscard]] vtkSmartVolumeMapper* getvtkWidget3DSmartVolumeMapper() const { return m_mapper; }
		[[nodiscard]] bool hasRenderableVolume() const { return m_volumeData && m_volumeData->ImageData; }
		[[nodiscard]] QString lastVolumeError() const { return m_lastVolumeError; }
		[[nodiscard]] QString lastVolumeWarning() const { return m_lastVolumeWarning; }

		//setters
		void setInteractor(vtkRenderWindowInteractor* t_interactor) override { m_interactor = t_interactor; }
        bool composeAndRenderVolume(const std::shared_ptr<VtkDicomVolume>& volume,
                const QString& initialFailureReason = QString());
		void setFilter(const QString& t_filePath);
		void render() override;
		void activateBoxWidget(const bool& t_flag);
		void updateFilter() const;

	private:
		std::unique_ptr<TransferFunction> m_transferFunction = {};
		vtkSmartPointer<vtkWidget3DInteractorStyle> m_interactorStyle = {};
		vtkSmartPointer<vtkSmartVolumeMapper> m_mapper;
		vtkSmartPointer<vtkRenderer> m_renderer;
		vtkSmartPointer<vtkVolume> m_volume;
		vtkSmartPointer<vtkBoxWidget2> m_boxWidget;
		vtkSmartPointer<vtkBoxWidget3DCallback> m_boxWidgetCallback = {};
        vtkRenderWindowInteractor* m_interactor = {};
        std::shared_ptr<VtkDicomVolume> m_volumeData = {};
        VtkDicomVolumeLoader m_volumeLoader = {};
		std::unordered_map<std::string, TransferFunction::Preset> m_seriesPresetCache = {};

		void initWidget();
		void initBoxWidget();
		void initBoxWidgetCallback();
		void initInteractorStyle();
        void setVolumeMapperBlend() const;
        std::shared_ptr<VtkDicomVolume> acquireVolume(QString* failureReason = nullptr) const;
		[[nodiscard]] std::tuple<int, int> getWindowLevel(const std::shared_ptr<VtkDicomVolume>& volume) const;
		void applyWindowLevelToTransferFunction(int window, int level);
		void syncBoxWidgetWithVolume();
		std::string seriesCacheKey() const;
		TransferFunction::Preset cachedPresetForSeries() const;
		void rememberPresetForSeries(TransferFunction::Preset preset);
		QString m_lastVolumeError = {};
		QString m_lastVolumeWarning = {};

	protected:
		void onRenderWindowAssigned(const vtkSmartPointer<vtkRenderWindow>& window) override;
	};
}

