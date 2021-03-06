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

#include "akonaditagqueries.h"

using namespace Akonadi;

TagQueries::TagQueries(const StorageInterface::Ptr &storage, const SerializerInterface::Ptr &serializer, const MonitorInterface::Ptr &monitor)
    : m_serializer(serializer),
      m_helpers(new LiveQueryHelpers(serializer, storage)),
      m_integrator(new LiveQueryIntegrator(serializer, monitor))
{
    m_integrator->addRemoveHandler([this] (const Tag &tag) {
        m_findTopLevel.remove(tag.id());
    });
}

TagQueries::TagResult::Ptr TagQueries::findAll() const
{
    auto fetch = m_helpers->fetchTags();
    auto predicate = [this] (const Akonadi::Tag &akonadiTag) {
        return akonadiTag.type() == Akonadi::Tag::PLAIN;
    };
    m_integrator->bind("TagQueries::findAll", m_findAll, fetch, predicate);
    return m_findAll->result();
}

TagQueries::NoteResult::Ptr TagQueries::findNotes(Domain::Tag::Ptr tag) const
{
    Akonadi::Tag akonadiTag = m_serializer->createAkonadiTagFromTag(tag);
    auto &query = m_findTopLevel[akonadiTag.id()];
    auto fetch = m_helpers->fetchItems(akonadiTag);
    auto predicate = [this, tag] (const Akonadi::Item &item) {
        return m_serializer->isTagChild(tag, item);
    };
    m_integrator->bind("TagQueries::findNotes", query, fetch, predicate);
    return query->result();
}
