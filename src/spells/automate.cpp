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

class uiAutomateDialogSetup
{
public:
	//! Add standard buttons to a dialog
	/*!
	 * \param dlg The dialog to add buttons to
	 * \param vbox Vertical box layout used by the dialog
	 */
	void dlgButtons( QDialog * dlg, QVBoxLayout * vbox, const QString & acceptText = "Accept" )
	{
		QHBoxLayout * hbox = new QHBoxLayout;
		vbox->addLayout( hbox );

		QPushButton * btAccept = new QPushButton( acceptText );
		hbox->addWidget( btAccept );
		QObject::connect( btAccept, &QPushButton::clicked, dlg, &QDialog::accept );

		QPushButton * btReject = new QPushButton( Spell::tr( "Cancel" ) );
		hbox->addWidget( btReject );
		QObject::connect( btReject, &QPushButton::clicked, dlg, &QDialog::reject );
	}

	//! Add a checkbox to a dialog
	/*!
	 * \param vbox Vertical box layout to add the checkbox to
	 * \param name The name to give the checkbox
	 * \param chk A checkbox that enables or disables this checkbox
	 * \return A pointer to the checkbox
	 */
	QCheckBox * dlgCheck( QVBoxLayout * vbox, const QString & name, QCheckBox * chk = nullptr )
	{
		QCheckBox * box = new QCheckBox( name );
		vbox->addWidget( box );

		if ( chk ) {
			QObject::connect( chk, &QCheckBox::toggled, box, &QCheckBox::setEnabled );
			box->setEnabled( chk->isChecked() );
		}

		return box;
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

// Automatically setup NiStringExtraData (and BSBehaviorGraphExtraData if needed) block(s) for weapons
class spAutoAddBSXFlags final : public Spell, uiAutomateDialogSetup
{
public:
	QString name() const override final { return Spell::tr( "Add BSXFlags" ); }
	QString page() const override final { return Spell::tr( "Automate" ); }

	bool isApplicable( const NifModel * nif, const QModelIndex & index ) override final
	{
		return nif && ( ( nif->getBlockName( index ) == "BSFadeNode" ) && ( nif->getBlockNumber( index ) == 0 ) ) && ( nif->checkVersion( 0x14020007, 0x14020007 ) && nif->getUserVersion2() == 83 || 100 );
	}

	QModelIndex cast( NifModel * nif, const QModelIndex & index ) override final
	{
		QModelIndex iBlock = nif->getBlock( index );

		if ( iBlock.isValid() ) {
			quint32 flags = nif->get<int>( index );

			QDialog dlg;
			QVBoxLayout * vbox = new QVBoxLayout( &dlg );
			//QStringList templateChoices = { "None", "Animated Static", "Clutter", "Static" };
			//QComboBox * cmbOptions = dlgCombo( vbox, Spell::tr( "Template" ), templateChoices );  //Templates don't work yet, commenting out for now
			//vbox->addWidget( new QLabel( "Templates" ) );
			//vbox->addWidget( new QLabel( "" ) );
			vbox->addWidget( new QLabel( "Flags to enable:" ) );

			QStringList flagChoices {
									  "Has Animations",                                 //Animated (1)
									  "Enable Collision",                               //Havok (2)
									  "Is a \"skeleton.nif\" file",                     //Ragdoll (4)
									  "Multiple Collision Blocks",                      //Complex (8)
									  "Uses Addon Nodes (e.g. candle flames)",          //Addon (16)
									  "Has Editor Marker(s)",                           //Editor Marker (32)
									  "Has Havok Physics (e.g. helmet ground model)",   //Dynamic (64)
									  "Has One Collision Block and/or Animation",       //Articulated (128)
									  "Has Moving Parts",                               //Needs Transform Updates (256)
									  "Enable External Emittance in CK"                 //External Emit (512)
									};

			/* // Add commented out flag values as seen in flags.cpp; Disabled by default
			QStringList flagChoicesExtra {
										   "Magic Shader Properties",            //1024
										   "Lights",                             //2048
										   "Breakable",                          //4096
										   "Searched Breakable (Runtime Only?)"  //8192
										 };
			flagChoices.append(flagChoicesExtra); */

			QList<QCheckBox *> chkBoxes;
			int x = 0;
			for ( const QString& flagName : flagChoices ) {
				chkBoxes << dlgCheck( vbox, QString( "%1" ).arg( flagName ) );
				x++;
			}
			chkBoxes.at( 8 )->hide(); //Hides the Needs Transform Updates option as Skyrim doesn't use it

			dlgButtons( &dlg, vbox, "Add BSXFlags" );

			if ( dlg.exec() == QDialog::Accepted ) {
				QModelIndex extraData;
				QModelIndex iArray = nif->getIndex( iBlock, "Extra Data List" );
				bool iBSXFExists = false;

				// Check if a BSXFlags block already exists, if it does then fill extraData value
				for ( int i = 0; i < nif->getBlockCount(); i++ ) {
					QModelIndex iBlockCheck = nif->getBlock( i );

					if ( nif->getBlockName( iBlockCheck ) == "BSXFlags" ) {
						iBSXFExists = true;
						extraData = nif->getBlock( i );
						break;
					}
				}

				// BSXFlags block does not exist
				// Add the new BSXFlags block and fill extraData value with the new block
				if ( iBSXFExists == false ) {
					auto newData = nif->insertNiBlock( "BSXFlags" );
					extraData = newData;
					nif->set<uint>( iBlock, "Num Extra Data List", ( nif->get<int>(iBlock, "Num Extra Data List") + 1 ) );
					nif->updateArray( iArray );
					nif->setLink( iArray.child( nif->get<int>(iBlock, "Num Extra Data List") - 1, 0 ), nif->getBlockNumber( extraData ) );
				}

				nif->set<QString>( extraData, "Name", "BSX" );

				// Get chkBoxes int value and toggle BSXFlags values accordingly
				x = 0;
				for ( QCheckBox * chk : chkBoxes ) {
					flags = ( flags & ( ~( 1 << x ) ) ) | ( chk->isChecked() ? 1 << x : 0 );
					x++;
				}
				nif->set<int>( extraData, "Integer Data", flags );
				
			}

		}

		return QModelIndex();
	}

};

REGISTER_SPELL( spAutoAddBSXFlags );

// Automatically setup NiStringExtraData (and BSBehaviorGraphExtraData if needed) block(s) for weapons
class spAutoAddWeaponData final : public Spell, uiAutomateDialogSetup
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

			QComboBox * cmbOptions = dlgCombo( vbox, Spell::tr( "Weapon type:" ), weaponChoices );

			dlgButtons( &dlg, vbox, "Add Data" );

			if ( dlg.exec() == QDialog::Accepted ) {
				QModelIndex extraStringData;
				QModelIndex extraBehaviourData;
				QModelIndex iArray = nif->getIndex( iBlock, "Extra Data List" );
				bool iNSEDExists = false;
				bool iBSBGEDExists = false;

				// Check if a NiStringExtraData block already exists, if it does then fill extraStringData value
				for ( int i = 0; i < nif->getBlockCount(); i++ ) {
					QModelIndex iBlockCheck = nif->getBlock( i );

					if ( nif->getBlockName( iBlockCheck ) == "NiStringExtraData" ) {
						iNSEDExists = true;
						extraStringData = nif->getBlock( i );
						break;
					}
				}

				// NiStringExtraData block does not exist
				// Add the new NiStringExtraData block and fill extraStringData value with the new block
				if ( iNSEDExists == false ) {
					auto newData = nif->insertNiBlock( "NiStringExtraData" );
					extraStringData = newData;
					nif->set<uint>( iBlock, "Num Extra Data List", ( nif->get<int>(iBlock, "Num Extra Data List") + 1 ) );
					nif->updateArray( iArray );
					nif->setLink( iArray.child( nif->get<int>(iBlock, "Num Extra Data List") - 1, 0 ), nif->getBlockNumber( extraStringData ) );
				}

				nif->set<QString>( extraStringData, "Name", "Prn" );

				// If user selected Bow or Crossbow, check if a BSBehaviorGraphExtraData block already exists, if it does then fill extraBehaviourData value
				if ( ( cmbOptions->currentIndex() ==  2 ) || ( cmbOptions->currentIndex() ==  3 ) ) {
					for ( int i = 0; i < nif->getBlockCount(); i++ ) {
						QModelIndex iBlockCheck = nif->getBlock( i );

						if ( nif->getBlockName( iBlockCheck ) == "BSBehaviorGraphExtraData" ) {
							iBSBGEDExists = true;
							extraBehaviourData = nif->getBlock( i );
							if ( ( nif->get<QString>( extraBehaviourData, "Name" ) != "BGED" ) ) {
								nif->set<QString>( extraBehaviourData, "Name", "BGED" );
							}
							break;
						}
					}
				}

				// User selected Bow or Crossbow and BSBehaviorGraphExtraData block does not exist
				// Add the new BSBehaviorGraphExtraData block and fill extraBehaviourData value with the new block
				if ( ( cmbOptions->currentIndex() ==  2 ) || ( cmbOptions->currentIndex() ==  3 ) ) {
					if ( iBSBGEDExists == false ) {
						auto newBehaviourData = nif->insertNiBlock( "BSBehaviorGraphExtraData" );
						nif->set<QString>( newBehaviourData, "Name", "BGED" );
						extraBehaviourData = newBehaviourData;
						nif->set<uint>( iBlock, "Num Extra Data List", ( nif->get<int>(iBlock, "Num Extra Data List") + 1 ) );
						nif->updateArray( iArray );
						nif->setLink( iArray.child( nif->get<int>(iBlock, "Num Extra Data List") - 1, 0 ), nif->getBlockNumber( extraBehaviourData ) );
					}
				}

				// Get cmbOptions value and fill NiStringExtraData block String Data value accordingly
				if ( cmbOptions->currentIndex() == 0 ) {
					nif->set<QString>( extraStringData, "String Data", "QUIVER" );
				}
				else if ( cmbOptions->currentIndex() == 1 ) {
					nif->set<QString>( extraStringData, "String Data", "WeaponBack" );
				}
				else if ( ( cmbOptions->currentIndex() ==  2 ) || ( cmbOptions->currentIndex() ==  3 ) ) {
					nif->set<QString>( extraStringData, "String Data", "WeaponBow" );
					// Fill BSBehaviorGraphExtraData Behaviour Graph File value depending on Bow or Crossbow choice
					if ( cmbOptions->currentIndex() ==  2 ) {
						nif->set<QString>( extraBehaviourData, "Behaviour Graph File", "Weapons\\Bow\\BowProject.hkx" );
					}
					else if ( cmbOptions->currentIndex() ==  3 ) {
						nif->set<QString>( extraBehaviourData, "Behaviour Graph File", "DLC01\\Weapons\\Crossbow\\CrossbowProject.hkx" );
					}
				}
				else if ( cmbOptions->currentIndex() == 4 ) {
					nif->set<QString>( extraStringData, "String Data", "WeaponDagger" );
				}
				else if ( cmbOptions->currentIndex() == 5 ) {
					nif->set<QString>( extraStringData, "String Data", "WeaponMace" );
				}
				else if ( cmbOptions->currentIndex() == 6 ) {
					nif->set<QString>( extraStringData, "String Data", "SHIELD" );
				}
				else if ( cmbOptions->currentIndex() == 7 ) {
					nif->set<QString>( extraStringData, "String Data", "WeaponSword" );
				}
				else if ( cmbOptions->currentIndex() == 8 ) {
					nif->set<QString>( extraStringData, "String Data", "WeaponAxe" );
				}
				
			}

		}

		return QModelIndex();
	}

};

REGISTER_SPELL( spAutoAddWeaponData );
