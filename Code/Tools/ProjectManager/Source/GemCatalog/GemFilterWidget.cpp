/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <GemCatalog/GemFilterWidget.h>
#include <QButtonGroup>
#include <QCheckBox>
#include <QLabel>
#include <QMap>
#include <QLineEdit>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>

namespace O3DE::ProjectManager
{
    FilterCategoryWidget::FilterCategoryWidget(const QString& header,
        const QVector<QString>& elementNames,
        const QVector<int>& elementCounts,
        bool showAllLessButton,
        int defaultShowCount,
        QWidget* parent)
        : QWidget(parent)
        , m_defaultShowCount(defaultShowCount)
    {
        AZ_Assert(elementNames.size() == elementCounts.size(), "Number of element names needs to match the counts.");

        QVBoxLayout* vLayout = new QVBoxLayout();
        setLayout(vLayout);

        // Collapse button
        QHBoxLayout* collapseLayout = new QHBoxLayout();
        m_collapseButton = new QPushButton();
        m_collapseButton->setCheckable(true);
        m_collapseButton->setFlat(true);
        m_collapseButton->setFocusPolicy(Qt::NoFocus);
        m_collapseButton->setFixedWidth(s_collapseButtonSize);
        m_collapseButton->setStyleSheet("border: 0px; border-radius: 0px;");
        connect(m_collapseButton, &QPushButton::clicked, this, [=]()
            {
                UpdateCollapseState();
            });
        collapseLayout->addWidget(m_collapseButton);

        // Category title
        QLabel* headerLabel = new QLabel(header);
        headerLabel->setStyleSheet("font-size: 11pt;");
        collapseLayout->addWidget(headerLabel);
        vLayout->addLayout(collapseLayout);

        vLayout->addSpacing(5);

        // Everything in the main widget will be collapsed/uncollapsed
        {
            m_mainWidget = new QWidget();
            vLayout->addWidget(m_mainWidget);

            QVBoxLayout* mainLayout = new QVBoxLayout();
            mainLayout->setMargin(0);
            mainLayout->setAlignment(Qt::AlignTop);
            m_mainWidget->setLayout(mainLayout);

            // Elements
            m_buttonGroup = new QButtonGroup();
            m_buttonGroup->setExclusive(false);
            for (int i = 0; i < elementNames.size(); ++i)
            {
                QWidget* elementWidget = new QWidget();
                QHBoxLayout* elementLayout = new QHBoxLayout();
                elementLayout->setMargin(0);
                elementWidget->setLayout(elementLayout);

                QCheckBox* checkbox = new QCheckBox(elementNames[i]);
                checkbox->setStyleSheet("font-size: 11pt;");
                m_buttonGroup->addButton(checkbox);
                elementLayout->addWidget(checkbox);

                elementLayout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding));

                QLabel* countLabel = new QLabel(QString::number(elementCounts[i]));
                countLabel->setStyleSheet("font-size: 11pt; background-color: #333333; border-radius: 3px; color: #94D2FF;");
                elementLayout->addWidget(countLabel);

                m_elementWidgets.push_back(elementWidget);
                mainLayout->addWidget(elementWidget);
            }

            // See more / less
            if (showAllLessButton)
            {
                m_seeAllLessLabel = new LinkLabel();
                connect(m_seeAllLessLabel, &LinkLabel::clicked, this, [=]()
                    {
                        m_seeAll = !m_seeAll;
                        UpdateSeeMoreLess();
                    });
                mainLayout->addWidget(m_seeAllLessLabel);
            }
            else
            {
                mainLayout->addSpacing(5);
            }
        }

        // Separating line
        QFrame* hLine = new QFrame();
        hLine->setFrameShape(QFrame::HLine);
        hLine->setStyleSheet("color: #666666;");
        vLayout->addWidget(hLine);

        UpdateCollapseState();
        UpdateSeeMoreLess();
    }

    void FilterCategoryWidget::UpdateCollapseState()
    {
        if (m_collapseButton->isChecked())
        {
            m_collapseButton->setIcon(QIcon(":/ArrowDownLine.svg"));
            m_mainWidget->hide();
        }
        else
        {
            m_collapseButton->setIcon(QIcon(":/ArrowUpLine.svg"));
            m_mainWidget->show();
        }
    }

    void FilterCategoryWidget::UpdateSeeMoreLess()
    {
        if (!m_seeAllLessLabel)
        {
            return;
        }

        if (m_elementWidgets.isEmpty())
        {
            m_seeAllLessLabel->hide();
            return;
        }
        else
        {
            m_seeAllLessLabel->show();
        }

        if (!m_seeAll)
        {
            m_seeAllLessLabel->setText("See all");
        }
        else
        {
            m_seeAllLessLabel->setText("See less");
        }

        int showCount = m_seeAll ? m_elementWidgets.size() : m_defaultShowCount;
        showCount = AZ::GetMin(showCount, m_elementWidgets.size());
        for (int i = 0; i < showCount; ++i)
        {
            m_elementWidgets[i]->show();
        }
        for (int i = showCount; i < m_elementWidgets.size(); ++i)
        {
            m_elementWidgets[i]->hide();
        }
    }

    QButtonGroup* FilterCategoryWidget::GetButtonGroup()
    {
        return m_buttonGroup;
    }

    GemFilterWidget::GemFilterWidget(GemSortFilterProxyModel* filterProxyModel, QWidget* parent)
        : QScrollArea(parent)
        , m_filterProxyModel(filterProxyModel)
    {
        m_gemModel = m_filterProxyModel->GetSourceModel();

        setWidgetResizable(true);
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

        QWidget* mainWidget = new QWidget();
        setWidget(mainWidget);

        m_mainLayout = new QVBoxLayout();
        m_mainLayout->setAlignment(Qt::AlignTop);
        mainWidget->setLayout(m_mainLayout);

        QLabel* filterByLabel = new QLabel("Filter by");
        filterByLabel->setStyleSheet("font-size: 15pt;");
        m_mainLayout->addWidget(filterByLabel);

        AddGemOriginFilter();
        AddTypeFilter();
        AddPlatformFilter();
        AddFeatureFilter();
    }

    void GemFilterWidget::AddGemOriginFilter()
    {
        QVector<QString> elementNames;
        QVector<int> elementCounts;
        const int numGems = m_gemModel->rowCount();
        for (int originIndex = 0; originIndex < GemInfo::NumGemOrigins; ++originIndex)
        {
            const GemInfo::GemOrigin gemOriginToBeCounted = static_cast<GemInfo::GemOrigin>(1 << originIndex);

            int gemOriginCount = 0;
            for (int gemIndex = 0; gemIndex < numGems; ++gemIndex)
            {
                const GemInfo::GemOrigin gemOrigin = m_gemModel->GetGemOrigin(m_gemModel->index(gemIndex, 0));

                // Is the gem of the given origin?
                if (gemOriginToBeCounted == gemOrigin)
                {
                    gemOriginCount++;
                }
            }

            elementNames.push_back(GemInfo::GetGemOriginString(gemOriginToBeCounted));
            elementCounts.push_back(gemOriginCount);
        }

        FilterCategoryWidget* filterWidget = new FilterCategoryWidget("Provider", elementNames, elementCounts, /*showAllLessButton=*/false);
        m_mainLayout->addWidget(filterWidget);

        const QList<QAbstractButton*> buttons = filterWidget->GetButtonGroup()->buttons();
        for (int i = 0; i < buttons.size(); ++i)
        {
            const GemInfo::GemOrigin gemOrigin = static_cast<GemInfo::GemOrigin>(1 << i);
            QAbstractButton* button = buttons[i];

            connect(button, &QAbstractButton::toggled, this, [=](bool checked)
                {
                    GemInfo::GemOrigins gemOrigins = m_filterProxyModel->GetGemOrigins();
                    if (checked)
                    {
                        gemOrigins |= gemOrigin;
                    }
                    else
                    {
                        gemOrigins &= ~gemOrigin;
                    }
                    m_filterProxyModel->SetGemOrigins(gemOrigins);
                });
        }
    }

    void GemFilterWidget::AddTypeFilter()
    {
        QVector<QString> elementNames;
        QVector<int> elementCounts;
        const int numGems = m_gemModel->rowCount();
        for (int typeIndex = 0; typeIndex < GemInfo::NumTypes; ++typeIndex)
        {
            const GemInfo::Type type = static_cast<GemInfo::Type>(1 << typeIndex);

            int typeGemCount = 0;
            for (int gemIndex = 0; gemIndex < numGems; ++gemIndex)
            {
                const GemInfo::Types types = m_gemModel->GetTypes(m_gemModel->index(gemIndex, 0));

                // Is type (Asset, Code, Tool) part of the gem?
                if (types & type)
                {
                    typeGemCount++;
                }
            }

            elementNames.push_back(GemInfo::GetTypeString(type));
            elementCounts.push_back(typeGemCount);
        }

        FilterCategoryWidget* filterWidget = new FilterCategoryWidget("Type", elementNames, elementCounts, /*showAllLessButton=*/false);
        m_mainLayout->addWidget(filterWidget);

        const QList<QAbstractButton*> buttons = filterWidget->GetButtonGroup()->buttons();
        for (int i = 0; i < buttons.size(); ++i)
        {
            const GemInfo::Type type = static_cast<GemInfo::Type>(1 << i);
            QAbstractButton* button = buttons[i];

            connect(button, &QAbstractButton::toggled, this, [=](bool checked)
                {
                    GemInfo::Types types = m_filterProxyModel->GetTypes();
                    if (checked)
                    {
                        types |= type;
                    }
                    else
                    {
                        types &= ~type;
                    }
                    m_filterProxyModel->SetTypes(types);
                });
        }
    }

    void GemFilterWidget::AddPlatformFilter()
    {
        QVector<QString> elementNames;
        QVector<int> elementCounts;
        const int numGems = m_gemModel->rowCount();
        for (int platformIndex = 0; platformIndex < GemInfo::NumPlatforms; ++platformIndex)
        {
            const GemInfo::Platform platform = static_cast<GemInfo::Platform>(1 << platformIndex);

            int platformGemCount = 0;
            for (int gemIndex = 0; gemIndex < numGems; ++gemIndex)
            {
                const GemInfo::Platforms platforms = m_gemModel->GetPlatforms(m_gemModel->index(gemIndex, 0));

                // Is platform supported?
                if (platforms & platform)
                {
                    platformGemCount++;
                }
            }

            elementNames.push_back(GemInfo::GetPlatformString(platform));
            elementCounts.push_back(platformGemCount);
        }

        FilterCategoryWidget* filterWidget = new FilterCategoryWidget("Supported Platforms", elementNames, elementCounts, /*showAllLessButton=*/false);
        m_mainLayout->addWidget(filterWidget);

        const QList<QAbstractButton*> buttons = filterWidget->GetButtonGroup()->buttons();
        for (int i = 0; i < buttons.size(); ++i)
        {
            const GemInfo::Platform platform = static_cast<GemInfo::Platform>(1 << i);
            QAbstractButton* button = buttons[i];

            connect(button, &QAbstractButton::toggled, this, [=](bool checked)
                {
                    GemInfo::Platforms platforms = m_filterProxyModel->GetPlatforms();
                    if (checked)
                    {
                        platforms |= platform;
                    }
                    else
                    {
                        platforms &= ~platform;
                    }
                    m_filterProxyModel->SetPlatforms(platforms);
                });
        }
    }

    void GemFilterWidget::AddFeatureFilter()
    {
        // Alphabetically sorted, unique features and their number of occurrences in the gem database.
        QMap<QString, int> uniqueFeatureCounts;
        const int numGems = m_gemModel->rowCount();
        for (int gemIndex = 0; gemIndex < numGems; ++gemIndex)
        {
            const QStringList features = m_gemModel->GetFeatures(m_gemModel->index(gemIndex, 0));
            for (const QString& feature : features)
            {
                if (!uniqueFeatureCounts.contains(feature))
                {
                    uniqueFeatureCounts.insert(feature, 1);
                }
                else
                {
                    int& featureeCount = uniqueFeatureCounts[feature];
                    featureeCount++;
                }
            }
        }

        QVector<QString> elementNames;
        QVector<int> elementCounts;
        for (auto iterator = uniqueFeatureCounts.begin(); iterator != uniqueFeatureCounts.end(); iterator++)
        {
            elementNames.push_back(iterator.key());
            elementCounts.push_back(iterator.value());
        }

        FilterCategoryWidget* filterWidget = new FilterCategoryWidget("Features", elementNames, elementCounts,
            /*showAllLessButton=*/true, /*defaultShowCount=*/5);
        m_mainLayout->addWidget(filterWidget);

        const QList<QAbstractButton*> buttons = filterWidget->GetButtonGroup()->buttons();
        for (int i = 0; i < buttons.size(); ++i)
        {
            const QString& feature = elementNames[i];
            QAbstractButton* button = buttons[i];

            connect(button, &QAbstractButton::toggled, this, [=](bool checked)
                {
                    QSet<QString> features = m_filterProxyModel->GetFeatures();
                    if (checked)
                    {
                        features.insert(feature);
                    }
                    else
                    {
                        features.remove(feature);
                    }
                    m_filterProxyModel->SetFeatures(features);
                });
        }
    }
} // namespace O3DE::ProjectManager