#include "spellbook.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QLabel>
#include <QLayout>
#include <QPushButton>
/*#include <QMessageBox> Commented out for now as it's useful for testing if statements*/


// Brief description is deliberately not autolinked to class Spell
/*! \file automate.cpp
 * \brief Automation spells
 *
 * All classes here inherit from the Spell class.
 */

void dlgButtons( QDialog * dlg, QVBoxLayout * vbox )
{
	QHBoxLayout * hbox = new QHBoxLayout;
	vbox->addLayout( hbox );

	QPushButton * btAccept = new QPushButton( Spell::tr( "Add Data" ) );
	hbox->addWidget( btAccept );
	QObject::connect( btAccept, &QPushButton::clicked, dlg, &QDialog::accept );

	QPushButton * btReject = new QPushButton( Spell::tr( "Cancel" ) );
	hbox->addWidget( btReject );
	QObject::connect( btReject, &QPushButton::clicked, dlg, &QDialog::reject );
}

// Automatically setup NiStringExtraData block for weapons
class spAutoAddWeaponData final : public Spell
{
public:
	QString name() const override final { return Spell::tr( "Add Weapon Data" ); }
	QString page() const override final { return Spell::tr( "Automate" ); }

	bool isApplicable( const NifModel * nif, const QModelIndex & index ) override final
	{
		return nif && ( ( nif->getBlockName( index ) == "BSFadeNode" ) && ( nif->getBlockNumber( index ) == 0 ) ) && ( nif->checkVersion( 0x14020007, 0x14020007 ) && nif->getUserVersion2() == 83 || 100 );
	}

	QModelIndex cast( NifModel * nif, const QModelIndex & index ) override final
	{
		QModelIndex iBlock = nif->getBlock( index );

		if ( iBlock.isValid() ) {
			QDialog dlg;
			QVBoxLayout * vbox = new QVBoxLayout( &dlg );

			QStringList weaponChoices = { "Arrow/Bolt", "Battleaxe/Greatsword/Warhammer", "Bow", "Crossbow", "Dagger", "Mace", "Shield", "Sword", "War Axe" };

			QComboBox * cmbOptions = dlgCombo( vbox, Spell::tr( "What type of weapon is this?" ), weaponChoices );

			dlgButtons( &dlg, vbox );

			if ( dlg.exec() == QDialog::Accepted ) {
				QModelIndex iArray = nif->getIndex( iBlock, "Extra Data List" );
				nif->set<uint>( iBlock, "Num Extra Data List", ( nif->get<int>(iBlock, "Num Extra Data List") + 1 ) );
				nif->updateArray( iArray );
				auto extraData = nif->insertNiBlock( "NiStringExtraData" );
				nif->setLink( iArray.child( nif->get<int>(iBlock, "Num Extra Data List") - 1, 0 ), nif->getBlockNumber( extraData ) );
				nif->set<QString>( extraData, "Name", "Prn" );

				if ( cmbOptions->currentIndex() == 0 ) {
					nif->set<QString>( extraData, "String Data", "QUIVER" );
				}
				else if ( cmbOptions->currentIndex() == 1 ) {
					nif->set<QString>( extraData, "String Data", "WeaponBack" );
				}
				else if ( ( cmbOptions->currentIndex() ==  2 ) || ( cmbOptions->currentIndex() ==  3 ) ) {
					nif->set<QString>( extraData, "String Data", "WeaponBow" );
					if ( cmbOptions->currentIndex() ==  2 ) {
						nif->set<uint>( iBlock, "Num Extra Data List", ( nif->get<int>(iBlock, "Num Extra Data List") + 1 ) );
						nif->updateArray( iArray );
						auto extraDataBow = nif->insertNiBlock( "BSBehaviorGraphExtraData" );
						nif->setLink( iArray.child( nif->get<int>(iBlock, "Num Extra Data List") - 1, 0 ), nif->getBlockNumber( extraDataBow ) );
						nif->set<QString>( extraDataBow, "Name", "BGED" );
						nif->set<QString>( extraDataBow, "Behaviour Graph File", "Weapons\\Bow\\BowProject.hkx" );
					}
					else if ( cmbOptions->currentIndex() ==  3 ) {
						nif->set<uint>( iBlock, "Num Extra Data List", ( nif->get<int>(iBlock, "Num Extra Data List") + 1 ) );
						nif->updateArray( iArray );
						auto extraDataCrossbow = nif->insertNiBlock( "BSBehaviorGraphExtraData" );
						nif->setLink( iArray.child( nif->get<int>(iBlock, "Num Extra Data List") - 1, 0 ), nif->getBlockNumber( extraDataCrossbow ) );
						nif->set<QString>( extraDataCrossbow, "Name", "BGED" );
						nif->set<QString>( extraDataCrossbow, "Behaviour Graph File", "DLC01\\Weapons\\Crossbow\\CrossbowProject.hkx" );
					}
				}
				else if ( cmbOptions->currentIndex() == 4 ) {
					nif->set<QString>( extraData, "String Data", "WeaponDagger" );
				}
				else if ( cmbOptions->currentIndex() == 5 ) {
					nif->set<QString>( extraData, "String Data", "WeaponMace" );
				}
				else if ( cmbOptions->currentIndex() == 6 ) {
					nif->set<QString>( extraData, "String Data", "SHIELD" );
				}
				else if ( cmbOptions->currentIndex() == 7 ) {
					nif->set<QString>( extraData, "String Data", "WeaponSword" );
				}
				else if ( cmbOptions->currentIndex() == 8 ) {
					nif->set<QString>( extraData, "String Data", "WeaponAxe" );
				}
				
			}

		}

		return QModelIndex();
	}

	//! Add a combobox to a dialog
	/*!
	 * \param vbox Vertical box layout to add the combobox to
	 * \param name The name to give the combobox
	 * \param items The items to add to the combobox
	 * \return A pointer to the combobox
	 */
	QComboBox * dlgCombo( QVBoxLayout * vbox, const QString & name, QStringList items )
	{
		vbox->addWidget( new QLabel( name ) );
		QComboBox * cmb = new QComboBox;
		vbox->addWidget( cmb );
		cmb->addItems( items );

		return cmb;
	}
};

REGISTER_SPELL( spAutoAddWeaponData );
