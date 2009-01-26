/* This file is part of Zanshin Todo.

   Copyright 2008-2009 Kevin Ottens <ervin@kde.org>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
   USA.
*/

#ifndef ZANSHIN_ACTIONLISTEDITOR_H
#define ZANSHIN_ACTIONLISTEDITOR_H

#include <QtCore/QModelIndex>
#include <QtGui/QWidget>

class ActionListModel;
class ActionListView;
class KAction;
class KActionCollection;
class KLineEdit;

class ActionListEditor : public QWidget
{
    Q_OBJECT

public:
    ActionListEditor(QWidget *parent, KActionCollection *ac);

    ActionListView *view() const;

public slots:
    void showNoProjectInbox();
    void focusOnProject(const QModelIndex &index);

    void showNoContextInbox();
    void focusOnContext(const QModelIndex &index);

private slots:
    void updateActions(const QModelIndex &index);
    void onAddActionRequested();
    void onRemoveAction();
    void focusActionEdit();

private:
    void setupActions(KActionCollection *ac);

    ActionListView *m_view;
    KLineEdit *m_addActionEdit;

    ActionListModel *m_model;

    KAction *m_add;
    KAction *m_remove;
    KAction *m_previous;
    KAction *m_next;
};

#endif

