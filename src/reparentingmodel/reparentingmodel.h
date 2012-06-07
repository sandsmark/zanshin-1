/*
    <one line to give the library's name and an idea of what it does.>
    Copyright (C) 2012  Christian Mollekopf <chrigi_1@fastmail.fm>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/


#ifndef REPARENTINGMODEL_H
#define REPARENTINGMODEL_H

#include "todoproxymodelbase.h"
#include "reparentingstrategy.h"

/**
 * A generic reparenting model.
 *
 * A strategy implements the parent relations.
 * The parentstructuremodel shoudl simply subclass this model, additionally providing the possiblity to insert new items.
 *
 * The default structure with an inbox should be built with a default implementation of the parentstructurestrategy, which creates an inbox.
 *
 */
class ReparentingModel : public TodoProxyModelBase
{
     Q_OBJECT

public:
    ReparentingModel(ReparentingStrategy *strategy, QObject *parent = 0);
    virtual ~ReparentingModel();

    virtual Qt::ItemFlags flags(const QModelIndex &index) const;
    virtual QMimeData *mimeData(const QModelIndexList &indexes) const;
    virtual bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent);
    virtual QStringList mimeTypes() const;
    virtual Qt::DropActions supportedDropActions() const;

    virtual bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
    virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;

protected:
    virtual void resetInternalData();

private slots:
    void onSourceDataChanged(const QModelIndex &begin, const QModelIndex &end);
    void onSourceInsertRows(const QModelIndex &sourceIndex, int begin, int end);
    void onSourceRemoveRows(const QModelIndex &sourceIndex, int begin, int end);

private:
    friend ReparentingStrategy;
    virtual void init();
    virtual TodoNode *createInbox() const;

    /**
     * Updates the parents of @param sourceIndex.
     *
     * Moves/adds/removes from parents.
     */
//     void itemParentsChanged(const QModelIndex &sourceIndex, const IdList &parents);
    TodoNode *reparentNode(const Id& p, const IdList& parents, const QModelIndex &index = QModelIndex());

    TodoNode *createNode(const Id &identifier, const IdList &parentIdentifier, const QString &name = QString(), const QModelIndex &index = QModelIndex());
    void removeNode(TodoNode *node, bool removeChildren = false);
    void removeNodeById(Id);
    void renameNode(const Id& identifier, const QString& name);
    
    TodoNode *m_rootNode;
    QMap<Id, TodoNode*> m_parentMap;

    QScopedPointer<ReparentingStrategy> m_strategy;
};

#endif // REPARENTINGMODEL_H