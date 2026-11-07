/*
Main.cpp -- CS2-style main menu: top bar with icons on the left and
a centered row of text items.

Based on Xash3D FWGS mainui (GPLv2+). Replaces the original vertical
PicButton menu. Removed: Hazard Course, New Game, Load/Save, Change Game,
Previews, minimize/close bitmaps, logo banners.
*/

#include "Framework.h"
#include "Action.h"
#include "Bitmap.h"
#include "YesNoMessageBox.h"
#include "keydefs.h"
#include "MenuStrings.h"
#include "gameinfo.h"
#include "FontManager.h"

// ---- layout (virtual 1024x768 space) --------------------------------
#define BAR_HEIGHT		56
#define ICON_SIZE		32
#define ICON_STEP		48
#define ICON_MARGIN		14
#define NAV_PAD_MAX		34
#define NAV_PAD_MIN		6

// ---- strings. Change here if you want another language ---------------
#define STR_PLAY		"ИГРАТЬ"
#define STR_CREATE		"СОЗДАТЬ ИГРУ"
#define STR_LAN			"ЛОКАЛЬНАЯ СЕТЬ"
#define STR_PROFILE		"ПРОФИЛЬ"
#define STR_CONTROLS	"УПРАВЛЕНИЕ"

#define STR_PLAY_HINT		"Список интернет-серверов"
#define STR_CREATE_HINT		"Запустить свой сервер"
#define STR_LAN_HINT		"Игры в локальной сети"
#define STR_PROFILE_HINT	"Имя, модель, цвета"
#define STR_CONTROLS_HINT	"Настройка управления"
#define STR_HOME_HINT		"Вернуться в игру"
#define STR_OPTIONS_HINT	"Настройки"
#define STR_QUIT_HINT		"Выйти из игры"
#define STR_DISCONNECT_HINT	"Отключиться от сервера"
#define STR_CONSOLE_HINT	"Консоль"

#define ART_HOME_N		"gfx/shell/mm_home_n"
#define ART_HOME_F		"gfx/shell/mm_home_f"
#define ART_GEAR_N		"gfx/shell/mm_gear_n"
#define ART_GEAR_F		"gfx/shell/mm_gear_f"
#define ART_POWER_N		"gfx/shell/mm_power_n"
#define ART_POWER_F		"gfx/shell/mm_power_f"
#define ART_DISC_N		"gfx/shell/mm_disc_n"
#define ART_DISC_F		"gfx/shell/mm_disc_f"
#define ART_CONS_N		"gfx/shell/mm_cons_n"
#define ART_CONS_F		"gfx/shell/mm_cons_f"

// semi-transparent dark strip across the top of the screen
class CMenuTopBar : public CMenuBaseItem
{
public:
	CMenuTopBar()
	{
		SetBits( iFlags, QMF_INACTIVE );
	}

	void Draw() override
	{
		UI_FillRect( m_scPos, m_scSize, PackRGBA( 18, 20, 24, 215 ));

		// thin accent line below the bar
		Point p = m_scPos;
		Size s = m_scSize;
		p.y += s.h - 1;
		s.h = 1;
		UI_FillRect( p, s, PackRGBA( 255, 255, 255, 40 ));
	}
};

// text item with underline when focused (or when marked as primary)
class CMenuNavItem : public CMenuAction
{
public:
	typedef CMenuAction BaseClass;

	CMenuNavItem() : BaseClass(), bPrimary( false ) { }

	void Draw() override
	{
		BaseClass::Draw();

		const bool focused = ( this == m_pParent->ItemAtCursor( )) && !FBitSet( iFlags, QMF_GRAYED );
		if( !focused && !bPrimary )
			return;

		Point p = m_scPos;
		Size s = m_scSize;
		int thick = Q_max( 2, (int)( 3 * uiStatic.scaleY ));

		p.x += s.w * 0.12f;
		s.w *= 0.76f;
		p.y += m_scSize.h - thick - 2;
		s.h = thick;

		UI_FillRect( p, s, focused ? PackRGBA( 90, 190, 255, 255 ) : PackRGBA( 255, 255, 255, 70 ));
	}

	bool bPrimary;
};

class CMenuMain: public CMenuFramework
{
public:
	CMenuMain() : CMenuFramework( "CMenuMain" ) { }

	bool KeyDown( int key ) override;

	void QuitDialogCb();

private:
	void _Init() override;
	void _VidInit( ) override;
	void Think() override;

	void VidInit( bool connected );
	void InitNav( CMenuNavItem &item, const char *name, const char *status, void (*cb)(), bool primary );
	void InitIcon( CMenuBitmap &icon, const char *n, const char *f, const char *status );

	void DisconnectCb();
	void DisconnectDialogCb();
	void ConsoleCb();

	CMenuTopBar		bar;

	// left icons
	CMenuBitmap		homeBtn;
	CMenuBitmap		consoleBtn;
	CMenuBitmap		disconnectBtn;
	CMenuBitmap		optionsBtn;
	CMenuBitmap		quitBtn;

	// center row
	CMenuNavItem	navCreate;
	CMenuNavItem	navLan;
	CMenuNavItem	navPlay;
	CMenuNavItem	navProfile;
	CMenuNavItem	navControls;

	// quit / disconnect dialog
	CMenuYesNoMessageBox dialog;
};

void CMenuMain::QuitDialogCb()
{
	if( CL_IsActive() && EngFuncs::GetCvarFloat( "host_serverstate" ) && EngFuncs::GetCvarFloat( "maxplayers" ) == 1.0f )
		dialog.SetMessage( L( "StringsList_235" ) );
	else
		dialog.SetMessage( L( "GameUI_QuitConfirmationText" ) );

	dialog.onPositive.SetCommand( false, "quit \"menu dialog\"\n" );
	dialog.Show();
}

void CMenuMain::DisconnectCb()
{
	EngFuncs::ClientCmd( false, "disconnect\n" );
	VidInit( false );
	CalcPosition();
	CalcSizes();
	VidInitItems();
}

void CMenuMain::DisconnectDialogCb()
{
	dialog.onPositive = VoidCb( &CMenuMain::DisconnectCb );
	dialog.SetMessage( L( "Really disconnect?" ) );
	dialog.Show();
}

void CMenuMain::ConsoleCb()
{
	UI_SetActiveMenu( false );
	EngFuncs::KEY_SetDest( KEY_CONSOLE );
}

bool CMenuMain::KeyDown( int key )
{
	if( UI::Key::IsEscape( key ) )
	{
		if ( CL_IsActive( ))
		{
			if( !dialog.IsVisible() )
				UI_CloseMenu();
		}
		else
		{
			QuitDialogCb( );
		}
		return true;
	}
	return CMenuFramework::KeyDown( key );
}

void CMenuMain::InitNav( CMenuNavItem &item, const char *name, const char *status, void (*cb)(), bool primary )
{
	item.SetNameAndStatus( name, status );
	item.iFlags |= QMF_DROPSHADOW;
	item.eTextAlignment = QM_CENTER;
	item.eFocusAnimation = QM_HIGHLIGHTIFFOCUS;
	item.bPrimary = primary;

	if( primary )
	{
		item.SetCharSize( QM_BIGFONT );
		item.charSize = 30;
		item.colorBase = PackRGBA( 255, 255, 255, 255 );
	}
	else
	{
		item.SetCharSize( QM_DEFAULTFONT );
		item.charSize = 22;
		item.colorBase = PackRGBA( 190, 196, 204, 255 );
	}
	item.colorFocus = PackRGBA( 255, 255, 255, 255 );
	item.onReleased = cb;
}

void CMenuMain::InitIcon( CMenuBitmap &icon, const char *n, const char *f, const char *status )
{
	icon.SetPicture( n, f, f );
	icon.SetNameAndStatus( "", status );
	icon.iFlags = QMF_MOUSEONLY;
	icon.eFocusAnimation = QM_HIGHLIGHTIFFOCUS;
}

void CMenuMain::_Init( void )
{
	InitIcon( homeBtn, ART_HOME_N, ART_HOME_F, STR_HOME_HINT );
	homeBtn.onReleased = UI_CloseMenu;

	InitIcon( consoleBtn, ART_CONS_N, ART_CONS_F, STR_CONSOLE_HINT );
	consoleBtn.onReleased = VoidCb( &CMenuMain::ConsoleCb );

	InitIcon( disconnectBtn, ART_DISC_N, ART_DISC_F, STR_DISCONNECT_HINT );
	disconnectBtn.onReleased = VoidCb( &CMenuMain::DisconnectDialogCb );

	InitIcon( optionsBtn, ART_GEAR_N, ART_GEAR_F, STR_OPTIONS_HINT );
	optionsBtn.onReleased = UI_Options_Menu;

	InitIcon( quitBtn, ART_POWER_N, ART_POWER_F, STR_QUIT_HINT );
	quitBtn.onReleased = VoidCb( &CMenuMain::QuitDialogCb );

	InitNav( navCreate,   STR_CREATE,   STR_CREATE_HINT,   UI_CreateGame_Menu,    false );
	InitNav( navLan,      STR_LAN,      STR_LAN_HINT,      UI_LanGame_Menu,       false );
	InitNav( navPlay,     STR_PLAY,     STR_PLAY_HINT,     UI_InternetGames_Menu, true  );
	InitNav( navProfile,  STR_PROFILE,  STR_PROFILE_HINT,  UI_PlayerSetup_Menu,   false );
	InitNav( navControls, STR_CONTROLS, STR_CONTROLS_HINT, UI_Controls_Menu,      false );

	// server.dll is needed to start a local game
	if( !EngFuncs::CheckGameDll( ))
		navCreate.SetGrayed( true );

	dialog.Link( this );

	AddItem( bar );

	AddItem( homeBtn );
	AddItem( consoleBtn );
	AddItem( disconnectBtn );
	AddItem( optionsBtn );
	AddItem( quitBtn );

	AddItem( navCreate );
	AddItem( navLan );
	AddItem( navPlay );
	AddItem( navProfile );
	AddItem( navControls );
}

void CMenuMain::VidInit( bool connected )
{
	const int W = uiStatic.width;

	bar.SetRect( 0, 0, W, BAR_HEIGHT );

	// ---- left icons: only those that make sense right now ----
	const bool developer = gpGlobals->developer != 0;

	CMenuBitmap *icons[5];
	int nIcons = 0;

	homeBtn.SetVisibility( connected );
	if( connected ) icons[nIcons++] = &homeBtn;

	disconnectBtn.SetVisibility( connected );
	if( connected ) icons[nIcons++] = &disconnectBtn;

	icons[nIcons++] = &optionsBtn;

	consoleBtn.SetVisibility( developer );
	if( developer ) icons[nIcons++] = &consoleBtn;

	icons[nIcons++] = &quitBtn;

	for( int i = 0; i < nIcons; i++ )
		icons[i]->SetRect( ICON_MARGIN + i * ICON_STEP, ( BAR_HEIGHT - ICON_SIZE ) / 2, ICON_SIZE, ICON_SIZE );

	const int leftEdge = ICON_MARGIN + nIcons * ICON_STEP;

	// ---- center row: measure text, then fit into the free space ----
	CMenuNavItem *items[] = { &navCreate, &navLan, &navPlay, &navProfile, &navControls };
	const int count = ARRAYSIZE( items );

	int textW[ARRAYSIZE( items )];
	int total = 0;

	for( int i = 0; i < count; i++ )
	{
		CMenuNavItem *it = items[i];
		textW[i] = g_FontMgr->GetTextWideScaled( it->font, it->szName, it->charSize ) / uiStatic.scaleX;
		total += textW[i];
	}

	int avail = W - 2 * leftEdge;
	int pad = NAV_PAD_MAX;

	// shrink the padding if the row doesn't fit (portrait / 4:3 screens)
	while( pad > NAV_PAD_MIN && total + count * 2 * pad > avail )
		pad -= 2;

	int x = ( W - ( total + count * 2 * pad )) / 2;
	if( x < leftEdge ) x = leftEdge;

	for( int i = 0; i < count; i++ )
	{
		int w = textW[i] + 2 * pad;
		items[i]->SetRect( x, 0, w, BAR_HEIGHT );
		x += w;
	}
}

void CMenuMain::_VidInit()
{
	VidInit( CL_IsActive() );
}

void CMenuMain::Think()
{
	// show / hide console icon when developer mode is toggled
	const bool dev = gpGlobals->developer != 0;
	if( dev != consoleBtn.IsVisible( ))
	{
		VidInit( CL_IsActive() );
		CalcPosition();
		CalcSizes();
		VidInitItems();
	}

	CMenuFramework::Think();
}

ADD_MENU( menu_main, CMenuMain, UI_Main_Menu );

/*
=================
UI_QuitDialog_f

called by the engine when the OS asks to quit (window close button, Cmd+Q on macOS...)
=================
*/
static void UI_QuitDialog_f( void )
{
	// nothing to lose, quit immediately
	if( !CL_IsActive( ))
	{
		EngFuncs::ClientCmd( false, "quit \"menu\"\n" );
		return;
	}

	if( !UI_IsVisible( ))
		UI_Main_Menu();

	menu_main->QuitDialogCb();
}
ADD_COMMAND( menu_quit, UI_QuitDialog_f );
