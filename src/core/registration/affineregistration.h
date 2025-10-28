/*
 * ------------------------------------------------------------------------------------
 *  File: affineregistration.h
 *  Project: Isis DICOM Viewer
 *
 *  Copyright (c) 2025  Thales Matheus Mendonca Santos
 *
 *  Description:
 *      Affine image registration (rotation + translation + scaling + shearing)
 *      with 12 degrees of freedom
 *
 *  License:
 *      Apache License 2.0
 * ------------------------------------------------------------------------------------
 */

#pragma once

#include <vtkSmartPointer.h>
#include <vtkImageData.h>
#include <vtkTransform.h>
#include <vtkMatrix4x4.h>

namespace isis::core::registration
{
    /**
     * @brief Affine registration algorithm
     *
     * Implements 12-DOF affine registration with rotation, translation,
     * scaling, and shearing transformations.
     */
    class AffineRegistration
    {
    public:
        AffineRegistration();
        ~AffineRegistration() = default;

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
         * @brief Set maximum number of iterations
         * @param iterations Maximum iterations
         */
        void setMaxIterations(int iterations);

        /**
         * @brief Execute affine registration
         * @return Registered image
         */
        vtkSmartPointer<vtkImageData> execute();

        /**
         * @brief Get the transformation matrix
         * @return 4x4 transformation matrix
         */
        [[nodiscard]] vtkMatrix4x4* getTransformationMatrix() const;

        /**
         * @brief Export transformation matrix to file
         * @param filename Output file path
         * @return true if successful
         */
        bool exportTransformMatrix(const char* filename) const;

    private:
        vtkSmartPointer<vtkImageData> m_fixedImage;
        vtkSmartPointer<vtkImageData> m_movingImage;
        vtkSmartPointer<vtkTransform> m_transform;
        vtkSmartPointer<vtkMatrix4x4> m_matrix;
        vtkSmartPointer<vtkImageData> m_registeredImage;
        int m_maxIterations = 200;

        void initializeTransform();
        void optimizeAffineParameters();
    };

} // namespace isis::core::registration
