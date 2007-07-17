/*---------------------------------------------------------------------\
|								       |
|		       __   __	  ____ _____ ____		       |
|		       \ \ / /_ _/ ___|_   _|___ \		       |
|			\ V / _` \___ \ | |   __) |		       |
|			 | | (_| |___) || |  / __/		       |
|			 |_|\__,_|____/ |_| |_____|		       |
|								       |
|				core system			       |
|							 (C) SuSE GmbH |
\----------------------------------------------------------------------/

  File:	      YQPkgStatusFilterView.cc

  Author:     Stefan Hundhammer <sh@suse.de>

  Textdomain "packages-qt"

/-*/

#include <qcheckbox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qpushbutton.h>
#include <qgroupbox.h>

#define y2log_component "qt-pkg"
#include <ycp/y2log.h>

#include "YQPkgStatusFilterView.h"
#include "YQIconPool.h"
#include "YQi18n.h"
#include "QY2LayoutUtils.h"
#include "utf8.h"
#include "YQUI.h"


#define SPACING			6	// between subwidgets
#define MARGIN			4	// around the widget


YQPkgStatusFilterView::YQPkgStatusFilterView( QWidget * parent )
    : QVBox( parent )
{
    setMargin( MARGIN );
    setSpacing( SPACING );

    addVStretch( this );

#if 0
    // Headline
    QLabel * label = new QLabel( _( "Changes Overview" ), this );
    CHECK_PTR( label );
    label->setFont( YQUI::ui()->headingFont() );
#endif


    //
    // Packages with what status to show
    //

    QGroupBox * gbox = new QGroupBox( 3, Qt::Horizontal, _( "Show packages with status" ), this );
    CHECK_PTR( gbox );

    _showDel		= addStatusCheckBox( gbox, _( "Delete" ), 	YQIconPool::disabledPkgDel(),	   	true );
    _showInstall	= addStatusCheckBox( gbox, _( "Install" ), 	YQIconPool::disabledPkgInstall(),	true );
    _showUpdate		= addStatusCheckBox( gbox, _( "Update" ), 	YQIconPool::disabledPkgUpdate(),	true );
    _showAutoDel	= addStatusCheckBox( gbox, _( "Autodelete" ), 	YQIconPool::disabledPkgAutoDel(),	true );
    _showAutoInstall	= addStatusCheckBox( gbox, _( "Autoinstall" ), 	YQIconPool::disabledPkgAutoInstall(),   true );
    _showAutoUpdate	= addStatusCheckBox( gbox, _( "Autoupdate" ), 	YQIconPool::disabledPkgAutoUpdate(),	true );
    _showTaboo		= addStatusCheckBox( gbox, _( "Taboo" ), 	YQIconPool::disabledPkgTaboo(),	   	true );
    _showProtected	= addStatusCheckBox( gbox, _( "Protected" ), 	YQIconPool::disabledPkgProtected(),	true );

    addVSpacing( gbox, 8 );
    addHStretch( gbox ); // For the other columns of the QGroupBox ( prevent wraparound )
    addHStretch( gbox );

    _showKeepInstalled	= addStatusCheckBox( gbox, _( "Keep" ), 	  YQIconPool::disabledPkgKeepInstalled(), false );
    _showNoInst		= addStatusCheckBox( gbox, _( "Do not install" ), YQIconPool::disabledPkgNoInst(),	  false );

    addVStretch( this );


    // Box for refresh button
    QHBox * hbox = new QHBox( this );
    CHECK_PTR( hbox );

    addHStretch( hbox );

    // Refresh button
    _refreshButton = new QPushButton( _( "&Refresh List" ), hbox );
    CHECK_PTR( _refreshButton );
    addHStretch( hbox );

    connect( _refreshButton,	SIGNAL( clicked() ),
	     this,		SLOT  ( filter()  ) );

    for ( int i=0; i < 6; i++ )
	addVStretch( this );
}


YQPkgStatusFilterView::~YQPkgStatusFilterView()
{
    // NOP
}



QCheckBox *
YQPkgStatusFilterView::addStatusCheckBox( QWidget *		parent,
					  const QString &	text,
					  const QPixmap &	icon,
					  bool			initiallyChecked )
{
    QCheckBox * checkBox = new QCheckBox( text, parent );
    CHECK_PTR( checkBox );
    checkBox->setChecked( initiallyChecked );

    QLabel * label = new QLabel( parent );
    CHECK_PTR( label );
    label->setPixmap( icon );

    addHStretch( parent );

    connect( checkBox,	SIGNAL( clicked() ),
	     this,	SLOT  ( filter()  ) );

    return checkBox;
}


QSize
YQPkgStatusFilterView::minimumSizeHint() const
{
    return QSize( 0, 0 );
}


void
YQPkgStatusFilterView::filterIfVisible()
{
    if ( isVisible() )
	filter();
}


void
YQPkgStatusFilterView::filter()
{
    emit filterStart();

    for ( ZyppPoolIterator it = zyppPkgBegin();
	  it != zyppPkgEnd();
	  ++it )
    {
	ZyppSel selectable = *it;

	bool match =
	    check( selectable, selectable->candidateObj() ) ||
	    check( selectable, selectable->installedObj() );

	// If there is neither an installed nor a candidate package, check
	// any other instance.

	if ( ! match			  &&
	     ! selectable->candidateObj() &&
	     ! selectable->installedObj()   )
	    check( selectable,  selectable->theObj() );
    }

    emit filterFinished();
}


bool
YQPkgStatusFilterView::check( ZyppSel	selectable,
			      ZyppObj	zyppObj )
{
    bool match = false;

    if ( ! zyppObj )
	return false;

    switch ( selectable->status() )
    {
	case S_AutoDel:		match = _showAutoDel->isChecked();		break;
	case S_AutoInstall:	match = _showAutoInstall->isChecked();		break;
	case S_AutoUpdate:	match = _showAutoUpdate->isChecked();		break;
	case S_Del:		match = _showDel->isChecked();			break;
	case S_Install:		match = _showInstall->isChecked();		break;
	case S_KeepInstalled:	match = _showKeepInstalled->isChecked();	break;
	case S_NoInst:		match = _showNoInst->isChecked();		break;
	case S_Protected:	match = _showProtected->isChecked();		break;
	case S_Taboo:		match = _showTaboo->isChecked();		break;
	case S_Update:		match = _showUpdate->isChecked();		break;

	    // Intentionally omitting 'default' branch so the compiler can
	    // catch unhandled enum states
    }

    if ( match )
    {
	ZyppPkg zyppPkg = tryCastToZyppPkg( zyppObj );

	if ( zyppPkg )
	    emit filterMatch( selectable, zyppPkg );
    }

    return match;
}



void YQPkgStatusFilterView::clear()
{
    _showDel->setChecked( false );
    _showInstall->setChecked( false );
    _showUpdate->setChecked( false );
    _showAutoDel->setChecked( false );
    _showAutoInstall->setChecked( false );
    _showAutoUpdate->setChecked( false );
    _showTaboo->setChecked( false );
    _showProtected->setChecked( false );
    _showKeepInstalled->setChecked( false );
    _showNoInst->setChecked( false );
}


void YQPkgStatusFilterView::showTransactions()
{
    showManualTransactions();
    showAutoTransactions();
}


void YQPkgStatusFilterView::showManualTransactions()
{
    _showDel->setChecked( true );
    _showInstall->setChecked( true );
    _showUpdate->setChecked( true );
}


void YQPkgStatusFilterView::showAutoTransactions()
{
    _showAutoDel->setChecked( true );
    _showAutoInstall->setChecked( true );
    _showAutoUpdate->setChecked( true );
}


void YQPkgStatusFilterView::showLocks()
{
    _showTaboo->setChecked( true );
    _showProtected->setChecked( true );
}


void YQPkgStatusFilterView::showInstalled()
{
    _showKeepInstalled->setChecked( true );
}


void YQPkgStatusFilterView::showNotInstalled()
{
    _showNoInst->setChecked( true );
}



#include "YQPkgStatusFilterView.moc"