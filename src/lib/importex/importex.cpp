/***** BEGIN LICENSE BLOCK *****

BSD License

Copyright (c) 2005-2015, NIF File Format Library and Tools
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the NIF File Format Library and Tools project may not be
   used to endorse or promote products derived from this software
   without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

***** END LICENCE BLOCK *****/

#include "nifskope.h"
#include "model/nifmodel.h"
#include "model/nifproxymodel.h"
#include "ui/widgets/nifview.h"

#include <QDockWidget>
#include <QFileInfo>
#include <QMenu>
#include <QModelIndex>


void exportObj( const NifModel * nif, const QModelIndex & index );
void exportCol( const NifModel * nif, QFileInfo );
void importObj( NifModel * nif, const QModelIndex & index, bool collision = false );
void import3ds( NifModel * nif, const QModelIndex & index );


void NifSkope::fillImportExportMenus()
{
	mExport->addAction( tr( "Export .OBJ" ) )->setShortcut( QKeySequence( Qt::CTRL + Qt::Key_E ) );
	//mExport->addAction( tr( "Export .DAE" ) );
	//mImport->addAction( tr( "Import .3DS" ) );
	mImport->addAction( tr( "Import .OBJ" ) )->setShortcut( QKeySequence( Qt::CTRL + Qt::Key_I ) );
	mImport->addAction( tr( "Import .OBJ as Collision" ) )->setShortcut( QKeySequence( Qt::CTRL + Qt::ALT + Qt::Key_I ) );
}

void NifSkope::sltImportExport( QAction * a )
{
	QModelIndex index;


	//Get the currently selected NiBlock index in the list or tree view
	if ( dList->isVisible() ) {
		if ( list->model() == proxy ) {
			index = proxy->mapTo( list->currentIndex() );
		} else if ( list->model() == nif ) {
			index = list->currentIndex();
		}
	} else if ( dTree->isVisible() ) {
		if ( tree->model() == proxy ) {
			index = proxy->mapTo( tree->currentIndex() );
		} else if ( tree->model() == nif ) {
			index = tree->currentIndex();
		}
	}

	if ( nif && nif->getVersionNumber() >= 0x14050000 ) {
		mExport->setDisabled( true );
		mImport->setDisabled( true );
		return;
	} else {
		mImport->setDisabled( false );
		mExport->setDisabled( false );

		if ( nif->getUserVersion2() >= 100 )
			mImport->actions().at(0)->setDisabled( true );
		else if ( nif->getUserVersion2() == 0 )
			mImport->actions().at(1)->setDisabled( true );
	}

	if ( a->text() == tr( "Export .OBJ" ) )
		exportObj( nif, index );
	else if ( a->text() == tr( "Import .OBJ" ) )
		importObj( nif, index );
	else if ( a->text() == tr( "Import .OBJ as Collision" ) )
		importObj( nif, index, true );
	//else if ( a->text() == tr( "Import .3DS" ) )
	//	import3ds( nif, index );
	//else if ( a->text() == tr( "Export .DAE" ) )
	//	exportCol( nif, currentFile );
}
