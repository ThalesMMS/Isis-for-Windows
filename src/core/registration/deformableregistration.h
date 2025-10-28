/*
 * ------------------------------------------------------------------------------------
 *  File: deformableregistration.h
 *  Project: Isis DICOM Viewer
 *
 *  Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Deformable (non-rigid) image registration using B-splines or vector fields
 *      with regularization control and deformation field visualization
 *
 *  License:
 *      Apache License 2.0
 * ------------------------------------------------------------------------------------
 */

#pragma once

#include <vtkSmartPointer.h>
#include <vtkImageData.h>
#include <vtkThinPlateSplineTransform.h>
#include <vtkPoints.h>
#include <vtkImageReslice.h>
#include <vector>

namespace isis::core::registration
{
    /**
     * @brief Control point pair for deformable registration
     */
    struct ControlPointPair
    {
        double sourcePoint[3];
        double targetPoint[3];
    };

    /**
     * @brief Deformable registration algorithm
     *
     * Implements non-rigid registration using thin-plate splines or B-splines
     * with control point-based deformation and regularization.
     */
    class DeformableRegistration
    {
    public:
        DeformableRegistration();
        ~DeformableRegistration() = default;

        /**
         * @brief Set fixed (reference) image
         * @param fixedImage Fixed image
         */
        void setFixedImage(vtkImageData* fixedImage);

        /**
         * @brief Set moving image to be registered
         * @param movingImage Moving image
         */
        void setMovingImage(vtkImageData* movingImage);

        /**
         * @brief Add control point pair
         * @param sourcePoint Source (moving) image point
         * @param targetPoint Target (fixed) image point
         */
        void addControlPointPair(const double sourcePoint[3], const double targetPoint[3]);

        /**
         * @brief Clear all control points
         */
        void clearControlPoints();

        /**
         * @brief Set regularization strength (prevents excessive deformation)
         * @param lambda Regularization parameter (0.0 = none, 1.0 = strong)
         */
        void setRegularization(double lambda);

        /**
         * @brief Execute deformable registration
         * @return Registered (warped) image
         */
        vtkSmartPointer<vtkImageData> execute();

        /**
         * @brief Get the deformation field
         * @return Vector field showing deformation at each point
         */
        vtkSmartPointer<vtkImageData> getDeformationField();

        /**
         * @brief Get the transform
         * @return Thin-plate spline transform
         */
        [[nodiscard]] vtkThinPlateSplineTransform* getTransform() const;

        /**
         * @brief Visualize deformation field
         * @return Magnitude image showing deformation strength
         */
        vtkSmartPointer<vtkImageData> visualizeDeformationMagnitude();

    private:
        vtkSmartPointer<vtkImageData> m_fixedImage;
        vtkSmartPointer<vtkImageData> m_movingImage;
        vtkSmartPointer<vtkThinPlateSplineTransform> m_transform;
        vtkSmartPointer<vtkImageData> m_registeredImage;
        vtkSmartPointer<vtkImageData> m_deformationField;
        std::vector<ControlPointPair> m_controlPoints;
        double m_regularization = 0.1;

        void buildTransform();
        void computeDeformationField();
    };

} // namespace isis::core::registration
