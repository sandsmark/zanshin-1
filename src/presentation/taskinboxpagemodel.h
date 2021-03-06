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


#ifndef PRESENTATION_TASKINBOXPAGEMODEL_H
#define PRESENTATION_TASKINBOXPAGEMODEL_H

#include "presentation/pagemodel.h"

#include "domain/taskqueries.h"
#include "domain/taskrepository.h"

namespace Presentation {

class TaskInboxPageModel : public PageModel
{
    Q_OBJECT
public:
    explicit TaskInboxPageModel(const Domain::TaskQueries::Ptr &taskQueries,
                                const Domain::TaskRepository::Ptr &taskRepository,
                                QObject *parent = Q_NULLPTR);

    Domain::Artifact::Ptr addItem(const QString &title, const QModelIndex &parentIndex = QModelIndex()) Q_DECL_OVERRIDE;
    void removeItem(const QModelIndex &index) Q_DECL_OVERRIDE;
    void promoteItem(const QModelIndex &index) Q_DECL_OVERRIDE;

private:
    QAbstractItemModel *createCentralListModel() Q_DECL_OVERRIDE;

    Domain::TaskQueries::Ptr m_taskQueries;
    Domain::TaskRepository::Ptr m_taskRepository;
};

}

#endif // PRESENTATION_TASKINBOXPAGEMODEL_H
