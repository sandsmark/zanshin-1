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


#include "applicationcomponents.h"

#include <memory>

#include <QBoxLayout>
#include <QLabel>
#include <QMimeData>
#include <QVariant>
#include <QWidget>
#include <QWidgetAction>

#include <KLocalizedString>

#include "availablepagesview.h"
#include "availablesourcesview.h"
#include "editorview.h"
#include "pageview.h"
#include "pageviewerrorhandler.h"
#include "quickselectdialog.h"

using namespace Widgets;

ApplicationComponents::ApplicationComponents(QWidget *parent)
    : QObject(parent),
      m_pageView(Q_NULLPTR),
      m_parent(parent),
      m_availableSourcesView(Q_NULLPTR),
      m_availablePagesView(Q_NULLPTR),
      m_editorView(Q_NULLPTR),
      m_errorHandler(new PageViewErrorHandler)
{
    m_quickSelectDialogFactory = [] (QWidget *parent) {
        return QuickSelectDialogPtr(new QuickSelectDialog(parent));
    };

    auto moveItemAction = new QAction(this);
    moveItemAction->setObjectName(QStringLiteral("moveItemAction"));
    moveItemAction->setText(i18n("Move Item"));
    moveItemAction->setShortcut(Qt::Key_M);
    connect(moveItemAction, &QAction::triggered, this, &ApplicationComponents::onMoveItemsRequested);

    m_actions.insert(QStringLiteral("page_view_move"), moveItemAction);
}

ApplicationComponents::~ApplicationComponents()
{
    setModel({});
}

QHash<QString, QAction*> ApplicationComponents::globalActions() const
{
    auto actions = QHash<QString, QAction*>();
    actions.unite(availableSourcesView()->globalActions());
    actions.unite(availablePagesView()->globalActions());
    actions.unite(pageView()->globalActions());
    actions.unite(m_actions);

    return actions;
}

QObjectPtr ApplicationComponents::model() const
{
    return m_model;
}

AvailableSourcesView *ApplicationComponents::availableSourcesView() const
{
    if (!m_availableSourcesView) {
        auto availableSourcesView = new AvailableSourcesView(m_parent);
        if (m_model) {
            availableSourcesView->setModel(m_model->property("availableSources").value<QObject*>());
        }

        ApplicationComponents *self = const_cast<ApplicationComponents*>(this);
        self->m_availableSourcesView = availableSourcesView;
    }

    return m_availableSourcesView;
}

AvailablePagesView *ApplicationComponents::availablePagesView() const
{
    if (!m_availablePagesView) {
        auto availablePagesView = new AvailablePagesView(m_parent);
        if (m_model) {
            availablePagesView->setModel(m_model->property("availablePages").value<QObject*>());
            auto availableSources = m_model->property("availableSources").value<QObject*>();
            if (availableSources)
                availablePagesView->setProjectSourcesModel(availableSources->property("sourceListModel").value<QAbstractItemModel*>());
        }

        ApplicationComponents *self = const_cast<ApplicationComponents*>(this);
        self->m_availablePagesView = availablePagesView;

        connect(self->m_availablePagesView, &AvailablePagesView::currentPageChanged, self, &ApplicationComponents::onCurrentPageChanged);
    }

    return m_availablePagesView;
}

PageView *ApplicationComponents::pageView() const
{
    if (!m_pageView) {
        auto pageView = new PageView(m_parent);
        if (m_model) {
            pageView->setModel(m_model->property("currentPage").value<QObject*>());
            connect(m_model.data(), SIGNAL(currentPageChanged(QObject*)),
                    pageView, SLOT(setModel(QObject*)));
        }

        ApplicationComponents *self = const_cast<ApplicationComponents*>(this);
        self->m_pageView = pageView;
        self->m_errorHandler->setPageView(pageView);

        connect(self->m_pageView, &PageView::currentArtifactChanged, self, &ApplicationComponents::onCurrentArtifactChanged);
    }

    return m_pageView;
}

EditorView *ApplicationComponents::editorView() const
{
    if (!m_editorView) {
        auto editorView = new EditorView(m_parent);
        if (m_model) {
            editorView->setModel(m_model->property("editor").value<QObject*>());
        }

        auto self = const_cast<ApplicationComponents*>(this);
        self->m_editorView = editorView;
    }

    return m_editorView;
}

ApplicationComponents::QuickSelectDialogFactory ApplicationComponents::quickSelectDialogFactory() const
{
    return m_quickSelectDialogFactory;
}

QWidget *ApplicationComponents::parentWidget() const
{
    return m_parent;
}

void ApplicationComponents::setModel(const QObjectPtr &model)
{
    if (m_model == model)
        return;

    if (m_model) {
        if (m_pageView)
            disconnect(m_model.data(), 0, m_pageView, 0);
        m_model->setProperty("errorHandler", 0);
    }

    // Delay deletion of the old model until we're out of scope
    auto tmp = m_model;
    Q_UNUSED(tmp);

    m_model = model;

    if (m_model)
        m_model->setProperty("errorHandler", QVariant::fromValue(errorHandler()));

    if (m_availableSourcesView) {
        m_availableSourcesView->setModel(m_model ? m_model->property("availableSources").value<QObject*>()
                                                 : Q_NULLPTR);
    }

    if (m_availablePagesView) {
        m_availablePagesView->setModel(m_model ? m_model->property("availablePages").value<QObject*>()
                                               : Q_NULLPTR);
        m_availablePagesView->setProjectSourcesModel(m_model ? m_model->property("dataSourcesModel").value<QAbstractItemModel*>()
                                                             : Q_NULLPTR);
    }

    if (m_pageView) {
        m_pageView->setModel(m_model ? m_model->property("currentPage").value<QObject*>()
                                     : Q_NULLPTR);
        if (m_model) {
            connect(m_model.data(), SIGNAL(currentPageChanged(QObject*)),
                    m_pageView, SLOT(setModel(QObject*)));
        }
    }

    if (m_editorView) {
        m_editorView->setModel(m_model ? m_model->property("editor").value<QObject*>()
                                       : Q_NULLPTR);
    }
}

void ApplicationComponents::setQuickSelectDialogFactory(const QuickSelectDialogFactory &factory)
{
    m_quickSelectDialogFactory = factory;
}

void ApplicationComponents::onCurrentPageChanged(QObject *page)
{
    if (!m_model)
        return;

    m_model->setProperty("currentPage", QVariant::fromValue(page));

    QObject *editorModel = m_model->property("editor").value<QObject*>();
    if (editorModel)
        editorModel->setProperty("artifact", QVariant::fromValue(Domain::Artifact::Ptr()));
}

void ApplicationComponents::onCurrentArtifactChanged(const Domain::Artifact::Ptr &artifact)
{
    if (!m_model)
        return;

    auto editorModel = m_model->property("editor").value<QObject*>();
    if (editorModel)
        editorModel->setProperty("artifact", QVariant::fromValue(artifact));
}

void ApplicationComponents::onMoveItemsRequested()
{
    if (!m_model)
        return;

    if (m_pageView->selectedIndexes().size() == 0)
        return;

    auto pageListModel = m_availablePagesView->model()->property("pageListModel").value<QAbstractItemModel*>();
    Q_ASSERT(pageListModel);

    QuickSelectDialogInterface::Ptr dlg = m_quickSelectDialogFactory(m_pageView);
    dlg->setModel(pageListModel);
    if (dlg->exec() == QDialog::Accepted)
        moveItems(dlg->selectedIndex(), m_pageView->selectedIndexes());
}

Presentation::ErrorHandler *ApplicationComponents::errorHandler() const
{
    return m_errorHandler.data();
}

void ApplicationComponents::moveItems(const QModelIndex &destination, const QModelIndexList &droppedItems)
{
    Q_ASSERT(destination.isValid());
    Q_ASSERT(!droppedItems.isEmpty());

    auto centralListModel = droppedItems.first().model();
    auto availablePagesModel = const_cast<QAbstractItemModel*>(destination.model());

    // drag
    const auto data = std::unique_ptr<QMimeData>(centralListModel->mimeData(droppedItems));

    // drop
    availablePagesModel->dropMimeData(data.get(), Qt::MoveAction, -1, -1, destination);
}

