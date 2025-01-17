/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzQtComponents/AzQtComponentsAPI.h>

#if !defined(Q_MOC_RUN)
#include <QFileDialog>
#endif

namespace AzQtComponents
{
    class AZ_QT_COMPONENTS_API FileDialog
    {
    public:
        //! Helper method that extends QFileDialog::getSaveFileName to prevent the user from
        //! saving a filename with invalid characters (e.g. AP doesn't allow @ characters because they are used for aliases)
        static QString GetSaveFileName(QWidget* parent = nullptr, const QString& caption = QString(),
            const QString& dir = QString(), const QString& filter = QString(),
            QString* selectedFilter = nullptr, QFileDialog::Options options = QFileDialog::Options());
    };

} // namespace AzQtComponents
