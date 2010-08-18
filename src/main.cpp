/* This file is part of Zanshin Todo.

   Copyright 2008-2010 Kevin Ottens <ervin@kde.org>

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

#include <KDE/KAboutData>
#include <KDE/KApplication>
#include <KDE/KCmdLineArgs>
#include <KDE/KLocale>

#include <KDE/Akonadi/ChangeRecorder>
#include <KDE/Akonadi/Session>
#include <KDE/Akonadi/CollectionFetchScope>
#include <KDE/Akonadi/EntityTreeModel>
#include <KDE/Akonadi/EntityTreeView>
#include <KDE/Akonadi/ItemFetchScope>

#include "actionlistdelegate.h"
#include "todomodel.h"
#include "todocategoriesmodel.h"
#include "todotreemodel.h"
#include "selectionproxymodel.h"
#include "sidebarmodel.h"

template<class ProxyModel>
void createViews(TodoModel *baseModel)
{
    ProxyModel *proxy = new ProxyModel;
    proxy->setSourceModel(baseModel);

    SideBarModel *sidebarModel = new SideBarModel;
    sidebarModel->setSourceModel(proxy);

    QString className = proxy->metaObject()->className();

    Akonadi::EntityTreeView *sidebar = new Akonadi::EntityTreeView;
    sidebar->setWindowTitle(className+"/SideBar");
    sidebar->setModel(sidebarModel);
    sidebar->setSelectionMode(QAbstractItemView::ExtendedSelection);
    sidebar->show();

    SelectionProxyModel *selectionProxy = new SelectionProxyModel(sidebar->selectionModel());
    selectionProxy->setSourceModel(proxy);

    Akonadi::EntityTreeView *mainView = new Akonadi::EntityTreeView;
    mainView->setItemsExpandable(false);
    mainView->setWindowTitle(className);
    mainView->setModel(selectionProxy);

    QObject::connect(selectionProxy, SIGNAL(modelReset()),
                     mainView, SLOT(expandAll()));
    QObject::connect(selectionProxy, SIGNAL(layoutChanged()),
                     mainView, SLOT(expandAll()));
    QObject::connect(selectionProxy, SIGNAL(rowsInserted(QModelIndex, int, int)),
                     mainView, SLOT(expandAll()));

    mainView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    mainView->setItemDelegate(new ActionListDelegate(mainView));

    mainView->show();
}

int main(int argc, char **argv)
{
    KAboutData about("zanshin2", "zanshin2",
                     ki18n("Zanshin Todo"), "0.2",
                     ki18n("A Getting Things Done application which aims at getting your mind like water"),
                     KAboutData::License_GPL_V3,
                     ki18n("Copyright 2008-2010, Kevin Ottens <ervin@kde.org>"));

    about.addAuthor(ki18n("Kevin Ottens"),
                    ki18n("Lead Developer"),
                    "ervin@kde.org");

    about.addAuthor(ki18n("Mario Bensi"),
                    ki18n("Developer"),
                    "nef@ipsquad.net");

    //TODO: Remove once we have a proper icon
    about.setProgramIconName("office-calendar");

    KCmdLineArgs::init(argc, argv, &about);

    KApplication app;

    Akonadi::Session *session = new Akonadi::Session( "zanshin2" );

    Akonadi::ItemFetchScope itemScope;
    itemScope.fetchFullPayload();
    itemScope.setAncestorRetrieval(Akonadi::ItemFetchScope::All);

    Akonadi::CollectionFetchScope collectionScope;
    collectionScope.setAncestorRetrieval(Akonadi::CollectionFetchScope::All);

    Akonadi::ChangeRecorder *changeRecorder = new Akonadi::ChangeRecorder;
    changeRecorder->setCollectionMonitored( Akonadi::Collection::root() );
    changeRecorder->setMimeTypeMonitored( "application/x-vnd.akonadi.calendar.todo" );
    changeRecorder->setCollectionFetchScope( collectionScope );
    changeRecorder->setItemFetchScope( itemScope );
    changeRecorder->setSession( session );

    TodoModel *todoModel = new TodoModel( changeRecorder );
    Akonadi::EntityTreeView *view = new Akonadi::EntityTreeView;
    view->setWindowTitle("TodoModel");
    view->setModel(todoModel);
    view->show();

    createViews<TodoTreeModel>(todoModel);
    createViews<TodoCategoriesModel>(todoModel);

    return app.exec();
}

