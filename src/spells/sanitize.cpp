#include "spellbook.h"
#include "sanitize.h"
#include "spells/misc.h"

#include <QInputDialog>
#include <QAbstractButton>
#include <QPushButton>

#include <algorithm> // std::stable_sort


// Brief description is deliberately not autolinked to class Spell
/*! \file sanitize.cpp
 * \brief Sanity spells
 *
 * These spells are called by SpellBook::sanitize.
 *
 * All classes here inherit from the Spell class.
 */

//! Reorders blocks to put shapes before nodes (for Oblivion / FO3)
class spReorderLinks : public Spell
{
public:
	QString name() const override { return Spell::tr( "Reorder Link Arrays" ); }
	QString page() const override { return Spell::tr( "Sanitize" ); }
	bool sanity() const override { return true; }

	bool isApplicable( const NifModel * nif, const QModelIndex & index ) override
	{
		return ( !index.isValid() && ( nif->getVersionNumber() >= 0x14000004 && nif->getUserVersion2() > 0 ) );
	}

	//! Comparator for link sort.
	/**
	 * If booleans of the pair are not equal, sort based on the first boolean.
	 * For spReorderLinks this will determine a sort of geometry before nodes
	 * in the children links array.
	 */
	static bool compareChildLinks( const QPair<qint32, bool> & a, const QPair<qint32, bool> & b )
	{
		return a.first < b.first;
	}

	static bool compareChildLinksShapeTop(const QPair<qint32, bool>& a, const QPair<qint32, bool>& b)
	{
		return a.second != b.second ? a.second : a.first < b.first;
	}

	static bool compareChildLinksShapeBtm(const QPair<qint32, bool>& a, const QPair<qint32, bool>& b)
	{
		return a.second != b.second ? !a.second : a.first < b.first;
	}

	QModelIndex cast( NifModel * nif, const QModelIndex & ) override
	{
		for ( int n = 0; n < nif->getBlockCount(); n++ ) {
			QModelIndex iBlock = nif->getBlock( n );

			QModelIndex iNumChildren = nif->getIndex( iBlock, "Num Children" );
			QModelIndex iChildren = nif->getIndex( iBlock, "Children" );

			if ( iNumChildren.isValid() && iChildren.isValid() ) {
				QList<QPair<qint32, bool>> links;

				for ( int r = 0; r < nif->rowCount( iChildren ); r++ ) {
					qint32 l = nif->getLink( iChildren.child( r, 0 ) );

					if ( l >= 0 ) {
						links.append( QPair<qint32, bool>( l, nif->inherits(nif->getBlock(l), {"NiTriBasedGeom", "BSTriShape"}) ) );
					}
				}

				auto compareFn = compareChildLinksShapeBtm;
				if ( nif->getUserVersion2() < 83 )
					compareFn = compareChildLinksShapeTop;

				std::stable_sort( links.begin(), links.end(), compareFn );

				for ( int r = 0; r < links.count(); r++ ) {
					if ( links[r].first != nif->getLink( iChildren.child( r, 0 ) ) ) {
						nif->setLink( iChildren.child( r, 0 ), links[r].first );
					}
				}

				// update child count & array even if there are no rows (i.e. prune empty children)
				nif->set<int>( iNumChildren, links.count() );
				nif->updateArray( iChildren );
			}
		}

		return QModelIndex();
	}
};

REGISTER_SPELL( spReorderLinks );

//! Removes empty members from link arrays
class spSanitizeLinkArrays final : public Spell
{
public:
	QString name() const override final { return Spell::tr( "Collapse Link Arrays" ); }
	QString page() const override final { return Spell::tr( "Sanitize" ); }
	bool sanity() const override final { return true; }

	bool isApplicable( const NifModel * nif, const QModelIndex & index ) override final
	{
		Q_UNUSED( nif );
		return !index.isValid();
	}

	QModelIndex cast( NifModel * nif, const QModelIndex & ) override final
	{
		for ( int n = 0; n < nif->getBlockCount(); n++ ) {
			QModelIndex iBlock = nif->getBlock( n );

			spCollapseArray arrayCollapser;

			// remove empty children links
			QModelIndex iNumChildren = nif->getIndex( iBlock, "Num Children" );
			QModelIndex iChildren = nif->getIndex( iBlock, "Children" );
			arrayCollapser.numCollapser( nif, iNumChildren, iChildren );

			// remove empty property links
			QModelIndex iNumProperties = nif->getIndex( iBlock, "Num Properties" );
			QModelIndex iProperties = nif->getIndex( iBlock, "Properties" );
			arrayCollapser.numCollapser( nif, iNumProperties, iProperties );

			// remove empty extra data links
			QModelIndex iNumExtraData = nif->getIndex( iBlock, "Num Extra Data List" );
			QModelIndex iExtraData = nif->getIndex( iBlock, "Extra Data List" );
			arrayCollapser.numCollapser( nif, iNumExtraData, iExtraData );

			// remove empty modifier links (NiParticleSystem crashes Oblivion for those)
			QModelIndex iNumModifiers = nif->getIndex( iBlock, "Num Modifiers" );
			QModelIndex iModifiers = nif->getIndex( iBlock, "Modifiers" );
			arrayCollapser.numCollapser( nif, iNumModifiers, iModifiers );
		}

		return QModelIndex();
	}
};

REGISTER_SPELL( spSanitizeLinkArrays );

//! Cleans texture paths and removes of unnecessary data
class spCleanupTexturePaths final : public Spell
{
public:
	QString name() const override final { return Spell::tr( "Cleanup Texture Paths" ); }
	QString page() const override final { return Spell::tr( "Sanitize" ); }
	bool sanity() const override final { return true; }

	bool isApplicable( const NifModel * nif, const QModelIndex & index ) override final
	{
		return nif && !index.isValid();
	}

	QModelIndex cast( NifModel * nif, const QModelIndex & ) override final
	{
		for ( int i = 0; i < nif->getBlockCount(); i++ ) {
			auto iBSLSP = nif->getBlock( i, "BSLightingShaderProperty" );
			auto iBSSTS = nif->getBlock( i, "BSShaderTextureSet" );
			auto iBSSNLP = nif->getBlock( i, "BSShaderNoLightingProperty" );
			auto iBSESP = nif->getBlock( i, "BSEffectShaderProperty" );
			auto iNiST = nif->getBlock( i, "NiSourceTexture" );

			// adjust material file path (fallout 4 only)
			if ( nif->checkVersion( 0x14020007, 0x14020007 ) && nif->getUserVersion2() == 130 ) {

				if ( iBSLSP.isValid() ) {
					QModelIndex iMaterial = nif->getIndex( iBSLSP, "Name" );
					QString iFileName = nif->get<QString>( iMaterial ).replace( "/", "\\" );
					int pos = iFileName.indexOf( QString( "\\materials\\" ), 0, Qt::CaseInsensitive );

					if ( iMaterial.isValid() ) // adjust file path
						if ( pos == 0 ) {
							nif->set<QString>( iMaterial, iFileName.remove( 0, 1 ) );
						}
						else {
							nif->set<QString>( iMaterial, iFileName.replace( iFileName.left( pos + 1 ), "" ) );
						}

				}
			}

			if ( iBSSTS.isValid() ) {
				QModelIndex iTextures = nif->getIndex( iBSSTS, "Textures" );

				if ( iTextures.isValid() ) // adjust file paths
					for ( int i = 0; i < nif->rowCount( iTextures ); i++ ) {
						QString iFileName = nif->get<QString>( iTextures.child( i, 0 ) ).replace( "/", "\\" );
						int pos = iFileName.indexOf( QString( "\\textures\\" ), 0, Qt::CaseInsensitive );
						if ( pos == 0 ) {
							nif->set<QString>( iTextures.child( i, 0 ), iFileName.remove( 0, 1 ) );
						}
						else {
							nif->set<QString>( iTextures.child( i, 0 ), iFileName.replace( iFileName.left( pos + 1 ), "" ) );
						}
					}

			}

			if ( iBSSNLP.isValid() ) {
				QModelIndex iTexture = nif->getIndex( iBSSNLP, "File Name" );
				QString iFileName = nif->get<QString>( iTexture ).replace( "/", "\\" );
				int pos = iFileName.indexOf( QString( "\\textures\\" ), 0, Qt::CaseInsensitive );

				if ( iTexture.isValid() ) // adjust file path
					if ( pos == 0 ) {
						nif->set<QString>( iTexture, iFileName.remove( 0, 1 ) );
					}
					else {
						nif->set<QString>( iTexture, iFileName.replace( iFileName.left( pos + 1 ), "" ) );
					}

			}

			if ( iBSESP.isValid() ) {
				QModelIndex iTextureSource = nif->getIndex( iBSESP, "Source Texture" );
				QModelIndex iTextureGreyscale = nif->getIndex( iBSESP, "Greyscale Texture" );
				QModelIndex iTextureEnvMap = nif->getIndex( iBSESP, "Env Map Texture" );
				QModelIndex iTextureNormal = nif->getIndex( iBSESP, "Normal Texture" );
				QModelIndex iTextureEnvMask = nif->getIndex( iBSESP, "Env Mask Texture" );
				QString iFileNameSource = nif->get<QString>( iTextureSource ).replace( "/", "\\" );
				QString iFileNameGreyscale = nif->get<QString>( iTextureGreyscale ).replace( "/", "\\" );
				QString iFileNameEnvMap = nif->get<QString>( iTextureEnvMap ).replace( "/", "\\" );
				QString iFileNameNormal = nif->get<QString>( iTextureNormal ).replace( "/", "\\" );
				QString iFileNameEnvMask = nif->get<QString>( iTextureEnvMask ).replace( "/", "\\" );
				int posSource = iFileNameSource.indexOf( QString( "\\textures\\" ), 0, Qt::CaseInsensitive );
				int posGreyscale = iFileNameGreyscale.indexOf( QString( "\\textures\\" ), 0, Qt::CaseInsensitive );
				int posEnvMap = iFileNameEnvMap.indexOf( QString( "\\textures\\" ), 0, Qt::CaseInsensitive );
				int posNormal = iFileNameNormal.indexOf( QString( "\\textures\\" ), 0, Qt::CaseInsensitive );
				int posEnvMask = iFileNameEnvMask.indexOf( QString( "\\textures\\" ), 0, Qt::CaseInsensitive );

				if ( iTextureSource.isValid() ) // adjust source file path
					if ( posSource == 0 ) {
						nif->set<QString>( iTextureSource, iFileNameSource.remove( 0, 1 ) );
					}
					else {
						nif->set<QString>( iTextureSource, iFileNameSource.replace( iFileNameSource.left( posSource + 1 ), "" ) );
					}

				if ( iTextureGreyscale.isValid() ) // adjust greyscale file path
					if ( posGreyscale == 0 ) {
						nif->set<QString>( iTextureGreyscale, iFileNameGreyscale.remove( 0, 1 ) );
					}
					else {
						nif->set<QString>( iTextureGreyscale, iFileNameGreyscale.replace( iFileNameGreyscale.left( posGreyscale + 1 ), "" ) );
					}

				if ( iTextureEnvMap.isValid() ) // adjust env map file path
					if ( posEnvMap == 0 ) {
						nif->set<QString>( iTextureEnvMap, iFileNameEnvMap.remove( 0, 1 ) );
					}
					else {
						nif->set<QString>( iTextureEnvMap, iFileNameEnvMap.replace( iFileNameEnvMap.left( posEnvMap + 1 ), "" ) );
					}

				if ( iTextureNormal.isValid() ) // adjust normal file path
					if ( posNormal == 0 ) {
						nif->set<QString>( iTextureNormal, iFileNameNormal.remove( 0, 1 ) );
					}
					else {
						nif->set<QString>( iTextureNormal, iFileNameNormal.replace( iFileNameNormal.left( posNormal + 1 ), "" ) );
					}

				if ( iTextureEnvMask.isValid() ) // adjust env mask file path
					if ( posEnvMask == 0 ) {
						nif->set<QString>( iTextureEnvMask, iFileNameEnvMask.remove( 0, 1 ) );
					}
					else {
						nif->set<QString>( iTextureEnvMask, iFileNameEnvMask.replace( iFileNameEnvMask.left( posEnvMask + 1 ), "" ) );
					}

			}

			if ( iNiST.isValid() ) {
				QModelIndex iTexture = nif->getIndex( iNiST, "File Name" );
				QString iFileName = nif->get<QString>( iTexture ).replace( "/", "\\" );
				int pos = iFileName.indexOf( QString( "\\textures\\" ), 0, Qt::CaseInsensitive );

				if ( iTexture.isValid() ) // adjust file path
					if ( pos == 0 ) {
						nif->set<QString>( iTexture, iFileName.remove( 0, 1 ) );
					}
					else {
						nif->set<QString>( iTexture, iFileName.replace( iFileName.left( pos + 1 ), "" ) );
					}

				if ( nif->checkVersion( 0x14000005, 0x14000005 ) ) {
					// adjust format options (oblivion only)
					nif->set<int>( iNiST, "Pixel Layout", 6 );
					nif->set<int>( iNiST, "Use Mipmaps", 1 );
					nif->set<int>( iNiST, "Alpha Format", 3 );
					nif->set<int>( iNiST, "Unknown Byte", 1 );
					nif->set<int>( iNiST, "Unknown Byte 2", 1 );
				}

			}
		}

		return QModelIndex();
	}
};

REGISTER_SPELL( spCleanupTexturePaths );

//! Reorders blocks
bool spSanitizeBlockOrder::isApplicable( const NifModel *, const QModelIndex & index )
{
	// all files
	return !index.isValid();
}

// check whether the block is of a type that comes before the parent or not
bool spSanitizeBlockOrder::childBeforeParent( NifModel * nif, qint32 block )
{
	// get index to the block
	QModelIndex iBlock( nif->getBlock( block ) );
	// check its type
	return (
		nif->inherits( iBlock, "bhkRefObject" )
		&& !nif->inherits( iBlock, "bhkConstraint" )
	);
}

// build the nif tree at node block; the block itself and its children are recursively added to
// the newblocks list
void spSanitizeBlockOrder::addTree( NifModel * nif, qint32 block, QList<qint32> & newblocks )
{
	// is the block already added?
	if ( newblocks.contains( block ) )
		return;

	// special case: add bhkConstraint entities before bhkConstraint
	// (these are actually links, not refs)
	QModelIndex iBlock( nif->getBlock( block ) );

	if ( nif->inherits( iBlock, "bhkConstraint" ) ) {
		for ( const auto entity : nif->getLinkArray( iBlock, "Entities" ) ) {
			addTree( nif, entity, newblocks );
		}
	}


	// add all children of block that should be before block
	for ( const auto child : nif->getChildLinks( block ) ) {
		if ( childBeforeParent( nif, child ) )
			addTree( nif, child, newblocks ); // now add this child and all of its children
	}

	// add the block
	newblocks.append( block );
	// add all children of block that should be after block
	for ( const auto child : nif->getChildLinks( block ) ) {
		if ( !childBeforeParent( nif, child ) )
			addTree( nif, child, newblocks ); // now add this child and all of its children
	}
}

QModelIndex spSanitizeBlockOrder::cast( NifModel * nif, const QModelIndex & )
{
	// list of root blocks
	QList<qint32> rootblocks = nif->getRootLinks();

	// list of blocks that have been added
	// newblocks[0] is the block number of the block that must be
	// assigned number 0
	// newblocks[1] is the block number of the block that must be
	// assigned number 1
	// etc.
	QList<qint32> newblocks;

	// add blocks recursively
	for ( const auto rootblock : rootblocks )
	{
		addTree( nif, rootblock, newblocks );
	}

	// check whether all blocks have been added
	if ( nif->getBlockCount() != newblocks.size() ) {
		qCCritical( nsSpell ) << Spell::tr( "failed to sanitize blocks order, corrupt nif tree?" );
		return QModelIndex();
	}

	// invert mapping
	QVector<qint32> order( nif->getBlockCount() );

	for ( qint32 n = 0; n < newblocks.size(); n++ ) {
		order[newblocks[n]] = n;
		//qDebug() << n << newblocks[n];
	}

	// reorder the blocks
	nif->reorderBlocks( order );

	return QModelIndex();
}

REGISTER_SPELL( spSanitizeBlockOrder );

//! Checks that links are correct
class spSanityCheckLinks final : public Spell
{
public:
	QString name() const override final { return Spell::tr( "Check Links" ); }
	QString page() const override final { return Spell::tr( "Sanitize" ); }
	bool sanity() const override final { return true; }

	bool isApplicable( const NifModel * nif, const QModelIndex & index ) override final
	{
		Q_UNUSED( nif );
		return !index.isValid();
	}

	QModelIndex cast( NifModel * nif, const QModelIndex & ) override final
	{
		for ( int b = 0; b < nif->getBlockCount(); b++ ) {
			QModelIndex iBlock = nif->getBlock( b );
			QModelIndex idx = check( nif, iBlock );

			if ( idx.isValid() )
				return idx;
		}

		return QModelIndex();
	}

	QModelIndex check( NifModel * nif, const QModelIndex & iParent )
	{
		for ( int r = 0; r < nif->rowCount( iParent ); r++ ) {
			QModelIndex idx = iParent.child( r, 0 );
			bool child;

			if ( nif->isLink( idx, &child ) ) {
				qint32 l = nif->getLink( idx );

				if ( l < 0 ) {
					/*
					if ( ! child )
					{
					    qDebug() << "unassigned parent link";
					    return idx;
					}
					*/
				} else if ( l >= nif->getBlockCount() ) {
					qCCritical( nsSpell ) << Spell::tr( "Invalid link '%1'." ).arg( QString::number(l) );
					return idx;
				} else {
					QString tmplt = nif->itemTmplt( idx );

					if ( !tmplt.isEmpty() ) {
						QModelIndex iBlock = nif->getBlock( l );

						if ( !nif->inherits( iBlock, tmplt ) ) {
							qCCritical( nsSpell ) << Spell::tr( "Link '%1' points to wrong block type." ).arg( QString::number(l) );
							return idx;
						}
					}
				}
			}

			if ( nif->rowCount( idx ) > 0 ) {
				QModelIndex x = check( nif, idx );

				if ( x.isValid() )
					return x;
			}
		}

		return QModelIndex();
	}
};

REGISTER_SPELL( spSanityCheckLinks );

//! Fixes invalid block names
class spFixInvalidNames final : public Spell
{
public:
	QString name() const override final { return Spell::tr( "Fix Invalid Block Names" ); }
	QString page() const override final { return Spell::tr( "Sanitize" ); }
	bool sanity() const override final { return true; }

	bool isApplicable( const NifModel * nif, const QModelIndex & index ) override final
	{
		return nif && nif->getIndex( nif->getHeader(), "Num Strings" ).isValid() && !index.isValid();
	}

	QModelIndex cast( NifModel * nif, const QModelIndex & ) override final
	{
		QVector<QString> stringsToAdd;
		QVector<QString> shapeNames;
		QMap<QModelIndex, QString> modifiedBlocks;

		auto iHeader = nif->getHeader();
		auto numStrings = nif->get<int>( iHeader, "Num Strings" );
		auto strings = nif->getArray<QString>( iHeader, "Strings" );

		// Provides a string index for the desired string
		auto rename = [&strings, &stringsToAdd, numStrings] ( int & newIdx, const QString & str ) {
			newIdx = strings.indexOf( str );
			if ( newIdx < 0 ) {
				bool inNew = stringsToAdd.contains( str );
				newIdx = (inNew) ? stringsToAdd.indexOf( str ) : stringsToAdd.count();
				newIdx += numStrings;

				if ( !inNew )
					stringsToAdd << str;
			}
		};

		// Provides a block name using a base string and the block number,
		//	incrementing the number if necessary to avoid duplicate names
		auto autoRename = [&strings, &stringsToAdd, numStrings] ( int & newIdx, const QString & parentName, int blockNum ) {
			QString newName;
			int j = 0;
			do {
				newName = QString( "%1:%2" ).arg( parentName ).arg( blockNum + j );
				j++;
			} while ( strings.contains( newName ) || stringsToAdd.contains( newName ) );

			newIdx = numStrings + stringsToAdd.count();
			stringsToAdd << newName;
		};

		for ( int i = 0; i < nif->getBlockCount(); i++ ) {
			QModelIndex iBlock = nif->getBlock( i );
			if ( !(nif->inherits( iBlock, "NiObjectNET" ) || nif->inherits( iBlock, "NiExtraData" )) )
				continue;

			auto nameIdx = nif->get<int>( iBlock, "Name" );
			auto nameString = nif->get<QString>( iBlock, "Name" );

			// Ignore Editor Markers and AddOnNodes
			if ( nameString.contains( "EditorMarker" ) || nif->inherits( iBlock, "BSValueNode" ) )
				continue;

			QModelIndex iBlockParent = nif->getBlock( nif->getParent( i ) );
			auto parentNameString = nif->get<QString>( iBlockParent, "Name" );
			if ( !iBlockParent.isValid() ) {
				parentNameString = nif->getFilename();
			}

			int newIdx = -1;

			bool isOutOfBounds = nameIdx >= numStrings;
			bool isProp = nif->isNiBlock( iBlock, 
										{ "BSLightingShaderProperty", "BSEffectShaderProperty", "NiAlphaProperty" } );
			bool isNiAV = nif->inherits( iBlock, "NiAVObject" );

			// Fix 'BSX' strings
			if ( nif->isNiBlock( iBlock, "BSXFlags" ) ) {
				if ( nameString == "BSX" )
					continue;
				rename( newIdx, "BSX" );
			} else if ( nameString == "BSX" ) {
				if ( isProp )
					rename( newIdx,"" );
				else
					autoRename( newIdx, parentNameString, i );
			}

			// Fix duplicate shape names
			if ( isNiAV && !isOutOfBounds ) {
				if ( shapeNames.contains( nameString ) )
					autoRename( newIdx, parentNameString, i );

				shapeNames << nameString;
			}

			// Fix "Root Material" field
			if ( isProp && nif->getIndex( iBlock, "Root Material" ).isValid() ) {
				auto wetIdx = nif->get<int>( iBlock, "Root Material" );
				auto wetString = nif->get<QString>( iBlock, "Root Material" );

				int newWetIdx = -1;

				bool invalidString = !wetString.isEmpty() && !wetString.endsWith( ".bgsm", Qt::CaseInsensitive );
				if ( wetIdx >= numStrings || invalidString ) {
					rename( newWetIdx, "" );
				}

				if ( newWetIdx > -1 ) {
					nif->set<int>( iBlock, "Root Material", newWetIdx );
					modifiedBlocks.insert( nif->getIndex( iBlock, "Root Material" ), "Root Material" );
				}
			}

			// Fix "Name" field
			if ( isOutOfBounds ) {
				// Fix out of bounds string indices
				// Rename scene objects, or blank out property names
				if ( !isProp )
					autoRename( newIdx, parentNameString, i );
				else
					rename( newIdx, "" );
			} else if ( isProp ) {
				if ( nameString.isEmpty() )
					continue;

				// Fix invalid property names
				if ( nif->isNiBlock( iBlock, "NiAlphaProperty" ) ) {
					rename( newIdx, "" );
				} else {
					auto ci = Qt::CaseInsensitive;
					if ( !(nameString.endsWith( ".bgsm", ci ) || nameString.endsWith( ".bgem", ci )) ) {
						rename( newIdx, "" );
					}
				}
			}

			if ( newIdx > -1 ) {
				nif->set<int>( iBlock, "Name", newIdx );
				modifiedBlocks.insert( nif->getIndex( iBlock, "Name" ), "Name" );
			}
		}

		if ( modifiedBlocks.count() < 1 )
			return QModelIndex();

		// Append new strings to header strings
		strings << stringsToAdd;

		// Update header
		nif->set<int>( iHeader, "Num Strings", strings.count() );
		nif->updateArray( iHeader, "Strings" );
		nif->setArray<QString>( iHeader, "Strings", strings );
		
		nif->updateHeader();

		for ( const auto & b : modifiedBlocks.toStdMap() ) {
			auto blockName = b.first.parent().data( Qt::DisplayRole ).toString();

			Message::append( Spell::tr( "One or more blocks have had their Name sanitized." ), 
							 QString( "%1 (%2) = '%3'" ).arg( blockName ).arg( b.second ).arg( nif->get<QString>( b.first ) )
			);
		}

		return QModelIndex();
	}
};

REGISTER_SPELL( spFixInvalidNames );

//! Fills blank "Controller Type" refs in NiControllerSequence
class spFillBlankControllerTypes final : public Spell
{
public:
	QString name() const override final { return Spell::tr( "Fill Blank NiControllerSequence Types" ); }
	QString page() const override final { return Spell::tr( "Sanitize" ); }

	bool isApplicable( const NifModel * nif, const QModelIndex & index ) override final
	{
		return nif && nif->getIndex( nif->getHeader(), "Num Strings" ).isValid() && !index.isValid();
	}

	QModelIndex cast( NifModel * nif, const QModelIndex & ) override final
	{
		QVector<QString> modifiedNames;

		auto iHeader = nif->getHeader();
		auto numStrings = nif->get<int>( iHeader, "Num Strings" );
		auto strings = nif->getArray<QString>( iHeader, "Strings" );

		bool ok = true;
		QString str = QInputDialog::getText( 0, Spell::tr( "Fill Blank NiControllerSequence Types" ),
											   Spell::tr( "Choose the default Controller Type" ), 
											   QLineEdit::Normal, "NiTransformController", &ok );

		if ( !ok )
			return QModelIndex();

		auto stringIdx = strings.indexOf( str );
		if ( stringIdx == -1 ) {
			// Append new strings to header strings
			strings << str;
			stringIdx = numStrings;
		}

		for ( int i = 0; i < nif->getBlockCount(); i++ ) {
			QModelIndex iBlock = nif->getBlock( i );
			if ( !(nif->inherits( iBlock, "NiControllerSequence" )) )
				continue;

			auto controlledBlocks = nif->getIndex( iBlock, "Controlled Blocks" );
			auto numBlocks = nif->rowCount( controlledBlocks );

			for ( int i = 0; i < numBlocks; i++ ) {
				auto ctrlrType =  nif->getIndex( controlledBlocks.child( i, 0 ), "Controller Type" );
				auto nodeName = nif->getIndex( controlledBlocks.child( i, 0 ), "Node Name" );

				auto ctrlrTypeIdx = nif->get<int>( ctrlrType );
				if ( ctrlrTypeIdx == -1 ) {
					nif->set<int>( ctrlrType, stringIdx );
					modifiedNames << nif->get<QString>( nodeName );
				}
			}
		}

		// Update header
		nif->set<int>( iHeader, "Num Strings", strings.count() );
		nif->updateArray( iHeader, "Strings" );
		nif->setArray<QString>( iHeader, "Strings", strings );

		nif->updateHeader();

		for ( const QString& s : modifiedNames ) {
			Message::append( Spell::tr( "One or more NiControllerSequence rows have been sanitized." ),
							 QString( "%1" ).arg( s ) );
		}

		return QModelIndex();
	}
};

REGISTER_SPELL( spFillBlankControllerTypes );


/*
 *  Error Checking
 */

bool spErrorNoneRefs::isApplicable( const NifModel *, const QModelIndex & index )
{
	return !index.isValid();
}

QModelIndex spErrorNoneRefs::cast( NifModel * nif, const QModelIndex & )
{
	for ( int i = 0; i < nif->getBlockCount(); i++ ) {
		auto iNiAV = nif->getBlock( i, "NiAVObject" );
		if ( iNiAV.isValid() )
			checkArray( nif, iNiAV, {"Properties"} ); // "Children"

		auto iNiNET = nif->getBlock( i, "NiObjectNET" );
		if ( iNiNET.isValid() )
			checkArray( nif, iNiNET, {"Extra Data List"} );

		auto iBSSI = nif->getBlock( i, "BSSkin::Instance" );
		if ( iBSSI.isValid() )
			checkRef( nif, iBSSI, "Skeleton Root" );
	}

	return QModelIndex();
}

void spErrorNoneRefs::checkArray( NifModel * nif, const QModelIndex & idx, QVector<QString> rows )
{
	int i = nif->getBlockNumber( idx );
	for ( const auto & r : rows ) {
		auto links = nif->getLinkArray( idx, r );
		auto noneCount = links.count( -1 );
		if ( links.size() > 0 && noneCount > 0 ) {
			auto err = tr( "[%1] '%2' link array contains %3 None Refs." ).arg( i ).arg( r ).arg( noneCount );
			nif->logMessage( message(), err, QMessageBox::Critical );
		}
	}
}

void spErrorNoneRefs::checkRef( NifModel * nif, const QModelIndex & idx, const QString & name )
{
	int i = nif->getBlockNumber( idx );
	if ( nif->getLink( idx, name ) == -1 ) {
		nif->logMessage( message(), tr( "[%1] '%2' link is None." ).arg( i ).arg( name ), QMessageBox::Critical );
	}
}

REGISTER_SPELL( spErrorNoneRefs );

bool spErrorInvalidPaths::isApplicable( const NifModel *, const QModelIndex & index )
{
	return !index.isValid();
}

QModelIndex spErrorInvalidPaths::cast( NifModel * nif, const QModelIndex & )
{
	for ( int i = 0; i < nif->getBlockCount(); i++ ) {
		auto iBSSTS = nif->getBlock( i, "BSShaderTextureSet" );
		if ( iBSSTS.isValid() )
			checkPath( nif, iBSSTS, "Textures" );
		
		auto iBSSNLP = nif->getBlock( i, "BSShaderNoLightingProperty" );
		if ( iBSSNLP.isValid() )
			checkPath( nif, iBSSNLP, "File Name" );

		auto iBSLSP = nif->getBlock( i, "BSLightingShaderProperty" );
		if ( iBSLSP.isValid() ) {
			checkPath( nif, iBSLSP, "Name", P_NO_EXT );
			checkPath( nif, iBSLSP, "Root Material", P_NO_EXT );
		}

		auto iBSESP = nif->getBlock( i, "BSEffectShaderProperty" );
		if ( iBSESP.isValid() ) {
			checkPath( nif, iBSESP, "Source Texture" );
			checkPath( nif, iBSESP, "Greyscale Texture" );
			checkPath( nif, iBSESP, "Env Map Texture" );
			checkPath( nif, iBSESP, "Normal Texture" );
			checkPath( nif, iBSESP, "Env Mask Texture" );
		}

		auto iNiST = nif->getBlock( i, "NiSourceTexture" );
		if ( iNiST.isValid() )
			checkPath( nif, iNiST, "File Name" );
	}

	return QModelIndex();
}

void spErrorInvalidPaths::checkPath( NifModel * nif, const QModelIndex & idx, const QString & name, InvalidPath invalid )
{
	auto iString = nif->getIndex( idx, name );
	if ( !iString.isValid() )
		return;

	int i = nif->getBlockNumber( idx );
	QVector<QString> paths;
	if ( nif->isArray( iString ) )
		paths << nif->getArray<QString>( iString );
	else
		paths << nif->get<QString>( iString );

	for ( const auto & path : paths ) {
		if ( (invalid & P_EMPTY) && path.isEmpty() )
			nif->logMessage( message(), tr( "[%1] '%2' cannot have empty filepaths." ).arg( i ).arg( name ) );
		else if ( path.isEmpty() )
			return;

		if ( (invalid & P_NO_EXT) && !path.contains( '.' ) )
			nif->logMessage( message(), tr( "[%1] '%2' has a filepath without a file extension." ).arg( i ).arg( name ) );
		else if ( (invalid & P_ABSOLUTE) && path.size() > 2 && path.at( 1 ) == ':' )
			nif->logMessage( message(), tr( "[%1] '%2' has an absolute filepath." ).arg( i ).arg( name ) );
	}
}

REGISTER_SPELL( spErrorInvalidPaths );

bool spWarningEnvironmentMapping::isApplicable(const NifModel * nif, const QModelIndex & index)
{
	return nif->getUserVersion2() > 0 && !index.isValid();
}

QModelIndex spWarningEnvironmentMapping::cast(NifModel * nif, const QModelIndex & idx)
{
	for ( int i = 0; i < nif->getBlockCount(); i++ ) {
		if ( nif->getUserVersion2() < 83 ) {
			auto iBSSP = nif->getBlock(i, "BSShaderPPLightingProperty");
			if ( iBSSP.isValid() ) {
				auto sf1 = nif->get<quint32>(iBSSP, "Shader Flags");
				auto sf2 = nif->get<quint32>(iBSSP, "Shader Flags 2");

				if ( sf1 & 0x80 && !(sf2 & 0x8000) )
					nif->logMessage(message(), tr("[%1] Flags lack 'Envmap_Light_Fade' which may be a mistake.").arg(i));
			}
		} else {
		
		}
	}
	return QModelIndex();
}

REGISTER_SPELL(spWarningEnvironmentMapping);

//! Check BSXFlags block and make sure correct flags are ticked for features present inside the nif
class spCheckBSXFlags final : public Spell
{
public:
	QString name() const override final { return Spell::tr( "Check BSXFlags" ); }
	QString page() const override final { return Spell::tr( "Error Checking" ); }
	bool constant() const override final { return true; }
	bool sanity() const override final { return true; }

	bool isApplicable( const NifModel * nif, const QModelIndex & index ) override final
	{
		return nif && !index.isValid() && ( nif->checkVersion( 0x14020007, 0x14020007 ) && nif->getUserVersion2() == 83 || 100 );
	}

	QModelIndex cast( NifModel * nif, const QModelIndex & ) override final
	{
		QStringList errors;
		bool isAnimated = false;  // 1
		bool isHavok = false;  // 2
		bool isComplex = false;  // 8
		bool isAddon = false;  // 16
		bool isEditorMarker = false;  // 32
		bool isDynamic = false;  // 64
		bool isArticulated = false;  // 128
		bool isExternalEmit = false;  // 512
		int iCollisionBlockCount = 0;
		QModelIndex iBSXBlock;
		quint32 iBSXValue;

		for ( int i = 0; i < nif->getBlockCount(); i++ ) {
			QModelIndex iBlock = nif->getBlock( i );
			auto iBSX = nif->getBlock( i, "BSXFlags" );
			auto iBSLSP = nif->getBlock( i, "BSLightingShaderProperty" );
			auto iBHK = nif->getBlock( i, "bhkCollisionObject" );
			auto iBHKSP = nif->getBlock( i, "bhkSPCollisionObject" );
			auto iBSVN = nif->getBlock( i, "BSValueNode" );

			if ( iBSX.isValid() ) {
				iBSXBlock = iBSX;
				iBSXValue = nif->get<quint32>( iBSXBlock, "Integer Data" );
			}

			if ( nif->inherits( iBlock, "NiTimeController" ) ) {
				if ( !( iBSXValue & 0x01 ) ) // Check BSXFlags
					isAnimated = true;
			}

			if ( iBHK.isValid() ) {
				auto iBHKChild = nif->getIndex( nif->getBlock( nif->getLink( iBHK, "Body" ) ), "Rigid Body Info" );
				iCollisionBlockCount++;

				if ( !( iBSXValue & 0x02 ) ) { // Check BSXFlags
					isHavok = true;
				}

				if ( nif->get<int>( iBHKChild.child( 31, 0 ) ) == 4 ) {
					if ( !( iBSXValue & 0x40 ) ) // Check BSXFlags
						isDynamic = true;
				}
			}

			if ( iBHKSP.isValid() ) {
				iCollisionBlockCount++;
			}

			if ( iBSVN.isValid() ) {
				if ( !( iBSXValue & 0x10 ) ) // Check BSXFlags
					isAddon = true;
			}

			if ( nif->getBlockName( iBlock ) == ( "NiTriShape" ) || nif->getBlockName( iBlock ) == ( "BSTriShape" ) ) {
				if ( nif->get<QString>( iBlock, "Name" ) == ( "EditorMarker" ) ) {
					if ( !( iBSXValue & 0x20 ) ) // Check BSXFlags
						isEditorMarker = true;
				}
			}

			if ( iBSLSP.isValid() ) {
				auto sf1 = nif->get<quint32>( iBSLSP, "Shader Flags 1" );

				if ( sf1 & 0x20000000 ) {
					if ( !( iBSXValue & 0x200 ) ) // Check BSXFlags
						isExternalEmit = true;
				}
			}
		}

		// Add errors to list outside for loop as we only need one error message for one or more occurrences
		if ( isAnimated == true ) {
			errors.append( QString( "One or more animation blocks present. Please enable the \"Animated\" flag in the BSXFlags block." ) );
		}
		if ( isHavok == true ) {
			errors.append( QString( "One or more collision blocks present. Please enable the \"Havok\" flag in the BSXFlags block." ) );
		}
		if ( isAddon == true ) {
			errors.append( QString( "One or more addon nodes present. Please enable the \"Addon\" flag in the BSXFlags block." ) );
		}
		if ( isEditorMarker == true ) {
			errors.append( QString( "One or more editor markers present. Please enable the \"Editor Marker\" flag in the BSXFlags block." ) );
		}
		if ( isDynamic == true ) {
			errors.append( QString( "One or more dynamic collision blocks present. Please enable the \"Dynamic\" flag in the BSXFlags block." ) );
		}
		if ( isExternalEmit == true ) {
			errors.append( QString( "One or more meshes use external emittance. Please enable the \"External Emit\" flag in the BSXFlags block." ) );
		}
		if ( iCollisionBlockCount > 1 ) {
			if ( !( iBSXValue & 0x08 ) ) { // Check BSXFlags
				isComplex = true;
				errors.append( QString( "More than one collision block present. Please enable the \"Complex\" flag and disable the \"Articulated\" flag in the BSXFlags block." ) );
			}
		}
		if ( iCollisionBlockCount == 1 ) {
			if ( !( iBSXValue & 0x80 ) ) { // Check BSXFlags
				isArticulated = true;
				errors.append( QString( "One collision block present. Please enable the \"Articulated\" flag and disable the \"Complex\" flag in the BSXFlags block." ) );
			}
		}

		if ( !errors.isEmpty() ) {
			QMessageBox msgBox;
			msgBox.setIcon( QMessageBox::Warning );
			msgBox.setTextFormat( Qt::PlainText );
			msgBox.setWindowTitle( "BSXFlags Warning" );
			msgBox.setText( "One or more warnings detected in BSXFlags block." );
			msgBox.setDetailedText( errors.join( "\n\n" ) );
			msgBox.setStandardButtons( QMessageBox::Ok );
			QAbstractButton * btnAutoFix = msgBox.addButton( "Auto Fix", QMessageBox::ApplyRole );
			QAbstractButton * detailsButton = NULL;

			foreach ( QAbstractButton * button, msgBox.buttons() ) {
				if ( msgBox.buttonRole( button ) == QMessageBox::ActionRole ) {
					detailsButton = button;
					break;
				}
			}

			if ( detailsButton ) {
				detailsButton->click();
			}

			msgBox.exec();

			if( msgBox.clickedButton() == btnAutoFix ) {
				quint32 flagsToAdd = 0;

				if ( isAddon == true ) {
					flagsToAdd = flagsToAdd + 0x10;
				}
				if ( isAnimated == true ) {
					flagsToAdd = flagsToAdd + 0x01;
				}
				if ( isArticulated == true ) {
					flagsToAdd = flagsToAdd + 0x80;
					if ( iBSXValue & 0x08 )
						iBSXValue = ( iBSXValue - 0x08 );
				}
				if ( isHavok == true ) {
					flagsToAdd = flagsToAdd + 0x02;
				}
				if ( isComplex == true ) {
					flagsToAdd = flagsToAdd + 0x08;
					if ( iBSXValue & 0x80 )
						iBSXValue = ( iBSXValue - 0x80 );
				}
				if ( isDynamic == true ) {
					flagsToAdd = flagsToAdd + 0x40;
				}
				if ( isEditorMarker == true ) {
					flagsToAdd = flagsToAdd + 0x20;
				}
				if ( isExternalEmit == true ) {
					flagsToAdd = flagsToAdd + 0x200;
				}

				nif->set<int>( iBSXBlock, "Integer Data",  ( iBSXValue + flagsToAdd ) );
			}
		}

		return QModelIndex();
	}
};

REGISTER_SPELL( spCheckBSXFlags );

//! Check collision block materials for NONE value materials (Skyrim LE/SE only)
class spCheckCollisionMaterials final : public Spell
{
public:
	QString name() const override final { return Spell::tr( "Invalid Collision Materials" ); }
	QString page() const override final { return Spell::tr( "Error Checking" ); }
	bool constant() const override final { return true; }
	bool sanity() const override final { return true; }

	bool isApplicable( const NifModel * nif, const QModelIndex & index ) override final
	{
		return nif && !index.isValid() && ( nif->checkVersion( 0x14020007, 0x14020007 ) && nif->getUserVersion2() == 83 || 100 );
	}

	QModelIndex cast( NifModel * nif, const QModelIndex & ) override final
	{
		QStringList errors;

		for ( int i = 0; i < nif->getBlockCount(); i++ ) {
			QModelIndex iBlock = nif->getBlock( i );
			QModelIndex iAnyBhkShape;
			auto iBhkCMSD = nif->getBlock( i, "bhkCompressedMeshShapeData" );

			if ( iBhkCMSD.isValid() ) {
				QModelIndex iChunkMaterials = nif->getIndex( iBhkCMSD, "Chunk Materials" );

				if ( iChunkMaterials.isValid() ) // check material
					for ( int i = 0; i < nif->rowCount( iChunkMaterials ); i++ ) {
						QModelIndex iMaterial = nif->getIndex( iChunkMaterials.child( i, 0 ), "Material" );
						int iValue = nif->get<int>( iMaterial );

						if ( iValue == 0 )
							errors.append( QString( nif->getBlockName( iBhkCMSD ) ) + QString( " (block " ) +
										   QString::number( nif->getBlockNumber( iBhkCMSD ) ) +
										   QString( "): Chunk Material " ) + QString::number( i )
							);
					}
			}

			if ( nif->inherits( iBlock, { "bhkSphereRepShape", "bhkConvexListShape", "bhkConvexTransformShape", "bhkListShape", "bhkTransformShape" } ) ) {
				iAnyBhkShape = iBlock;
				if ( iAnyBhkShape.isValid() ) {
					QModelIndex iMaterial = nif->getIndex( iAnyBhkShape, "Material" );
					int iValue = nif->get<int>( iMaterial );

					if ( iMaterial.isValid() ) // check material
						if ( iValue == 0 )
							errors.append( QString( nif->getBlockName( iAnyBhkShape ) ) + QString( " (block " ) +
										   QString::number( nif->getBlockNumber( iAnyBhkShape ) ) +
										   QString( ")" )
							);
				}
			}
		}

		if ( !errors.isEmpty() ) {
			QMessageBox msgBox;
			msgBox.setIcon( QMessageBox::Warning );
			msgBox.setTextFormat( Qt::PlainText );
			msgBox.setWindowTitle( "Collision Material Warning" );
			msgBox.setText( "One or more counts of SKY_HAV_MAT_NONE collision material detected." );
			msgBox.setDetailedText( errors.join( "\n" ) );
			QAbstractButton * detailsButton = NULL;

			foreach ( QAbstractButton * button, msgBox.buttons() ) {
				if ( msgBox.buttonRole( button ) == QMessageBox::ActionRole) {
					detailsButton = button;
					break;
				}
			}

			if ( detailsButton ) {
				detailsButton->click();
			}

			msgBox.exec();
		}

		return QModelIndex();
	}
};

REGISTER_SPELL( spCheckCollisionMaterials );
