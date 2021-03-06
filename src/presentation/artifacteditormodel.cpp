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


#include "artifacteditormodel.h"

#include <QTimer>

#include <KLocalizedString>

#include "domain/task.h"
#include "errorhandler.h"

using namespace Presentation;

ArtifactEditorModel::ArtifactEditorModel(QObject *parent)
    : QObject(parent),
      m_done(false),
      m_saveTimer(new QTimer(this)),
      m_saveNeeded(false),
      m_editingInProgress(false)
{
    m_saveTimer->setSingleShot(true);
    m_saveTimer->setInterval(autoSaveDelay());
    connect(m_saveTimer, &QTimer::timeout, this, &ArtifactEditorModel::save);
}

ArtifactEditorModel::~ArtifactEditorModel()
{
    save();
}
Domain::Artifact::Ptr ArtifactEditorModel::artifact() const
{
    return m_artifact;
}

void ArtifactEditorModel::setArtifact(const Domain::Artifact::Ptr &artifact)
{
    if (m_artifact == artifact)
        return;

    save();

    m_text = QString();
    m_title = QString();
    m_done = false;
    m_start = QDateTime();
    m_due = QDateTime();
    m_delegateText = QString();

    if (m_artifact)
        disconnect(m_artifact.data(), Q_NULLPTR, this, Q_NULLPTR);

    m_artifact = artifact;

    if (m_artifact) {
        m_text = m_artifact->text();
        m_title = m_artifact->title();

        connect(m_artifact.data(), &Domain::Artifact::textChanged, this, &ArtifactEditorModel::onTextChanged);
        connect(m_artifact.data(), &Domain::Artifact::titleChanged, this, &ArtifactEditorModel::onTitleChanged);
    }

    if (auto task = artifact.objectCast<Domain::Task>()) {
        m_done = task->isDone();
        m_start = task->startDate();
        m_due = task->dueDate();
        m_delegateText = task->delegate().display();

        connect(task.data(), &Domain::Task::doneChanged, this, &ArtifactEditorModel::onDoneChanged);
        connect(task.data(), &Domain::Task::startDateChanged, this, &ArtifactEditorModel::onStartDateChanged);
        connect(task.data(), &Domain::Task::dueDateChanged, this, &ArtifactEditorModel::onDueDateChanged);
        connect(task.data(), &Domain::Task::delegateChanged, this, &ArtifactEditorModel::onDelegateChanged);
    }

    emit textChanged(m_text);
    emit titleChanged(m_title);
    emit doneChanged(m_done);
    emit startDateChanged(m_start);
    emit dueDateChanged(m_due);
    emit delegateTextChanged(m_delegateText);
    emit hasTaskPropertiesChanged(hasTaskProperties());
    emit artifactChanged(m_artifact);
}

bool ArtifactEditorModel::hasSaveFunction() const
{
    return bool(m_saveFunction);
}

void ArtifactEditorModel::setSaveFunction(const SaveFunction &function)
{
    m_saveFunction = function;
}

bool ArtifactEditorModel::hasDelegateFunction() const
{
    return bool(m_delegateFunction);
}

void ArtifactEditorModel::setDelegateFunction(const DelegateFunction &function)
{
    m_delegateFunction = function;
}

bool ArtifactEditorModel::hasTaskProperties() const
{
    return m_artifact.objectCast<Domain::Task>();
}

QString ArtifactEditorModel::text() const
{
    return m_text;
}

QString ArtifactEditorModel::title() const
{
    return m_title;
}

bool ArtifactEditorModel::isDone() const
{
    return m_done;
}

QDateTime ArtifactEditorModel::startDate() const
{
    return m_start;
}

QDateTime ArtifactEditorModel::dueDate() const
{
    return m_due;
}

QString ArtifactEditorModel::delegateText() const
{
    return m_delegateText;
}

int ArtifactEditorModel::autoSaveDelay()
{
    return 500;
}

bool ArtifactEditorModel::editingInProgress() const
{
    return m_editingInProgress;
}

void ArtifactEditorModel::setText(const QString &text)
{
    if (m_text == text)
        return;
    applyNewText(text);
    setSaveNeeded(true);
}

void ArtifactEditorModel::setTitle(const QString &title)
{
    if (m_title == title)
        return;
    applyNewTitle(title);
    setSaveNeeded(true);
}

void ArtifactEditorModel::setDone(bool done)
{
    if (m_done == done)
        return;
    applyNewDone(done);
    setSaveNeeded(true);
}

void ArtifactEditorModel::setStartDate(const QDateTime &start)
{
    if (m_start == start)
        return;
    applyNewStartDate(start);
    setSaveNeeded(true);
}

void ArtifactEditorModel::setDueDate(const QDateTime &due)
{
    if (m_due == due)
        return;
    applyNewDueDate(due);
    setSaveNeeded(true);
}

void ArtifactEditorModel::delegate(const QString &name, const QString &email)
{
    auto task = m_artifact.objectCast<Domain::Task>();
    Q_ASSERT(task);
    auto delegate = Domain::Task::Delegate(name, email);
    m_delegateFunction(task, delegate);
}

void ArtifactEditorModel::setEditingInProgress(bool editing)
{
    m_editingInProgress = editing;
}

void ArtifactEditorModel::onTextChanged(const QString &text)
{
    if (!m_editingInProgress)
        applyNewText(text);
}

void ArtifactEditorModel::onTitleChanged(const QString &title)
{
    if (!m_editingInProgress)
        applyNewTitle(title);
}

void ArtifactEditorModel::onDoneChanged(bool done)
{
    if (!m_editingInProgress)
        applyNewDone(done);
}

void ArtifactEditorModel::onStartDateChanged(const QDateTime &start)
{
    if (!m_editingInProgress)
        applyNewStartDate(start);
}

void ArtifactEditorModel::onDueDateChanged(const QDateTime &due)
{
    if (!m_editingInProgress)
        applyNewDueDate(due);
}

void ArtifactEditorModel::onDelegateChanged(const Domain::Task::Delegate &delegate)
{
    m_delegateText = delegate.display();
    emit delegateTextChanged(m_delegateText);
}

void ArtifactEditorModel::save()
{
    if (!isSaveNeeded())
        return;

    Q_ASSERT(m_artifact);

    const auto currentTitle = m_artifact->title();
    m_artifact->setTitle(m_title);
    m_artifact->setText(m_text);

    if (auto task = m_artifact.objectCast<Domain::Task>()) {
        task->setDone(m_done);
        task->setStartDate(m_start);
        task->setDueDate(m_due);
    }

    const auto job = m_saveFunction(m_artifact);
    installHandler(job, i18n("Cannot modify task %1", currentTitle));
    setSaveNeeded(false);
}

void ArtifactEditorModel::setSaveNeeded(bool needed)
{
    if (needed)
        m_saveTimer->start();
    else
        m_saveTimer->stop();

    m_saveNeeded = needed;
}

bool ArtifactEditorModel::isSaveNeeded() const
{
    return m_saveNeeded;
}

void ArtifactEditorModel::applyNewText(const QString &text)
{
    m_text = text;
    emit textChanged(m_text);
}

void ArtifactEditorModel::applyNewTitle(const QString &title)
{
    m_title = title;
    emit titleChanged(m_title);
}

void ArtifactEditorModel::applyNewDone(bool done)
{
    m_done = done;
    emit doneChanged(m_done);
}

void ArtifactEditorModel::applyNewStartDate(const QDateTime &start)
{
    m_start = start;
    emit startDateChanged(m_start);
}

void ArtifactEditorModel::applyNewDueDate(const QDateTime &due)
{
    m_due = due;
    emit dueDateChanged(m_due);
}
