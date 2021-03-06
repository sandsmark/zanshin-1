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

#include <testlib/qtest_gui_zanshin.h>

#include <QAbstractItemModel>
#include <QAction>
#include <QHeaderView>
#include <QLayout>
#include <QLineEdit>
#include <QMessageBox>
#include <QStandardItemModel>
#include <QToolButton>
#include <QTreeView>
#include <runningtaskmodelinterface.h>

#include <KLocalizedString>
#include <KMessageWidget>

#include "domain/task.h"

#include "presentation/artifactfilterproxymodel.h"
#include "presentation/metatypes.h"
#include "presentation/querytreemodelbase.h"

#include "widgets/filterwidget.h"
#include "widgets/itemdelegate.h"
#include "widgets/pageview.h"

#include "messageboxstub.h"

class PageModelStub : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QAbstractItemModel* centralListModel READ centralListModel)
public:
    QAbstractItemModel *centralListModel()
    {
        return &itemModel;
    }

    QStandardItem *addStubItem(const QString &title, QStandardItem *parentItem = Q_NULLPTR)
    {
        QStandardItem *item = new QStandardItem;
        item->setData(title, Qt::DisplayRole);
        if (!parentItem)
            itemModel.appendRow(item);
        else
            parentItem->appendRow(item);

        taskNames << title;
        return item;
    }

    Domain::Task::Ptr addTaskItem(const QString &title, QStandardItem *parentItem = Q_NULLPTR)
    {
        auto item = addStubItem(title, parentItem);
        auto task = Domain::Task::Ptr::create();
        task->setTitle(title);
        item->setData(QVariant::fromValue<Domain::Artifact::Ptr>(task), Presentation::QueryTreeModelBase::ObjectRole);
        return task;
    }

    void addStubItems(const QStringList &list)
    {
        foreach (const QString &title, list) {
            addStubItem(title);
        }
    }

public slots:
    void addItem(const QString &name, const QModelIndex &parentIndex)
    {
        taskNames << name;
        parentIndices << parentIndex;
    }

    void removeItem(const QModelIndex &index)
    {
        removedIndices << index;
    }

    void promoteItem(const QModelIndex &index)
    {
        promotedIndices << index;
    }

public:
    QStringList taskNames;
    QList<QPersistentModelIndex> parentIndices;
    QList<QPersistentModelIndex> removedIndices;
    QList<QPersistentModelIndex> promotedIndices;
    QStandardItemModel itemModel;
};

class RunningTaskModelStub : public Presentation::RunningTaskModelInterface
{
    Q_OBJECT
public:
    Domain::Task::Ptr runningTask() const Q_DECL_OVERRIDE { return m_runningTask; }
    void setRunningTask(const Domain::Task::Ptr &task) Q_DECL_OVERRIDE { m_runningTask = task; }
    void taskDeleted(const Domain::Task::Ptr &task) Q_DECL_OVERRIDE { m_deletedTask = task; }
    void stopTask() Q_DECL_OVERRIDE {}
    void doneTask() Q_DECL_OVERRIDE {}
private:
    Domain::Task::Ptr m_runningTask;
    Domain::Task::Ptr m_deletedTask;
};

class PageViewTest : public QObject
{
    Q_OBJECT
private slots:
    void shouldHaveDefaultState()
    {
        Widgets::PageView page;
        QCOMPARE(page.contentsMargins(), QMargins(0, 0, 0, 0));
        QCOMPARE(page.layout()->contentsMargins(), QMargins(0, 0, 0, 3));

        auto messageWidget = page.findChild<KMessageWidget*>(QStringLiteral("messageWidget"));
        QVERIFY(messageWidget);
        QVERIFY(!messageWidget->isVisibleTo(&page));
        QVERIFY(!messageWidget->isCloseButtonVisible());
        QVERIFY(messageWidget->wordWrap());
        QVERIFY(messageWidget->text().isEmpty());
        QVERIFY(messageWidget->icon().isNull());
        QCOMPARE(messageWidget->messageType(), KMessageWidget::Error);
        QVERIFY(!messageWidget->isShowAnimationRunning());
        QVERIFY(!messageWidget->isHideAnimationRunning());

        auto centralView = page.findChild<QTreeView*>(QStringLiteral("centralView"));
        QVERIFY(centralView);
        QVERIFY(centralView->isVisibleTo(&page));
        QVERIFY(!centralView->header()->isVisibleTo(&page));
        QVERIFY(qobject_cast<Widgets::ItemDelegate*>(centralView->itemDelegate()));
        QVERIFY(centralView->alternatingRowColors());
        QCOMPARE(centralView->dragDropMode(), QTreeView::DragDrop);

        auto filter = page.findChild<Widgets::FilterWidget*>(QStringLiteral("filterWidget"));
        QVERIFY(filter);
        QVERIFY(!filter->isVisibleTo(&page));
        QVERIFY(filter->proxyModel());
        QCOMPARE(filter->proxyModel(), centralView->model());

        QLineEdit *quickAddEdit = page.findChild<QLineEdit*>(QStringLiteral("quickAddEdit"));
        QVERIFY(quickAddEdit);
        QVERIFY(quickAddEdit->isVisibleTo(&page));
        QVERIFY(quickAddEdit->text().isEmpty());
        QCOMPARE(quickAddEdit->placeholderText(), i18n("Type and press enter to add an item"));

        auto addAction = page.findChild<QAction*>(QStringLiteral("addItemAction"));
        QVERIFY(addAction);
        auto cancelAddAction = page.findChild<QAction*>(QStringLiteral("cancelAddItemAction"));
        QVERIFY(cancelAddAction);
        auto removeAction = page.findChild<QAction*>(QStringLiteral("removeItemAction"));
        QVERIFY(removeAction);
        auto promoteAction = page.findChild<QAction*>(QStringLiteral("promoteItemAction"));
        QVERIFY(promoteAction);
        auto filterAction = page.findChild<QAction*>(QStringLiteral("filterViewAction"));
        QVERIFY(filterAction);
        QVERIFY(filterAction->isCheckable());
        QVERIFY(!filterAction->isChecked());
        auto runTaskAction = page.findChild<QAction*>(QStringLiteral("runTaskAction"));
        QVERIFY(runTaskAction);
        QVERIFY(!runTaskAction->isEnabled());

        auto actions = page.globalActions();
        QCOMPARE(actions.value(QStringLiteral("page_view_add")), addAction);
        QCOMPARE(actions.value(QStringLiteral("page_view_remove")), removeAction);
        QCOMPARE(actions.value(QStringLiteral("page_view_promote")), promoteAction);
        QCOMPARE(actions.value(QStringLiteral("page_view_filter")), filterAction);
        QCOMPARE(actions.value(QStringLiteral("page_run_task")), runTaskAction);
    }

    void shouldDisplayListFromPageModel()
    {
        // GIVEN
        QStandardItemModel model;

        QObject stubPageModel;
        stubPageModel.setProperty("centralListModel", QVariant::fromValue(static_cast<QAbstractItemModel*>(&model)));

        Widgets::PageView page;
        auto centralView = page.findChild<QTreeView*>(QStringLiteral("centralView"));
        QVERIFY(centralView);
        auto proxyModel = qobject_cast<Presentation::ArtifactFilterProxyModel*>(centralView->model());
        QVERIFY(proxyModel);
        QVERIFY(!proxyModel->sourceModel());

        // WHEN
        page.setModel(&stubPageModel);

        // THEN
        QCOMPARE(page.model(), &stubPageModel);
        QVERIFY(page.isEnabled());
        QCOMPARE(proxyModel->sourceModel(), &model);
    }

    void shouldNotCrashWithNullModel()
    {
        // GIVEN
        QStandardItemModel model;
        QObject stubPageModel;
        stubPageModel.setProperty("centralListModel", QVariant::fromValue(static_cast<QAbstractItemModel*>(&model)));

        Widgets::PageView page;
        page.setModel(&stubPageModel);

        auto centralView = page.findChild<QTreeView*>(QStringLiteral("centralView"));
        QVERIFY(centralView);
        auto proxyModel = qobject_cast<Presentation::ArtifactFilterProxyModel*>(centralView->model());
        QVERIFY(proxyModel);
        QCOMPARE(proxyModel->sourceModel(), &model);

        // WHEN
        page.setModel(Q_NULLPTR);

        // THEN
        QVERIFY(!page.model());
        QVERIFY(!page.isEnabled());
        QVERIFY(!proxyModel->sourceModel());
    }

    void shouldManageFocusThroughActions()
    {
        // GIVEN
        Widgets::PageView page;
        auto centralView = page.findChild<QTreeView*>(QStringLiteral("centralView"));
        auto quickAddEdit = page.findChild<QLineEdit*>(QStringLiteral("quickAddEdit"));
        auto filter = page.findChild<Widgets::FilterWidget*>(QStringLiteral("filterWidget"));
        auto filterEdit = filter->findChild<QLineEdit*>();
        QVERIFY(filterEdit);
        page.show();
        QTest::qWaitForWindowShown(&page);

        centralView->setFocus();
        QVERIFY(centralView->hasFocus());
        QVERIFY(!quickAddEdit->hasFocus());
        QVERIFY(!filter->isVisibleTo(&page));
        QVERIFY(!filterEdit->hasFocus());

        auto addAction = page.findChild<QAction*>(QStringLiteral("addItemAction"));
        auto cancelAddAction = page.findChild<QAction*>(QStringLiteral("cancelAddItemAction"));
        auto filterAction = page.findChild<QAction*>(QStringLiteral("filterViewAction"));

        // WHEN
        addAction->trigger();

        // THEN
        QVERIFY(!centralView->hasFocus());
        QVERIFY(quickAddEdit->hasFocus());
        QVERIFY(!filter->isVisibleTo(&page));
        QVERIFY(!filterEdit->hasFocus());

        // WHEN
        cancelAddAction->trigger();

        // THEN
        QVERIFY(centralView->hasFocus());
        QVERIFY(!quickAddEdit->hasFocus());
        QVERIFY(!filter->isVisibleTo(&page));
        QVERIFY(!filterEdit->hasFocus());

        // WHEN
        filterAction->trigger();

        // THEN
        QVERIFY(!centralView->hasFocus());
        QVERIFY(!quickAddEdit->hasFocus());
        QVERIFY(filter->isVisibleTo(&page));
        QVERIFY(filterEdit->hasFocus());

        // WHEN
        cancelAddAction->trigger();

        // THEN
        QVERIFY(centralView->hasFocus());
        QVERIFY(!quickAddEdit->hasFocus());
        QVERIFY(filter->isVisibleTo(&page));
        QVERIFY(!filterEdit->hasFocus());
    }

    void shouldManageFilterVisibilityThroughAction()
    {
        // GIVEN
        Widgets::PageView page;
        auto centralView = page.findChild<QTreeView*>(QStringLiteral("centralView"));
        auto filter = page.findChild<Widgets::FilterWidget*>(QStringLiteral("filterWidget"));
        auto filterEdit = filter->findChild<QLineEdit*>();
        QVERIFY(filterEdit);
        page.show();
        QTest::qWaitForWindowShown(&page);

        centralView->setFocus();
        QVERIFY(centralView->hasFocus());
        QVERIFY(!filter->isVisibleTo(&page));
        QVERIFY(!filterEdit->hasFocus());

        auto filterAction = page.findChild<QAction*>(QStringLiteral("filterViewAction"));

        // WHEN
        filterAction->trigger();

        // THEN
        QVERIFY(!centralView->hasFocus());
        QVERIFY(filter->isVisibleTo(&page));
        QVERIFY(filterEdit->hasFocus());

        // WHEN
        filterEdit->setText("Foo");

        // THEN
        QCOMPARE(filterEdit->text(), QString("Foo"));

        // WHEN
        filterAction->trigger();

        // THEN
        QVERIFY(centralView->hasFocus());
        QVERIFY(!filter->isVisibleTo(&page));
        QVERIFY(!filterEdit->hasFocus());
        QVERIFY(filterEdit->text().isEmpty());
    }

    void shouldCreateTasksWithNoParentWhenHittingReturnWithoutSelectedIndex()
    {
        // GIVEN
        PageModelStub stubPageModel;
        Widgets::PageView page;
        page.setModel(&stubPageModel);
        auto quickAddEdit = page.findChild<QLineEdit*>(QStringLiteral("quickAddEdit"));

        // WHEN
        QTest::keyClick(quickAddEdit, Qt::Key_Return); // Does nothing (edit empty)
        QTest::keyClicks(quickAddEdit, QStringLiteral("Foo"));
        QTest::keyClick(quickAddEdit, Qt::Key_Return);
        QTest::keyClick(quickAddEdit, Qt::Key_Return); // Does nothing (edit empty)
        QTest::keyClicks(quickAddEdit, QStringLiteral("Bar"));
        QTest::keyClick(quickAddEdit, Qt::Key_Return);
        QTest::keyClick(quickAddEdit, Qt::Key_Return); // Does nothing (edit empty)

        // THEN
        QCOMPARE(stubPageModel.taskNames, QStringList() << QStringLiteral("Foo") << QStringLiteral("Bar"));
        QCOMPARE(stubPageModel.parentIndices.size(), 2);
        QCOMPARE(stubPageModel.parentIndices.first(), QPersistentModelIndex());
        QCOMPARE(stubPageModel.parentIndices.last(), QPersistentModelIndex());
    }

    void shouldCreateTasksWithNoParentWhenHittingReturnWithSeveralSelectedIndices()
    {
        // GIVEN
        PageModelStub stubPageModel;
        Q_ASSERT(stubPageModel.property("centralListModel").canConvert<QAbstractItemModel*>());
        stubPageModel.addStubItems(QStringList() << QStringLiteral("A") << QStringLiteral("B") << QStringLiteral("C"));
        QPersistentModelIndex index0 = stubPageModel.itemModel.index(0, 0);
        QPersistentModelIndex index1 = stubPageModel.itemModel.index(1, 0);

        Widgets::PageView page;
        page.setModel(&stubPageModel);

        auto centralView = page.findChild<QTreeView*>(QStringLiteral("centralView"));
        centralView->selectionModel()->select(index0, QItemSelectionModel::ClearAndSelect);
        centralView->selectionModel()->select(index1, QItemSelectionModel::Select);

        auto quickAddEdit = page.findChild<QLineEdit*>(QStringLiteral("quickAddEdit"));

        // WHEN
        QTest::keyClicks(quickAddEdit, QStringLiteral("Foo"));
        QTest::keyClick(quickAddEdit, Qt::Key_Return);

        // THEN
        QCOMPARE(stubPageModel.taskNames, QStringList() << QStringLiteral("A")
                                                        << QStringLiteral("B")
                                                        << QStringLiteral("C")
                                                        << QStringLiteral("Foo"));
        QCOMPARE(stubPageModel.parentIndices.size(), 1);
        QCOMPARE(stubPageModel.parentIndices.first(), QPersistentModelIndex());
    }

    void shouldCreateTasksWithParentWhenHittingReturnWithOneSelectedIndex()
    {
        // GIVEN
        PageModelStub stubPageModel;
        Q_ASSERT(stubPageModel.property("centralListModel").canConvert<QAbstractItemModel*>());
        stubPageModel.addStubItems(QStringList() << QStringLiteral("A") << QStringLiteral("B") << QStringLiteral("C"));
        QPersistentModelIndex index = stubPageModel.itemModel.index(1, 0);

        Widgets::PageView page;
        page.setModel(&stubPageModel);

        auto centralView = page.findChild<QTreeView*>(QStringLiteral("centralView"));
        centralView->selectionModel()->select(index, QItemSelectionModel::ClearAndSelect);

        auto quickAddEdit = page.findChild<QLineEdit*>(QStringLiteral("quickAddEdit"));

        // WHEN
        QTest::keyClicks(quickAddEdit, QStringLiteral("Foo"));
        QTest::keyClick(quickAddEdit, Qt::Key_Return);

        // THEN
        QCOMPARE(stubPageModel.taskNames, QStringList() << QStringLiteral("A")
                                                        << QStringLiteral("B")
                                                        << QStringLiteral("C")
                                                        << QStringLiteral("Foo"));
        QCOMPARE(stubPageModel.parentIndices.size(), 1);
        QCOMPARE(stubPageModel.parentIndices.first(), index);
    }

    void shouldDeleteItemWhenHittingTheDeleteKey()
    {
        // GIVEN
        PageModelStub stubPageModel;
        Q_ASSERT(stubPageModel.property("centralListModel").canConvert<QAbstractItemModel*>());
        stubPageModel.addStubItems(QStringList() << QStringLiteral("A") << QStringLiteral("B") << QStringLiteral("C"));
        QPersistentModelIndex index = stubPageModel.itemModel.index(1, 0);

        Widgets::PageView page;
        page.setModel(&stubPageModel);

        QTreeView *centralView = page.findChild<QTreeView*>(QStringLiteral("centralView"));
        centralView->selectionModel()->setCurrentIndex(index, QItemSelectionModel::ClearAndSelect);
        centralView->setFocus();

        // Needed for shortcuts to work
        page.show();
        QTest::qWaitForWindowShown(&page);
        QTest::qWait(100);

        // WHEN
        QTest::keyPress(centralView, Qt::Key_Delete);

        // THEN
        QCOMPARE(stubPageModel.removedIndices.size(), 1);
        QCOMPARE(stubPageModel.removedIndices.first(), index);
    }

    void shouldNoteTryToDeleteIfThereIsNoSelection()
    {
        // GIVEN
        PageModelStub stubPageModel;
        Q_ASSERT(stubPageModel.property("centralListModel").canConvert<QAbstractItemModel*>());
        stubPageModel.addStubItems(QStringList() << QStringLiteral("A") << QStringLiteral("B") << QStringLiteral("C"));

        Widgets::PageView page;
        page.setModel(&stubPageModel);

        QTreeView *centralView = page.findChild<QTreeView*>(QStringLiteral("centralView"));
        centralView->clearSelection();
        page.findChild<QLineEdit*>(QStringLiteral("quickAddEdit"))->setFocus();

        // Needed for shortcuts to work
        page.show();
        QTest::qWaitForWindowShown(&page);
        QTest::qWait(100);

        // WHEN
        QTest::keyPress(centralView, Qt::Key_Delete);

        // THEN
        QVERIFY(stubPageModel.removedIndices.isEmpty());
    }

    void shouldDisplayNotificationWhenHittingTheDeleteKeyOnAnItemWithChildren()
    {
        // GIVEN
        PageModelStub stubPageModel;
        Q_ASSERT(stubPageModel.property("centralListModel").canConvert<QAbstractItemModel*>());
        stubPageModel.addStubItems(QStringList() << QStringLiteral("A") << QStringLiteral("B"));
        QStandardItem *parentIndex = stubPageModel.itemModel.item(1, 0);
        stubPageModel.addStubItem(QStringLiteral("C"), parentIndex);
        QPersistentModelIndex index = stubPageModel.itemModel.index(1, 0);

        Widgets::PageView page;
        page.setModel(&stubPageModel);
        auto msgbox = MessageBoxStub::Ptr::create();
        page.setMessageBoxInterface(msgbox);

        QTreeView *centralView = page.findChild<QTreeView*>(QStringLiteral("centralView"));
        centralView->selectionModel()->setCurrentIndex(index, QItemSelectionModel::ClearAndSelect);
        QVERIFY(centralView->selectionModel()->currentIndex().isValid());
        centralView->setFocus();

        // Needed for shortcuts to work
        page.show();
        QTest::qWaitForWindowShown(&page);
        QTest::qWait(100);

        // WHEN
        QTest::keyPress(centralView, Qt::Key_Delete);

        // THEN
        QVERIFY(msgbox->called());
        QCOMPARE(stubPageModel.removedIndices.size(), 1);
        QCOMPARE(stubPageModel.removedIndices.first(), index);
    }

    void shouldDeleteItemsWhenHittingTheDeleteKey()
    {
        // GIVEN
        PageModelStub stubPageModel;
        Q_ASSERT(stubPageModel.property("centralListModel").canConvert<QAbstractItemModel*>());
        stubPageModel.addStubItems(QStringList() << QStringLiteral("A") << QStringLiteral("B") << QStringLiteral("C"));
        QPersistentModelIndex index = stubPageModel.itemModel.index(1, 0);
        QPersistentModelIndex index2 = stubPageModel.itemModel.index(2, 0);

        Widgets::PageView page;
        page.setModel(&stubPageModel);
        auto msgbox = MessageBoxStub::Ptr::create();
        page.setMessageBoxInterface(msgbox);

        QTreeView *centralView = page.findChild<QTreeView*>(QStringLiteral("centralView"));
        centralView->selectionModel()->setCurrentIndex(index, QItemSelectionModel::ClearAndSelect);
        centralView->selectionModel()->setCurrentIndex(index2, QItemSelectionModel::Select);
        centralView->setFocus();

        // Needed for shortcuts to work
        page.show();
        QTest::qWaitForWindowShown(&page);
        QTest::qWait(100);

        // WHEN
        QTest::keyPress(centralView, Qt::Key_Delete);

        // THEN
        QVERIFY(msgbox->called());
        QCOMPARE(stubPageModel.removedIndices.size(), 2);
        QCOMPARE(stubPageModel.removedIndices.first(), index);
        QCOMPARE(stubPageModel.removedIndices.at(1), index2);
    }

    void shouldPromoteItem()
    {
        // GIVEN
        PageModelStub stubPageModel;
        Q_ASSERT(stubPageModel.property("centralListModel").canConvert<QAbstractItemModel*>());
        stubPageModel.addStubItems(QStringList() << QStringLiteral("A") << QStringLiteral("B") << QStringLiteral("C"));
        QPersistentModelIndex index = stubPageModel.itemModel.index(1, 0);

        Widgets::PageView page;
        page.setModel(&stubPageModel);

        auto promoteAction = page.findChild<QAction*>(QStringLiteral("promoteItemAction"));
        QVERIFY(promoteAction);

        QTreeView *centralView = page.findChild<QTreeView*>(QStringLiteral("centralView"));
        centralView->selectionModel()->setCurrentIndex(index, QItemSelectionModel::ClearAndSelect);
        centralView->setFocus();

        // WHEN
        promoteAction->trigger();

        // THEN
        QCOMPARE(stubPageModel.promotedIndices.size(), 1);
        QCOMPARE(stubPageModel.promotedIndices.first(), index);
    }

    void shouldNotTryToPromoteItemIfThereIsNoSelection()
    {
        // GIVEN
        PageModelStub stubPageModel;
        Q_ASSERT(stubPageModel.property("centralListModel").canConvert<QAbstractItemModel*>());
        stubPageModel.addStubItems(QStringList() << QStringLiteral("A") << QStringLiteral("B") << QStringLiteral("C"));

        Widgets::PageView page;
        page.setModel(&stubPageModel);

        auto promoteAction = page.findChild<QAction*>(QStringLiteral("promoteItemAction"));
        QVERIFY(promoteAction);

        QTreeView *centralView = page.findChild<QTreeView*>(QStringLiteral("centralView"));
        centralView->selectionModel()->clear();
        centralView->setFocus();

        // WHEN
        promoteAction->trigger();

        // THEN
        QVERIFY(stubPageModel.promotedIndices.isEmpty());
    }

    void shouldClearCentralViewSelectionOnEscape()
    {
        // GIVEN
        PageModelStub stubPageModel;
        Q_ASSERT(stubPageModel.property("centralListModel").canConvert<QAbstractItemModel*>());
        stubPageModel.addStubItems(QStringList() << QStringLiteral("A") << QStringLiteral("B") << QStringLiteral("C"));
        QPersistentModelIndex index = stubPageModel.itemModel.index(1, 0);

        Widgets::PageView page;
        page.setModel(&stubPageModel);

        auto centralView = page.findChild<QTreeView*>(QStringLiteral("centralView"));
        centralView->selectionModel()->select(index, QItemSelectionModel::ClearAndSelect);
        QVERIFY(!centralView->selectionModel()->selectedIndexes().isEmpty());

        // WHEN
        QTest::keyClick(centralView, Qt::Key_Escape);

        // THEN
        QVERIFY(centralView->selectionModel()->selectedIndexes().isEmpty());
    }

    void shouldReturnSelectedIndexes()
    {
        // GIVEN
        PageModelStub stubPageModel;
        Q_ASSERT(stubPageModel.property("centralListModel").canConvert<QAbstractItemModel*>());
        stubPageModel.addStubItems(QStringList() << QStringLiteral("A") << QStringLiteral("B") << QStringLiteral("C"));
        auto index = stubPageModel.itemModel.index(1, 0);
        auto index2 = stubPageModel.itemModel.index(2, 0);

        Widgets::PageView page;
        page.setModel(&stubPageModel);

        auto centralView = page.findChild<QTreeView*>(QStringLiteral("centralView"));
        auto filterWidget = page.findChild<Widgets::FilterWidget*>(QStringLiteral("filterWidget"));

        auto displayedModel = filterWidget->proxyModel();
        auto displayedIndex = displayedModel->index(1, 0);
        auto displayedIndex2 = displayedModel->index(2, 0);

        // WHEN
        centralView->selectionModel()->setCurrentIndex(displayedIndex, QItemSelectionModel::ClearAndSelect);
        centralView->selectionModel()->setCurrentIndex(displayedIndex2, QItemSelectionModel::Select);

        // THEN
        auto selectedIndexes = page.selectedIndexes();
        QCOMPARE(selectedIndexes.size(), 2);

        QCOMPARE(selectedIndexes.at(0), index);
        QCOMPARE(selectedIndexes.at(1), index2);

        QCOMPARE(selectedIndexes.at(0).model(), index.model());
        QCOMPARE(selectedIndexes.at(1).model(), index2.model());
    }

    void shouldDisplayMessageOnError()
    {
        // GIVEN
        Widgets::PageView page;
        page.show();
        QTest::qWaitForWindowShown(&page);
        QTest::qWait(100);

        auto messageWidget = page.findChild<KMessageWidget*>(QStringLiteral("messageWidget"));
        QVERIFY(messageWidget);
        QVERIFY(!messageWidget->isVisibleTo(&page));

        QCOMPARE(messageWidget->findChildren<QToolButton*>().size(), 1);
        auto closeButton = messageWidget->findChildren<QToolButton*>().first();
        QVERIFY(closeButton);

        // WHEN
        page.displayErrorMessage(QStringLiteral("Foo Error"));

        // THEN
        QVERIFY(messageWidget->isVisibleTo(&page));
        QVERIFY(messageWidget->isCloseButtonVisible());
        QCOMPARE(messageWidget->text(), QStringLiteral("Foo Error"));
        QVERIFY(messageWidget->icon().isNull());
        QCOMPARE(messageWidget->messageType(), KMessageWidget::Error);
        QVERIFY(messageWidget->isShowAnimationRunning());
        QVERIFY(!messageWidget->isHideAnimationRunning());

        // WHEN
        QTest::qWait(800);

        // THEN
        QVERIFY(!messageWidget->isShowAnimationRunning());
        QVERIFY(!messageWidget->isHideAnimationRunning());

        // WHEN
        closeButton->click();

        // THEN
        QVERIFY(!messageWidget->isShowAnimationRunning());
        QVERIFY(messageWidget->isHideAnimationRunning());

        // WHEN
        QTest::qWait(800);

        // THEN
        QVERIFY(!messageWidget->isShowAnimationRunning());
        QVERIFY(!messageWidget->isHideAnimationRunning());
    }

    void shouldRunTask()
    {
        // GIVEN
        PageModelStub stubPageModel;
        Q_ASSERT(stubPageModel.property("centralListModel").canConvert<QAbstractItemModel*>());
        auto task1 = stubPageModel.addTaskItem(QStringLiteral("Task1"));
        auto task2 = stubPageModel.addTaskItem(QStringLiteral("Task2"));
        Widgets::PageView page;
        page.setModel(&stubPageModel);
        RunningTaskModelStub stubRunningTaskModel;
        page.setRunningTaskModel(&stubRunningTaskModel);
        auto centralView = page.findChild<QTreeView*>(QStringLiteral("centralView"));

        QModelIndex index = stubPageModel.itemModel.index(0, 0);
        centralView->selectionModel()->setCurrentIndex(index, QItemSelectionModel::ClearAndSelect);
        QVERIFY(centralView->selectionModel()->currentIndex().isValid());

        auto runTaskAction = page.findChild<QAction*>(QStringLiteral("runTaskAction"));
        QVERIFY(runTaskAction);
        QVERIFY(runTaskAction->isEnabled());

        // WHEN starting the first task
        runTaskAction->trigger();

        // THEN
        QCOMPARE(stubRunningTaskModel.property("runningTask").value<Domain::Task::Ptr>(), task1);
        QCOMPARE(task1->startDate().date(), QDate::currentDate());

        // WHEN starting the second task
        QModelIndex index2 = stubPageModel.itemModel.index(1, 0);
        centralView->selectionModel()->setCurrentIndex(index2, QItemSelectionModel::ClearAndSelect);
        runTaskAction->trigger();

        // THEN
        QCOMPARE(stubRunningTaskModel.property("runningTask").value<Domain::Task::Ptr>(), task2);
        QCOMPARE(task2->startDate().date(), QDate::currentDate());
    }
};

ZANSHIN_TEST_MAIN(PageViewTest)

#include "pageviewtest.moc"
