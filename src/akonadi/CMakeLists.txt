set(akonadi_SRCS
    akonadiapplicationselectedattribute.cpp
    akonadicache.cpp
    akonadicachingstorage.cpp
    akonadicollectionfetchjobinterface.cpp
    akonadiconfigdialog.cpp
    akonadicontextqueries.cpp
    akonadicontextrepository.cpp
    akonadidatasourcequeries.cpp
    akonadidatasourcerepository.cpp
    akonadiitemfetchjobinterface.cpp
    akonadilivequeryhelpers.cpp
    akonadilivequeryintegrator.cpp
    akonadimessaging.cpp
    akonadimessaginginterface.cpp
    akonadimonitorimpl.cpp
    akonadimonitorinterface.cpp
    akonadinotequeries.cpp
    akonadinoterepository.cpp
    akonadiprojectqueries.cpp
    akonadiprojectrepository.cpp
    akonadiserializer.cpp
    akonadiserializerinterface.cpp
    akonadistorage.cpp
    akonadistorageinterface.cpp
    akonadistoragesettings.cpp
    akonaditagfetchjobinterface.cpp
    akonaditagqueries.cpp
    akonaditagrepository.cpp
    akonaditaskqueries.cpp
    akonaditaskrepository.cpp
    akonaditimestampattribute.cpp
)

add_library(akonadi STATIC ${akonadi_SRCS})
target_link_libraries(akonadi
    KF5::AkonadiCalendar
    KF5::AkonadiCore
    KF5::AkonadiNotes
    KF5::AkonadiWidgets
    KF5::Mime
    KF5::CalendarCore
    KF5::IdentityManagement
)
