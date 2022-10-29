#include "spellbook.h"



// Brief description is deliberately not autolinked to class Spell
/*! \file array.cpp
 * \brief Array editing spells
 *
 * All classes here inherit from the Spell class.
 */

//! Moves an array entry up by 1
class spArrayMoveUp final : public Spell
{
public:
    QString name() const override final { return Spell::tr( "Move Up" ); }
    QString page() const override final { return Spell::tr( "Array" ); }
    QKeySequence hotkey() const override final { return { Qt::CTRL + Qt::Key_Up }; }

	bool isApplicable( const NifModel * nif, const QModelIndex & index ) override final
	{
        return ( ! nif->isNiBlock(index) ) && nif->isArray(index.parent()) && index.row() > 0;
	}

	QModelIndex cast( NifModel * nif, const QModelIndex & index ) override final
    {
        auto row = index.row();
        auto ret = index.siblingAtRow( row - 1 );
        nif->ShiftRow( index.parent(), row, -1 );
        return ret;
	}
};

REGISTER_SPELL( spArrayMoveUp );



//! Moves an array entry down by 1
class spArrayMoveDown final : public Spell
{
public:
    QString name() const override final { return Spell::tr( "Move Down" ); }
    QString page() const override final { return Spell::tr( "Array" ); }
    QKeySequence hotkey() const override final { return { Qt::CTRL + Qt::Key_Down }; }

    bool isApplicable( const NifModel * nif, const QModelIndex & index ) override final
    {
        return ( ! nif->isNiBlock(index) ) && nif->isArray(index.parent()) && index.row() < nif->rowCount( index.parent() );
    }

    QModelIndex cast( NifModel * nif, const QModelIndex & index ) override final
    {
        auto row = index.row();
        auto ret = index.siblingAtRow( row + 1 );
        nif->ShiftRow( index.parent(), row, +1 );
        return ret;
    }
};

REGISTER_SPELL( spArrayMoveDown );
