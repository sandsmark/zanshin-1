/* This file is part of Zanshin

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

#include <testlib/qtest_zanshin.h>

#include "akonadi/akonadicache.h"
#include "akonadi/akonadiserializer.h"

#include "testlib/akonadifakemonitor.h"
#include "testlib/gencollection.h"
#include "testlib/gentodo.h"
#include "testlib/gentag.h"

using namespace Testlib;

Q_DECLARE_METATYPE(Akonadi::StorageInterface::FetchContentTypes);

class AkonadiCacheTest : public QObject
{
    Q_OBJECT

private slots:
    void shouldHaveDefaultState()
    {
        // GIVEN
        auto monitor = AkonadiFakeMonitor::Ptr::create();
        auto cache = Akonadi::Cache::Ptr::create(Akonadi::SerializerInterface::Ptr(new Akonadi::Serializer), monitor);

        // THEN
        QVERIFY(!cache->isContentTypesPopulated(Akonadi::StorageInterface::AllContent));
        QVERIFY(!cache->isContentTypesPopulated(Akonadi::StorageInterface::Tasks));
        QVERIFY(!cache->isContentTypesPopulated(Akonadi::StorageInterface::Notes));
        QVERIFY(!cache->isContentTypesPopulated(Akonadi::StorageInterface::Notes|Akonadi::StorageInterface::Tasks));

        QVERIFY(cache->collections(Akonadi::StorageInterface::AllContent).isEmpty());
        QVERIFY(cache->collections(Akonadi::StorageInterface::Tasks).isEmpty());
        QVERIFY(cache->collections(Akonadi::StorageInterface::Notes).isEmpty());
        QVERIFY(cache->collections(Akonadi::StorageInterface::Notes|Akonadi::StorageInterface::Tasks).isEmpty());

        QVERIFY(!cache->isTagListPopulated());
        QVERIFY(cache->tags().isEmpty());
    }

    void shouldStoreCollectionsForTypesAndUpdate()
    {
        // GIVEN
        const auto noteCollection = Akonadi::Collection(GenCollection().withRootAsParent()
                                                                       .withId(1)
                                                                       .withName("notes")
                                                                       .withNoteContent());
        const auto taskCollection = Akonadi::Collection(GenCollection().withRootAsParent()
                                                                       .withId(2)
                                                                       .withName("tasks")
                                                                       .withTaskContent());
        const auto noteTaskCollection = Akonadi::Collection(GenCollection().withRootAsParent()
                                                                           .withId(3)
                                                                           .withName("tasks+notes")
                                                                           .withTaskContent()
                                                                           .withNoteContent());
        const auto stuffCollection = Akonadi::Collection(GenCollection().withRootAsParent()
                                                                        .withId(4)
                                                                        .withName("stuff"));

        auto monitor = AkonadiFakeMonitor::Ptr::create();
        auto cache = Akonadi::Cache::Ptr::create(Akonadi::Serializer::Ptr(new Akonadi::Serializer), monitor);

        // WHEN
        cache->setCollections(Akonadi::StorageInterface::AllContent,
                              Akonadi::Collection::List() << stuffCollection << noteTaskCollection
                                                          << taskCollection << noteCollection);

        // THEN
        QVERIFY(cache->isContentTypesPopulated(Akonadi::StorageInterface::AllContent));
        QVERIFY(cache->isCollectionKnown(stuffCollection.id()));
        QVERIFY(!cache->isCollectionPopulated(stuffCollection.id()));
        QVERIFY(cache->items(stuffCollection).isEmpty());
        QCOMPARE(cache->collections(Akonadi::StorageInterface::AllContent),
                 Akonadi::Collection::List() << stuffCollection << noteTaskCollection
                                             << taskCollection << noteCollection);
        QCOMPARE(cache->collection(stuffCollection.id()), stuffCollection);
        QCOMPARE(cache->collection(stuffCollection.id()).name(), stuffCollection.name());

        // WHEN
        monitor->changeCollection(GenCollection(stuffCollection).withName("stuff2"));

        // THEN
        QCOMPARE(cache->collection(stuffCollection.id()).name(), QStringLiteral("stuff2"));

        // WHEN
        cache->setCollections(Akonadi::StorageInterface::Notes|Akonadi::StorageInterface::Tasks,
                              Akonadi::Collection::List() << noteTaskCollection << taskCollection << noteCollection);

        // THEN
        QVERIFY(cache->isContentTypesPopulated(Akonadi::StorageInterface::Notes|Akonadi::StorageInterface::Tasks));
        QVERIFY(cache->isCollectionKnown(noteTaskCollection.id()));
        QVERIFY(!cache->isCollectionPopulated(noteTaskCollection.id()));
        QVERIFY(cache->items(noteTaskCollection).isEmpty());
        QCOMPARE(cache->collections(Akonadi::StorageInterface::Notes|Akonadi::StorageInterface::Tasks),
                 Akonadi::Collection::List() << noteTaskCollection << taskCollection << noteCollection);
        QCOMPARE(cache->collection(noteTaskCollection.id()), noteTaskCollection);
        QCOMPARE(cache->collection(noteTaskCollection.id()).name(), noteTaskCollection.name());

        // WHEN
        monitor->changeCollection(GenCollection(noteTaskCollection).withName("note+task2"));

        // THEN
        QCOMPARE(cache->collection(noteTaskCollection.id()).name(), QStringLiteral("note+task2"));

        // WHEN
        cache->setCollections(Akonadi::StorageInterface::Tasks,
                              Akonadi::Collection::List() << noteTaskCollection << taskCollection);

        // THEN
        QVERIFY(cache->isContentTypesPopulated(Akonadi::StorageInterface::Tasks));
        QVERIFY(cache->isCollectionKnown(taskCollection.id()));
        QVERIFY(!cache->isCollectionPopulated(taskCollection.id()));
        QVERIFY(cache->items(taskCollection).isEmpty());
        QCOMPARE(cache->collections(Akonadi::StorageInterface::Tasks),
                 Akonadi::Collection::List() << noteTaskCollection << taskCollection);
        QCOMPARE(cache->collection(taskCollection.id()), taskCollection);
        QCOMPARE(cache->collection(taskCollection.id()).name(), taskCollection.name());

        // WHEN
        monitor->changeCollection(GenCollection(taskCollection).withName("task2"));

        // THEN
        QCOMPARE(cache->collection(taskCollection.id()).name(), QStringLiteral("task2"));

        // WHEN
        cache->setCollections(Akonadi::StorageInterface::Notes,
                              Akonadi::Collection::List() << noteTaskCollection << noteCollection);

        // THEN
        QVERIFY(cache->isContentTypesPopulated(Akonadi::StorageInterface::Notes));
        QVERIFY(cache->isCollectionKnown(noteCollection.id()));
        QVERIFY(!cache->isCollectionPopulated(noteCollection.id()));
        QVERIFY(cache->items(noteCollection).isEmpty());
        QCOMPARE(cache->collections(Akonadi::StorageInterface::Notes),
                 Akonadi::Collection::List() << noteTaskCollection << noteCollection);
        QCOMPARE(cache->collection(noteCollection.id()), noteCollection);
        QCOMPARE(cache->collection(noteCollection.id()).name(), noteCollection.name());
    }

    void shouldHandleCollectionAdds_data()
    {
        QTest::addColumn<Akonadi::StorageInterface::FetchContentTypes>("contentTypes");
        QTest::addColumn<Akonadi::Collection>("collection");
        QTest::addColumn<bool>("seen");

        const auto allContent = Akonadi::StorageInterface::FetchContentTypes(Akonadi::StorageInterface::AllContent);
        const auto taskContent = Akonadi::StorageInterface::FetchContentTypes(Akonadi::StorageInterface::Tasks);
        const auto noteContent = Akonadi::StorageInterface::FetchContentTypes(Akonadi::StorageInterface::Notes);
        const auto taskNoteContent = Akonadi::StorageInterface::FetchContentTypes(Akonadi::StorageInterface::Notes
                                                                                | Akonadi::StorageInterface::Tasks);

        const auto none = Akonadi::Collection(GenCollection().withRootAsParent()
                                                             .withId(2)
                                                             .withName("collection"));
        const auto task = Akonadi::Collection(GenCollection(none).withTaskContent());
        const auto note = Akonadi::Collection(GenCollection(none).withNoteContent());
        const auto taskNote = Akonadi::Collection(GenCollection(none).withNoteContent().withTaskContent());

        QTest::newRow("all vs none") << allContent << none << true;
        QTest::newRow("all vs task") << allContent << task << true;
        QTest::newRow("all vs note") << allContent << note << true;
        QTest::newRow("all vs taskNote") << allContent << taskNote << true;
        QTest::newRow("tasks vs none") << taskContent << none << false;
        QTest::newRow("tasks vs task") << taskContent << task << true;
        QTest::newRow("tasks vs note") << taskContent << note << false;
        QTest::newRow("tasks vs taskNote") << taskContent << taskNote << true;
        QTest::newRow("notes vs none") << noteContent << none << false;
        QTest::newRow("notes vs task") << noteContent << task << false;
        QTest::newRow("notes vs note") << noteContent << note << true;
        QTest::newRow("notes vs taskNote") << noteContent << taskNote << true;
        QTest::newRow("notes+tasks vs none") << taskNoteContent << none << false;
        QTest::newRow("notes+tasks vs task") << taskNoteContent << task << true;
        QTest::newRow("notes+tasks vs note") << taskNoteContent << note << true;
        QTest::newRow("notes+tasks vs taskNote") << taskNoteContent << taskNote << true;
    }

    void shouldHandleCollectionAdds()
    {
        // GIVEN
        QFETCH(Akonadi::StorageInterface::FetchContentTypes, contentTypes);
        QFETCH(Akonadi::Collection, collection);

        auto monitor = AkonadiFakeMonitor::Ptr::create();
        auto cache = Akonadi::Cache::Ptr::create(Akonadi::Serializer::Ptr(new Akonadi::Serializer), monitor);

        // WHEN
        monitor->addCollection(collection);

        // THEN
        QVERIFY(!cache->isContentTypesPopulated(contentTypes));
        QVERIFY(cache->collections(contentTypes).isEmpty());
        QCOMPARE(cache->collection(collection.id()), Akonadi::Collection());

        // WHEN
        cache->setCollections(contentTypes, Akonadi::Collection::List());
        monitor->addCollection(collection);

        // THEN
        QVERIFY(cache->isContentTypesPopulated(contentTypes));
        QFETCH(bool, seen);
        if (seen) {
            QVERIFY(!cache->collections(contentTypes).isEmpty());
            QCOMPARE(cache->collection(collection.id()), collection);
            QCOMPARE(cache->collection(collection.id()).name(), collection.name());
        } else {
            QVERIFY(cache->collections(contentTypes).isEmpty());
            QCOMPARE(cache->collection(collection.id()), Akonadi::Collection());
        }
    }

    void shouldHandleCollectionChanges()
    {
        // GIVEN
        const auto collection = Akonadi::Collection(GenCollection().withRootAsParent()
                                                                   .withId(2)
                                                                   .withName("tasks")
                                                                   .withTaskContent());

        auto monitor = AkonadiFakeMonitor::Ptr::create();
        auto cache = Akonadi::Cache::Ptr::create(Akonadi::Serializer::Ptr(new Akonadi::Serializer), monitor);
        cache->setCollections(Akonadi::StorageInterface::Tasks, Akonadi::Collection::List() << collection);

        // WHEN
        const auto collection2 = Akonadi::Collection(GenCollection().withRootAsParent()
                                                                    .withId(2)
                                                                    .withName("tasks2")
                                                                    .withTaskContent());
        monitor->changeCollection(collection2);

        // THEN
        QCOMPARE(cache->collection(collection.id()).name(), QStringLiteral("tasks2"));
    }

    void shouldStoreTagsAndUpdate()
    {
        // GIVEN
        const auto tag1 = Akonadi::Tag(GenTag().withId(1).asPlain().withName("tag1"));
        const auto tag2 = Akonadi::Tag(GenTag().withId(2).asContext().withName("tag2"));

        auto monitor = AkonadiFakeMonitor::Ptr::create();
        auto cache = Akonadi::Cache::Ptr::create(Akonadi::Serializer::Ptr(new Akonadi::Serializer), monitor);

        // THEN
        QVERIFY(!cache->isTagKnown(tag1.id()));
        QVERIFY(!cache->isTagPopulated(tag1.id()));
        QVERIFY(!cache->isTagKnown(tag2.id()));
        QVERIFY(!cache->isTagPopulated(tag2.id()));

        // WHEN
        cache->setTags(Akonadi::Tag::List() << tag1 << tag2);

        // THEN
        QVERIFY(cache->isTagListPopulated());
        QCOMPARE(cache->tags(), Akonadi::Tag::List() << tag1 << tag2);
        QCOMPARE(cache->tag(tag1.id()), tag1);
        QCOMPARE(cache->tag(tag1.id()).name(), tag1.name());
        QCOMPARE(cache->tag(tag2.id()), tag2);
        QCOMPARE(cache->tag(tag2.id()).name(), tag2.name());
        QVERIFY(cache->isTagKnown(tag1.id()));
        QVERIFY(!cache->isTagPopulated(tag1.id()));
        QVERIFY(cache->isTagKnown(tag2.id()));
        QVERIFY(!cache->isTagPopulated(tag2.id()));

        // WHEN
        cache->setTags(Akonadi::Tag::List() << GenTag(tag1).withName("tag1bis") << tag2);

        // THEN
        QCOMPARE(cache->tags(), Akonadi::Tag::List() << tag1 << tag2);
        QCOMPARE(cache->tag(tag1.id()), tag1);
        QCOMPARE(cache->tag(tag1.id()).name(), QStringLiteral("tag1bis"));
        QCOMPARE(cache->tag(tag2.id()), tag2);
        QCOMPARE(cache->tag(tag2.id()).name(), tag2.name());
        QVERIFY(cache->isTagKnown(tag1.id()));
        QVERIFY(!cache->isTagPopulated(tag1.id()));
        QVERIFY(cache->isTagKnown(tag2.id()));
        QVERIFY(!cache->isTagPopulated(tag2.id()));

        // WHEN
        monitor->changeTag(GenTag(tag1).withName("tag1ter"));

        // THEN
        QCOMPARE(cache->tags(), Akonadi::Tag::List() << tag1 << tag2);
        QCOMPARE(cache->tag(tag1.id()), tag1);
        QCOMPARE(cache->tag(tag1.id()).name(), QStringLiteral("tag1ter"));
        QCOMPARE(cache->tag(tag2.id()), tag2);
        QCOMPARE(cache->tag(tag2.id()).name(), tag2.name());
        QVERIFY(cache->isTagKnown(tag1.id()));
        QVERIFY(!cache->isTagPopulated(tag1.id()));
        QVERIFY(cache->isTagKnown(tag2.id()));
        QVERIFY(!cache->isTagPopulated(tag2.id()));
    }

    void shouldHandleTagAdds()
    {
        // GIVEN
        const auto tag = Akonadi::Tag(GenTag().withId(1).asPlain().withName("tag"));

        auto monitor = AkonadiFakeMonitor::Ptr::create();
        auto cache = Akonadi::Cache::Ptr::create(Akonadi::Serializer::Ptr(new Akonadi::Serializer), monitor);

        // WHEN
        monitor->addTag(tag);

        // THEN
        QVERIFY(cache->tags().isEmpty());

        // WHEN
        cache->setTags(Akonadi::Tag::List());
        monitor->addTag(tag);

        // THEN
        QVERIFY(cache->isTagListPopulated());
        QCOMPARE(cache->tags(), Akonadi::Tag::List() << tag);
        QCOMPARE(cache->tag(tag.id()), tag);
        QCOMPARE(cache->tag(tag.id()).name(), QStringLiteral("tag"));
    }

    void shouldHandleTagChanges()
    {
        // GIVEN
        const auto tag = Akonadi::Tag(GenTag().withId(1).asPlain().withName("tag"));

        auto monitor = AkonadiFakeMonitor::Ptr::create();
        auto cache = Akonadi::Cache::Ptr::create(Akonadi::Serializer::Ptr(new Akonadi::Serializer), monitor);
        cache->setTags(Akonadi::Tag::List() << tag);

        // WHEN
        const auto tagbis = Akonadi::Tag(GenTag().withId(1).asPlain().withName("tagbis"));
        monitor->changeTag(tagbis);

        // THEN
        QCOMPARE(cache->tag(tag.id()).name(), QStringLiteral("tagbis"));
    }

    void shouldPopulateCollectionsWithItems()
    {
        // GIVEN
        const auto taskCollection1 = Akonadi::Collection(GenCollection().withRootAsParent()
                                                                        .withId(1)
                                                                        .withName("tasks1")
                                                                        .withTaskContent());
        const auto items1 = Akonadi::Item::List() << Akonadi::Item(GenTodo().withId(1).withTitle("item1"))
                                                  << Akonadi::Item(GenTodo().withId(2).withTitle("item2"));
        const auto taskCollection2 = Akonadi::Collection(GenCollection().withRootAsParent()
                                                                        .withId(2)
                                                                        .withName("tasks2")
                                                                        .withTaskContent());
        const auto items2 = Akonadi::Item::List() << Akonadi::Item(GenTodo().withId(3).withTitle("item3"))
                                                  << Akonadi::Item(GenTodo().withId(4).withTitle("item4"));

        auto monitor = AkonadiFakeMonitor::Ptr::create();
        auto serializer = Akonadi::Serializer::Ptr(new Akonadi::Serializer);
        auto cache = Akonadi::Cache::Ptr::create(serializer, monitor);
        cache->setCollections(Akonadi::StorageInterface::Tasks,
                              Akonadi::Collection::List() << taskCollection1 << taskCollection2);

        // WHEN
        cache->populateCollection(taskCollection1, items1);

        // THEN
        QVERIFY(cache->isCollectionPopulated(taskCollection1.id()));
        QCOMPARE(cache->items(taskCollection1), items1);
        QCOMPARE(cache->item(items1.at(0).id()), items1.at(0));
        QCOMPARE(cache->item(items1.at(1).id()), items1.at(1));

        // WHEN
        cache->populateCollection(taskCollection2, items2);

        // THEN
        QVERIFY(cache->isCollectionPopulated(taskCollection2.id()));
        QCOMPARE(cache->items(taskCollection2), items2);
        QCOMPARE(cache->item(items2.at(0).id()), items2.at(0));
        QCOMPARE(cache->item(items2.at(1).id()), items2.at(1));
    }

    void shouldHandleCollectionRemoves()
    {
        // GIVEN
        const auto tag = Akonadi::Tag(GenTag().withId(1).withName("tag"));
        const auto collection1 = Akonadi::Collection(GenCollection().withRootAsParent()
                                                                    .withId(1)
                                                                    .withName("tasks1")
                                                                    .withTaskContent());
        const auto items1 = Akonadi::Item::List() << Akonadi::Item(GenTodo().withId(1).withTitle("item1"))
                                                  << Akonadi::Item(GenTodo().withId(2).withTitle("item2"));
        const auto collection2 = Akonadi::Collection(GenCollection().withRootAsParent()
                                                                    .withId(2)
                                                                    .withName("tasks2")
                                                                    .withTaskContent());
        const auto items2 = Akonadi::Item::List() << Akonadi::Item(GenTodo().withId(3).withTitle("item3"))
                                                  << Akonadi::Item(GenTodo().withId(4).withTitle("item4"));

        auto monitor = AkonadiFakeMonitor::Ptr::create();
        auto cache = Akonadi::Cache::Ptr::create(Akonadi::Serializer::Ptr(new Akonadi::Serializer), monitor);
        cache->setCollections(Akonadi::StorageInterface::Tasks,
                              Akonadi::Collection::List() << collection1 << collection2);
        cache->populateCollection(collection1, items1);
        cache->populateCollection(collection2, items2);
        cache->populateTag(tag, items1 + items2);

        // WHEN
        monitor->removeCollection(collection1);

        // THEN
        QVERIFY(!cache->isCollectionPopulated(collection1.id()));
        QVERIFY(cache->items(collection1).isEmpty());
        QCOMPARE(cache->item(items1.at(0).id()), Akonadi::Item());
        QCOMPARE(cache->item(items1.at(1).id()), Akonadi::Item());

        QVERIFY(cache->isCollectionPopulated(collection2.id()));
        QCOMPARE(cache->items(collection2), items2);
        QCOMPARE(cache->item(items2.at(0).id()), items2.at(0));
        QCOMPARE(cache->item(items2.at(1).id()), items2.at(1));

        QVERIFY(cache->isTagPopulated(tag.id()));
        QCOMPARE(cache->items(tag), items2);
    }

    void shouldPopulateTagsWithItems()
    {
        // GIVEN
        const auto tag1 = Akonadi::Tag(GenTag().withId(1).withName("tag1"));
        const auto items1 = Akonadi::Item::List() << Akonadi::Item(GenTodo().withId(1).withTitle("item1"))
                                                  << Akonadi::Item(GenTodo().withId(2).withTitle("item2"));
        const auto tag2 = Akonadi::Tag(GenTag().withId(2).withName("tag2"));
        const auto items2 = Akonadi::Item::List() << Akonadi::Item(GenTodo().withId(3).withTitle("item3"))
                                                  << Akonadi::Item(GenTodo().withId(4).withTitle("item4"));

        auto monitor = AkonadiFakeMonitor::Ptr::create();
        auto serializer = Akonadi::Serializer::Ptr(new Akonadi::Serializer);
        auto cache = Akonadi::Cache::Ptr::create(serializer, monitor);
        cache->setTags(Akonadi::Tag::List() << tag1 << tag2);

        // WHEN
        cache->populateTag(tag1, items1);

        // THEN
        QVERIFY(cache->isTagPopulated(tag1.id()));
        QCOMPARE(cache->items(tag1), items1);
        QCOMPARE(cache->item(items1.at(0).id()), items1.at(0));
        QCOMPARE(cache->item(items1.at(1).id()), items1.at(1));

        // WHEN
        cache->populateTag(tag2, items2);

        // THEN
        QVERIFY(cache->isTagPopulated(tag2.id()));
        QCOMPARE(cache->items(tag2), items2);
        QCOMPARE(cache->item(items2.at(0).id()), items2.at(0));
        QCOMPARE(cache->item(items2.at(1).id()), items2.at(1));
    }

    void shouldHandleTagRemoves()
    {
        // GIVEN
        const auto collection = Akonadi::Collection(GenCollection().withRootAsParent()
                                                                   .withId(1)
                                                                   .withName("collection")
                                                                   .withTaskContent());
        const auto tag1 = Akonadi::Tag(GenTag().withId(1).withName("tag1"));
        const auto items1 = Akonadi::Item::List() << Akonadi::Item(GenTodo().withId(1).withTitle("item1"))
                                                  << Akonadi::Item(GenTodo().withId(2).withTitle("item2"));
        const auto tag2 = Akonadi::Tag(GenTag().withId(2).withName("tag2"));
        const auto items2 = Akonadi::Item::List() << Akonadi::Item(GenTodo().withId(3).withTitle("item3"))
                                                  << Akonadi::Item(GenTodo().withId(4).withTitle("item4"));

        auto monitor = AkonadiFakeMonitor::Ptr::create();
        auto cache = Akonadi::Cache::Ptr::create(Akonadi::Serializer::Ptr(new Akonadi::Serializer), monitor);
        cache->setTags(Akonadi::Tag::List() << tag1 << tag2);
        cache->populateCollection(collection, items1 + items2);
        cache->populateTag(tag1, items1);
        cache->populateTag(tag2, items2);

        // WHEN
        monitor->removeTag(tag1);

        // THEN
        QVERIFY(!cache->isTagPopulated(tag1.id()));
        QVERIFY(cache->items(tag1).isEmpty());
        QCOMPARE(cache->item(items1.at(0).id()), items1.at(0));
        QCOMPARE(cache->item(items1.at(1).id()), items1.at(1));

        QVERIFY(cache->isTagPopulated(tag2.id()));
        QCOMPARE(cache->items(tag2), items2);
        QCOMPARE(cache->item(items2.at(0).id()), items2.at(0));
        QCOMPARE(cache->item(items2.at(1).id()), items2.at(1));

        QVERIFY(cache->isCollectionPopulated(collection.id()));
        QCOMPARE(cache->items(collection), items1 + items2);
    }

    void shouldHandleItemChanges()
    {
        // GIVEN
        const auto collection1 = Akonadi::Collection(GenCollection().withRootAsParent()
                                                                    .withId(1)
                                                                    .withName("tasks1")
                                                                    .withTaskContent());
        const auto collection2 = Akonadi::Collection(GenCollection().withRootAsParent()
                                                                    .withId(2)
                                                                    .withName("tasks2")
                                                                    .withTaskContent());
        const auto collection3 = Akonadi::Collection(GenCollection().withRootAsParent()
                                                                    .withId(3)
                                                                    .withName("tasks3")
                                                                    .withTaskContent());
        const auto tag1 = Akonadi::Tag(GenTag().withId(1).withName("tag1"));
        const auto tag2 = Akonadi::Tag(GenTag().withId(2).withName("tag2"));
        const auto tag3 = Akonadi::Tag(GenTag().withId(3).withName("tag3"));
        const auto items = Akonadi::Item::List() << Akonadi::Item(GenTodo().withId(1).withParent(1).withTags({1}).withTitle("item1"))
                                                 << Akonadi::Item(GenTodo().withId(2).withParent(1).withTags({1}).withTitle("item2"));
        const auto item3 = Akonadi::Item(GenTodo().withId(3).withParent(1).withTags({1}).withTitle("item3"));


        auto monitor = AkonadiFakeMonitor::Ptr::create();
        auto serializer = Akonadi::Serializer::Ptr(new Akonadi::Serializer);
        auto cache = Akonadi::Cache::Ptr::create(serializer, monitor);
        cache->setCollections(Akonadi::StorageInterface::Tasks,
                              Akonadi::Collection::List() << collection1 << collection2 << collection3);
        cache->populateCollection(collection1, items);
        cache->populateCollection(collection2, Akonadi::Item::List());
        cache->populateTag(tag1, items);
        cache->populateTag(tag2, Akonadi::Item::List());

        // WHEN
        monitor->changeItem(GenTodo(items.at(0)).withTitle("item1bis"));

        // THEN
        auto todo = serializer->createTaskFromItem(cache->item(items.at(0).id()));
        QCOMPARE(todo->title(), QStringLiteral("item1bis"));

        QVERIFY(cache->isCollectionPopulated(collection1.id()));
        QVERIFY(cache->items(collection1).contains(items.at(0)));
        QVERIFY(cache->isCollectionPopulated(collection2.id()));
        QVERIFY(!cache->items(collection2).contains(items.at(0)));
        QVERIFY(!cache->isCollectionPopulated(collection3.id()));
        QVERIFY(!cache->items(collection3).contains(items.at(0)));

        QVERIFY(cache->isTagPopulated(tag1.id()));
        QVERIFY(cache->items(tag1).contains(items.at(0)));
        QVERIFY(cache->isTagPopulated(tag2.id()));
        QVERIFY(!cache->items(tag2).contains(items.at(0)));
        QVERIFY(!cache->isTagPopulated(tag3.id()));
        QVERIFY(!cache->items(tag3).contains(items.at(0)));

        // WHEN
        monitor->changeItem(GenTodo(items.at(0)).withParent(2));

        // THEN
        QVERIFY(cache->isCollectionPopulated(collection1.id()));
        QVERIFY(!cache->items(collection1).contains(items.at(0)));
        QVERIFY(cache->isCollectionPopulated(collection2.id()));
        QVERIFY(cache->items(collection2).contains(items.at(0)));
        QVERIFY(!cache->isCollectionPopulated(collection3.id()));
        QVERIFY(!cache->items(collection3).contains(items.at(0)));

        QVERIFY(cache->isTagPopulated(tag1.id()));
        QVERIFY(cache->items(tag1).contains(items.at(0)));
        QVERIFY(cache->isTagPopulated(tag2.id()));
        QVERIFY(!cache->items(tag2).contains(items.at(0)));
        QVERIFY(!cache->isTagPopulated(tag3.id()));
        QVERIFY(!cache->items(tag3).contains(items.at(0)));

        // WHEN
        monitor->changeItem(GenTodo(items.at(0)).withParent(3));

        // THEN
        QVERIFY(cache->isCollectionPopulated(collection1.id()));
        QVERIFY(!cache->items(collection1).contains(items.at(0)));
        QVERIFY(cache->isCollectionPopulated(collection2.id()));
        QVERIFY(!cache->items(collection2).contains(items.at(0)));
        QVERIFY(!cache->isCollectionPopulated(collection3.id()));
        QVERIFY(!cache->items(collection3).contains(items.at(0)));

        QVERIFY(cache->isTagPopulated(tag1.id()));
        QVERIFY(cache->items(tag1).contains(items.at(0)));
        QVERIFY(cache->isTagPopulated(tag2.id()));
        QVERIFY(!cache->items(tag2).contains(items.at(0)));
        QVERIFY(!cache->isTagPopulated(tag3.id()));
        QVERIFY(!cache->items(tag3).contains(items.at(0)));

        // WHEN
        monitor->changeItem(GenTodo().withId(1).withParent(2).withTags({2}).withTitle("item1"));

        // THEN
        QVERIFY(cache->isCollectionPopulated(collection1.id()));
        QVERIFY(!cache->items(collection1).contains(items.at(0)));
        QVERIFY(cache->isCollectionPopulated(collection2.id()));
        QVERIFY(cache->items(collection2).contains(items.at(0)));
        QVERIFY(!cache->isCollectionPopulated(collection3.id()));
        QVERIFY(!cache->items(collection3).contains(items.at(0)));

        QVERIFY(cache->isTagPopulated(tag1.id()));
        QVERIFY(!cache->items(tag1).contains(items.at(0)));
        QVERIFY(cache->isTagPopulated(tag2.id()));
        QVERIFY(cache->items(tag2).contains(items.at(0)));
        QVERIFY(!cache->isTagPopulated(tag3.id()));
        QVERIFY(!cache->items(tag3).contains(items.at(0)));

        // WHEN
        monitor->changeItem(item3);

        // THEN
        QVERIFY(cache->items(collection1).contains(item3));
        QVERIFY(cache->items(tag1).contains(item3));
    }

    void shouldHandleItemAdds()
    {
        // GIVEN
        const auto collection1 = Akonadi::Collection(GenCollection().withRootAsParent()
                                                                    .withId(1)
                                                                    .withName("tasks1")
                                                                    .withTaskContent());
        const auto collection2 = Akonadi::Collection(GenCollection().withRootAsParent()
                                                     .withId(2)
                                                     .withName("tasks2")
                                                     .withTaskContent());
        const auto tag1 = Akonadi::Tag(GenTag().withId(1).withName("tag1"));
        const auto tag2 = Akonadi::Tag(GenTag().withId(2).withName("tag2"));
        const auto item1 = Akonadi::Item(GenTodo().withId(1).withParent(1).withTags({1}).withTitle("item1"));
        const auto item2 = Akonadi::Item(GenTodo().withId(2).withParent(2).withTags({2}).withTitle("item2"));


        auto monitor = AkonadiFakeMonitor::Ptr::create();
        auto serializer = Akonadi::Serializer::Ptr(new Akonadi::Serializer);
        auto cache = Akonadi::Cache::Ptr::create(serializer, monitor);
        cache->setCollections(Akonadi::StorageInterface::Tasks,
                              Akonadi::Collection::List() << collection1 << collection2);
        cache->populateCollection(collection1, Akonadi::Item::List());
        cache->populateTag(tag1, Akonadi::Item::List());

        // WHEN
        monitor->addItem(item1);

        // THEN
        QVERIFY(cache->isCollectionPopulated(collection1.id()));
        QVERIFY(!cache->items(collection1).isEmpty());
        QCOMPARE(cache->items(collection1), Akonadi::Item::List() << item1);
        QVERIFY(!cache->isCollectionPopulated(collection2.id()));
        QVERIFY(cache->items(collection2).isEmpty());

        QVERIFY(cache->isTagPopulated(tag1.id()));
        QVERIFY(!cache->items(tag1).isEmpty());
        QCOMPARE(cache->items(tag1), Akonadi::Item::List() << item1);
        QVERIFY(!cache->isTagPopulated(tag2.id()));
        QVERIFY(cache->items(tag2).isEmpty());

        // WHEN
        monitor->addItem(item2);

        // THEN
        QVERIFY(cache->isCollectionPopulated(collection1.id()));
        QVERIFY(!cache->items(collection1).isEmpty());
        QCOMPARE(cache->items(collection1), Akonadi::Item::List() << item1);
        QVERIFY(!cache->isCollectionPopulated(collection2.id()));
        QVERIFY(cache->items(collection2).isEmpty());

        QVERIFY(cache->isTagPopulated(tag1.id()));
        QVERIFY(!cache->items(tag1).isEmpty());
        QCOMPARE(cache->items(tag1), Akonadi::Item::List() << item1);
        QVERIFY(!cache->isTagPopulated(tag2.id()));
        QVERIFY(cache->items(tag2).isEmpty());
    }

    void shouldHandleItemRemoves()
    {
        // GIVEN
        const auto collection = Akonadi::Collection(GenCollection().withRootAsParent()
                                                                   .withId(1)
                                                                   .withName("tasks")
                                                                   .withTaskContent());
        const auto tag = Akonadi::Tag(GenTag().withId(1).withName("tag"));
        const auto items = Akonadi::Item::List() << Akonadi::Item(GenTodo().withId(1).withTitle("item1"))
                                                  << Akonadi::Item(GenTodo().withId(2).withTitle("item2"));

        auto monitor = AkonadiFakeMonitor::Ptr::create();
        auto cache = Akonadi::Cache::Ptr::create(Akonadi::Serializer::Ptr(new Akonadi::Serializer), monitor);
        cache->setCollections(Akonadi::StorageInterface::Tasks,
                              Akonadi::Collection::List() << collection);
        cache->setTags(Akonadi::Tag::List() << tag);
        cache->populateCollection(collection, items);
        cache->populateTag(tag, items);

        // WHEN
        monitor->removeItem(items.at(0));

        // THEN
        QVERIFY(cache->isCollectionPopulated(collection.id()));
        QCOMPARE(cache->items(collection), Akonadi::Item::List() << items.at(1));
        QVERIFY(cache->isTagPopulated(tag.id()));
        QCOMPARE(cache->items(tag), Akonadi::Item::List() << items.at(1));
        QCOMPARE(cache->item(items.at(0).id()), Akonadi::Item());
        QCOMPARE(cache->item(items.at(1).id()), items.at(1));
    }
};

ZANSHIN_TEST_MAIN(AkonadiCacheTest)

#include "akonadicachetest.moc"
