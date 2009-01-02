/* This file is part of Zanshin Todo.

   Copyright 2008 Kevin Ottens <ervin@kde.org>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
   USA.
*/

#include "modeltestbase.h"
#include <qtest_kde.h>

#include <akonadi/collection.h>
#include <akonadi/item.h>
#include <akonadi/itemdeletejob.h>

#include <QtGui/QSortFilterProxyModel>

#include "todoflatmodel.h"
#include "todotreemodel.h"

class TodoTreeModelTest : public ModelTestBase
{
    Q_OBJECT

private slots:
    void initTestCase();
    void testInitialState();
    void testSingleModification();
    void testReparentModification();
    void testSingleRemoved();
    void testDragAndDrop();

private:
    TodoFlatModel m_flatModel;
    QSortFilterProxyModel m_flatSortedModel;
    TodoTreeModel m_model;
    QSortFilterProxyModel m_sortedModel;
};

QTEST_KDEMAIN(TodoTreeModelTest, GUI)

void TodoTreeModelTest::initTestCase()
{
    ModelTestBase::initTestCase();
    m_model.setSourceModel(&m_flatModel);
    m_model.setCollection(m_collection);
    flushNotifications();

    m_flatSortedModel.setSourceModel(&m_flatModel);
    m_flatSortedModel.sort(TodoFlatModel::RemoteId);

    m_sortedModel.setSourceModel(&m_model);
    m_sortedModel.sort(TodoFlatModel::Summary);
}

class TreeNode
{
public:
    TreeNode(const QString &i, const QString &s)
        : id(i), summary(s) { }

    TreeNode &operator<<(const TreeNode &child)
    {
        children << child;
        return *this;
    }

    QString id;
    QString summary;
    QList<TreeNode> children;
};

#if 0
static void dumpTree(const QList<TreeNode> &tree, int indent = 0)
{
    QString prefix;
    for (int i=0; i<indent; ++i) {
        prefix+="    ";
    }

    foreach (const TreeNode &node, tree) {
        qDebug() << prefix << "[" << node.id << node.summary;
        dumpTree(node.children, indent+1);
        qDebug() << prefix << "]";
    }
}
#endif

static void compareTrees(QAbstractItemModel *model, const QList<TreeNode> &tree,
                         const QModelIndex &root = QModelIndex())
{
    int row = 0;
    foreach (const TreeNode &node, tree) {
        QCOMPARE(model->data(model->index(row, TodoFlatModel::RemoteId, root)).toString(), node.id);
        QCOMPARE(model->data(model->index(row, TodoFlatModel::Summary, root)).toString(), node.summary);

        QCOMPARE(model->rowCount(model->index(row, 0, root)), node.children.size());
        compareTrees(model, node.children, model->index(row, 0, root));
        row++;
    }
}

void TodoTreeModelTest::testInitialState()
{
    QList<TreeNode> tree;

    tree << (TreeNode("fake-12", "First Folder")
             << (TreeNode("fake-01", "Becoming Astronaut")
                 << TreeNode("fake-07", "Learn the constellations")
                 << TreeNode("fake-02", "Look at the stars")
                )
             << (TreeNode("fake-11", "Becoming more relaxed")
                 << TreeNode("fake-09", "Listen new age album 2")
                 << TreeNode("fake-06", "Read magazine")
                )
            )

         << (TreeNode("fake-04", "Second Folder")
             << (TreeNode("fake-10", "Pet Project")
                 << TreeNode("fake-08", "Choose a puppy")
                 << TreeNode("fake-05", "Feed the dog")
                 << TreeNode("fake-03", "Walk around with the dog")
                )
            );

    compareTrees(&m_sortedModel, tree);
}

void TodoTreeModelTest::testSingleModification()
{
    Akonadi::Item item = m_flatModel.itemForIndex(m_flatSortedModel.mapToSource(m_flatSortedModel.index(6, 0)));
    QModelIndex index = m_model.indexForItem(item, TodoFlatModel::Summary);

    QSignalSpy spy(&m_model, SIGNAL(dataChanged(QModelIndex, QModelIndex)));

    QVERIFY(m_model.setData(index, "Learn something"));

    flushNotifications();

    QCOMPARE(m_model.data(index).toString(), QString("Learn something"));

    QCOMPARE(spy.count(), 1);
    QVariantList signal = spy.takeFirst();
    QCOMPARE(signal.count(), 2);
    QCOMPARE(signal.at(0).value<QModelIndex>(), m_model.index(index.row(), 0, index.parent()));
    QCOMPARE(signal.at(1).value<QModelIndex>(), m_model.index(index.row(), TodoFlatModel::LastColumn, index.parent()));
}

void TodoTreeModelTest::testReparentModification()
{
    Akonadi::Item item = m_flatModel.itemForIndex(m_flatSortedModel.mapToSource(m_flatSortedModel.index(1, 0)));
    QModelIndex index = m_model.indexForItem(item, TodoFlatModel::ParentRemoteId);

    QModelIndex oldParent = index.parent();

    item = m_flatModel.itemForIndex(m_flatSortedModel.mapToSource(m_flatSortedModel.index(9, 0)));
    QModelIndex newParent = m_model.indexForItem(item, 0);

    QSignalSpy rowsInserted(&m_model, SIGNAL(rowsInserted(QModelIndex, int, int)));
    QSignalSpy rowsRemoved(&m_model, SIGNAL(rowsRemoved(QModelIndex, int, int)));



    QVERIFY(m_model.setData(index, "fake-10"));

    flushNotifications();

    QCOMPARE(rowsRemoved.count(), 1);
    QVariantList signal = rowsRemoved.takeFirst();
    QCOMPARE(signal.count(), 3);
    QCOMPARE(signal.at(0).value<QModelIndex>(), oldParent);
    QCOMPARE(signal.at(1).toInt(), 1);
    QCOMPARE(signal.at(2).toInt(), 1);

    QCOMPARE(rowsInserted.count(), 1);
    signal = rowsInserted.takeFirst();
    QCOMPARE(signal.count(), 3);
    QCOMPARE(signal.at(0).value<QModelIndex>(), newParent);
    QCOMPARE(signal.at(1).toInt(), 3);
    QCOMPARE(signal.at(2).toInt(), 3);


    item = m_flatModel.itemForIndex(m_flatSortedModel.mapToSource(m_flatSortedModel.index(1, 0)));
    index = m_model.indexForItem(item, TodoFlatModel::ParentRemoteId);
    QVERIFY(m_model.setData(index, "fake-01"));

    flushNotifications();

    QCOMPARE(rowsRemoved.count(), 1);
    signal = rowsRemoved.takeFirst();
    QCOMPARE(signal.count(), 3);
    qDebug() << signal.at(0).value<QModelIndex>() << newParent;
    QCOMPARE(signal.at(0).value<QModelIndex>(), newParent);
    QCOMPARE(signal.at(1).toInt(), 3);
    QCOMPARE(signal.at(2).toInt(), 3);

    QCOMPARE(rowsInserted.count(), 1);
    signal = rowsInserted.takeFirst();
    QCOMPARE(signal.count(), 3);
    QCOMPARE(signal.at(0).value<QModelIndex>(), oldParent);
    QCOMPARE(signal.at(1).toInt(), 1);
    QCOMPARE(signal.at(2).toInt(), 1);
}

void TodoTreeModelTest::testSingleRemoved()
{
    Akonadi::Item item = m_flatModel.itemForIndex(m_flatSortedModel.mapToSource(m_flatSortedModel.index(2, 0)));
    QModelIndex index = m_model.indexForItem(item, TodoFlatModel::ParentRemoteId);

    QModelIndex parent = index.parent();
    int count = m_model.rowCount(parent);

    QSignalSpy spy(&m_model, SIGNAL(rowsAboutToBeRemoved(QModelIndex, int, int)));

    Akonadi::ItemDeleteJob *job = new Akonadi::ItemDeleteJob(item);
    QVERIFY(job->exec());

    flushNotifications();

    QCOMPARE(m_model.rowCount(parent), count - 1);

    QCOMPARE(spy.count(), 1);
    QVariantList signal = spy.takeFirst();
    QCOMPARE(signal.count(), 3);
    QCOMPARE(signal.at(0).value<QModelIndex>(), parent);
    QCOMPARE(signal.at(1).toInt(), 2);
    QCOMPARE(signal.at(1).toInt(), 2);
}

void TodoTreeModelTest::testDragAndDrop()
{
    Akonadi::Item item = m_flatModel.itemForIndex(m_flatSortedModel.mapToSource(m_flatSortedModel.index(5, 0)));
    QModelIndex index = m_model.indexForItem(item, TodoFlatModel::ParentRemoteId);

    QCOMPARE(m_model.data(index).toString(), QString("fake-01"));
    QModelIndex parent = index.parent();
    QModelIndex parentIndex = m_model.mapToSource(index);

    QModelIndexList indexes;
    indexes << index;
    QMimeData *mimeData = m_model.mimeData(indexes);
    QVERIFY(m_model.dropMimeData(mimeData, Qt::MoveAction, 0, 0, QModelIndex()));

    index = m_model.indexForItem(item, TodoFlatModel::ParentRemoteId);
    QCOMPARE(m_model.data(index).toString(), QString());

    indexes.clear();
}

#include "todotreemodeltest.moc"
