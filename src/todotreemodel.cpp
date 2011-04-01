/* This file is part of Zanshin Todo.

   Copyright 2008 Kevin Ottens <ervin@kde.org>
   Copyright 2008, 2009 Mario Bensi <nef@ipsquad.net>

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

#include "todotreemodel.h"

#include <QtCore/QStringList>

#include <KDE/Akonadi/EntityTreeModel>
#include <KDE/Akonadi/ItemFetchJob>
#include <KDE/Akonadi/ItemFetchScope>
#include <KDE/KCalCore/Todo>
#include <KDE/KDebug>
#include <KDE/KIcon>
#include <KDE/KLocale>
#include <KDE/KUrl>


#include <algorithm>
#include <boost/bind.hpp>

#include "kmodelindexproxymapper.h"
#include "globaldefs.h"
#include "todohelpers.h"
#include "todonode.h"
#include "todonodemanager.h"

TodoTreeModel::TodoTreeModel(QObject *parent)
    : TodoProxyModelBase(SimpleMapping, parent)
{
}

TodoTreeModel::~TodoTreeModel()
{
}

bool _k_indexLessThan(const QModelIndex &left, const QModelIndex &right)
{
    Zanshin::ItemType leftType = (Zanshin::ItemType) left.data(Zanshin::ItemTypeRole).toInt();
    Zanshin::ItemType rightType = (Zanshin::ItemType) right.data(Zanshin::ItemTypeRole).toInt();

    if (leftType!=rightType) {
        return (leftType==Zanshin::Collection && rightType==Zanshin::ProjectTodo)
            || (leftType==Zanshin::Collection && rightType==Zanshin::StandardTodo)
            || (leftType==Zanshin::ProjectTodo && rightType==Zanshin::StandardTodo);
    }

    if (leftType==Zanshin::Collection) {
        qint64 leftId = left.data(Akonadi::EntityTreeModel::CollectionIdRole).toLongLong();
        qint64 rightId = right.data(Akonadi::EntityTreeModel::CollectionIdRole).toLongLong();

        return leftId<rightId;

    } else if (leftType==Zanshin::ProjectTodo) {
        QStringList leftAncestors = left.data(Zanshin::AncestorsUidRole).toStringList();
        QStringList rightAncestors = right.data(Zanshin::AncestorsUidRole).toStringList();

        return leftAncestors.size()<rightAncestors.size();

    } else if (leftType==Zanshin::StandardTodo) {
        QString leftId = left.data(Zanshin::UidRole).toString();
        QString rightId = right.data(Zanshin::UidRole).toString();
        return leftId<rightId;

    } else {
        kFatal() << "Shouldn't happen, we must get only collections or todos";
        return false;
    }
}

void TodoTreeModel::onSourceInsertRows(const QModelIndex &sourceIndex, int begin, int end)
{
    QList<QModelIndex> sourceChildIndexes;

    // Walking through all the items to harvest the needed info for sorting
    for (int i = begin; i <= end; i++) {
        QModelIndex sourceChildIndex = sourceModel()->index(i, 0, sourceIndex);
        if (sourceChildIndex.isValid()) {
            sourceChildIndexes << sourceChildIndex;
        }
    }

    // Sort, the top level ones first, then one level deep, and so on...
    qSort(sourceChildIndexes.begin(), sourceChildIndexes.end(), &_k_indexLessThan);

    // Now we're sure to add them in the right order, so let's do that!
    TodoNode *collectionNode = m_manager->nodeForSourceIndex(sourceIndex);
    QHash<QString, TodoNode*> uidHash = m_collectionToUidsHash[collectionNode];

    foreach (const QModelIndex &sourceChildIndex, sourceChildIndexes) {
        Zanshin::ItemType type = (Zanshin::ItemType) sourceChildIndex.data(Zanshin::ItemTypeRole).toInt();

        if (type==Zanshin::Collection) {
            //kDebug() << "Adding collection";
            addChildNode(sourceChildIndex, collectionNode);
            onSourceInsertRows(sourceChildIndex, 0, sourceModel()->rowCount(sourceChildIndex)-1);

        } else {
            QString parentUid = sourceChildIndex.data(Zanshin::ParentUidRole).toString();
            TodoNode *parentNode = 0;

            if (uidHash.contains(parentUid)) {
                parentNode = uidHash[parentUid];

            } else {
                if (type==Zanshin::ProjectTodo) {
                    parentNode = collectionNode;
                } else if (type==Zanshin::StandardTodo) {
                    parentNode = m_inboxNode;
                } else {
                    kFatal() << "Shouldn't happen, we must get only collections or todos";
                }
            }

            TodoNode *child = addChildNode(sourceChildIndex, parentNode);
            QString uid = child->data(0, Zanshin::UidRole).toString();
            //kDebug() << "Adding node:" << uid << parentUid;
            uidHash[uid] = child;
        }
    }

    m_collectionToUidsHash[collectionNode] = uidHash;
}

void TodoTreeModel::onSourceRemoveRows(const QModelIndex &sourceIndex, int begin, int end)
{
    for (int i = begin; i <= end; ++i) {
        QModelIndex sourceChildIndex = sourceModel()->index(i, 0, sourceIndex);
        TodoNode *node = m_manager->nodeForSourceIndex(sourceChildIndex);
        if (node) {
            destroyBranch(node);
        }
    }
}

void TodoTreeModel::onSourceDataChanged(const QModelIndex &begin, const QModelIndex &end)
{
    for (int row = begin.row(); row <= end.row(); ++row) {
        QModelIndex sourceChildIndex = sourceModel()->index(row, 0, begin.parent());

        if (!sourceChildIndex.isValid()) {
            continue;
        }

        TodoNode *node = m_manager->nodeForSourceIndex(sourceChildIndex);

        // Collections are just reemited
        if (node->data(0, Zanshin::ItemTypeRole).toInt()==Zanshin::Collection) {
            emit dataChanged(mapFromSource(sourceChildIndex),
                             mapFromSource(sourceChildIndex));
            continue;
        }

        QString oldParentUid = node->parent()->data(0, Zanshin::UidRole).toString();
        QString newParentUid = sourceChildIndex.data(Zanshin::ParentUidRole).toString();

        // If the parent didn't change we just reemit
        if (oldParentUid==newParentUid) {
            emit dataChanged(mapFromSource(sourceChildIndex), mapFromSource(sourceChildIndex));
            continue;
        }

        // The parent did change, so first destroy the old branch...
        destroyBranch(node);

        // Then simulate a row insertion signal
        createChild(sourceChildIndex, sourceChildIndex.parent(), row);
    }
}

TodoNode *TodoTreeModel::createInbox() const
{
    TodoNode *node = new TodoNode;

    node->setData(i18n("Inbox"), 0, Qt::DisplayRole);
    node->setData(KIcon("mail-folder-inbox"), 0, Qt::DecorationRole);
    node->setRowData(Zanshin::Inbox, Zanshin::ItemTypeRole);

    return node;
}

void TodoTreeModel::destroyBranch(TodoNode *root)
{
    foreach (TodoNode *child, root->children()) {
        destroyBranch(child);
    }

    QModelIndex proxyParentIndex = m_manager->indexForNode(root->parent(), 0);
    int row = 0;

    if (root->parent()) {
        row = root->parent()->children().indexOf(root);
    } else {
        row = m_manager->roots().indexOf(root);
    }

    beginRemoveRows(proxyParentIndex, row, row);
    m_manager->removeNode(root);
    delete root;
    endRemoveRows();
}

Qt::ItemFlags TodoTreeModel::flags(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return Qt::NoItemFlags;
    }

    if (index.data(Zanshin::ItemTypeRole).toInt() == Zanshin::Inbox) {
        return Qt::ItemIsSelectable | Qt::ItemIsDropEnabled | Qt::ItemIsEnabled;
    }
    return sourceModel()->flags(mapToSource(index)) | Qt::ItemIsDropEnabled;
}

QMimeData *TodoTreeModel::mimeData(const QModelIndexList &indexes) const
{
    QModelIndexList sourceIndexes;
    foreach (const QModelIndex &proxyIndex, indexes) {
        sourceIndexes << mapToSource(proxyIndex);
    }

    return sourceModel()->mimeData(sourceIndexes);
}

QStringList TodoTreeModel::mimeTypes() const
{
    QStringList types;
    if (sourceModel()) {
        types << sourceModel()->mimeTypes();
    }
    return types;
}

void TodoTreeModel::createChild(const QModelIndex &child, const QModelIndex &parent, int row)
{
    if (!child.isValid() || !parent.isValid()) {
        return;
    }

    onSourceInsertRows(parent, row, row);

    QModelIndexList children = child.data(Zanshin::ChildIndexesRole).value<QModelIndexList>();
    foreach (const QModelIndex &index, children) {
        Q_ASSERT(index.model()==sourceModel());
        createChild(index, child.parent(), index.row());
    }
}

bool TodoTreeModel::dropMimeData(const QMimeData *mimeData, Qt::DropAction action,
                                 int /*row*/, int /*column*/, const QModelIndex &parent)
{
    if (action != Qt::MoveAction || !KUrl::List::canDecode(mimeData)) {
        return false;
    }

    KUrl::List urls = KUrl::List::fromMimeData(mimeData);

    Akonadi::Collection collection;
    Zanshin::ItemType parentType = (Zanshin::ItemType)parent.data(Zanshin::ItemTypeRole).toInt();
    if (parentType == Zanshin::Collection) {
        collection = parent.data(Akonadi::EntityTreeModel::CollectionRole).value<Akonadi::Collection>();
    } else {
        const Akonadi::Item parentItem = parent.data(Akonadi::EntityTreeModel::ItemRole).value<Akonadi::Item>();
        collection = parentItem.parentCollection();
    }

    QString parentUid = parent.data(Zanshin::UidRole).toString();

    foreach (const KUrl &url, urls) {
        const Akonadi::Item urlItem = Akonadi::Item::fromUrl(url);
        if (urlItem.isValid()) {
            Akonadi::ItemFetchJob *job = new Akonadi::ItemFetchJob(urlItem);
            Akonadi::ItemFetchScope scope;
            scope.setAncestorRetrieval(Akonadi::ItemFetchScope::Parent);
            scope.fetchFullPayload();
            job->setFetchScope(scope);

            if ( !job->exec() ) {
                return false;
            }

            foreach (const Akonadi::Item &item, job->items()) {
                if (item.hasPayload<KCalCore::Todo::Ptr>()) {

                    QModelIndexList indexes = Akonadi::EntityTreeModel::modelIndexesForItem(sourceModel(), item);
                    if (indexes.isEmpty()) {
                        return false;
                    }
                    QModelIndex index = indexes.first();
                    TodoHelpers::moveTodoToProject(index, parentUid, parentType, collection);
                }
            }
        }
    }

    return true;
}

Qt::DropActions TodoTreeModel::supportedDropActions() const
{
    if (!sourceModel()) {
        return Qt::IgnoreAction;
    }
    return sourceModel()->supportedDropActions();
}
