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


#include "akonadidatasourcequeries.h"

#include "akonadistoragesettings.h"

using namespace Akonadi;

DataSourceQueries::DataSourceQueries(StorageInterface::FetchContentTypes contentTypes,
                                     const StorageInterface::Ptr &storage,
                                     const SerializerInterface::Ptr &serializer,
                                     const MonitorInterface::Ptr &monitor)
    : m_contentTypes(contentTypes),
      m_serializer(serializer),
      m_helpers(new LiveQueryHelpers(serializer, storage)),
      m_integrator(new LiveQueryIntegrator(serializer, monitor))
{
    m_integrator->addRemoveHandler([this] (const Collection &collection) {
        m_findChildren.remove(collection.id());
    });
}

bool DataSourceQueries::isDefaultSource(Domain::DataSource::Ptr source) const
{
    auto sourceCollection = m_serializer->createCollectionFromDataSource(source);
    if (m_contentTypes == StorageInterface::Tasks)
        return sourceCollection == StorageSettings::instance().defaultTaskCollection();
    else if (m_contentTypes == StorageInterface::Notes)
        return sourceCollection == StorageSettings::instance().defaultNoteCollection();
    else
        return false;
}

void DataSourceQueries::changeDefaultSource(Domain::DataSource::Ptr source)
{
    auto sourceCollection = m_serializer->createCollectionFromDataSource(source);
    if (m_contentTypes == StorageInterface::Tasks) {
        StorageSettings::instance().setDefaultTaskCollection(sourceCollection);
    } else if (m_contentTypes == StorageInterface::Notes) {
        StorageSettings::instance().setDefaultNoteCollection(sourceCollection);
    }
}

DataSourceQueries::DataSourceResult::Ptr DataSourceQueries::findTopLevel() const
{
    auto fetch = m_helpers->fetchCollections(Collection::root(), m_contentTypes);
    auto predicate = createFetchPredicate(Collection::root());
    m_integrator->bind("DataSourceQueries::findTopLevel", m_findTopLevel, fetch, predicate);
    return m_findTopLevel->result();
}

DataSourceQueries::DataSourceResult::Ptr DataSourceQueries::findChildren(Domain::DataSource::Ptr source) const
{
    Collection root = m_serializer->createCollectionFromDataSource(source);
    auto &query = m_findChildren[root.id()];
    auto fetch = m_helpers->fetchCollections(root, m_contentTypes);
    auto predicate = createFetchPredicate(root);
    m_integrator->bind("DataSourceQueries::findChildren", query, fetch, predicate);
    return query->result();
}

DataSourceQueries::CollectionInputQuery::PredicateFunction DataSourceQueries::createFetchPredicate(const Collection &root) const
{
    return [this, root] (const Collection &collection) {
        return collection.isValid()
            && collection.parentCollection() == root;
    };
}
