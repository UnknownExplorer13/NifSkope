#include "spellbook.h"

#include "ui/widgets/nifeditors.h"


// Brief description is deliberately not autolinked to class Spell
/*! \file materialedit.cpp
 * \brief Material editing spells (spMaterialEdit)
 *
 * All classes here inherit from the Spell class.
 */

//! Edit a material
class spMaterialEdit final : public Spell
{
public:
	QString name() const override final { return Spell::tr( "Material" ); }
	QString page() const override final { return Spell::tr( "" ); }
	bool instant() const override final { return true; }
	QIcon icon() const override { return QIcon( ":/img/materialedit" ); }

	bool isApplicable( const NifModel * nif, const QModelIndex & index ) override final
	{
		QModelIndex iBlock  = nif->getBlock( index, "NiMaterialProperty" );
		QModelIndex sibling = index.sibling( index.row(), 0 );
		return index.isValid() && ( iBlock == sibling || nif->getIndex( iBlock, "Name" ) == sibling );
	}

	QModelIndex cast( NifModel * nif, const QModelIndex & index ) override final
	{
		QModelIndex iMaterial = nif->getBlock( index );
		NifBlockEditor * me = new NifBlockEditor( nif, iMaterial );

		me->pushLayout( new QHBoxLayout );
		me->add( new NifColorEdit( nif, nif->getIndex( iMaterial, "Ambient Color" ) ) );
		me->add( new NifColorEdit( nif, nif->getIndex( iMaterial, "Diffuse Color" ) ) );
		me->popLayout();
		me->pushLayout( new QHBoxLayout );
		me->add( new NifColorEdit( nif, nif->getIndex( iMaterial, "Specular Color" ) ) );
		me->add( new NifColorEdit( nif, nif->getIndex( iMaterial, "Emissive Color" ) ) );
		me->popLayout();
		me->add( new NifFloatSlider( nif, nif->getIndex( iMaterial, "Alpha" ), 0.0, 1.0 ) );
		me->add( new NifFloatSlider( nif, nif->getIndex( iMaterial, "Glossiness" ), 0.0, 100.0 ) );
		me->setWindowModality( Qt::ApplicationModal );
		me->show();

		return index;
	}
};

REGISTER_SPELL( spMaterialEdit );

