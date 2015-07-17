/* This file is part of Zanshin

   Copyright 2015 Kevin Ottens <ervin@kde.org>

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

#include "akonadifakedataxmlloader.h"
#include "akonadifakedata.h"

#include <akonadi/xml/xmldocument.h>

#include <KCalCore/Todo>

using namespace Testlib;

AkonadiFakeDataXmlLoader::AkonadiFakeDataXmlLoader(AkonadiFakeData *data)
    : m_data(data)
{
}

void AkonadiFakeDataXmlLoader::load(const QString &fileName) const
{
    Akonadi::XmlDocument doc(fileName);
    Q_ASSERT(doc.isValid());

    Akonadi::Tag::Id tagId = 1;
    Akonadi::Collection::Id collectionId = 1;
    Akonadi::Item::Id itemId = 1;

    QHash<QString, Akonadi::Tag> tagByRid;

    foreach (const Akonadi::Tag &tag, doc.tags()) {
        auto t = tag;
        t.setId(tagId++);
        m_data->createTag(t);
        tagByRid[t.remoteId()] = t;
    }

    QHash<QString, Akonadi::Collection> collectionByRid;

    foreach (const Akonadi::Collection &collection, doc.collections()) {
        auto c = collection;
        c.setId(collectionId++);
        collectionByRid[c.remoteId()] = c;
    }

    foreach (const Akonadi::Collection &collection, doc.collections()) {
        auto c = collectionByRid.value(collection.remoteId());
        c.setParentCollection(collectionByRid.value(c.parentCollection().remoteId()));
        m_data->createCollection(c);

        foreach (const Akonadi::Item &item, doc.items(collection)) {
            auto i = item;
            i.setId(itemId++);
            i.setParentCollection(c);
            i.setModificationTime(QDateTime::currentDateTime());

            auto tags = QList<Akonadi::Tag>();
            std::transform(i.tags().constBegin(), i.tags().constEnd(),
                           std::back_inserter(tags),
                           [&tagByRid] (const Akonadi::Tag &tag) {
                               return tagByRid.value(tag.remoteId());
                           });
            i.setTags(tags);
            m_data->createItem(i);
        }
    }
}
