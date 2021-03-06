/* This file is part of Zanshin

   Copyright 2015 Mario Bensi <mbensi@ipsquad.net>
   Copyright 2017 Kevin Ottens <ervin@kde.org>

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

#ifndef AKONADI_CACHING_STORAGE_H
#define AKONADI_CACHING_STORAGE_H

#include "akonadistorageinterface.h"
#include "akonadicache.h"

namespace Akonadi {

class CachingStorage : public StorageInterface
{
public:
    explicit CachingStorage(const Cache::Ptr &cache, const StorageInterface::Ptr &storage);
    virtual ~CachingStorage();

    Akonadi::Collection defaultTaskCollection() Q_DECL_OVERRIDE;
    Akonadi::Collection defaultNoteCollection() Q_DECL_OVERRIDE;

    KJob *createItem(Item item, Collection collection) Q_DECL_OVERRIDE;
    KJob *updateItem(Item item, QObject *parent = Q_NULLPTR) Q_DECL_OVERRIDE;
    KJob *removeItem(Akonadi::Item item) Q_DECL_OVERRIDE;
    KJob *removeItems(Item::List items, QObject *parent = Q_NULLPTR) Q_DECL_OVERRIDE;
    KJob *moveItem(Item item, Collection collection, QObject *parent = Q_NULLPTR) Q_DECL_OVERRIDE;
    KJob *moveItems(Item::List item, Collection collection, QObject *parent = Q_NULLPTR) Q_DECL_OVERRIDE;

    KJob *createCollection(Collection collection, QObject *parent = Q_NULLPTR) Q_DECL_OVERRIDE;
    KJob *updateCollection(Collection collection, QObject *parent = Q_NULLPTR) Q_DECL_OVERRIDE;
    KJob *removeCollection(Collection collection, QObject *parent = Q_NULLPTR) Q_DECL_OVERRIDE;

    KJob *createTransaction() Q_DECL_OVERRIDE;

    KJob *createTag(Akonadi::Tag tag) Q_DECL_OVERRIDE;
    KJob *updateTag(Akonadi::Tag tag) Q_DECL_OVERRIDE;
    KJob *removeTag(Akonadi::Tag tag) Q_DECL_OVERRIDE;

    CollectionFetchJobInterface *fetchCollections(Akonadi::Collection collection, FetchDepth depth, FetchContentTypes types) Q_DECL_OVERRIDE;
    ItemFetchJobInterface *fetchItems(Akonadi::Collection collection) Q_DECL_OVERRIDE;
    ItemFetchJobInterface *fetchItem(Akonadi::Item item) Q_DECL_OVERRIDE;
    ItemFetchJobInterface *fetchTagItems(Akonadi::Tag tag) Q_DECL_OVERRIDE;
    TagFetchJobInterface *fetchTags() Q_DECL_OVERRIDE;

private:
    Cache::Ptr m_cache;
    StorageInterface::Ptr m_storage;
};

}

#endif // AKONADI_CACHING_STORAGE_H
