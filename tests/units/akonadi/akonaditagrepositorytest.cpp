/* This file is part of Zanshin

   Copyright 2014 Kevin Ottens <ervin@kde.org>
   Copyright 2014 Franck Arrecot <franck.arrecot@gmail.com>

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

#include "utils/mockobject.h"

#include "testlib/akonadifakejobs.h"
#include "testlib/akonadifakemonitor.h"

#include "akonadi/akonaditagrepository.h"
#include "akonadi/akonadiserializerinterface.h"
#include "akonadi/akonadistorageinterface.h"

using namespace mockitopp;
using namespace mockitopp::matcher;

Q_DECLARE_METATYPE(Testlib::AkonadiFakeItemFetchJob*)
Q_DECLARE_METATYPE(Testlib::AkonadiFakeTagFetchJob*)

class AkonadiTagRepositoryTest : public QObject
{
    Q_OBJECT
private slots:
    void shouldCreateTag()
    {
        // GIVEN

        // A Tag and its corresponding Akonadi Tag
        Akonadi::Tag akonadiTag; // not existing yet
        auto tag = Domain::Tag::Ptr::create();

        // A mock creating job
        auto tagCreateJob = new FakeJob(this);

        // Storage mock returning the tagCreatejob
        Utils::MockObject<Akonadi::StorageInterface> storageMock;
        storageMock(&Akonadi::StorageInterface::createTag).when(akonadiTag)
                                                          .thenReturn(tagCreateJob);

        // Serializer mock
        Utils::MockObject<Akonadi::SerializerInterface> serializerMock;
        serializerMock(&Akonadi::SerializerInterface::createAkonadiTagFromTag).when(tag).thenReturn(akonadiTag);


        // WHEN
        QScopedPointer<Akonadi::TagRepository> repository(new Akonadi::TagRepository(storageMock.getInstance(),
                                                                                     serializerMock.getInstance()));
        repository->create(tag)->exec();

        //THEN
        QVERIFY(storageMock(&Akonadi::StorageInterface::createTag).when(akonadiTag).exactly(1));
    }

    void shouldRemoveTag()
    {
        // GIVEN
        Akonadi::Tag akonadiTag(42);
        auto tag = Domain::Tag::Ptr::create();
        tag->setProperty("tagId", qint64(42)); // must be set
        tag->setName(QStringLiteral("42"));

        // A mock of removal job
        auto tagRemoveJob = new FakeJob(this);

        // Storage mock returning the tagCreatejob
        Utils::MockObject<Akonadi::StorageInterface> storageMock;
        storageMock(&Akonadi::StorageInterface::removeTag).when(akonadiTag)
                                                          .thenReturn(tagRemoveJob);
        // Serializer mock
        Utils::MockObject<Akonadi::SerializerInterface> serializerMock;
        serializerMock(&Akonadi::SerializerInterface::createAkonadiTagFromTag).when(tag).thenReturn(akonadiTag);

        // WHEN
        QScopedPointer<Akonadi::TagRepository> repository(new Akonadi::TagRepository(storageMock.getInstance(),
                                                                                     serializerMock.getInstance()));
        repository->remove(tag)->exec();

        // THEN
        QVERIFY(storageMock(&Akonadi::StorageInterface::removeTag).when(akonadiTag).exactly(1));
    }

    void shouldAssociateNoteToTag()
    {
        // GIVEN
        auto tag = Domain::Tag::Ptr::create();
        Akonadi::Tag akonadiTag(42);

        auto note = Domain::Note::Ptr::create();
        Akonadi::Item noteItem(42);

        // A mock of update job
        auto itemModifyJob = new FakeJob(this);

        auto itemFetchJob = new Testlib::AkonadiFakeItemFetchJob(this);
        itemFetchJob->setItems(Akonadi::Item::List() << noteItem);

        // Storage mock returning the tagCreatejob
        Utils::MockObject<Akonadi::StorageInterface> storageMock;
        storageMock(&Akonadi::StorageInterface::updateItem).when(noteItem, Q_NULLPTR)
                                                           .thenReturn(itemModifyJob);

        storageMock(&Akonadi::StorageInterface::fetchItem).when(noteItem)
                                                          .thenReturn(itemFetchJob);
        // Serializer mock
        Utils::MockObject<Akonadi::SerializerInterface> serializerMock;
        serializerMock(&Akonadi::SerializerInterface::createAkonadiTagFromTag).when(tag).thenReturn(akonadiTag);
        serializerMock(&Akonadi::SerializerInterface::createItemFromNote).when(note).thenReturn(noteItem);

        // WHEN
        QScopedPointer<Akonadi::TagRepository> repository(new Akonadi::TagRepository(storageMock.getInstance(),
                                                                                     serializerMock.getInstance()));
        repository->associate(tag, note)->exec();

        // THEN
        QVERIFY(serializerMock(&Akonadi::SerializerInterface::createAkonadiTagFromTag).when(tag).exactly(1));
        QVERIFY(serializerMock(&Akonadi::SerializerInterface::createItemFromNote).when(note).exactly(1));

        QVERIFY(storageMock(&Akonadi::StorageInterface::updateItem).when(noteItem, Q_NULLPTR).exactly(1));
    }

    void shouldDissociateNoteFromTag()
    {
        // GIVEN
        Akonadi::Item item(42);
        Domain::Note::Ptr note(new Domain::Note);

        Akonadi::Tag akonadiTag(qint64(42));
        auto tag = Domain::Tag::Ptr::create();

        auto itemFetchJob = new Testlib::AkonadiFakeItemFetchJob(this);
        itemFetchJob->setExpectedError(KJob::KilledJobError);

        auto itemFetchJobFilled = new Testlib::AkonadiFakeItemFetchJob(this);
        itemFetchJobFilled->setItems(Akonadi::Item::List() << item);

        // A mock update job
        auto itemModifyJob = new FakeJob(this);

        // Storage mock returning the create job
        Utils::MockObject<Akonadi::StorageInterface> storageMock;
        storageMock(&Akonadi::StorageInterface::fetchItem).when(item)
                                                          .thenReturn(itemFetchJob)
                                                          .thenReturn(itemFetchJobFilled);
        storageMock(&Akonadi::StorageInterface::updateItem).when(item, Q_NULLPTR)
                                                           .thenReturn(itemModifyJob);

        // Serializer mock returning the item for the note
        Utils::MockObject<Akonadi::SerializerInterface> serializerMock;
        serializerMock(&Akonadi::SerializerInterface::createItemFromNote).when(note)
                                                                         .thenReturn(item);
        serializerMock(&Akonadi::SerializerInterface::createAkonadiTagFromTag).when(tag)
                                                                           .thenReturn(akonadiTag);
        // WHEN
        QScopedPointer<Akonadi::TagRepository> repository(new Akonadi::TagRepository(storageMock.getInstance(),
                                                                                     serializerMock.getInstance()));
        repository->dissociate(tag, note)->exec();

        // THEN
        QVERIFY(storageMock(&Akonadi::StorageInterface::fetchItem).when(item).exactly(1));
        QVERIFY(serializerMock(&Akonadi::SerializerInterface::createAkonadiTagFromTag).when(tag).exactly(0));
        QVERIFY(storageMock(&Akonadi::StorageInterface::updateItem).when(item, Q_NULLPTR).exactly(0));

        // WHEN
        repository->dissociate(tag, note)->exec();

        // THEN
        QVERIFY(storageMock(&Akonadi::StorageInterface::fetchItem).when(item).exactly(2));
        QVERIFY(serializerMock(&Akonadi::SerializerInterface::createAkonadiTagFromTag).when(tag).exactly(1));
        QVERIFY(storageMock(&Akonadi::StorageInterface::updateItem).when(item, Q_NULLPTR).exactly(1));
    }

    void shouldDissociateNoteFromAllTags_data()
    {
        QTest::addColumn<Akonadi::Item>("item");
        QTest::addColumn<Domain::Note::Ptr>("note");
        QTest::addColumn<Testlib::AkonadiFakeItemFetchJob*>("itemFetchJob");
        QTest::addColumn<bool>("execJob");

        Akonadi::Item item(42);
        auto note = Domain::Note::Ptr::create();

        auto itemFetchJob = new Testlib::AkonadiFakeItemFetchJob(this);
        itemFetchJob->setItems(Akonadi::Item::List() << item);
        QTest::newRow("nominal case") << item << note << itemFetchJob << true;

        itemFetchJob = new Testlib::AkonadiFakeItemFetchJob(this);
        itemFetchJob->setExpectedError(KJob::KilledJobError);
        QTest::newRow("task job error, cannot find task") << item << note << itemFetchJob << false;
    }

    void shouldDissociateNoteFromAllTags()
    {
        QFETCH(Akonadi::Item,item);
        QFETCH(Domain::Note::Ptr, note);
        QFETCH(Testlib::AkonadiFakeItemFetchJob*,itemFetchJob);
        QFETCH(bool,execJob);

        // A mock update job
        auto itemModifyJob = new FakeJob(this);

        // Storage mock returning the create job
        Utils::MockObject<Akonadi::StorageInterface> storageMock;
        storageMock(&Akonadi::StorageInterface::fetchItem).when(item)
                                                          .thenReturn(itemFetchJob);
        storageMock(&Akonadi::StorageInterface::updateItem).when(item, Q_NULLPTR)
                                                           .thenReturn(itemModifyJob);

        // Serializer mock returning the item for the task
        Utils::MockObject<Akonadi::SerializerInterface> serializerMock;
        serializerMock(&Akonadi::SerializerInterface::createItemFromNote).when(note)
                                                                         .thenReturn(item);

        // WHEN
        QScopedPointer<Akonadi::TagRepository> repository(new Akonadi::TagRepository(storageMock.getInstance(),
                                                                                     serializerMock.getInstance()));

        auto dissociateJob = repository->dissociateAll(note);

        if (execJob)
            dissociateJob->exec();

        // THEN
        QVERIFY(storageMock(&Akonadi::StorageInterface::fetchItem).when(item).exactly(1));
        if (execJob) {
            QVERIFY(storageMock(&Akonadi::StorageInterface::updateItem).when(item, Q_NULLPTR).exactly(1));
        } else {
            delete dissociateJob;
        }

        // Give a chance to itemFetchJob to delete itself
        // in case of an error (since it uses deleteLater() internally)
        QTest::qWait(10);
    }
};

ZANSHIN_TEST_MAIN(AkonadiTagRepositoryTest)

#include "akonaditagrepositorytest.moc"
