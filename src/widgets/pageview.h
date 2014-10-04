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


#ifndef WIDGETS_PAGEVIEW_H
#define WIDGETS_PAGEVIEW_H

#include <QWidget>

#include <QSharedPointer>

#include <functional>

#include "domain/artifact.h"

class QLineEdit;
class QModelIndex;
class QTreeView;
class QMessageBox;

namespace Widgets {

class FilterWidget;

class PageView : public QWidget
{
    Q_OBJECT
public:
    typedef std::function<int(const QString&, QWidget*)> AskConfirmationFunction;

    explicit PageView(QWidget *parent = 0);

    QObject *model() const;
    AskConfirmationFunction askConfirmationFunction() const;

public slots:
    void setModel(QObject *model);
    void setAskConfirmationFunction(const AskConfirmationFunction &factory);

signals:
    void currentArtifactChanged(const Domain::Artifact::Ptr &artifact);

private slots:
    void onEditingFinished();
    void onRemoveItemRequested();
    void onCurrentChanged(const QModelIndex &current);

private:
    QObject *m_model;
    FilterWidget *m_filterWidget;
    QTreeView *m_centralView;
    QLineEdit *m_quickAddEdit;
    AskConfirmationFunction m_askConfirmationFunction;
};

}

#endif // WIDGETS_PAGEVIEW_H
