/* This file is part of Zanshin Todo.

   Copyright 2008-2010 Kevin Ottens <ervin@kde.org>
   Copyright 2008, 2009 Mario Bensi <nef@ipsquad.net>

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

#include "todohelpers.h"

#include <KDE/Akonadi/ItemCreateJob>
#include <KDE/KCalCore/Todo>

void TodoHelpers::addProject(const QString &summary, const Akonadi::Collection &collection)
{
    KCalCore::Todo::Ptr todo(new KCalCore::Todo());
    todo->setSummary(summary);
    todo->addComment("X-Zanshin-Project");

    Akonadi::Item item;
    item.setMimeType("application/x-vnd.akonadi.calendar.todo");
    item.setPayload<KCalCore::Todo::Ptr>(todo);

    Akonadi::ItemCreateJob *job = new Akonadi::ItemCreateJob(item, collection);
    job->start();
}

void TodoHelpers::addProject(const QString &summary, const Akonadi::Item &parentProject)
{
    KCalCore::Todo::Ptr todo(new KCalCore::Todo());
    todo->setSummary(summary);
    todo->addComment("X-Zanshin-Project");

    KCalCore::Todo::Ptr parentTodo = parentProject.payload<KCalCore::Todo::Ptr>();
    todo->setRelatedTo(parentTodo->uid());

    Akonadi::Item item;
    item.setMimeType("application/x-vnd.akonadi.calendar.todo");
    item.setPayload<KCalCore::Todo::Ptr>(todo);

    Akonadi::Collection collection = parentProject.parentCollection();

    Akonadi::ItemCreateJob *job = new Akonadi::ItemCreateJob(item, collection);
    job->start();
}

void TodoHelpers::addCategory(const QString &summary)
{

}

