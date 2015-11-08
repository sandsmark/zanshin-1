/* This file is part of Zanshin

   Copyright 2014 Kevin Ottens <ervin@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License or (at your option) version 3 or any later version
   accepted by the membership of KDE e.V. (or its successor approved
   by the membership of KDE e.V.), which shall act as a proxy
   defined in Section 14 of version 3 of the license.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
   USA.
*/


#include "availablesourcesview.h"

#include <QAction>
#include <QHeaderView>
#include <QSortFilterProxyModel>
#include <QToolBar>
#include <QTreeView>
#include <QVBoxLayout>

#include <KLineEdit>

#include "presentation/metatypes.h"

#include "widgets/datasourcedelegate.h"

using namespace Widgets;

AvailableSourcesView::AvailableSourcesView(QWidget *parent)
    : QWidget(parent),
      m_model(Q_NULLPTR),
      m_sortProxy(new QSortFilterProxyModel(this)),
      m_sourcesView(new QTreeView(this))
{
    m_sortProxy->setDynamicSortFilter(true);
    m_sortProxy->sort(0);

    auto searchEdit = new KLineEdit(this);
    searchEdit->setObjectName("searchEdit");
    searchEdit->setClearButtonShown(true);
    searchEdit->setClickMessage(tr("Search..."));
    connect(searchEdit, SIGNAL(textChanged(QString)),
            this, SLOT(onSearchTextChanged(QString)));

    m_sourcesView->setObjectName("sourcesView");
    m_sourcesView->header()->hide();
    m_sourcesView->setModel(m_sortProxy);

    auto delegate = new DataSourceDelegate(m_sourcesView);
    connect(delegate, SIGNAL(actionTriggered(Domain::DataSource::Ptr,int)),
            this, SLOT(onActionTriggered(Domain::DataSource::Ptr,int)));
    m_sourcesView->setItemDelegate(delegate);

    auto actionBar = new QToolBar(this);
    actionBar->setObjectName("actionBar");
    actionBar->setIconSize(QSize(16, 16));

    auto addAction = new QAction(this);
    addAction->setObjectName("defaultAction");
    addAction->setText(tr("Use as default source"));
    addAction->setIcon(QIcon::fromTheme("folder-favorites"));
    connect(addAction, SIGNAL(triggered()), this, SLOT(onDefaultTriggered()));
    actionBar->addAction(addAction);

    auto layout = new QVBoxLayout;
    layout->addWidget(searchEdit);
    layout->addWidget(m_sourcesView);

    auto actionBarLayout = new QHBoxLayout;
    actionBarLayout->setAlignment(Qt::AlignRight);
    actionBarLayout->addWidget(actionBar);
    layout->addLayout(actionBarLayout);
    setLayout(layout);
}

QObject *AvailableSourcesView::model() const
{
    return m_model;
}

void AvailableSourcesView::setModel(QObject *model)
{
    if (model == m_model)
        return;

    m_sortProxy->setSourceModel(Q_NULLPTR);

    m_model = model;

    setSourceModel("sourceListModel");
}

void AvailableSourcesView::onDefaultTriggered()
{
    const auto currentIndex = m_sourcesView->currentIndex();
    const auto index = m_sortProxy->mapToSource(currentIndex);
    if (index.isValid())
        QMetaObject::invokeMethod(m_model, "setDefaultItem",
                                  Q_ARG(QModelIndex, index));
}

void AvailableSourcesView::onActionTriggered(const Domain::DataSource::Ptr &source, int action)
{
    switch (action) {
    case DataSourceDelegate::AddToList:
        QMetaObject::invokeMethod(m_model, "listSource",
                                  Q_ARG(Domain::DataSource::Ptr, source));
        break;
    case DataSourceDelegate::RemoveFromList:
        QMetaObject::invokeMethod(m_model, "unlistSource",
                                  Q_ARG(Domain::DataSource::Ptr, source));
        break;
    case DataSourceDelegate::Bookmark:
        QMetaObject::invokeMethod(m_model, "bookmarkSource",
                                  Q_ARG(Domain::DataSource::Ptr, source));
        break;
    default:
        qFatal("Shouldn't happen");
        break;
    }
}

void AvailableSourcesView::setSourceModel(const QByteArray &propertyName)
{
    QVariant modelProperty = m_model->property(propertyName);
    if (modelProperty.canConvert<QAbstractItemModel*>())
        m_sortProxy->setSourceModel(modelProperty.value<QAbstractItemModel*>());
}

void AvailableSourcesView::onSearchTextChanged(const QString &text)
{
    if (text.size() <= 2) {
        m_model->setProperty("searchTerm", QString());
        setSourceModel("sourceListModel");
    } else {
        m_model->setProperty("searchTerm", text);
        setSourceModel("searchListModel");
    }
}
