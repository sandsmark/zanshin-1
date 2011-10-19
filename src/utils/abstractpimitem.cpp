/*
    <one line to give the library's name and an idea of what it does.>
    Copyright (C) <year>  <name of author>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/


#include "abstractpimitem.h"

#include <KMime/KMimeMessage>
#include <KCalCore/Incidence>
#include <KCalCore/Todo>
#include <KCalCore/Event>

#include <Nepomuk/Thing>
#include <Nepomuk/Variant>
#include <nepomuk/pimo.h>
#include <nepomuk/nie.h>
#include <nepomuk/nfo.h>

#include <Akonadi/EntityDisplayAttribute>
#include <akonadi/itemfetchjob.h>
#include <Akonadi/ItemFetchScope>
#include <Akonadi/ItemModifyJob>
#include <Akonadi/Monitor>
#include <Akonadi/Session>

#include "note.h"
#include "incidenceitem.h"
#include "tagmanager.h"
#include "pimitem.h"

#include "todo.h"
#include "event.h"
#include "htmldocument.h"
#include <nepomuk/resourcewatcher.h>
#include <Nepomuk/Vocabulary/NCAL>
#include <Soprano/Vocabulary/NAO>
#include <Nepomuk/Query/QueryServiceClient>
#include "queries.h"

AbstractPimItem::AbstractPimItem(QObject *parent)
: QObject(parent),
m_dataFetched(false),
m_itemOutdated(false),
m_monitor(0),
m_textIsRich(false),
m_titleIsRich(false)
{

}

AbstractPimItem::AbstractPimItem(const Akonadi::Item &item, QObject *parent)
: QObject(parent),
m_dataFetched(false),
m_itemOutdated(false),
m_monitor(0),
m_textIsRich(false),
m_titleIsRich(false)
{
    m_item = item;
}

AbstractPimItem::AbstractPimItem(AbstractPimItem &item, QObject* parent)
:   QObject(parent),
m_dataFetched(false),
m_itemOutdated(false),
m_monitor(0),
m_textIsRich(false),
m_titleIsRich(false)
{
    m_title = item.getTitle();
    m_textIsRich = item.textIsRich();
    m_text = item.getText();
    m_titleIsRich = item.titleIsRich();
    m_dataFetched = true;
}

AbstractPimItem::~AbstractPimItem()
{

}

void AbstractPimItem::enableMonitor()
{
    if (!m_item.isValid()) {
        kWarning() << "item is not valid, monitor not enabled";
        return;
    }
    if (m_monitor) {
        kDebug() << "monitor already enabled";
        return;
    }
    m_monitor = new Akonadi::Monitor(this);
    /*
     * when monitoring is enabled, we work for a longer period with the item, and also save the item, so we will need the full payload
     */
    m_monitor->itemFetchScope().fetchFullPayload();
    m_monitor->itemFetchScope().fetchAttribute<Akonadi::EntityDisplayAttribute>(true);
    Q_ASSERT(m_item.isValid());
    m_monitor->setItemMonitored(m_item);
    connect( m_monitor, SIGNAL(itemChanged(Akonadi::Item,QSet<QByteArray>)), this, SLOT(updateItem(Akonadi::Item,QSet<QByteArray>)));
    connect( m_monitor, SIGNAL(itemRemoved(Akonadi::Item)), this, SIGNAL(removed()));
    
    const Nepomuk::Thing &thing = PimItemUtils::getThing(m_item); //FIXME This is flawed as the thing does not necessarily already exist. We should use a query instead.
    if (thing.isValid()) {
        Nepomuk::ResourceWatcher *m_resourceWatcher = new Nepomuk::ResourceWatcher(this); //TODO use a propertycache instead
        m_resourceWatcher->addResource(thing);
        m_resourceWatcher->addProperty(Nepomuk::Vocabulary::PIMO::isRelated());
        m_resourceWatcher->addProperty(Soprano::Vocabulary::NAO::hasTag());
        connect(m_resourceWatcher, SIGNAL(propertyAdded(Nepomuk::Resource,Nepomuk::Types::Property,QVariant)), this, SLOT(propertyChanged(Nepomuk::Resource,Nepomuk::Types::Property,QVariant)));
        connect(m_resourceWatcher, SIGNAL(propertyRemoved(Nepomuk::Resource,Nepomuk::Types::Property,QVariant)), this, SLOT(propertyChanged(Nepomuk::Resource,Nepomuk::Types::Property,QVariant)));
        m_resourceWatcher->start();
    }
    
    Nepomuk::Query::QueryServiceClient *topicsClient = new Nepomuk::Query::QueryServiceClient(this);
    connect(topicsClient, SIGNAL(newEntries(QList<Nepomuk::Query::Result>)), SLOT(newTopics(QList<Nepomuk::Query::Result>)));
    connect(topicsClient, SIGNAL(entriesRemoved(QList<QUrl>)), SLOT(topicsRemoved(QList<QUrl>)));
    topicsClient->sparqlQuery(MindMirrorQueries::itemTopicsQuery(m_item));

    //connect( m_monitor, SIGNAL(itemRemoved(Akonadi::Item)), SLOT(itemRemoved()) );
    kDebug() << "monitoring of item " << m_item.id() << " started";
}

void AbstractPimItem::newTopics(QList< Nepomuk::Query::Result > )
{
    emit changed(Topic);
}

void AbstractPimItem::topicsRemoved(QList< QUrl > )
{
    emit changed(Topic);
}

QMap< QUrl, QString > AbstractPimItem::topics()
{
    QMap<QUrl, QString> list;
    QList<Nepomuk::Query::Result> results = Nepomuk::Query::QueryServiceClient::syncSparqlQuery(MindMirrorQueries::itemTopicsQuery(m_item));
    foreach (const Nepomuk::Query::Result &result, results) {
        const Nepomuk::Resource &res = result.resource();
        list.insert(res.resourceUri(), res.label());
    }
    return list;
}


void AbstractPimItem::propertyChanged(Nepomuk::Resource resource, Nepomuk::Types::Property property, QVariant value)
{
    kDebug();
    if (property == Nepomuk::Types::Property(Nepomuk::Vocabulary::PIMO::isRelated())) {
        kDebug() << "is related " << value;
        emit changed(Topic);
    } else if (property == Nepomuk::Types::Property(Soprano::Vocabulary::NAO::hasTag())) {
        kDebug() << "has tag" << value;
        emit changed(Tags);
    } else {
        kDebug() << property;
        Q_ASSERT(0);
    }
/*
    Nepomuk::Thing thing = resource.pimoThing(); //Normally this should already be the thing (we're monitoring the thing
    if (thing != resource) {
        kWarning() << "thing != resource";
    }
    Q_ASSERT(thing.groundingOccurrences().size() >= 1);
    Q_ASSERT(thing.groundingOccurrences().first().property(Soprano::Vocabulary::NIE::uri()) == m_item.url());
    Q_ASSERT(thing.groundingOccurrences().first().property(Soprano::Vocabulary::NIE::uri()));
*/
}

AbstractPimItem::ItemType AbstractPimItem::itemType(const Akonadi::Item &item)
{
    Q_ASSERT(!item.mimeType().isEmpty());
    if (item.mimeType() == mimeType(Note)) {
        return Note;
    } else if (item.mimeType() == mimeType(Event)) {
        return Event;
    } else if (item.mimeType() == mimeType(Todo)) {
        return Todo;
    }
    kWarning() << "attention, unknown type" << item.mimeType();
    //Q_ASSERT(false);
    return Unknown;
}

void AbstractPimItem::setText(const QString &text, bool isRich)
{
    m_textIsRich = isRich;
    m_text = text;
}

QString AbstractPimItem::getText()
{
    fetchData();
    return m_text;
}

void AbstractPimItem::setTitle(const QString &text, bool isRich)
{
    m_titleIsRich = isRich;
    m_title = text;
    if (m_item.hasAttribute<Akonadi::EntityDisplayAttribute>()) {
        commitData(); //We need to commit already to update the EDA
    }
}

QString AbstractPimItem::getTitle()
{
    if (m_item.hasAttribute<Akonadi::EntityDisplayAttribute>()) {
        Akonadi::EntityDisplayAttribute *att = m_item.attribute<Akonadi::EntityDisplayAttribute>();
        m_title = att->displayName();
        return att->displayName();
    }
    fetchData();
    return m_title;
}

KDateTime AbstractPimItem::getLastModifiedDate()
{
    if (!m_item.isValid()) {
        kWarning() << "invalid item";
        return KDateTime();
    }
    return KDateTime(m_item.modificationTime(), KDateTime::LocalZone);
}


void AbstractPimItem::setCreationDate(const KDateTime &creationDate)
{
    m_creationDate = creationDate;
}

KDateTime AbstractPimItem::getCreationDate()
{
    fetchData();
    return m_creationDate;
}

QString AbstractPimItem::mimeType(AbstractPimItem::ItemType type)
{
    switch (type) {
        case Note:
            return QString::fromLatin1( "text/x-vnd.akonadi.note" );
        case Event:
            return QString::fromLatin1( "application/x-vnd.akonadi.calendar.event" );
        case Todo:
            return QString::fromLatin1( "application/x-vnd.akonadi.calendar.todo" );
        case Incidence:
            return QString::fromLatin1( "text/calendar" );
        default:
            kWarning() << "not implemented";
            Q_ASSERT(0);
    }
    return QString();
}

QStringList AbstractPimItem::mimeTypes()
{
    QStringList list;
    list << mimeType(Note);
    list << mimeType(Event);
    list << mimeType(Todo);
    return list;
}

QUrl AbstractPimItem::pimoType(AbstractPimItem::ItemType type)
{
    switch (type) {
        case Note:
            return Nepomuk::Vocabulary::NFO::HtmlDocument();
        case Event:
            return Nepomuk::Vocabulary::NCAL::Event();
        case Todo:
            return Nepomuk::Vocabulary::NCAL::Todo();
        default:
            kWarning() << "not implemented";
            Q_ASSERT(0);
    }
    return QUrl();
}


void AbstractPimItem::fetchPayload(bool blocking)
{
    if (hasValidPayload()) {
        kDebug() << "has validpayload";
        emit payloadFetchComplete();
        return;
    }
    kDebug() << "no valid payload, fetching...";
    Akonadi::ItemFetchJob *fetchJob = new Akonadi::ItemFetchJob(m_item, this);
    fetchJob->fetchScope().fetchFullPayload();
    connect( fetchJob, SIGNAL( result( KJob* ) ),this, SLOT( itemFetchDone( KJob* ) ) );
    if (blocking) {
        //fetchJob->exec();
        kWarning() << "not implemented";
        Q_ASSERT(0);
    }
    m_itemOutdated = true;
}

void AbstractPimItem::itemFetchDone( KJob *job )
{
    Akonadi::ItemFetchJob *fetchJob = static_cast<Akonadi::ItemFetchJob*>( job );
    if ( job->error() ) {
        kError() << job->errorString();
        return;
    }

    m_item = fetchJob->items().first();
    if ( !m_item.isValid() ) {
        kWarning() << "Item not valid";
        return;
    }
    Q_ASSERT(m_item.hasPayload());
    kDebug() << "item fetch complete";
    fetchData();
    m_itemOutdated = false;
    emit payloadFetchComplete();

}


bool AbstractPimItem::payloadFetched()
{
    if (m_item.hasPayload()) {
        return true;
    }
    return false;
}

const Akonadi::Item& AbstractPimItem::getItem() const
{
    if (m_itemOutdated) {
        kWarning() << "the item is outdated";
    }
    if (!m_item.isValid()) {
        kWarning() << "invalid item";
    }
    return m_item;
}

void AbstractPimItem::saveItem()
{
    kDebug();
    if (m_itemOutdated) {
        kWarning() << "item fetch in progress, cannot save without conflict";
        return;
    }
    
    if (!m_item.isValid()) {
        commitData(); //We still commit the data also to an invalid item (so we can create the item afterwards
        kWarning() << "invalid item";
        return;
    }

    if (!hasValidPayload()) { //TODO is this really invalid, or couldn't we save also if ther is no payload?
        kWarning() << "tried to save item without payload";
        return;
    }
    if (!m_dataFetched) {
        kDebug() << "data not fetched from payload yet, fetching";
        fetchData();
    }
    commitData();
    m_itemOutdated = true;

    //create a session which is ignored by this monitor, other items still receive the changed signal from the monitor
    //FIXME this doesn't work. Noone will receive signals about this change since everyone listens to the main session. At least thats what the tests say, according to the docs it should be different. Investigate.
    Akonadi::Session *session = new Akonadi::Session();
    if (m_monitor) {
        m_monitor->ignoreSession(session);
    }
    //kDebug() << m_item.revision();

    Akonadi::ItemModifyJob *modifyJob = new Akonadi::ItemModifyJob(m_item, session);
    connect(modifyJob, SIGNAL(result(KJob*)), SLOT(modifyDone(KJob*)) );
    connect(modifyJob, SIGNAL(result(KJob*)), session, SLOT(deleteLater())); //This will delete the session as soon as the job finished which will in turn delete the job
}


void AbstractPimItem::modifyDone( KJob *job )
{
    //Updateing the item does not help because the akonadi resource will modifiy the item again to update the remote revision,
    //which will result anyways in an outdate item here if the monitor is not enabled
    if ( job->error() ) {
        kWarning() << job->errorString();
        return;
    }
    Akonadi::ItemModifyJob *modjob = qobject_cast<Akonadi::ItemModifyJob*>(job);
    m_item = modjob->item();
    Q_ASSERT(m_item.isValid());
    kDebug() << "item modified";
}

/*
 * TODO
 * why are we updating the akonadi item?
 * 
 * if we would just remove the revision number, we could probably save without conflict.
 * Whis would then allow to factor out the monitor part to another class.
 */
void AbstractPimItem::updateItem(const Akonadi::Item &item, const QSet<QByteArray> &changes)
{
    kDebug() << "new item" << item.id() << item.revision() << item.modificationTime() << "old item" << m_item.id() << m_item.revision() << m_item.modificationTime();
    kDebug() << changes;
    Q_ASSERT(item.id() == m_item.id());

    if (changes.contains("REMOTEID") && changes.size() == 1) { //we don't care if the remoteid changed
        kDebug() << "remoteid changed";
        m_item = item;
        return;
    }

    int parts = 0;

    //TODO this sould always be true, right?
    if (item.modificationTime() != m_item.modificationTime()) {
        parts |= LastModifiedDate;
    }

    /* TODO we should check here if this really is the data which was modified in the last call to saveItem
     * It could be possible that we receive a foreign update (another app changed the same akonadi item),
     * during the time where we wait for the updated content after the item save
     */
    Q_ASSERT(item.isValid());
    m_item = item;
    m_itemOutdated = false;

    //TODO check what has changed, i.e we don't care about a change of the remoteid
    //TODO what about lastModified
    //FIXME the changes strings seem to be subject to change (PLD: -> PLD:RFC822), is there a not string based version?
    if (changes.contains("ATR:ENTITYDISPLAY") || changes.contains("PLD:RFC822")) { //only the displayattribute and the payload are relevant for the content
        const QString oldText = m_text;
        const QString oldTitle = m_title;
        //kDebug() << "old: " << m_creationDate << m_title << m_text;

        //TODO update local variable like m_text, m_title, etc. otherwise the changes are overwritten on the next save item
        //we have to figure out though which data was update in case i.e. the title is stored in the entitydisplayattribue and the payload
        m_dataFetched = false;
        fetchData(); //this loads the title from the payload
        parts |= Payload;

        if (changes.contains("ATR:ENTITYDISPLAY")) { //the entitydisplayattribue was modified, so it is more up to date than the title from the payload
            Q_ASSERT(m_item.hasAttribute<Akonadi::EntityDisplayAttribute>());
            Akonadi::EntityDisplayAttribute *att = m_item.attribute<Akonadi::EntityDisplayAttribute>();
            m_title = att->displayName();
            //kDebug() << "displayattribute was modified " << m_title;
        }
        kDebug() << "new: " << m_creationDate << m_title << m_text;

        if (oldText != m_text) {
            kDebug() << "text changed";
            parts |= Text;
        }
        if (oldText != m_title) {
            kDebug() << "title changed";
            parts |= Title;
        }
    }

    if (parts) {
        emit changed(static_cast<ChangedParts>(parts));
    }

}


bool AbstractPimItem::textIsRich()
{
    fetchData();
    return m_textIsRich;
}

bool AbstractPimItem::titleIsRich()
{
    fetchData();
    return m_titleIsRich;
}

