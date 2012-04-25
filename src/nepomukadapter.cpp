/* This file is part of Zanshin Todo.
 * 
 * Copyright 2011 Christian Mollekopf <chrigi_1@fastmail.fm>
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
 * USA.
 */

#include "nepomukadapter.h"
#include <queries.h>
#include <pimitem.h>
#include <tagmanager.h>
#include "globaldefs.h"
#include "topicsmodel.h"
#include <Nepomuk/Query/Query>
#include <Nepomuk/Query/QueryServiceClient>
#include <Nepomuk/Query/Result>
#include <Nepomuk/Types/Class>
#include <Nepomuk/Resource>
#include <Nepomuk/Vocabulary/PIMO>
#include <KDebug>
#include <KIcon>
#include <nepomuk/resourcetypeterm.h>
#include <nepomuk/resourcewatcher.h>
#include <Soprano/Vocabulary/NAO>
#include <QMimeData>

StructureAdapter::StructureAdapter(QObject* parent): QObject(parent), m_model(0)
{

}


void StructureAdapter::setModel(TopicsModel* model)
{
    m_model = model;
}


TestStructureAdapter::TestStructureAdapter(QObject* parent)
: StructureAdapter(parent)
{

}


TopicsModel::IdList TestStructureAdapter::onSourceInsertRow(const QModelIndex &sourceChildIndex)
{
    if (!sourceChildIndex.isValid()) {
        kWarning() << "invalid indexx";
        return TopicsModel::IdList();
    }

    if (!sourceChildIndex.data(TopicParentRole).isValid()) {
        return TopicsModel::IdList();
    }
    const TopicsModel::Id &parent = sourceChildIndex.data(TopicParentRole).value<TopicsModel::Id>();

    return TopicsModel::IdList() << parent;
}

TopicsModel::IdList TestStructureAdapter::onSourceDataChanged(const QModelIndex &sourceIndex)
{
    return onSourceInsertRow(sourceIndex);
}


void TestStructureAdapter::addParent(const TopicsModel::Id& identifier, const TopicsModel::Id& parentIdentifier, const QString& name)
{
    kDebug() << identifier << parentIdentifier << name;
    m_model->createOrUpdateParent(identifier, parentIdentifier, name);
}

void TestStructureAdapter::setParent(const QModelIndex &item, const qint64& parentIdentifier)
{
    m_model->itemParentsChanged(item, TopicsModel::IdList() << parentIdentifier);
}


void TestStructureAdapter::removeParent(const TopicsModel::Id& identifier)
{
    m_model->removeNode(identifier);
}



NepomukAdapter::NepomukAdapter(QObject* parent)
: StructureAdapter(parent), m_counter(0)
{
    setType(Nepomuk::Vocabulary::PIMO::Topic());
}

void NepomukAdapter::init()
{
    Nepomuk::Query::Query query;
    query.setTerm(Nepomuk::Query::ResourceTypeTerm(Nepomuk::Types::Class(m_type)));
    
    query.addRequestProperty(Nepomuk::Query::Query::RequestProperty(Nepomuk::Vocabulary::PIMO::superTopic()));
    
    Nepomuk::Query::QueryServiceClient *queryServiceClient = new Nepomuk::Query::QueryServiceClient(this);
    connect(queryServiceClient, SIGNAL(newEntries(QList<Nepomuk::Query::Result>)), this, SLOT(checkResults(QList<Nepomuk::Query::Result>)));
    connect(queryServiceClient, SIGNAL(finishedListing()), this, SLOT(queryFinished()));
    connect(queryServiceClient, SIGNAL(entriesRemoved(QList<QUrl>)), this, SLOT(removeResult(QList<QUrl>)));
    if ( !queryServiceClient->query(query) ) {
        kWarning() << "error";
    }
}


void NepomukAdapter::setType(const QUrl &type)
{
    m_type = type;
    
}

void NepomukAdapter::checkResults(const QList< Nepomuk::Query::Result > &results)
{
    //kDebug() <<  results.size() << results.first().resource().resourceUri() << results.first().resource().label() << results.first().resource().types() << results.first().resource().className();
    foreach (const Nepomuk::Query::Result &result, results) {
        Nepomuk::Resource res(result.resource().resourceUri());
        const QUrl parent = result.requestProperty(Nepomuk::Vocabulary::PIMO::superTopic()).uri();
        kDebug() << res.resourceUri() << res.label() << res.types() << res.className() << parent;
        if (res.types().contains(m_type)) {
            if (parent.isValid()) {
                addParent(res, parent);
            } else {
                addParent(res);
            }
        } else {
            kWarning() << "unknown result " << res.types();
        }
    }
}


void NepomukAdapter::addParent (const Nepomuk::Resource& topic, const QUrl &parent)
{
    kDebug() << "add topic" << topic.label() << topic.uri() << parent;
    if (parent.isValid() && !m_topicMap.contains(parent)) {
        addParent(parent);
    }
    QObject *guard = new QObject(this);
    m_guardMap[topic.resourceUri()] = guard;
    
    Nepomuk::ResourceWatcher *m_resourceWatcher = new Nepomuk::ResourceWatcher(guard);
    m_resourceWatcher->addResource(topic);
    m_resourceWatcher->addProperty(Soprano::Vocabulary::NAO::prefLabel());
    connect(m_resourceWatcher, SIGNAL(propertyAdded(Nepomuk::Resource,Nepomuk::Types::Property,QVariant)), this, SLOT(propertyChanged(Nepomuk::Resource,Nepomuk::Types::Property,QVariant)));
    m_resourceWatcher->start();
    
    Nepomuk::Query::QueryServiceClient *queryServiceClient = new Nepomuk::Query::QueryServiceClient(guard);
    queryServiceClient->setProperty("resourceuri", topic.resourceUri());
    connect(queryServiceClient, SIGNAL(newEntries(QList<Nepomuk::Query::Result>)), this, SLOT(itemsWithTopicAdded(QList<Nepomuk::Query::Result>)));
    connect(queryServiceClient, SIGNAL(entriesRemoved(QList<QUrl>)), this, SLOT(itemsFromTopicRemoved(QList<QUrl>)));
    connect(queryServiceClient, SIGNAL(finishedListing()), this, SLOT(queryFinished()));
    if ( !queryServiceClient->sparqlQuery(MindMirrorQueries::itemsWithTopicsQuery(QList <QUrl>() << topic.resourceUri())) ) {
        kWarning() << "error";
    }
    qint64 id = -1;
    if (m_topicMap.contains(topic.resourceUri())) {
        id = m_topicMap[topic.resourceUri()];
    } else {
        id = m_counter++;
        m_topicMap.insert(topic.resourceUri(), id);
    }
    qint64 pid = -1;
    if (m_topicMap.contains(parent)) {
        pid = m_topicMap[parent];
    }
    m_model->createOrUpdateParent(id, pid, topic.label());
}

void NepomukAdapter::onNodeRemoval(const qint64& id)
{
    kDebug() << id;
    const QUrl &targetTopic = m_topicMap.key(id);
    if (targetTopic.isValid()) {
        NepomukUtils::deleteTopic(targetTopic); //TODO delete subtopics with subresource handling?
    }
}


void NepomukAdapter::removeResult(const QList<QUrl> &results)
{
    foreach (const QUrl &result, results) {
        Nepomuk::Resource res(result);
        kDebug() << res.resourceUri() << res.label() << res.types() << res.className();
        if (res.types().contains(m_type)) {
            Q_ASSERT(m_topicMap.contains(res.resourceUri()));
            m_model->removeNode(m_topicMap.take(res.resourceUri())); //We remove it right here, because otherwise we would try to remove the topic again in onNodeRemoval
            m_guardMap.take(res.resourceUri())->deleteLater();
        } else {
            kWarning() << "unknown result " << res.types();
        }
    }
}

void NepomukAdapter::queryFinished()
{
    kWarning();
}


void NepomukAdapter::itemsWithTopicAdded(const QList<Nepomuk::Query::Result> &results)
{
    const QUrl &parent = sender()->property("resourceuri").toUrl();
    kDebug() << parent;
    
    QModelIndexList list;
    foreach (const Nepomuk::Query::Result &result, results) {
        Nepomuk::Resource res = Nepomuk::Resource(result.resource().resourceUri());
        kDebug() << res.resourceUri() << res.label() << res.types() << res.className();
        const Akonadi::Item item = PimItemUtils::getItemFromResource(res);
        if (!item.isValid()) {
            kWarning() << "invalid Item";
            continue;
        }
        Q_ASSERT(m_topicMap.contains(parent));
        m_topicCache.insert(item.url(),  TopicsModel::IdList() << m_topicMap[parent]); //TODO preserve existing topics (multi topic items)
        //If the index is already available change it right away
        const QModelIndexList &indexes = Akonadi::EntityTreeModel::modelIndexesForItem(m_model->sourceModel(), item);
        if (indexes.isEmpty()) {
            kWarning() << "item not found" << item.url() << m_topicMap[parent];
            continue;
        }
        list.append(indexes.first()); //TODO hanle all
        m_model->itemParentsChanged(indexes.first(), m_topicCache.value(item.url()));
    }
}

QList< qint64 > NepomukAdapter::onSourceInsertRow(const QModelIndex& sourceChildIndex)
{
    const Akonadi::Item &item = sourceChildIndex.data(Akonadi::EntityTreeModel::ItemRole).value<Akonadi::Item>();
    kDebug() << item.url() << m_topicCache.value(item.url());
    return m_topicCache.value(item.url());
//     return StructureAdapter::onSourceInsertRow(sourceChildIndex);
}

QList< qint64 > NepomukAdapter::onSourceDataChanged(const QModelIndex& changed)
{
    return onSourceInsertRow(changed);
}

void NepomukAdapter::itemsFromTopicRemoved(const QList<QUrl> &items)
{
    const QUrl &topic = sender()->property("topic").toUrl();
    kDebug() << "removing nodes from topic: " << topic;
    QModelIndexList list;
    foreach (const QUrl &uri, items) {
        Nepomuk::Resource res = Nepomuk::Resource(uri);
        const Akonadi::Item item = PimItemUtils::getItemFromResource(res);
        if (!item.isValid()) {
            continue;
        }
        kDebug() << item.url();
        m_topicCache.remove(item.url()); //TODO preserve other topics
        const QModelIndexList &indexes = Akonadi::EntityTreeModel::modelIndexesForItem(m_model->sourceModel(), item);
        if (indexes.isEmpty()) {
            kDebug() << "item not found" << item.url();
            continue;
        }
        list.append(indexes.first()); //TODO handle all
        m_model->itemParentsChanged(indexes.first(), m_topicCache.value(item.url()));
    }
}

void NepomukAdapter::propertyChanged(const Nepomuk::Resource &res, const Nepomuk::Types::Property &property, const QVariant &value)
{
    if (property.uri() == Soprano::Vocabulary::NAO::prefLabel()) {
        kDebug() << "renamed " << res.resourceUri() << " to " << value.toString();
        Q_ASSERT(m_topicMap.contains(res.resourceUri()));
        m_model->renameParent(m_topicMap[res.resourceUri()], value.toString());
    }
    if (property.uri() == Nepomuk::Vocabulary::PIMO::superTopic()) {
        //TODO handle move of topic
    }
}

bool NepomukAdapter::onDropMimeData(const QMimeData* mimeData, Qt::DropAction action,  qint64 id)
{
    bool moveToTrash = false;
    QUrl targetTopic;
    if (id >= 0) {
        //kDebug() << "dropped on item " << data(parent, UriRole) << data(parent, Qt::DisplayRole).toString();
        targetTopic = m_topicMap.key(id);
        
    }
    
    if (!mimeData->hasUrls()) {
        kWarning() << "no urls in drop";
        return false;
    }
    kDebug() << mimeData->urls();
    
    foreach (const KUrl &url, mimeData->urls()) {
        const Akonadi::Item item = Akonadi::Item::fromUrl(url);
        if (!item.isValid()) {
            kDebug() << "invalid item";
            continue;
        }
        
        if (targetTopic.isValid()) {
            kDebug() << "set topic: " << targetTopic << " on dropped item: " << item.url();
            NepomukUtils::moveToTopic(item, targetTopic);
        } else {
            kDebug() << "remove all topics from item:" << item.url();
            NepomukUtils::removeAllTopics(item);
        }
        
    }
    return true;
}

bool NepomukAdapter::onSetData(qint64 id, const QVariant &value, int role) {
    const QUrl &targetTopic = m_topicMap.key(id);
    if (!targetTopic.isValid()) {
        kWarning() << "tried to rename invalid topic";
        return false;
    }
    NepomukUtils::renameTopic(targetTopic, value.toString());
    return true;
}

void NepomukAdapter::setData(TodoNode* node, qint64 id)
{
    node->setData(KIcon("view-pim-notes"), 0, Qt::DecorationRole);
    node->setRowData(Zanshin::Topic, Zanshin::ItemTypeRole);
    node->setRowData(m_topicMap.key(id), Zanshin::UriRole);
}
