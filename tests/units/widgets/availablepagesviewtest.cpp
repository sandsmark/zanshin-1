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

#include <QtTestGui>

#include <QAbstractItemModel>
#include <QAction>
#include <QHeaderView>
#include <QStringListModel>
#include <QToolBar>
#include <QTreeView>

#include "presentation/metatypes.h"

#include "widgets/availablepagesview.h"
#include "widgets/newpagedialog.h"

class NewPageDialogStub : public Widgets::NewPageDialogInterface
{
public:
    typedef QSharedPointer<NewPageDialogStub> Ptr;

    explicit NewPageDialogStub()
        : parent(0),
          execCount(0),
          sourceModel(0),
          source(Domain::DataSource::Ptr::create())
    {
    }

    int exec()
    {
        execCount++;
        return QDialog::Accepted;
    }

    void setDataSourcesModel(QAbstractItemModel *model)
    {
        sourceModel = model;
    }

    void setDefaultSource(const Domain::DataSource::Ptr &source)
    {
        defaultSource = source;
    }

    QString name() const
    {
        return "name";
    }

    Domain::DataSource::Ptr dataSource() const
    {
        return source;
    }

    QWidget *parent;
    int execCount;
    QAbstractItemModel *sourceModel;
    Domain::DataSource::Ptr defaultSource;
    Domain::DataSource::Ptr source;
};

class AvailablePagesModelStub : public QObject
{
    Q_OBJECT
public:
    explicit AvailablePagesModelStub(QObject *parent = 0)
        : QObject(parent)
    {
    }
public slots:
    void addProject(const QString &name, const Domain::DataSource::Ptr &source)
    {
        projectNames << name;
        sources << source;
    }

public:
    QStringList projectNames;
    QList<Domain::DataSource::Ptr> sources;
};

class AvailablePagesViewTest : public QObject
{
    Q_OBJECT
private slots:
    void shouldHaveDefaultState()
    {
        Widgets::AvailablePagesView available;

        QVERIFY(!available.model());
        QVERIFY(!available.projectSourcesModel());
        QVERIFY(available.defaultProjectSource().isNull());

        auto pagesView = available.findChild<QTreeView*>("pagesView");
        QVERIFY(pagesView);
        QVERIFY(pagesView->isVisibleTo(&available));
        QVERIFY(!pagesView->header()->isVisibleTo(&available));
        QCOMPARE(pagesView->dragDropMode(), QTreeView::DropOnly);

        auto actionBar = available.findChild<QToolBar*>("actionBar");
        QVERIFY(actionBar);
        QVERIFY(actionBar->isVisibleTo(&available));

        auto addAction = available.findChild<QAction*>("addAction");
        QVERIFY(addAction);

        auto factory = available.dialogFactory();
        QVERIFY(factory(&available).dynamicCast<Widgets::NewPageDialog>());
    }

    void shouldDisplayListFromPageModel()
    {
        // GIVEN
        QStringListModel model(QStringList() << "A" << "B" << "C" );

        QObject stubPagesModel;
        stubPagesModel.setProperty("pageListModel", QVariant::fromValue(static_cast<QAbstractItemModel*>(&model)));

        Widgets::AvailablePagesView available;
        auto pagesView = available.findChild<QTreeView*>("pagesView");
        QVERIFY(pagesView);
        QVERIFY(!pagesView->model());

        // WHEN
        available.setModel(&stubPagesModel);

        // THEN
        QCOMPARE(pagesView->model(), &model);
    }

    void shouldAddNewProjects()
    {
        // GIVEN
        AvailablePagesModelStub model;
        QStringListModel sourceModel;
        auto dialogStub = NewPageDialogStub::Ptr::create();

        auto source = Domain::DataSource::Ptr::create();

        Widgets::AvailablePagesView available;
        available.setModel(&model);
        available.setProjectSourcesModel(&sourceModel);
        available.setDefaultProjectSource(source);
        available.setDialogFactory([dialogStub] (QWidget *parent) {
            dialogStub->parent = parent;
            return dialogStub;
        });

        auto addAction = available.findChild<QAction*>("addAction");

        // WHEN
        addAction->trigger();

        // THEN
        QCOMPARE(dialogStub->execCount, 1);
        QCOMPARE(dialogStub->parent, &available);
        QCOMPARE(dialogStub->sourceModel, &sourceModel);
        QCOMPARE(dialogStub->defaultSource, source);
        QCOMPARE(model.projectNames.size(), 1);
        QCOMPARE(model.projectNames.first(), dialogStub->name());
        QCOMPARE(model.sources.size(), 1);
        QCOMPARE(model.sources.first(), dialogStub->dataSource());
        QCOMPARE(available.defaultProjectSource(), dialogStub->dataSource());
    }
};

QTEST_MAIN(AvailablePagesViewTest)

#include "availablepagesviewtest.moc"
