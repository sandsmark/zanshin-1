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


#ifndef PRESENTATION_NOTEINBOXPAGEMODEL_H
#define PRESENTATION_NOTEINBOXPAGEMODEL_H

#include "presentation/pagemodel.h"

#include "domain/notequeries.h"
#include "domain/noterepository.h"

namespace Presentation {

class NoteInboxPageModel : public PageModel
{
    Q_OBJECT
public:
    explicit NoteInboxPageModel(const Domain::NoteQueries::Ptr &noteQueries,
                                const Domain::NoteRepository::Ptr &noteRepository,
                                QObject *parent = Q_NULLPTR);

    Domain::Artifact::Ptr addItem(const QString &title, const QModelIndex &parentIndex = QModelIndex()) Q_DECL_OVERRIDE;
    void removeItem(const QModelIndex &index) Q_DECL_OVERRIDE;
    void promoteItem(const QModelIndex &index) Q_DECL_OVERRIDE;

private:
    QAbstractItemModel *createCentralListModel() Q_DECL_OVERRIDE;

    Domain::NoteQueries::Ptr m_noteQueries;
    Domain::NoteRepository::Ptr m_noteRepository;
};

}

#endif // PRESENTATION_NOTEINBOXPAGEMODEL_H
