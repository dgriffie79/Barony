//! @file CharacterSheet.cpp
//! Extracted from GameUI.cpp � Player::CharacterSheet_t implementation

#include "GameUI.hpp"
#include "MainMenu.hpp"
#include "Frame.hpp"
#include "Image.hpp"
#include "Field.hpp"
#include "Button.hpp"
#include "Text.hpp"
#include "Slider.hpp"
#include "Widget.hpp"

#include "../main.h"
#include "../game.h"
#include "../menu.h"
#include "../interface/interface.hpp"
#include "../stat.h"
#include "../player.h"
#include "../draw.h"
#include "../items.h"
#include "../mod_tools.hpp"
#include "../input.hpp"
#include "../collision.h"
#include "../monster.h"
#include "../classdescriptions.h"
#include "../shops.hpp"
#include "../colors.hpp"
#include "../book.hpp"
#include <assert.h>

// Forward declarations for symbols defined in GameUI.cpp
static const char* smallfont_outline = "fonts/pixel_maz_multiline.ttf#16#2";
extern std::string formatSkillSheetEffects(int playernum, int proficiency, std::string& tag, std::string& rawValue);
extern std::string actionPromptBackingIconPath00;
extern std::string actionPromptBackingIconPath20;
extern std::string actionPromptBackingIconPath60;
extern std::string actionPromptBackingIconPath100;

std::map<std::string, std::pair<std::string, std::string>> Player::CharacterSheet_t::mapDisplayNamesDescriptions;
std::string Player::CharacterSheet_t::defaultString = "";
std::map<std::string, std::string> Player::CharacterSheet_t::hoverTextStrings;
void Player::CharacterSheet_t::loadCharacterSheetJSON()
{
	printlog("[JSON]: Loading character sheet...");
	if ( !PHYSFS_getRealDir("/data/charsheet.json") )
	{
		printlog("[JSON]: Error: Could not find file: data/charsheet.json");
	}
	else
	{
		std::string inputPath = PHYSFS_getRealDir("/data/charsheet.json");
		inputPath.append("/data/charsheet.json");

		File* fp = FileIO::open(inputPath.c_str(), "rb");
		if ( !fp )
		{
			printlog("[JSON]: Error: Could not open json file %s", inputPath.c_str());
		}
		else
		{
			char buf[65536];
			int count = fp->read(buf, sizeof(buf[0]), sizeof(buf));
			buf[count] = '\0';
			cJSON* d = cJSON_Parse(buf);
			FileIO::close(fp);
			if ( !d || !cJSON_HasObjectItem(d, "version") )
			{
				printlog("[JSON]: Error: No 'version' value in json file, or JSON syntax incorrect! %s", inputPath.c_str());
			}
			else
			{
				if ( cJSON_HasObjectItem(d, "level_strings") )
				{
					mapDisplayNamesDescriptions.clear();
					for ( cJSON* itr = cJSON_GetObjectItem(d, "level_strings")->child;
						itr != nullptr; itr = itr->next )
					{
						std::string name = "";
						std::string desc = "";
						if ( cJSON_HasObjectItem(itr, "display_name") )
						{
							name = cJSON_GetStringValue(cJSON_GetObjectItem(itr, "display_name"));
						}
						if ( cJSON_HasObjectItem(itr, "description") )
						{
							desc = cJSON_GetStringValue(cJSON_GetObjectItem(itr, "description"));
						}
						mapDisplayNamesDescriptions[itr->string] = std::make_pair(name, desc);
					}
				}
				if ( cJSON_HasObjectItem(d, "hover_text") )
				{
					hoverTextStrings.clear();
					for ( cJSON* itr = cJSON_GetObjectItem(d, "hover_text")->child;
						itr != nullptr; itr = itr->next )
					{
						if ( cJSON_IsObject(itr) )
						{
							for ( cJSON* inner_itr = itr->child;
								inner_itr != nullptr; inner_itr = inner_itr->next )
							{
								hoverTextStrings[inner_itr->string] = inner_itr->valuestring;
							}
						}
						else
						{
							hoverTextStrings[itr->string] = itr->valuestring;
						}
					}
				}
			}
		}
	}
}

const int NUM_CHARSHEET_TOOLTIP_BACKING_FRAMES = 9;
const int NUM_CHARSHEET_TOOLTIP_TEXT_FIELDS = 16;
std::map<int, Field*> characterSheetTooltipTextFields[MAXPLAYERS];
std::map<int, Frame*> characterSheetTooltipTextBackingFrames[MAXPLAYERS];

static void charsheet_deselect_fn(Widget& widget) {
	if ( widget.isSelected()
		&& players[widget.getOwner()]->GUI.activeModule != Player::GUI_t::MODULE_CHARACTERSHEET
		&& !inputs.getVirtualMouse(widget.getOwner())->draw_cursor )
	{
		widget.deselect();
	}
};

void Player::CharacterSheet_t::createCharacterSheet()
{
	char name[32];
	snprintf(name, sizeof(name), "player sheet %d", player.playernum);
	if ( !gameUIFrame[player.playernum]->findFrame(name) )
	{
		characterSheetTooltipTextFields[player.playernum].clear();
		characterSheetTooltipTextBackingFrames[player.playernum].clear();

		Frame* sheetFrame = gameUIFrame[player.playernum]->addFrame(name);
		sheetFrame->setHollow(true);
		sheetFrame->setBorder(0);
		sheetFrame->setOwner(player.playernum);
		sheetFrame->setSize(SDL_Rect{ players[player.playernum]->camera_virtualx1(),
			players[player.playernum]->camera_virtualy1(),
			Frame::virtualScreenX,
			Frame::virtualScreenY });
		this->sheetFrame = sheetFrame;

		const int bgWidth = 208;
		const int leftAlignX = sheetFrame->getSize().w - bgWidth;
		{
			Frame* fullscreenBg = sheetFrame->addFrame("sheet bg fullscreen");
			fullscreenBg->setSize(SDL_Rect{ leftAlignX,
				0, 208, Frame::virtualScreenY });
			// if splitscreen 3/4 - disable the fullscreen background + title text.
			fullscreenBg->addImage(SDL_Rect{ 0, 0, fullscreenBg->getSize().w, 360 },
				0xFFFFFFFF, "*#images/ui/CharSheet/HUD_CharSheet_Window_01A_Top.png", "bg image");

			const char* titleFont = "fonts/pixel_maz.ttf#32#2";
			auto characterSheetTitleText = fullscreenBg->addField("character sheet title text", 32);
			characterSheetTitleText->setFont(titleFont);
			characterSheetTitleText->setSize(SDL_Rect{ 6, 177, 202, 32 });
			characterSheetTitleText->setText(Language::get(5968));
			characterSheetTitleText->setVJustify(Field::justify_t::CENTER);
			characterSheetTitleText->setHJustify(Field::justify_t::CENTER);
		}

		// log / map buttons
		{
			const char* buttonFont = "fonts/pixel_maz.ttf#32#2";
			SDL_Rect buttonFramePos{ leftAlignX + 9, 6, 196, 82 };
			auto buttonFrame = sheetFrame->addFrame("log map buttons");
			buttonFrame->setSize(buttonFramePos);
			buttonFrame->setDrawCallback([](const Widget& widget, SDL_Rect pos) {
				auto frame = (Frame*)(&widget);
			std::vector<const char*> buttons = {
				"map button",
				"log button"
			};
			for ( auto button : buttons )
			{
				if ( auto b = frame->findButton(button) )
				{
					if ( players[frame->getOwner()]->characterSheet.sheetDisplayType == CHARSHEET_DISPLAY_NORMAL )
					{
						b->setBackgroundHighlighted("*#images/ui/CharSheet/HUD_CharSheet_ButtonHigh_00.png");
						b->setBackground("*#images/ui/CharSheet/HUD_CharSheet_Button_00.png");
						b->setBackgroundActivated("*#images/ui/CharSheet/HUD_CharSheet_ButtonPress_00.png");
					}
					else
					{
						b->setBackgroundHighlighted("*#images/ui/CharSheet/HUD_CharSheet_ButtonHighCompact_00.png");
						b->setBackground("*#images/ui/CharSheet/HUD_CharSheet_ButtonCompact_00.png");
						b->setBackgroundActivated("*#images/ui/CharSheet/HUD_CharSheet_ButtonCompactPress_00.png");
					}
				}
			}
				});

			SDL_Rect buttonPos{ 0, 0, buttonFramePos.w, 40 };
			auto mapButton = buttonFrame->addButton("map button");
			mapButton->setText(Language::get(4069));
			mapButton->setFont(buttonFont);
			mapButton->setBackground("*#images/ui/CharSheet/HUD_CharSheet_Button_00.png");
			mapButton->setSize(buttonPos);
			mapButton->setHideGlyphs(true);
			mapButton->setHideKeyboardGlyphs(true);
			mapButton->setHideSelectors(true);
			mapButton->setMenuConfirmControlType(Widget::MENU_CONFIRM_CONTROLLER);
			mapButton->setColor(makeColor(255, 255, 255, 255));
			mapButton->setHighlightColor(makeColor(255, 255, 255, 255));
			mapButton->setCallback([](Button& button) {
				openMapWindow(button.getOwner());
			if ( players[button.getOwner()]->minimap.mapWindow ) {
				button.deselect();
			}
				});
			mapButton->setTickCallback(charsheet_deselect_fn);

			auto mapSelector = buttonFrame->addFrame("map button selector");
			mapSelector->setSize(buttonPos);
			mapSelector->setHollow(true);
			//mapSelector->setClickable(true);

			buttonPos.y = buttonPos.y + buttonPos.h + 2;
			auto logButton = buttonFrame->addButton("log button");
			logButton->setText(Language::get(4070));
			logButton->setFont(buttonFont);
			logButton->setBackground("*#images/ui/CharSheet/HUD_CharSheet_Button_00.png");
			logButton->setSize(buttonPos);
			logButton->setHideGlyphs(true);
			logButton->setHideKeyboardGlyphs(true);
			logButton->setHideSelectors(true);
			logButton->setMenuConfirmControlType(Widget::MENU_CONFIRM_CONTROLLER);
			logButton->setColor(makeColor(255, 255, 255, 255));
			logButton->setHighlightColor(makeColor(255, 255, 255, 255));
			logButton->setCallback([](Button& button) {
				openLogWindow(button.getOwner());
				});
			logButton->setTickCallback(charsheet_deselect_fn);

			auto logSelector = buttonFrame->addFrame("log button selector");
			logSelector->setSize(buttonPos);
			logSelector->setHollow(true);
			//logSelector->setClickable(true);
		}

		// game timer
		{
			const char* timerFont = "fonts/pixel_maz.ttf#32#2";
			Uint32 timerTextColor = makeColor(188, 154, 114, 255);

			Frame* timerFrame = sheetFrame->addFrame("game timer");
			timerFrame->setSize(SDL_Rect{ leftAlignX + 36, 90, 142, 26 });
			timerFrame->setDrawCallback([](const Widget& widget, SDL_Rect pos) {
				auto frame = (Frame*)(&widget);
			auto timerToggleImg = frame->findImage("timer icon img");
			auto timerSelector = frame->findButton("timer selector");
			if ( timerToggleImg && timerSelector )
			{
				if ( !players[frame->getOwner()]->characterSheet.showGameTimerAlways )
				{
					if ( timerSelector->isCurrentlyPressed() )
					{
						timerToggleImg->path = "images/ui/CharSheet/HUD_Button_Timer_PressOff00.png";
					}
					else if ( timerSelector->isHighlighted() || (!inputs.getVirtualMouse(widget.getOwner())->draw_cursor && timerSelector->isSelected()) )
					{
						timerToggleImg->path = "images/ui/CharSheet/HUD_Button_Timer_SelectOff00.png";
					}
					else
					{
						timerToggleImg->path = "images/ui/CharSheet/HUD_Button_Timer_UnselectOff00.png";
					}
				}
				else
				{
					if ( timerSelector->isCurrentlyPressed() )
					{
						timerToggleImg->path = "images/ui/CharSheet/HUD_Button_Timer_PressOn00.png";
					}
					else if ( timerSelector->isHighlighted() || (!inputs.getVirtualMouse(widget.getOwner())->draw_cursor && timerSelector->isSelected()) )
					{
						timerToggleImg->path = "images/ui/CharSheet/HUD_Button_Timer_SelectOn00.png";
					}
					else
					{
						timerToggleImg->path = "images/ui/CharSheet/HUD_Button_Timer_UnselectOn00.png";
					}
				}
			}
				});
			auto timerToggleImg = timerFrame->addImage(SDL_Rect{ 0, 0, 26, 26 }, 0xFFFFFFFF, "*#images/ui/CharSheet/HUD_Button_Timer_SelectOff00.png", "timer icon img");
			auto timerImg = timerFrame->addImage(SDL_Rect{ 30, 0, 112, 26 }, 0xFFFFFFFF, "*#images/ui/CharSheet/HUD_CharSheet_Timer_Backing_00.png", "timer bg img");
			auto timerText = timerFrame->addField("timer text", 32);
			timerText->setFont(timerFont);

			auto timerButton = timerFrame->addButton("timer selector");
			timerButton->setSize(SDL_Rect{ 0, 0, timerToggleImg->pos.w + timerImg->pos.w, 26 });
			timerButton->setColor(makeColor(0, 0, 0, 0));
			timerButton->setHighlightColor(makeColor(0, 0, 0, 0));
			timerButton->setHideGlyphs(true);
			timerButton->setHideKeyboardGlyphs(true);
			timerButton->setHideSelectors(true);
			timerButton->setMenuConfirmControlType(Widget::MENU_CONFIRM_CONTROLLER);
			timerButton->setCallback([](Button& button) {
				bool& bShowTimer = players[button.getOwner()]->characterSheet.showGameTimerAlways;
			bShowTimer = !bShowTimer;
			if ( bShowTimer )
			{
				Player::soundActivate();
			}
			else
			{
				Player::soundCancel();
			}
				});
			timerButton->setTickCallback(charsheet_deselect_fn);

			SDL_Rect textPos = timerImg->pos;
			textPos.x += 12;
			timerText->setSize(textPos);
			timerText->setVJustify(Field::justify_t::CENTER);
			timerText->setText("00:92:30:89");
			timerText->setColor(timerTextColor);
		}

		// skills button
		{
			const char* skillsFont = "fonts/pixel_maz.ttf#32#2";
			Frame* skillsButtonFrame = sheetFrame->addFrame("skills button frame");
			skillsButtonFrame->setSize(SDL_Rect{ leftAlignX + 14, 360 - 8 - 42, 186, 42 });
			auto skillsButton = skillsButtonFrame->addButton("skills button");
			skillsButton->setText(Language::get(4074));
			skillsButton->setFont(skillsFont);
			skillsButton->setBackground("*#images/ui/CharSheet/HUD_CharSheet_ButtonWide_00.png");
			skillsButton->setBackgroundActivated("*#images/ui/CharSheet/HUD_CharSheet_ButtonWidePress_00.png");
			skillsButton->setBackgroundHighlighted("*#images/ui/CharSheet/HUD_CharSheet_ButtonWideHigh_00.png");
			skillsButton->setSize(SDL_Rect{ 0, 0, skillsButtonFrame->getSize().w, skillsButtonFrame->getSize().h });
			skillsButton->setHideGlyphs(true);
			skillsButton->setHideKeyboardGlyphs(true);
			skillsButton->setHideSelectors(true);
			skillsButton->setMenuConfirmControlType(Widget::MENU_CONFIRM_CONTROLLER);
			skillsButton->setColor(makeColor(255, 255, 255, 255));
			skillsButton->setHighlightColor(makeColor(255, 255, 255, 255));
			skillsButton->setCallback([](Button& button) {
				players[button.getOwner()]->skillSheet.openSkillSheet();
				});
			skillsButton->setTickCallback(charsheet_deselect_fn);
		}

		// dungeon floor and level descriptor
		{
			const char* dungeonFont = "fonts/pixel_maz.ttf#32#2";
			Uint32 dungeonTextColor = makeColor(188, 154, 114, 255);
			Frame* dungeonFloorFrame = sheetFrame->addFrame("dungeon floor frame");

			dungeonFloorFrame->setSize(SDL_Rect{ leftAlignX + 6, 118, 202, 52 });
			auto dungeonButton = dungeonFloorFrame->addButton("dungeon floor selector");
			dungeonButton->setSize(SDL_Rect{ 6, 6, 190, 44 });
			dungeonButton->setColor(makeColor(0, 0, 0, 0));
			dungeonButton->setHighlightColor(makeColor(0, 0, 0, 0));
			dungeonButton->setHideGlyphs(true);
			dungeonButton->setHideKeyboardGlyphs(true);
			dungeonButton->setHideSelectors(true);
			dungeonButton->setMenuConfirmControlType(Widget::MENU_CONFIRM_CONTROLLER);
			dungeonButton->setTickCallback(charsheet_deselect_fn);

			auto floorNameText = dungeonFloorFrame->addField("dungeon name text", 32);
			floorNameText->setFont(dungeonFont);
			floorNameText->setSize(SDL_Rect{ 10, 0, 182, 26 });
			floorNameText->setText("");
			floorNameText->setVJustify(Field::justify_t::CENTER);
			floorNameText->setHJustify(Field::justify_t::CENTER);
			floorNameText->setColor(dungeonTextColor);

			auto floorLevelText = dungeonFloorFrame->addField("dungeon level text", 32);
			floorLevelText->setFont(dungeonFont);
			floorLevelText->setSize(SDL_Rect{ 10, 26, 182, 26 });
			floorLevelText->setText("");
			floorLevelText->setVJustify(Field::justify_t::CENTER);
			floorLevelText->setHJustify(Field::justify_t::CENTER);
			floorLevelText->setColor(dungeonTextColor);
		}

		Frame* characterFrame = sheetFrame->addFrame("character info");
		const char* infoFont = "fonts/pixel_maz.ttf#32#2";
		characterFrame->setSize(SDL_Rect{ leftAlignX, 206, bgWidth, 116 });
		characterFrame->setHollow(true);
		Uint32 infoTextColor = makeColor(188, 154, 114, 255);
		Uint32 classTextColor = makeColor(74, 66, 207, 255);

		sheetFrame->addImage(SDL_Rect{ characterFrame->getSize().x, characterFrame->getSize().y - 60,
			214, 170 }, 0xFFFFFFFF,
			"*#images/ui/CharSheet/HUD_CharSheet_Window_01A_TopCompact.png", "character info compact img");

		Frame* characterInnerFrame = characterFrame->addFrame("character info inner frame");
		characterInnerFrame->setSize(SDL_Rect{ 6, 0, 202, 104 });
		{
			//characterInnerFrame->addImage(SDL_Rect{ 0, 0, characterInnerFrame->getSize().w, characterInnerFrame->getSize().h }, 0xFFFFFFFF,
			//	"*#images/ui/CharSheet/HUD_CharSheet_Window_01A_TopTmp.png", "character info tmp img");

			SDL_Rect characterTextPos{ 2, 0, 198, 24 };
			auto nameText = characterInnerFrame->addField("character name text", 32);
			nameText->setFont(infoFont);
			nameText->setSize(SDL_Rect{ characterTextPos.x,
				characterTextPos.y,
				characterTextPos.w,
				characterTextPos.h });
			nameText->setText("Slartibartfast");
			nameText->setVJustify(Field::justify_t::CENTER);
			nameText->setHJustify(Field::justify_t::CENTER);
			nameText->setColor(infoTextColor);

			/*characterInnerFrame->addImage(SDL_Rect{ 2, 2, 198, 20 }, makeColor(255, 255, 255, 64),
				"images/system/white.png", "character name frame");*/
				//characterInnerFrame->addFrame("character name selector")->setSize(SDL_Rect{ 2, 2, 198, 20 });

			characterTextPos.x = 8;
			characterTextPos.w = 190;
			characterTextPos.y = 52;
			characterTextPos.h = 26;
			auto levelText = characterInnerFrame->addField("character level text", 32);
			levelText->setFont(infoFont);
			levelText->setSize(SDL_Rect{ characterTextPos.x,
				characterTextPos.y,
				characterTextPos.w,
				characterTextPos.h });
			levelText->setText("");
			levelText->setVJustify(Field::justify_t::CENTER);
			levelText->setColor(infoTextColor);

			/*characterInnerFrame->addImage(SDL_Rect{ 4, 54, 194, 22 }, makeColor(255, 255, 255, 64),
				"images/system/white.png", "character level frame");*/
			auto classButton = characterInnerFrame->addButton("character class selector");
			classButton->setSize(SDL_Rect{ 4, 54, 194, 22 });
			classButton->setColor(makeColor(0, 0, 0, 0));
			classButton->setHighlightColor(makeColor(0, 0, 0, 0));
			classButton->setHideGlyphs(true);
			classButton->setHideKeyboardGlyphs(true);
			classButton->setHideSelectors(true);
			classButton->setMenuConfirmControlType(Widget::MENU_CONFIRM_CONTROLLER);
			classButton->setTickCallback(charsheet_deselect_fn);

			characterTextPos.x = 8;
			characterTextPos.w = 190;
			characterTextPos.y = 52;
			characterTextPos.h = 26;
			auto classText = characterInnerFrame->addField("character class text", 32);
			classText->setFont(infoFont);
			classText->setSize(SDL_Rect{ characterTextPos.x,
				characterTextPos.y,
				characterTextPos.w,
				characterTextPos.h });
			classText->setText("");
			classText->setVJustify(Field::justify_t::CENTER);
			classText->setHJustify(Field::justify_t::LEFT);
			classText->setTextColor(classTextColor);

			characterTextPos.x = 4;
			characterTextPos.w = 194;
			characterTextPos.y = 26;
			characterTextPos.h = 26;
			auto raceText = characterInnerFrame->addField("character race text", 32);
			raceText->setFont(infoFont);
			raceText->setSize(SDL_Rect{ characterTextPos.x,
				characterTextPos.y,
				characterTextPos.w,
				characterTextPos.h });
			raceText->setText("");
			raceText->setVJustify(Field::justify_t::CENTER);
			raceText->setHJustify(Field::justify_t::CENTER);
			raceText->setColor(infoTextColor);

			auto sexImg = characterInnerFrame->addImage(
				SDL_Rect{ characterTextPos.x + characterTextPos.w - 40, characterTextPos.y - 4, 16, 28 }, 0xFFFFFFFF,
				"*#images/ui/CharSheet/HUD_CharSheet_Sex_M_01.png", "character sex img");

			/*characterInnerFrame->addImage(SDL_Rect{ 4, 28, 194, 22 }, makeColor(255, 255, 255, 64),
				"images/system/white.png", "character race frame");*/
			auto raceButton = characterInnerFrame->addButton("character race selector");
			raceButton->setSize(SDL_Rect{ 4, 28, 194, 22 });
			raceButton->setColor(makeColor(0, 0, 0, 0));
			raceButton->setHighlightColor(makeColor(0, 0, 0, 0));
			raceButton->setHideGlyphs(true);
			raceButton->setHideKeyboardGlyphs(true);
			raceButton->setHideSelectors(true);
			raceButton->setMenuConfirmControlType(Widget::MENU_CONFIRM_CONTROLLER);
			raceButton->setTickCallback(charsheet_deselect_fn);

			characterTextPos.x = 4;
			characterTextPos.w = 194;
			characterTextPos.y = 78;
			characterTextPos.h = 26;
			auto goldTitleText = characterInnerFrame->addField("gold text title", 8);
			goldTitleText->setFont(infoFont);
			goldTitleText->setSize(SDL_Rect{ 48, characterTextPos.y, 52, characterTextPos.h });
			goldTitleText->setText(Language::get(5969));
			goldTitleText->setVJustify(Field::justify_t::CENTER);
			goldTitleText->setColor(infoTextColor);

			auto goldText = characterInnerFrame->addField("gold text", 32);
			goldText->setFont(infoFont);
			goldText->setSize(SDL_Rect{ 92, characterTextPos.y, 88, characterTextPos.h });
			goldText->setText("0");
			goldText->setVJustify(Field::justify_t::CENTER);
			goldText->setHJustify(Field::justify_t::CENTER);
			goldText->setColor(infoTextColor);

			auto goldImg = characterInnerFrame->addImage(SDL_Rect{ 24, characterTextPos.y - 4, 20, 28 },
				0xFFFFFFFF, "*#images/ui/CharSheet/HUD_CharSheet_ButtonMoney_00.png", "gold img");

			/*characterInnerFrame->addImage(SDL_Rect{ 24, 80, 156, 22 }, makeColor(255, 255, 255, 64),
				"images/system/white.png", "character gold frame");*/
			auto goldButton = characterInnerFrame->addButton("character gold selector");
			goldButton->setSize(SDL_Rect{ 22, 80, 158, 22 });
			goldButton->setColor(makeColor(0, 0, 0, 0));
			goldButton->setHighlightColor(makeColor(0, 0, 0, 0));
			goldButton->setHideGlyphs(true);
			goldButton->setHideKeyboardGlyphs(true);
			goldButton->setHideSelectors(true);
			goldButton->setMenuConfirmControlType(0);
			goldButton->setTickCallback(charsheet_deselect_fn);
		}

		{
			Frame* statsFrame = sheetFrame->addFrame("stats");
			const int statsFrameHeight = 182;
			statsFrame->setSize(SDL_Rect{ leftAlignX, 344, bgWidth, statsFrameHeight });
			statsFrame->addImage(SDL_Rect{ 0, 0, statsFrame->getSize().w, statsFrame->getSize().h },
				0xFFFFFFFF, "*#images/ui/CharSheet/HUD_CharSheet_Window_01B_BotA.png", "stats bg img");

			Frame* statsInnerFrame = statsFrame->addFrame("stats inner frame");
			statsInnerFrame->setSize(SDL_Rect{ 0, 0, statsFrame->getSize().w, statsFrame->getSize().h });

			SDL_Rect iconPos{ 20, 8, 24, 24 };
			const int headingLeftX = iconPos.x + iconPos.w + 4;
			const int baseStatLeftX = headingLeftX + 32;
			const int modifiedStatLeftX = baseStatLeftX + 64;
			SDL_Rect textPos{ headingLeftX, iconPos.y, 40, iconPos.h };
			Uint32 statTextColor = hudColors.characterSheetNeutral;

			const char* statFont = "fonts/pixel_maz.ttf#32#2";
			textPos.y = iconPos.y + 1;
			statsInnerFrame->addImage(iconPos, 0xFFFFFFFF, "*#images/ui/CharSheet/HUD_CharSheet_STR_00.png", "str icon");
			{
				auto textBase = statsInnerFrame->addField("str text title", 8);
				textBase->setVJustify(Field::justify_t::CENTER);
				textBase->setFont(statFont);
				textPos.x = headingLeftX;
				textBase->setSize(textPos);
				textBase->setText(Language::get(5300));
				textBase->setColor(statTextColor);
				auto textStat = statsInnerFrame->addField("str text stat", 32);
				textStat->setVJustify(Field::justify_t::CENTER);
				textStat->setHJustify(Field::justify_t::CENTER);
				textStat->setFont(statFont);
				textPos.x = baseStatLeftX;
				textStat->setSize(textPos);
				textStat->setText("0");
				textStat->setColor(statTextColor);
				auto textStatModified = statsInnerFrame->addField("str text modified", 32);
				textStatModified->setVJustify(Field::justify_t::CENTER);
				textStatModified->setHJustify(Field::justify_t::CENTER);
				textStatModified->setFont(statFont);
				textPos.x = modifiedStatLeftX;
				textStatModified->setSize(textPos);
				textStatModified->setText("");
				textStatModified->setColor(statTextColor);

				auto statButton = statsInnerFrame->addButton("str button");
				statButton->setSize(SDL_Rect{ 12, iconPos.y + 2, statsFrame->getSize().w - 34, iconPos.h - 2 });
				statButton->setColor(makeColor(0, 0, 0, 0));
				statButton->setHighlightColor(makeColor(0, 0, 0, 0));
				statButton->setHideGlyphs(true);
				statButton->setHideKeyboardGlyphs(true);
				statButton->setHideSelectors(true);
				statButton->setMenuConfirmControlType(Widget::MENU_CONFIRM_CONTROLLER);
				statButton->setTickCallback(charsheet_deselect_fn);
			}
			const int rowSpacing = 4;
			iconPos.y += iconPos.h + rowSpacing;
			textPos.y = iconPos.y + 1;
			statsInnerFrame->addImage(iconPos, 0xFFFFFFFF, "*#images/ui/CharSheet/HUD_CharSheet_DEX_00.png", "dex icon");
			{
				auto textBase = statsInnerFrame->addField("dex text title", 8);
				textBase->setVJustify(Field::justify_t::CENTER);
				textBase->setFont(statFont);
				textPos.x = headingLeftX;
				textBase->setSize(textPos);
				textBase->setText(Language::get(5301));
				textBase->setColor(statTextColor);
				auto textStat = statsInnerFrame->addField("dex text stat", 32);
				textStat->setVJustify(Field::justify_t::CENTER);
				textStat->setHJustify(Field::justify_t::CENTER);
				textStat->setFont(statFont);
				textPos.x = baseStatLeftX;
				textStat->setSize(textPos);
				textStat->setText("0");
				textStat->setColor(statTextColor);
				auto textStatModified = statsInnerFrame->addField("dex text modified", 32);
				textStatModified->setVJustify(Field::justify_t::CENTER);
				textStatModified->setHJustify(Field::justify_t::CENTER);
				textStatModified->setFont(statFont);
				textPos.x = modifiedStatLeftX;
				textStatModified->setSize(textPos);
				textStatModified->setText("2");
				textStatModified->setColor(statTextColor);

				auto statButton = statsInnerFrame->addButton("dex button");
				statButton->setSize(SDL_Rect{ 12, iconPos.y + 2, statsFrame->getSize().w - 34, iconPos.h - 2 });
				statButton->setColor(makeColor(0, 0, 0, 0));
				statButton->setHighlightColor(makeColor(0, 0, 0, 0));
				statButton->setHideGlyphs(true);
				statButton->setHideKeyboardGlyphs(true);
				statButton->setHideSelectors(true);
				statButton->setMenuConfirmControlType(Widget::MENU_CONFIRM_CONTROLLER);
				statButton->setTickCallback(charsheet_deselect_fn);
			}

			iconPos.y += iconPos.h + rowSpacing;
			textPos.y = iconPos.y + 1;
			statsInnerFrame->addImage(iconPos, 0xFFFFFFFF, "*#images/ui/CharSheet/HUD_CharSheet_CON_00.png", "con icon");
			{
				auto textBase = statsInnerFrame->addField("con text title", 8);
				textBase->setVJustify(Field::justify_t::CENTER);
				textBase->setFont(statFont);
				textPos.x = headingLeftX;
				textBase->setSize(textPos);
				textBase->setText(Language::get(5302));
				textBase->setColor(statTextColor);
				auto textStat = statsInnerFrame->addField("con text stat", 32);
				textStat->setVJustify(Field::justify_t::CENTER);
				textStat->setHJustify(Field::justify_t::CENTER);
				textStat->setFont(statFont);
				textPos.x = baseStatLeftX;
				textStat->setSize(textPos);
				textStat->setText("0");
				textStat->setColor(statTextColor);
				auto textStatModified = statsInnerFrame->addField("con text modified", 32);
				textStatModified->setVJustify(Field::justify_t::CENTER);
				textStatModified->setHJustify(Field::justify_t::CENTER);
				textStatModified->setFont(statFont);
				textPos.x = modifiedStatLeftX;
				textStatModified->setSize(textPos);
				textStatModified->setText("");
				textStatModified->setColor(statTextColor);

				auto statButton = statsInnerFrame->addButton("con button");
				statButton->setSize(SDL_Rect{ 12, iconPos.y + 2, statsFrame->getSize().w - 34, iconPos.h - 2 });
				statButton->setColor(makeColor(0, 0, 0, 0));
				statButton->setHighlightColor(makeColor(0, 0, 0, 0));
				statButton->setHideGlyphs(true);
				statButton->setHideKeyboardGlyphs(true);
				statButton->setHideSelectors(true);
				statButton->setMenuConfirmControlType(Widget::MENU_CONFIRM_CONTROLLER);
				statButton->setTickCallback(charsheet_deselect_fn);
			}

			iconPos.y += iconPos.h + rowSpacing;
			textPos.y = iconPos.y + 1;
			statsInnerFrame->addImage(iconPos, 0xFFFFFFFF, "*#images/ui/CharSheet/HUD_CharSheet_INT_00.png", "int icon");
			{
				auto textBase = statsInnerFrame->addField("int text title", 8);
				textBase->setVJustify(Field::justify_t::CENTER);
				textBase->setFont(statFont);
				textPos.x = headingLeftX;
				textBase->setSize(textPos);
				textBase->setText(Language::get(5303));
				textBase->setColor(statTextColor);
				auto textStat = statsInnerFrame->addField("int text stat", 32);
				textStat->setVJustify(Field::justify_t::CENTER);
				textStat->setHJustify(Field::justify_t::CENTER);
				textStat->setFont(statFont);
				textPos.x = baseStatLeftX;
				textStat->setSize(textPos);
				textStat->setText("0");
				textStat->setColor(statTextColor);
				auto textStatModified = statsInnerFrame->addField("int text modified", 32);
				textStatModified->setVJustify(Field::justify_t::CENTER);
				textStatModified->setHJustify(Field::justify_t::CENTER);
				textStatModified->setFont(statFont);
				textPos.x = modifiedStatLeftX;
				textStatModified->setSize(textPos);
				textStatModified->setText("");
				textStatModified->setColor(statTextColor);

				auto statButton = statsInnerFrame->addButton("int button");
				statButton->setSize(SDL_Rect{ 12, iconPos.y + 2, statsFrame->getSize().w - 34, iconPos.h - 2 });
				statButton->setColor(makeColor(0, 0, 0, 0));
				statButton->setHighlightColor(makeColor(0, 0, 0, 0));
				statButton->setHideGlyphs(true);
				statButton->setHideKeyboardGlyphs(true);
				statButton->setHideSelectors(true);
				statButton->setMenuConfirmControlType(Widget::MENU_CONFIRM_CONTROLLER);
				statButton->setTickCallback(charsheet_deselect_fn);
			}

			iconPos.y += iconPos.h + rowSpacing;
			textPos.y = iconPos.y + 1;
			statsInnerFrame->addImage(iconPos, 0xFFFFFFFF, "*#images/ui/CharSheet/HUD_CharSheet_PER_00.png", "per icon");
			{
				auto textBase = statsInnerFrame->addField("per text title", 8);
				textBase->setVJustify(Field::justify_t::CENTER);
				textBase->setFont(statFont);
				textPos.x = headingLeftX;
				textBase->setSize(textPos);
				textBase->setText(Language::get(5304));
				textBase->setColor(statTextColor);
				auto textStat = statsInnerFrame->addField("per text stat", 32);
				textStat->setVJustify(Field::justify_t::CENTER);
				textStat->setHJustify(Field::justify_t::CENTER);
				textStat->setFont(statFont);
				textPos.x = baseStatLeftX;
				textStat->setSize(textPos);
				textStat->setText("0");
				textStat->setColor(statTextColor);
				auto textStatModified = statsInnerFrame->addField("per text modified", 32);
				textStatModified->setVJustify(Field::justify_t::CENTER);
				textStatModified->setHJustify(Field::justify_t::CENTER);
				textStatModified->setFont(statFont);
				textPos.x = modifiedStatLeftX;
				textStatModified->setSize(textPos);
				textStatModified->setText("");
				textStatModified->setColor(statTextColor);

				auto statButton = statsInnerFrame->addButton("per button");
				statButton->setSize(SDL_Rect{ 12, iconPos.y + 2, statsFrame->getSize().w - 34, iconPos.h - 2 });
				statButton->setColor(makeColor(0, 0, 0, 0));
				statButton->setHighlightColor(makeColor(0, 0, 0, 0));
				statButton->setHideGlyphs(true);
				statButton->setHideKeyboardGlyphs(true);
				statButton->setHideSelectors(true);
				statButton->setMenuConfirmControlType(Widget::MENU_CONFIRM_CONTROLLER);
				statButton->setTickCallback(charsheet_deselect_fn);
			}

			iconPos.y += iconPos.h + rowSpacing;
			textPos.y = iconPos.y + 1;
			statsInnerFrame->addImage(iconPos, 0xFFFFFFFF, "*#images/ui/CharSheet/HUD_CharSheet_CHA_00.png", "chr icon");
			{
				auto textBase = statsInnerFrame->addField("chr text title", 8);
				textBase->setVJustify(Field::justify_t::CENTER);
				textBase->setFont(statFont);
				textPos.x = headingLeftX;
				textBase->setSize(textPos);
				textBase->setText(Language::get(5305));
				textBase->setColor(statTextColor);
				auto textStat = statsInnerFrame->addField("chr text stat", 32);
				textStat->setVJustify(Field::justify_t::CENTER);
				textStat->setHJustify(Field::justify_t::CENTER);
				textStat->setFont(statFont);
				textPos.x = baseStatLeftX;
				textStat->setSize(textPos);
				textStat->setText("0");
				textStat->setColor(statTextColor);
				auto textStatModified = statsInnerFrame->addField("chr text modified", 32);
				textStatModified->setVJustify(Field::justify_t::CENTER);
				textStatModified->setHJustify(Field::justify_t::CENTER);
				textStatModified->setFont(statFont);
				textPos.x = modifiedStatLeftX;
				textStatModified->setSize(textPos);
				textStatModified->setText("");
				textStatModified->setColor(statTextColor);

				auto statButton = statsInnerFrame->addButton("chr button");
				statButton->setSize(SDL_Rect{ 12, iconPos.y + 2, statsFrame->getSize().w - 34, iconPos.h - 2 });
				statButton->setColor(makeColor(0, 0, 0, 0));
				statButton->setHighlightColor(makeColor(0, 0, 0, 0));
				statButton->setHideGlyphs(true);
				statButton->setHideKeyboardGlyphs(true);
				statButton->setHideSelectors(true);
				statButton->setMenuConfirmControlType(Widget::MENU_CONFIRM_CONTROLLER);
				statButton->setTickCallback(charsheet_deselect_fn);
			}
		}

		{
			Frame* attributesFrame = sheetFrame->addFrame("attributes");
			attributesFrame->setSize(SDL_Rect{ leftAlignX, 550, bgWidth, 182 });

			attributesFrame->addImage(SDL_Rect{ 0, 0, attributesFrame->getSize().w, attributesFrame->getSize().h },
				0xFFFFFFFF, "*#images/ui/CharSheet/HUD_CharSheet_Window_01B_BotB.png", "attributes bg img");

			Frame* attributesInnerFrame = attributesFrame->addFrame("attributes inner frame");
			attributesInnerFrame->setSize(SDL_Rect{ 0, 0, attributesFrame->getSize().w, attributesFrame->getSize().h });

			SDL_Rect iconPos{ 20, 8, 24, 24 };
			const int headingLeftX = iconPos.x + iconPos.w + 4;
			const int baseStatLeftX = headingLeftX + 48;
			SDL_Rect textPos{ headingLeftX, iconPos.y, 80, iconPos.h };
			Uint32 statTextColor = hudColors.characterSheetNeutral;

			const char* attributeFont = "fonts/pixel_maz.ttf#32#2";
			textPos.y = iconPos.y + 1;
			attributesInnerFrame->addImage(iconPos, 0xFFFFFFFF, "*#images/ui/CharSheet/HUD_CharSheet_ATT_00.png", "atk icon");
			{
				auto textBase = attributesInnerFrame->addField("atk text title", 8);
				textBase->setVJustify(Field::justify_t::CENTER);
				textBase->setFont(attributeFont);
				textPos.x = headingLeftX;
				textBase->setSize(textPos);
				textBase->setText(Language::get(6093));
				textBase->setColor(statTextColor);
				auto textStat = attributesInnerFrame->addField("atk text stat", 32);
				textStat->setVJustify(Field::justify_t::CENTER);
				textStat->setHJustify(Field::justify_t::CENTER);
				textStat->setFont(attributeFont);
				textPos.x = baseStatLeftX;
				textStat->setSize(textPos);
				textStat->setText("14");
				textStat->setColor(statTextColor);

				auto attributeButton = attributesInnerFrame->addButton("atk button");
				attributeButton->setSize(SDL_Rect{ 12, iconPos.y + 2, attributesFrame->getSize().w - 34, iconPos.h - 2 });
				attributeButton->setColor(makeColor(0, 0, 0, 0));
				attributeButton->setHighlightColor(makeColor(0, 0, 0, 0));
				attributeButton->setHideGlyphs(true);
				attributeButton->setHideKeyboardGlyphs(true);
				attributeButton->setHideSelectors(true);
				attributeButton->setMenuConfirmControlType(Widget::MENU_CONFIRM_CONTROLLER);
				attributeButton->setTickCallback(charsheet_deselect_fn);
			}

			const int rowSpacing = 4;
			iconPos.y += iconPos.h + rowSpacing;
			textPos.y = iconPos.y + 1;
			attributesInnerFrame->addImage(iconPos, 0xFFFFFFFF, "*#images/ui/CharSheet/HUD_CharSheet_AC_00.png", "ac icon");
			{
				auto textBase = attributesInnerFrame->addField("ac text title", 8);
				textBase->setVJustify(Field::justify_t::CENTER);
				textBase->setFont(attributeFont);
				textPos.x = headingLeftX;
				textBase->setSize(textPos);
				textBase->setText(Language::get(6094));
				textBase->setColor(statTextColor);
				auto textStat = attributesInnerFrame->addField("ac text stat", 32);
				textStat->setVJustify(Field::justify_t::CENTER);
				textStat->setHJustify(Field::justify_t::CENTER);
				textStat->setFont(attributeFont);
				textPos.x = baseStatLeftX;
				textStat->setSize(textPos);
				textStat->setText("3");
				textStat->setColor(statTextColor);

				auto attributeButton = attributesInnerFrame->addButton("ac button");
				attributeButton->setSize(SDL_Rect{ 12, iconPos.y + 2, attributesFrame->getSize().w - 34, iconPos.h - 2 });
				attributeButton->setColor(makeColor(0, 0, 0, 0));
				attributeButton->setHighlightColor(makeColor(0, 0, 0, 0));
				attributeButton->setHideGlyphs(true);
				attributeButton->setHideKeyboardGlyphs(true);
				attributeButton->setHideSelectors(true);
				attributeButton->setMenuConfirmControlType(Widget::MENU_CONFIRM_CONTROLLER);
				attributeButton->setTickCallback(charsheet_deselect_fn);
			}

			iconPos.y += iconPos.h + rowSpacing;
			textPos.y = iconPos.y + 1;
			attributesInnerFrame->addImage(iconPos, 0xFFFFFFFF, "*#images/ui/CharSheet/HUD_CharSheet_SPWR_00.png", "pwr icon");
			{
				auto textBase = attributesInnerFrame->addField("pwr text title", 8);
				textBase->setVJustify(Field::justify_t::CENTER);
				textBase->setFont(attributeFont);
				textPos.x = headingLeftX;
				textBase->setSize(textPos);
				textBase->setText(Language::get(6095));
				textBase->setColor(statTextColor);
				auto textStat = attributesInnerFrame->addField("pwr text stat", 32);
				textStat->setVJustify(Field::justify_t::CENTER);
				textStat->setHJustify(Field::justify_t::CENTER);
				textStat->setFont(attributeFont);
				textPos.x = baseStatLeftX;
				textStat->setSize(textPos);
				textStat->setText("115%");
				textStat->setColor(statTextColor);

				auto attributeButton = attributesInnerFrame->addButton("pow button");
				attributeButton->setSize(SDL_Rect{ 12, iconPos.y + 2, attributesFrame->getSize().w - 34, iconPos.h - 2 });
				attributeButton->setColor(makeColor(0, 0, 0, 0));
				attributeButton->setHighlightColor(makeColor(0, 0, 0, 0));
				attributeButton->setHideGlyphs(true);
				attributeButton->setHideKeyboardGlyphs(true);
				attributeButton->setHideSelectors(true);
				attributeButton->setMenuConfirmControlType(Widget::MENU_CONFIRM_CONTROLLER);
				attributeButton->setTickCallback(charsheet_deselect_fn);
			}

			iconPos.y += iconPos.h + rowSpacing;
			textPos.y = iconPos.y + 1;
			attributesInnerFrame->addImage(iconPos, 0xFFFFFFFF, "*#images/ui/CharSheet/HUD_CharSheet_RES_00.png", "res icon");
			{
				auto textBase = attributesInnerFrame->addField("res text title", 8);
				textBase->setVJustify(Field::justify_t::CENTER);
				textBase->setFont(attributeFont);
				textPos.x = headingLeftX;
				textBase->setSize(textPos);
				textBase->setText(Language::get(6096));
				textBase->setColor(statTextColor);
				auto textStat = attributesInnerFrame->addField("res text stat", 32);
				textStat->setVJustify(Field::justify_t::CENTER);
				textStat->setHJustify(Field::justify_t::CENTER);
				textStat->setFont(attributeFont);
				textPos.x = baseStatLeftX;
				textStat->setSize(textPos);
				textStat->setText("100%");
				textStat->setColor(statTextColor);

				auto attributeButton = attributesInnerFrame->addButton("res button");
				attributeButton->setSize(SDL_Rect{ 12, iconPos.y + 2, attributesFrame->getSize().w - 34, iconPos.h - 2 });
				attributeButton->setColor(makeColor(0, 0, 0, 0));
				attributeButton->setHighlightColor(makeColor(0, 0, 0, 0));
				attributeButton->setHideGlyphs(true);
				attributeButton->setHideKeyboardGlyphs(true);
				attributeButton->setHideSelectors(true);
				attributeButton->setMenuConfirmControlType(Widget::MENU_CONFIRM_CONTROLLER);
				attributeButton->setTickCallback(charsheet_deselect_fn);
			}

			iconPos.y += iconPos.h + rowSpacing;
			textPos.y = iconPos.y + 1;
			attributesInnerFrame->addImage(iconPos, 0xFFFFFFFF, "*#images/ui/CharSheet/HUD_CharSheet_REGEN_00.png", "regen icon");
			{
				auto textBase = attributesInnerFrame->addField("regen text title", 8);
				textBase->setVJustify(Field::justify_t::CENTER);
				textBase->setFont(attributeFont);
				textPos.x = headingLeftX;
				textBase->setSize(textPos);
				textBase->setText(Language::get(6097));
				textBase->setColor(statTextColor);

				auto textDiv = attributesInnerFrame->addField("regen text divider", 4);
				textDiv->setVJustify(Field::justify_t::CENTER);
				textDiv->setHJustify(Field::justify_t::CENTER);
				textDiv->setFont(attributeFont);
				textPos.x = baseStatLeftX + 0;
				textDiv->setSize(textPos);
				textDiv->setText(" ");
				textDiv->setColor(statTextColor);

				SDL_Rect hpmpTextPos = textPos;
				const int middleX = (textPos.x + textPos.w / 2);
				auto textRegenHP = attributesInnerFrame->addField("regen text hp", 16);
				textRegenHP->setVJustify(Field::justify_t::CENTER);
				textRegenHP->setHJustify(Field::justify_t::RIGHT);
				textRegenHP->setFont(attributeFont);
				hpmpTextPos.x = middleX - 4 - hpmpTextPos.w;
				textRegenHP->setSize(hpmpTextPos);
				textRegenHP->setText("0.2");
				textRegenHP->setColor(statTextColor);

				auto textRegenMP = attributesInnerFrame->addField("regen text mp", 16);
				textRegenMP->setVJustify(Field::justify_t::CENTER);
				textRegenMP->setHJustify(Field::justify_t::LEFT);
				textRegenMP->setFont(attributeFont);
				hpmpTextPos.x = middleX + 4;
				textRegenMP->setSize(hpmpTextPos);
				textRegenMP->setText("0.1");
				textRegenMP->setColor(statTextColor);

				auto attributeButton = attributesInnerFrame->addButton("rgn button");
				const int fullWidth = attributesFrame->getSize().w - 34 + 12;
				attributeButton->setSize(SDL_Rect{ 12, iconPos.y + 2, attributesFrame->getSize().w - 34 - 50, iconPos.h - 2 });
				attributeButton->setColor(makeColor(0, 0, 0, 0));
				attributeButton->setHighlightColor(makeColor(0, 0, 0, 0));
				attributeButton->setHideGlyphs(true);
				attributeButton->setHideKeyboardGlyphs(true);
				attributeButton->setHideSelectors(true);
				attributeButton->setMenuConfirmControlType(Widget::MENU_CONFIRM_CONTROLLER);
				attributeButton->setTickCallback(charsheet_deselect_fn);

				auto attributeButton2 = attributesInnerFrame->addButton("rgn mp button");
				attributeButton2->setSize(SDL_Rect{ attributeButton->getSize().x + attributeButton->getSize().w,
					iconPos.y + 2, fullWidth - (attributeButton->getSize().x + attributeButton->getSize().w), iconPos.h - 2 });
				attributeButton2->setColor(makeColor(0, 0, 0, 0));
				attributeButton2->setHighlightColor(makeColor(0, 0, 0, 0));
				attributeButton2->setHideGlyphs(true);
				attributeButton2->setHideKeyboardGlyphs(true);
				attributeButton2->setHideSelectors(true);
				attributeButton2->setMenuConfirmControlType(Widget::MENU_CONFIRM_CONTROLLER);
				attributeButton2->setTickCallback(charsheet_deselect_fn);
			}

			iconPos.y += iconPos.h + rowSpacing;
			textPos.y = iconPos.y + 1;
			attributesInnerFrame->addImage(iconPos, 0xFFFFFFFF, "*#images/ui/CharSheet/HUD_CharSheet_WGT_00.png", "weight icon");
			{
				auto textBase = attributesInnerFrame->addField("weight text title", 8);
				textBase->setVJustify(Field::justify_t::CENTER);
				textBase->setFont(attributeFont);
				textPos.x = headingLeftX;
				textBase->setSize(textPos);
				textBase->setText(Language::get(6098));
				textBase->setColor(statTextColor);
				auto textStat = attributesInnerFrame->addField("weight text stat", 32);
				textStat->setVJustify(Field::justify_t::CENTER);
				textStat->setHJustify(Field::justify_t::CENTER);
				textStat->setFont(attributeFont);
				textPos.x = baseStatLeftX;
				textStat->setSize(textPos);
				textStat->setText("120");
				textStat->setColor(statTextColor);

				auto attributeButton = attributesInnerFrame->addButton("wgt button");
				attributeButton->setSize(SDL_Rect{ 12, iconPos.y + 2, attributesFrame->getSize().w - 34, iconPos.h - 2 });
				attributeButton->setColor(makeColor(0, 0, 0, 0));
				attributeButton->setHighlightColor(makeColor(0, 0, 0, 0));
				attributeButton->setHideGlyphs(true);
				attributeButton->setHideKeyboardGlyphs(true);
				attributeButton->setHideSelectors(true);
				attributeButton->setMenuConfirmControlType(Widget::MENU_CONFIRM_CONTROLLER);
				attributeButton->setTickCallback(charsheet_deselect_fn);
			}
		}

		{
			auto tooltipFrame = sheetFrame->addFrame("sheet tooltip");
			tooltipFrame->setSize(SDL_Rect{ leftAlignX - 200, 0, 200, 200 });
			tooltipFrame->setHollow(true);
			tooltipFrame->setInheritParentFrameOpacity(false);
			tooltipFrame->setDisabled(true);
			Uint32 color = makeColor(255, 255, 255, 255);
			tooltipFrame->addImage(SDL_Rect{ 0, 0, 6, 6 },
				color, "*#images/ui/CharSheet/HUD_CharSheet_Tooltip_TL_00.png", Player::GUI_t::tooltipEffectBackgroundImages[Player::GUI_t::TOP_LEFT].c_str());
			tooltipFrame->addImage(SDL_Rect{ 0, 0, 6, 6 },
				color, "*#images/ui/CharSheet/HUD_CharSheet_Tooltip_TR_00.png", Player::GUI_t::tooltipEffectBackgroundImages[Player::GUI_t::TOP_RIGHT].c_str());
			tooltipFrame->addImage(SDL_Rect{ 0, 0, 6, 6 },
				color, "*#images/ui/CharSheet/HUD_CharSheet_Tooltip_T_00.png", Player::GUI_t::tooltipEffectBackgroundImages[Player::GUI_t::TOP].c_str());
			tooltipFrame->addImage(SDL_Rect{ 0, 0, 6, 6 },
				color, "*#images/ui/CharSheet/HUD_CharSheet_Tooltip_L_00.png", Player::GUI_t::tooltipEffectBackgroundImages[Player::GUI_t::MIDDLE_LEFT].c_str());
			tooltipFrame->addImage(SDL_Rect{ 0, 0, 6, 6 },
				color, "*#images/ui/CharSheet/HUD_CharSheet_Tooltip_R_00.png", Player::GUI_t::tooltipEffectBackgroundImages[Player::GUI_t::MIDDLE_RIGHT].c_str());
			tooltipFrame->addImage(SDL_Rect{ 0, 0, 6, 6 },
				makeColor(22, 24, 29, 255), "images/system/white.png", Player::GUI_t::tooltipEffectBackgroundImages[Player::GUI_t::MIDDLE].c_str());
			tooltipFrame->addImage(SDL_Rect{ 0, 0, 6, 6 },
				color, "*#images/ui/CharSheet/HUD_CharSheet_Tooltip_BL_00.png", Player::GUI_t::tooltipEffectBackgroundImages[Player::GUI_t::BOTTOM_LEFT].c_str());
			tooltipFrame->addImage(SDL_Rect{ 0, 0, 6, 6 },
				color, "*#images/ui/CharSheet/HUD_CharSheet_Tooltip_BR_00.png", Player::GUI_t::tooltipEffectBackgroundImages[Player::GUI_t::BOTTOM_RIGHT].c_str());
			tooltipFrame->addImage(SDL_Rect{ 0, 0, 6, 6 },
				color, "*#images/ui/CharSheet/HUD_CharSheet_Tooltip_B_00.png", Player::GUI_t::tooltipEffectBackgroundImages[Player::GUI_t::BOTTOM].c_str());
			Player::GUI_t::imageSetWidthHeight9x9(tooltipFrame, Player::GUI_t::tooltipEffectBackgroundImages);
			Player::GUI_t::imageResizeToContainer9x9(tooltipFrame, SDL_Rect{ 0, 0, 200, 200 }, Player::GUI_t::tooltipEffectBackgroundImages);
			auto txt = tooltipFrame->addField("tooltip text", 1024);
			auto txtRightAlignHint = tooltipFrame->addField("tooltip text right align hint", 128);
			const char* tooltipFont = "fonts/pixel_maz_multiline.ttf#16#2";
			txt->setFont(tooltipFont);
			txt->setColor(makeColor(188, 154, 114, 255));
			txtRightAlignHint->setFont(tooltipFont);
			txtRightAlignHint->setColor(hudColors.characterSheetFaintText);
			auto glyph1 = tooltipFrame->addImage(SDL_Rect{ 0, 0, 0, 0 }, 0xFFFFFFFF, "images/system/white.png", "glyph 1");
			glyph1->disabled = true;
			auto glyph2 = tooltipFrame->addImage(SDL_Rect{ 0, 0, 0, 0 }, 0xFFFFFFFF, "images/system/white.png", "glyph 2");
			glyph2->disabled = true;
			auto glyph3 = tooltipFrame->addImage(SDL_Rect{ 0, 0, 0, 0 }, 0xFFFFFFFF, "images/system/white.png", "glyph 3");
			glyph3->disabled = true;
			auto glyph4 = tooltipFrame->addImage(SDL_Rect{ 0, 0, 0, 0 }, 0xFFFFFFFF, "images/system/white.png", "glyph 4");
			glyph4->disabled = true;

			for ( int i = 1; i <= NUM_CHARSHEET_TOOLTIP_TEXT_FIELDS; ++i )
			{
				char txtEntryFieldName[64] = "";
				snprintf(txtEntryFieldName, sizeof(txtEntryFieldName), "txt %d", i);
				auto txtEntry = tooltipFrame->addField(txtEntryFieldName, 1024);
				txtEntry->setFont(tooltipFont);
				txtEntry->setDisabled(true);
				txtEntry->setVJustify(Field::justify_t::CENTER);
				txtEntry->setColor(makeColor(188, 154, 114, 255));
				txtEntry->setOntop(true);

				characterSheetTooltipTextFields[player.playernum][i] = txtEntry;
			}

			auto div = tooltipFrame->addImage(SDL_Rect{ 0, 0, 0, 1 },
				makeColor(49, 53, 61, 255),
				"images/system/white.png", "tooltip divider 1");
			div->disabled = true;
			div = tooltipFrame->addImage(SDL_Rect{ 0, 0, 0, 1 },
				makeColor(49, 53, 61, 255),
				"images/system/white.png", "tooltip divider 2");
			div->disabled = true;

			for ( int i = 1; i <= NUM_CHARSHEET_TOOLTIP_BACKING_FRAMES; ++i )
			{
				char backingFrameName[64] = "";
				snprintf(backingFrameName, sizeof(backingFrameName), "txt value backing frame %d", i);
				auto txtValueBackingFrame = tooltipFrame->addFrame(backingFrameName);
				txtValueBackingFrame->setSize(SDL_Rect{ 0, 0, tooltipFrame->getSize().w, tooltipFrame->getSize().h });
				txtValueBackingFrame->setDisabled(true);
				Uint32 color = makeColor(51, 33, 26, 255);
				txtValueBackingFrame->addImage(SDL_Rect{ 0, 0, 6, 6 },
					color, "*#images/ui/SkillSheet/UI_Skills_EffectBG_TL00.png", Player::GUI_t::tooltipEffectBackgroundImages[Player::GUI_t::TOP_LEFT].c_str());
				txtValueBackingFrame->addImage(SDL_Rect{ 0, 0, 6, 6 },
					color, "*#images/ui/SkillSheet/UI_Skills_EffectBG_TR00.png", Player::GUI_t::tooltipEffectBackgroundImages[Player::GUI_t::TOP_RIGHT].c_str());
				txtValueBackingFrame->addImage(SDL_Rect{ 0, 0, 6, 6 },
					color, "*#images/ui/SkillSheet/UI_Skills_EffectBG_T00.png", Player::GUI_t::tooltipEffectBackgroundImages[Player::GUI_t::TOP].c_str());
				txtValueBackingFrame->addImage(SDL_Rect{ 0, 0, 6, 6 },
					color, "*#images/ui/SkillSheet/UI_Skills_EffectBG_L00.png", Player::GUI_t::tooltipEffectBackgroundImages[Player::GUI_t::MIDDLE_LEFT].c_str());
				txtValueBackingFrame->addImage(SDL_Rect{ 0, 0, 6, 6 },
					color, "*#images/ui/SkillSheet/UI_Skills_EffectBG_R00.png", Player::GUI_t::tooltipEffectBackgroundImages[Player::GUI_t::MIDDLE_RIGHT].c_str());
				txtValueBackingFrame->addImage(SDL_Rect{ 0, 0, 6, 6 },
					color, "*#images/ui/SkillSheet/UI_Skills_EffectBG_M00.png", Player::GUI_t::tooltipEffectBackgroundImages[Player::GUI_t::MIDDLE].c_str());
				txtValueBackingFrame->addImage(SDL_Rect{ 0, 0, 6, 6 },
					color, "*#images/ui/SkillSheet/UI_Skills_EffectBG_BL00.png", Player::GUI_t::tooltipEffectBackgroundImages[Player::GUI_t::BOTTOM_LEFT].c_str());
				txtValueBackingFrame->addImage(SDL_Rect{ 0, 0, 6, 6 },
					color, "*#images/ui/SkillSheet/UI_Skills_EffectBG_BR00.png", Player::GUI_t::tooltipEffectBackgroundImages[Player::GUI_t::BOTTOM_RIGHT].c_str());
				txtValueBackingFrame->addImage(SDL_Rect{ 0, 0, 6, 6 },
					color, "*#images/ui/SkillSheet/UI_Skills_EffectBG_B00.png", Player::GUI_t::tooltipEffectBackgroundImages[Player::GUI_t::BOTTOM].c_str());
				Player::GUI_t::imageSetWidthHeight9x9(txtValueBackingFrame, Player::GUI_t::tooltipEffectBackgroundImages);

				characterSheetTooltipTextBackingFrames[player.playernum][i] = txtValueBackingFrame;
			}

			{
				auto raceTooltip = tooltipFrame->addFrame("sheet race tooltip");
				raceTooltip->setSize(SDL_Rect{ 0, 24, 324, 664 });
				raceTooltip->setDisabled(true);
				raceTooltip->setHollow(true);
				const auto font = smallfont_outline;

				auto details_text = raceTooltip->addField("details", 1024);
				details_text->setFont(font);
				details_text->setSize(SDL_Rect{ 0, 0, 242, 300 });
				details_text->setColor(hudColors.characterSheetOffWhiteText);

				auto details_text_right = raceTooltip->addField("details_right", 1024);
				details_text_right->setFont(font);
				details_text_right->setSize(SDL_Rect{ 121, 0, 121, 300 });
				details_text_right->setColor(hudColors.characterSheetOffWhiteText);

				MainMenu::RaceDescriptions::update_details_text(*raceTooltip, stats[player.playernum]);
			}

			{
				auto classTooltip = tooltipFrame->addFrame("sheet class tooltip");
				classTooltip->setDisabled(true);
				classTooltip->setHollow(true);
				auto statGrowths = classTooltip->addFrame("stat growths");
				// stats definitions
				const char* class_stats_text[] = {
					ItemTooltips.getItemStatShortName("STR").c_str(), 
					ItemTooltips.getItemStatShortName("DEX").c_str(),
					ItemTooltips.getItemStatShortName("CON").c_str(),
					ItemTooltips.getItemStatShortName("INT").c_str(),
					ItemTooltips.getItemStatShortName("PER").c_str(),
					ItemTooltips.getItemStatShortName("CHR").c_str()
				};
				constexpr int num_class_stats = sizeof(class_stats_text) / sizeof(class_stats_text[0]);
				constexpr SDL_Rect bottom{ 0, 0, 236, 36 };
				constexpr int column = bottom.w / num_class_stats;

				classTooltip->setSize(SDL_Rect{ 0, 0, bottom.w, bottom.h });
				statGrowths->setSize(bottom);

				for ( int c = 0; c < num_class_stats; ++c ) 
				{
					char buf[16];
					snprintf(buf, sizeof(buf), "%d", c);
					auto class_stat = statGrowths->addField(buf, 16);
					class_stat->setSize(SDL_Rect{
						bottom.x + column * c, bottom.y, column, bottom.h });
					class_stat->setHJustify(Field::justify_t::CENTER);
					class_stat->setVJustify(Field::justify_t::TOP);
					class_stat->setFont(smallfont_outline);
					class_stat->setText(class_stats_text[c]);
					//class_stat->setTickCallback([](Widget& widget) {class_stat_fn(*static_cast<Field*>(&widget), widget.getOwner()); });

					SDL_Rect imgPos = class_stat->getSize();
					imgPos.x += imgPos.w / 2;
					imgPos.w = 14;
					imgPos.x -= imgPos.w / 2;
					imgPos.h = 16;
					imgPos.y -= imgPos.h - 4;

					char buf2[32];
					/*snprintf(buf2, sizeof(buf2), "stat img top %d", c);
					auto class_stat_img_top = statGrowths->addImage(imgPos, 0xFFFFFFFF,
						"*#images/ui/Main Menus/Play/PlayerCreation/ClassSelection/statgrowth_hi2.png", buf2);
					class_stat_img_top->disabled = true;*/
					snprintf(buf2, sizeof(buf2), "stat img bottom %d", c);
					imgPos.y = class_stat->getSize().y + 17;
					auto class_stat_img_bottom = statGrowths->addImage(imgPos, 0xFFFFFFFF,
						"*#images/ui/Main Menus/Play/PlayerCreation/ClassSelection/statgrowth_lo2.png", buf2);
					class_stat_img_bottom->disabled = true;
				}
			}
		}
	}
}

void Player::CharacterSheet_t::selectElement(SheetElements element, bool usingMouse, bool moveCursor)
{
	selectedElement = element;

	Frame* elementFrame = nullptr;
	Frame::image_t* img = nullptr;
	Field* elementField = nullptr;
	Button* elementButton = nullptr;
	bool selectedAButton = false;
	switch ( element )
	{
		case SHEET_OPEN_LOG:
			if ( elementFrame = sheetFrame->findFrame("log map buttons") )
			{
				if ( !inputs.getVirtualMouse(player.playernum)->draw_cursor )
				{
					elementFrame->findButton("log button")->select();
					selectedAButton = true;
				}
				elementFrame = elementFrame->findFrame("log button selector");
				//img = elementFrame->findImage("log button img");
			}
			break;
		case SHEET_OPEN_MAP:
			if ( elementFrame = sheetFrame->findFrame("log map buttons") )
			{
				if ( !inputs.getVirtualMouse(player.playernum)->draw_cursor )
				{
					elementFrame->findButton("map button")->select();
					selectedAButton = true;
				}
				elementFrame = elementFrame->findFrame("map button selector");
				//img = elementFrame->findImage("map button img");
			}
			break;
		case SHEET_SKILL_LIST:
			elementFrame = sheetFrame->findFrame("skills button frame");
			if ( !inputs.getVirtualMouse(player.playernum)->draw_cursor )
			{
				elementFrame->findButton("skills button")->select();
				selectedAButton = true;
			}
			break;
		case SHEET_TIMER:
			elementFrame = sheetFrame->findFrame("game timer");
			if ( !inputs.getVirtualMouse(player.playernum)->draw_cursor )
			{
				elementFrame->findButton("timer selector")->select();
				selectedAButton = true;
			}
			break;
		case SHEET_GOLD:
			if ( elementFrame = sheetFrame->findFrame("character info") )
			{
				if ( elementFrame = elementFrame->findFrame("character info inner frame") )
				{
					elementButton = elementFrame->findButton("character gold selector");
				}
			}
			break;
		case SHEET_DUNGEON_FLOOR:
			if ( elementFrame = sheetFrame->findFrame("dungeon floor frame") )
			{
				elementButton = elementFrame->findButton("dungeon floor selector");
			}
			break;
		case SHEET_CHAR_CLASS:
			if ( elementFrame = sheetFrame->findFrame("character info") )
			{
				if ( elementFrame = elementFrame->findFrame("character info inner frame") )
				{
					elementButton = elementFrame->findButton("character class selector");
				}
			}
			break;
		case SHEET_CHAR_RACE_SEX:
			if ( elementFrame = sheetFrame->findFrame("character info") )
			{
				if ( elementFrame = elementFrame->findFrame("character info inner frame") )
				{
					elementButton = elementFrame->findButton("character race selector");
				}
			}
			break;
		case SHEET_STR:
			if ( elementFrame = sheetFrame->findFrame("stats") )
			{
				if ( elementFrame = elementFrame->findFrame("stats inner frame") )
				{
					elementButton = elementFrame->findButton("str button");
				}
			}
			break;
		case SHEET_DEX:
			if ( elementFrame = sheetFrame->findFrame("stats") )
			{
				if ( elementFrame = elementFrame->findFrame("stats inner frame") )
				{
					elementButton = elementFrame->findButton("dex button");
				}
			}
			break;
		case SHEET_CON:
			if ( elementFrame = sheetFrame->findFrame("stats") )
			{
				if ( elementFrame = elementFrame->findFrame("stats inner frame") )
				{
					elementButton = elementFrame->findButton("con button");
				}
			}
			break;
		case SHEET_INT:
			if ( elementFrame = sheetFrame->findFrame("stats") )
			{
				if ( elementFrame = elementFrame->findFrame("stats inner frame") )
				{
					elementButton = elementFrame->findButton("int button");
				}
			}
			break;
		case SHEET_PER:
			if ( elementFrame = sheetFrame->findFrame("stats") )
			{
				if ( elementFrame = elementFrame->findFrame("stats inner frame") )
				{
					elementButton = elementFrame->findButton("per button");
				}
			}
			break;
		case SHEET_CHR:
			if ( elementFrame = sheetFrame->findFrame("stats") )
			{
				if ( elementFrame = elementFrame->findFrame("stats inner frame") )
				{
					elementButton = elementFrame->findButton("chr button");
				}
			}
			break;
		case SHEET_ATK:
			if ( elementFrame = sheetFrame->findFrame("attributes") )
			{
				if ( elementFrame = elementFrame->findFrame("attributes inner frame") )
				{
					elementButton = elementFrame->findButton("atk button");
				}
			}
			break;
		case SHEET_AC:
			if ( elementFrame = sheetFrame->findFrame("attributes") )
			{
				if ( elementFrame = elementFrame->findFrame("attributes inner frame") )
				{
					elementButton = elementFrame->findButton("ac button");
				}
			}
			break;
		case SHEET_POW:
			if ( elementFrame = sheetFrame->findFrame("attributes") )
			{
				if ( elementFrame = elementFrame->findFrame("attributes inner frame") )
				{
					elementButton = elementFrame->findButton("pow button");
				}
			}
			break;
		case SHEET_RES:
			if ( elementFrame = sheetFrame->findFrame("attributes") )
			{
				if ( elementFrame = elementFrame->findFrame("attributes inner frame") )
				{
					elementButton = elementFrame->findButton("res button");
				}
			}
			break;
		case SHEET_RGN:
			if ( elementFrame = sheetFrame->findFrame("attributes") )
			{
				if ( elementFrame = elementFrame->findFrame("attributes inner frame") )
				{
					elementButton = elementFrame->findButton("rgn button");
				}
			}
			break;
		case SHEET_RGN_MP:
			if ( elementFrame = sheetFrame->findFrame("attributes") )
			{
				if ( elementFrame = elementFrame->findFrame("attributes inner frame") )
				{
					elementButton = elementFrame->findButton("rgn mp button");
				}
			}
			break;
		case SHEET_WGT:
			if ( elementFrame = sheetFrame->findFrame("attributes") )
			{
				if ( elementFrame = elementFrame->findFrame("attributes inner frame") )
				{
					elementButton = elementFrame->findButton("wgt button");
				}
			}
			break;
		default:
			elementFrame = nullptr;
			break;
	}

	SDL_Rect pos{ 0, 0, 0, 0 };
	if ( elementFrame && moveCursor )
	{
		elementFrame->warpMouseToFrame(player.playernum, (Inputs::SET_CONTROLLER));
		pos = elementFrame->getAbsoluteSize();
		if ( img )
		{
			pos.x += img->pos.x;
			pos.y += img->pos.y;
			pos.w = img->pos.w;
			pos.h = img->pos.h;
		}
		if ( elementButton )
		{
			pos = elementButton->getAbsoluteSize();
		}
		if ( elementField )
		{
			pos = elementField->getAbsoluteSize();
		}
		// make sure to adjust absolute size to camera viewport
		pos.x -= player.camera_virtualx1();
		pos.y -= player.camera_virtualy1();
		player.hud.setCursorDisabled(false);
		if ( !isInteractable )
		{
			player.characterSheet.queuedElement = player.characterSheet.selectedElement;
		}
		else
		{
			player.characterSheet.queuedElement = SHEET_UNSELECTED;
			player.hud.updateCursorAnimation(pos.x - 1, pos.y - 1, pos.w, pos.h, usingMouse);
		}
	}
	if ( !selectedAButton && elementFrame && !inputs.getVirtualMouse(player.playernum)->draw_cursor )
	{
		if ( elementButton )
		{
			elementButton->select();
		}
		else
		{
			elementFrame->select();
		}
	}
}

void Player::CharacterSheet_t::updateGameTimer()
{
	auto characterInfoFrame = sheetFrame->findFrame("character info");
	auto timerFrame = sheetFrame->findFrame("game timer");
	auto timerText = timerFrame->findField("timer text");
	char buf[32];

	Uint32 sec = (completionTime / TICKS_PER_SECOND) % 60;
	Uint32 min = ((completionTime / TICKS_PER_SECOND) / 60) % 60;
	Uint32 hour = (((completionTime / TICKS_PER_SECOND) / 60) / 60) % 24;
	Uint32 day = ((completionTime / TICKS_PER_SECOND) / 60) / 60 / 24;
	snprintf(buf, sizeof(buf), "%02d:%02d:%02d:%02d", day, hour, min, sec);
	timerText->setText(buf);

	bool enableTooltips = !player.GUI.isDropdownActive() && !player.GUI.dropdownMenu.bClosedThisTick && inputs.getVirtualMouse(player.playernum)->draw_cursor;

	bool bCompactView = player.bUseCompactGUIHeight();
	if ( bCompactView )
	{
		enableTooltips = false;
	}

	if ( selectedElement == SHEET_TIMER && enableTooltips )
	{
		SDL_Rect tooltipPos = characterInfoFrame->getSize();
		tooltipPos.y = timerFrame->getSize().y;
		Player::PanelJustify_t tooltipJustify = PANEL_JUSTIFY_RIGHT;
		if ( (panelJustify == PANEL_JUSTIFY_LEFT && !bCompactView) || (panelJustify == PANEL_JUSTIFY_RIGHT && bCompactView) )
		{
			tooltipJustify = PANEL_JUSTIFY_LEFT;
			tooltipPos.x += tooltipPos.w;
		}
		updateCharacterSheetTooltip(SHEET_TIMER, tooltipPos, tooltipJustify);
	}
}

std::string& Player::CharacterSheet_t::getHoverTextString(std::string key)
{
	if ( hoverTextStrings.find(key) != hoverTextStrings.end() )
	{
		return hoverTextStrings[key];
	}
	return defaultString;
}

bool getAttackTooltipLines(int playernum, AttackHoverText_t& attackHoverTextInfo, int lineNumber, char titleBuf[128], char valueBuf[128])
{
	std::string skillName = "-";
	Sint32 skillLVL = 0;
	for ( auto& skill : Player::SkillSheet_t::skillSheetData.skillEntries )
	{
		if ( skill.skillId == attackHoverTextInfo.proficiency )
		{
			skillName = skill.getSkillName();
			skillLVL = stats[playernum]->getModifiedProficiency(attackHoverTextInfo.proficiency);
			break;
		}
	}

	if ( attackHoverTextInfo.hoverType == AttackHoverText_t::ATK_HOVER_TYPE_MELEE_WEAPON
		|| attackHoverTextInfo.hoverType == AttackHoverText_t::ATK_HOVER_TYPE_RAPIER )
	{
		switch ( lineNumber )
		{
			case 1:
				snprintf(titleBuf, 127, "%s", Player::CharacterSheet_t::getHoverTextString("attributes_atk_avg").c_str());
				snprintf(valueBuf, 127,
					Player::CharacterSheet_t::getHoverTextString("attributes_atk_average_format").c_str(),
					(real_t)attackHoverTextInfo.attackMinRange + ((attackHoverTextInfo.attackMaxRange - attackHoverTextInfo.attackMinRange) / 2.0));
				return true;
			case 2:
				snprintf(titleBuf, 127, Player::CharacterSheet_t::getHoverTextString("attributes_atk_range").c_str(),
					skillName.c_str(), skillLVL);
				snprintf(valueBuf, 127,
					Player::CharacterSheet_t::getHoverTextString("attributes_atk_range_format").c_str(), 
					attackHoverTextInfo.attackMinRange, attackHoverTextInfo.attackMaxRange);
				return true;
			case 3:
				if ( attackHoverTextInfo.hoverType == AttackHoverText_t::ATK_HOVER_TYPE_RAPIER )
				{
					snprintf(titleBuf, 127, "%s", Player::CharacterSheet_t::getHoverTextString("attributes_atk_entry_attr_bonus_ranged").c_str());
				}
				else
				{
					snprintf(titleBuf, 127, "%s", Player::CharacterSheet_t::getHoverTextString("attributes_atk_entry_attr_bonus_melee").c_str());
				}
				snprintf(valueBuf, 127,
					Player::CharacterSheet_t::getHoverTextString("attributes_atk_bonus_format").c_str(),
					attackHoverTextInfo.mainAttributeBonus);
				return true;
			case 4:
				snprintf(titleBuf, 127, "%s", Player::CharacterSheet_t::getHoverTextString("attributes_atk_entry_weapon_bonus").c_str());
				snprintf(valueBuf, 127,
					Player::CharacterSheet_t::getHoverTextString("attributes_atk_bonus_format").c_str(),
					attackHoverTextInfo.weaponBonus);
				return true;
			case 5:
				snprintf(titleBuf, 127, "%s", Player::CharacterSheet_t::getHoverTextString("attributes_atk_entry_melee_weapon_base").c_str());
				snprintf(valueBuf, 127,
					Player::CharacterSheet_t::getHoverTextString("attributes_atk_bonus_format").c_str(),
					BASE_MELEE_DAMAGE);
				return true;
			case 6:
				return false;
			default:
				return false;
		}
	}
	else if ( attackHoverTextInfo.hoverType == AttackHoverText_t::ATK_HOVER_TYPE_UNARMED )
	{
		switch ( lineNumber )
		{
			case 1:
				snprintf(titleBuf, 127, "%s", Player::CharacterSheet_t::getHoverTextString("attributes_atk_avg").c_str());
				snprintf(valueBuf, 127,
					Player::CharacterSheet_t::getHoverTextString("attributes_atk_average_format").c_str(),
					(real_t)attackHoverTextInfo.attackMinRange + ((attackHoverTextInfo.attackMaxRange - attackHoverTextInfo.attackMinRange) / 2.0));
				return true;
			case 2:
				snprintf(titleBuf, 127, Player::CharacterSheet_t::getHoverTextString("attributes_atk_range").c_str(),
					skillName.c_str(), skillLVL);
				snprintf(valueBuf, 127,
					Player::CharacterSheet_t::getHoverTextString("attributes_atk_range_format").c_str(),
					attackHoverTextInfo.attackMinRange, attackHoverTextInfo.attackMaxRange);
				return true;
			case 3:
				snprintf(titleBuf, 127, "%s", Player::CharacterSheet_t::getHoverTextString("attributes_atk_entry_attr_bonus_melee").c_str());
				snprintf(valueBuf, 127,
					Player::CharacterSheet_t::getHoverTextString("attributes_atk_bonus_format").c_str(),
					attackHoverTextInfo.mainAttributeBonus);
				return true;
			case 4:
				snprintf(titleBuf, 127, "%s", Player::CharacterSheet_t::getHoverTextString("attributes_atk_entry_items_bonus").c_str());
				snprintf(valueBuf, 127,
					Player::CharacterSheet_t::getHoverTextString("attributes_atk_bonus_format").c_str(),
					attackHoverTextInfo.equipmentAndEffectBonus);
				return true;
			case 5:
				snprintf(titleBuf, 127, "%s", Player::CharacterSheet_t::getHoverTextString("attributes_atk_entry_skill_bonus").c_str());
				snprintf(valueBuf, 127,
					Player::CharacterSheet_t::getHoverTextString("attributes_atk_bonus_format").c_str(),
					attackHoverTextInfo.proficiencyBonus);
				return true;
			case 6:
				snprintf(titleBuf, 127, "%s", Player::CharacterSheet_t::getHoverTextString("attributes_atk_entry_unarmed_weapon_base").c_str());
				snprintf(valueBuf, 127,
					Player::CharacterSheet_t::getHoverTextString("attributes_atk_bonus_format").c_str(),
					BASE_PLAYER_UNARMED_DAMAGE);
				return true;
			default:
				return false;
		}
	}
	else if ( attackHoverTextInfo.hoverType == AttackHoverText_t::ATK_HOVER_TYPE_PICKAXE )
	{
		switch ( lineNumber )
		{
			case 1:
				snprintf(titleBuf, 127, "%s", Player::CharacterSheet_t::getHoverTextString("attributes_atk_avg").c_str());
				snprintf(valueBuf, 127,
					Player::CharacterSheet_t::getHoverTextString("attributes_atk_average_format").c_str(),
					(real_t)attackHoverTextInfo.attackMinRange + ((attackHoverTextInfo.attackMaxRange - attackHoverTextInfo.attackMinRange) / 2.0));
				return true;
			case 2:
				snprintf(titleBuf, 127, Player::CharacterSheet_t::getHoverTextString("attributes_atk_range_noskill").c_str());
				snprintf(valueBuf, 127,
					Player::CharacterSheet_t::getHoverTextString("attributes_atk_range_format").c_str(),
					attackHoverTextInfo.attackMinRange, attackHoverTextInfo.attackMaxRange);
				return true;
			case 3:
				snprintf(titleBuf, 127, "%s", Player::CharacterSheet_t::getHoverTextString("attributes_atk_entry_attr_bonus_melee").c_str());
				snprintf(valueBuf, 127,
					Player::CharacterSheet_t::getHoverTextString("attributes_atk_bonus_format").c_str(),
					attackHoverTextInfo.mainAttributeBonus);
				return true;
			case 4:
				snprintf(titleBuf, 127, "%s", Player::CharacterSheet_t::getHoverTextString("attributes_atk_entry_weapon_bonus").c_str());
				snprintf(valueBuf, 127,
					Player::CharacterSheet_t::getHoverTextString("attributes_atk_bonus_format").c_str(),
					attackHoverTextInfo.weaponBonus);
				return true;
			case 5:
				snprintf(titleBuf, 127, "%s", Player::CharacterSheet_t::getHoverTextString("attributes_atk_entry_melee_weapon_base").c_str());
				snprintf(valueBuf, 127,
					Player::CharacterSheet_t::getHoverTextString("attributes_atk_bonus_format").c_str(),
					BASE_MELEE_DAMAGE);
				return true;
			default:
				return false;
		}
	}
	else if ( attackHoverTextInfo.hoverType == AttackHoverText_t::ATK_HOVER_TYPE_RANGED )
	{
		switch ( lineNumber )
		{
			case 1:
				snprintf(titleBuf, 127, "%s", Player::CharacterSheet_t::getHoverTextString("attributes_atk_avg").c_str());
				snprintf(valueBuf, 127,
					Player::CharacterSheet_t::getHoverTextString("attributes_atk_average_format").c_str(),
					(real_t)attackHoverTextInfo.attackMinRange + ((attackHoverTextInfo.attackMaxRange - attackHoverTextInfo.attackMinRange) / 2.0));
				return true;
			case 2:
				snprintf(titleBuf, 127, Player::CharacterSheet_t::getHoverTextString("attributes_atk_range").c_str(),
					skillName.c_str(), skillLVL);
				snprintf(valueBuf, 127,
					Player::CharacterSheet_t::getHoverTextString("attributes_atk_range_format").c_str(),
					attackHoverTextInfo.attackMinRange, attackHoverTextInfo.attackMaxRange);
				return true;
			case 3:
				snprintf(titleBuf, 127, "%s", Player::CharacterSheet_t::getHoverTextString("attributes_atk_entry_attr_bonus_ranged").c_str());
				snprintf(valueBuf, 127,
					Player::CharacterSheet_t::getHoverTextString("attributes_atk_bonus_format").c_str(),
					attackHoverTextInfo.mainAttributeBonus);
				return true;
			case 4:
				snprintf(titleBuf, 127, "%s", Player::CharacterSheet_t::getHoverTextString("attributes_atk_entry_weapon_bonus").c_str());
				snprintf(valueBuf, 127,
					Player::CharacterSheet_t::getHoverTextString("attributes_atk_bonus_format").c_str(),
					attackHoverTextInfo.weaponBonus);
				return true;
			case 5:
				snprintf(titleBuf, 127, "%s", Player::CharacterSheet_t::getHoverTextString("attributes_atk_entry_items_bonus").c_str());
				snprintf(valueBuf, 127,
					Player::CharacterSheet_t::getHoverTextString("attributes_atk_bonus_format").c_str(),
					attackHoverTextInfo.equipmentAndEffectBonus);
				return true;
			case 6:
				snprintf(titleBuf, 127, "%s", Player::CharacterSheet_t::getHoverTextString("attributes_atk_entry_ranged_weapon_base").c_str());
				snprintf(valueBuf, 127,
					Player::CharacterSheet_t::getHoverTextString("attributes_atk_bonus_format").c_str(),
					BASE_RANGED_DAMAGE);
				return true;
			default:
				return false;
		}
	}
	else if ( attackHoverTextInfo.hoverType == AttackHoverText_t::ATK_HOVER_TYPE_THROWN
		|| attackHoverTextInfo.hoverType == AttackHoverText_t::ATK_HOVER_TYPE_THROWN_MISC
		|| attackHoverTextInfo.hoverType == AttackHoverText_t::ATK_HOVER_TYPE_THROWN_GEM )
	{
		switch ( lineNumber )
		{
			case 1:
				snprintf(titleBuf, 127, Player::CharacterSheet_t::getHoverTextString("attributes_atk_range").c_str(),
					skillName.c_str(), skillLVL);
				snprintf(valueBuf, 127,
					Player::CharacterSheet_t::getHoverTextString("attributes_atk_range_format").c_str(),
					attackHoverTextInfo.attackMinRange, attackHoverTextInfo.attackMaxRange);
				return true;
			case 2:
				strcpy(titleBuf, "");
				strcpy(valueBuf, "");
				return true;
			case 3:
				snprintf(titleBuf, 127, "%s", Player::CharacterSheet_t::getHoverTextString("attributes_atk_entry_attr_bonus_ranged").c_str());
				if ( attackHoverTextInfo.hoverType == AttackHoverText_t::ATK_HOVER_TYPE_THROWN_MISC )
				{
					snprintf(valueBuf, 127, "-");
				}
				else
				{
					snprintf(valueBuf, 127,
						Player::CharacterSheet_t::getHoverTextString("attributes_atk_bonus_format").c_str(),
						attackHoverTextInfo.mainAttributeBonus);
				}
				return true;
			case 4:
				snprintf(titleBuf, 127, "%s", Player::CharacterSheet_t::getHoverTextString("attributes_atk_entry_weapon_bonus").c_str());
				snprintf(valueBuf, 127,
					Player::CharacterSheet_t::getHoverTextString("attributes_atk_bonus_format").c_str(),
					attackHoverTextInfo.weaponBonus);
				return true;
			case 5:
				snprintf(titleBuf, 127, "%s", Player::CharacterSheet_t::getHoverTextString("attributes_atk_entry_skill_bonus").c_str());
				if ( attackHoverTextInfo.hoverType == AttackHoverText_t::ATK_HOVER_TYPE_THROWN_MISC )
				{
					snprintf(valueBuf, 127, "-");
				}
				else
				{
					snprintf(valueBuf, 127,
						Player::CharacterSheet_t::getHoverTextString("attributes_atk_bonus_format").c_str(),
						attackHoverTextInfo.proficiencyBonus);
				}
				return true;
			case 6:
				snprintf(titleBuf, 127, "%s", Player::CharacterSheet_t::getHoverTextString("attributes_atk_entry_thrown_weapon_fully_charged").c_str());
				snprintf(valueBuf, 127,
					Player::CharacterSheet_t::getHoverTextString("attributes_atk_bonus_format").c_str(),
					3);
				return true;
			case 7:
				snprintf(titleBuf, 127, "%s", Player::CharacterSheet_t::getHoverTextString("attributes_atk_entry_thrown_weapon_base").c_str());
				if ( attackHoverTextInfo.hoverType == AttackHoverText_t::ATK_HOVER_TYPE_THROWN_MISC )
				{
					snprintf(valueBuf, 127, "-");
				}
				else
				{
					snprintf(valueBuf, 127,
						Player::CharacterSheet_t::getHoverTextString("attributes_atk_bonus_format").c_str(),
						BASE_THROWN_DAMAGE);
				}
				return true;
			default:
				return false;
		}
	}
	else if ( attackHoverTextInfo.hoverType == AttackHoverText_t::ATK_HOVER_TYPE_THROWN_POTION )
	{
		switch ( lineNumber )
		{
			case 1:
				snprintf(titleBuf, 127, "%s", Player::CharacterSheet_t::getHoverTextString("attributes_atk_avg").c_str());
				snprintf(valueBuf, 127,
					Player::CharacterSheet_t::getHoverTextString("attributes_atk_average_format").c_str(),
					(real_t)attackHoverTextInfo.attackMinRange + ((attackHoverTextInfo.attackMaxRange - attackHoverTextInfo.attackMinRange) / 2.0));
				return true;
			case 2:
				snprintf(titleBuf, 127, Player::CharacterSheet_t::getHoverTextString("attributes_atk_range").c_str(),
					skillName.c_str(), skillLVL);
				snprintf(valueBuf, 127,
					Player::CharacterSheet_t::getHoverTextString("attributes_atk_range_format").c_str(),
					attackHoverTextInfo.attackMinRange, attackHoverTextInfo.attackMaxRange);
				return true;
			case 3:
				snprintf(titleBuf, 127, "%s", Player::CharacterSheet_t::getHoverTextString("attributes_atk_entry_weapon_bonus").c_str());
				snprintf(valueBuf, 127,
					Player::CharacterSheet_t::getHoverTextString("attributes_atk_bonus_format").c_str(),
					attackHoverTextInfo.weaponBonus);
				return true;
			case 4:
				snprintf(titleBuf, 127, "%s", Player::CharacterSheet_t::getHoverTextString("attributes_atk_entry_skill_bonus").c_str());
				snprintf(valueBuf, 127,
					Player::CharacterSheet_t::getHoverTextString("attributes_atk_bonus_format").c_str(),
					attackHoverTextInfo.proficiencyBonus);
				return true;
			case 5:
				snprintf(titleBuf, 127, "%s", Player::CharacterSheet_t::getHoverTextString("attributes_atk_entry_thrown_weapon_base").c_str());
				snprintf(valueBuf, 127,
					Player::CharacterSheet_t::getHoverTextString("attributes_atk_bonus_format").c_str(),
					BASE_THROWN_DAMAGE);
				return true;
			default:
				return false;
		}
	}
	else if ( attackHoverTextInfo.hoverType == AttackHoverText_t::ATK_HOVER_TYPE_WHIP )
	{
		switch ( lineNumber )
		{
			case 1:
				snprintf(titleBuf, 127, "%s", Player::CharacterSheet_t::getHoverTextString("attributes_atk_avg").c_str());
				snprintf(valueBuf, 127,
					Player::CharacterSheet_t::getHoverTextString("attributes_atk_nobonus_format").c_str(),
					attackHoverTextInfo.totalAttack);
				return true;
			case 2:
				snprintf(titleBuf, 127, Player::CharacterSheet_t::getHoverTextString("attributes_atk_range").c_str(),
					skillName.c_str(), skillLVL);
				snprintf(valueBuf, 127,
					Player::CharacterSheet_t::getHoverTextString("attributes_atk_range_format").c_str(),
					attackHoverTextInfo.attackMinRange, attackHoverTextInfo.attackMaxRange);
				return true;
			case 3:
				snprintf(titleBuf, 127, "%s", Player::CharacterSheet_t::getHoverTextString("attributes_atk_entry_attr_bonus_whip").c_str());
				snprintf(valueBuf, 127,
					Player::CharacterSheet_t::getHoverTextString("attributes_atk_bonus_format").c_str(),
					attackHoverTextInfo.mainAttributeBonus);
				return true;
			case 4:
				snprintf(titleBuf, 127, "%s", Player::CharacterSheet_t::getHoverTextString("attributes_atk_entry_weapon_bonus").c_str());
				snprintf(valueBuf, 127,
					Player::CharacterSheet_t::getHoverTextString("attributes_atk_bonus_format").c_str(),
					attackHoverTextInfo.weaponBonus);
				return true;
			case 5:
				snprintf(titleBuf, 127, "%s", Player::CharacterSheet_t::getHoverTextString("attributes_atk_entry_melee_weapon_base").c_str());
				snprintf(valueBuf, 127,
					Player::CharacterSheet_t::getHoverTextString("attributes_atk_bonus_format").c_str(),
					BASE_MELEE_DAMAGE);
				return true;
			default:
				return false;
		}
	}
	else
	{
		switch ( lineNumber )
		{
			case 1:
				snprintf(titleBuf, 127, "%s", Player::CharacterSheet_t::getHoverTextString("attributes_atk_avg").c_str());
				snprintf(valueBuf, 127, "-");
				return true;
			case 2:
				strcpy(titleBuf, "");
				strcpy(valueBuf, "");
				return true;
			default:
				return false;
		}
	}
	return false;
}

real_t getDisplayedHPRegen(Entity* my, Stat& myStats, Uint32* outColor, char buf[32], bool excludeItemsEffectsBonus = false)
{
	real_t regen = 0.0;
	if ( outColor )
	{
		*outColor = hudColors.characterSheetNeutral;
	}
	if ( myStats.HP > 0 )
	{
		regen = (static_cast<real_t>(Entity::getHealthRegenInterval(my,
			myStats, true, excludeItemsEffectsBonus)) / TICKS_PER_SECOND);
		/*if ( myStats.type == SKELETON )
		{
			if ( !(svFlags & SV_FLAG_HUNGER) )
			{
				regen = HEAL_TIME * 4 / TICKS_PER_SECOND;
			}
		}*/
		if ( regen < 0 )
		{
			regen = 0.0;
			/*if ( !(svFlags & SV_FLAG_HUNGER) )
			{
				if ( outColor )
				{
					*outColor = hudColors.characterSheetNeutral;
				}
			}
			else*/
			{
				if ( outColor )
				{
					*outColor = hudColors.characterSheetRed;
				}
			}
		}
		else
		{
			if ( outColor )
			{
				real_t regenWithoutItems = (static_cast<real_t>(Entity::getHealthRegenInterval(my,
					myStats, true, true)) / TICKS_PER_SECOND);
				if ( regen < (regenWithoutItems - 0.001) )
				{
					*outColor = hudColors.characterSheetGreen;
				}
			}
		}
	}
	else
	{
		regen = HEAL_TIME / TICKS_PER_SECOND;
	}

	if ( regen > 0.01 )
	{
		real_t nominalRegen = HEAL_TIME / TICKS_PER_SECOND;
		regen = nominalRegen / regen;
	}
	if ( buf )
	{
		/*if ( !(svFlags & SV_FLAG_HUNGER) && regen < 0.01 )
		{
			snprintf(buf, 32, "- ");
		}
		else*/
		{
			snprintf(buf, 32, "%.f%%", regen * 100.0);
		}
	}
	return regen * 100.0;
}

real_t getDisplayedMPRegen(Entity* my, Stat& myStats, Uint32* outColor, char buf[32], bool excludeItemsEffectsBonus = false)
{
	real_t regen = 0.0;
	bool isNegative = false;
	bool isInsectoid = false;
	if ( /*players[player.playernum]->entity*/ true )
	{
		regen = (static_cast<real_t>(Entity::getManaRegenInterval(my, myStats, true, excludeItemsEffectsBonus)) / TICKS_PER_SECOND);
		if ( myStats.type == AUTOMATON )
		{
			if ( myStats.HUNGER <= 300 )
			{
				isNegative = true;
				regen /= 6; // degrade faster
			}
			else if ( myStats.HUNGER > 1200 )
			{
				if ( myStats.MP / static_cast<real_t>(std::max(1, myStats.MAXMP)) <= 0.5 )
				{
					regen /= 4; // increase faster at < 50% mana
				}
				else
				{
					regen /= 2; // increase less faster at > 50% mana
				}
			}
			else if ( myStats.HUNGER > 300 )
			{
				// normal manaRegenInterval 300-1200 hunger.
			}
		}

		if ( regen < 0.0 /*stats[player]->playerRace == RACE_INSECTOID && stats[player]->appearance == 0*/ )
		{
			regen = 0.0;
		}

		if ( myStats.type == AUTOMATON )
		{
			if ( myStats.HUNGER <= 300 )
			{
				if ( outColor )
				{
					*outColor = hudColors.characterSheetRed;
				}
			}
			else if ( regen < static_cast<real_t>(getBaseManaRegen(my, myStats)) / TICKS_PER_SECOND )
			{
				if ( outColor )
				{
					*outColor = hudColors.characterSheetGreen;
				}
			}
		}
		else if ( myStats.playerRace == RACE_INSECTOID && myStats.stat_appearance == 0 )
		{
			isInsectoid = true;
			if ( !(svFlags & SV_FLAG_HUNGER) )
			{
				if ( outColor )
				{
					*outColor = hudColors.characterSheetNeutral;
				}
			}
			else
			{
				if ( outColor )
				{
					*outColor = hudColors.characterSheetRed;
				}
			}
		}
		else
		{
			int baseRegen = static_cast<real_t>(getBaseManaRegen(my, myStats));
			real_t regenPerMinute = 60 * TICKS_PER_SECOND / (real_t)(baseRegen);
			const int regenTicks = TICKS_PER_SECOND * 60 / regenPerMinute;
			real_t compareRegen = regenTicks / (real_t)TICKS_PER_SECOND;
			if ( regen < (compareRegen - 0.001) )
			{
				if ( outColor )
				{
					*outColor = hudColors.characterSheetGreen;
				}
			}
		}
	}
	else
	{
		regen = MAGIC_REGEN_TIME / TICKS_PER_SECOND;
	}

	if ( isNegative )
	{
		regen *= -1; // negative
	}

	if ( isInsectoid )
	{
		if ( svFlags & SV_FLAG_HUNGER )
		{
			real_t normalRegenTime = (1000.f * 30 * 1.5) / static_cast<float>(TICKS_PER_SECOND); // 30 base, insectoid does 1.5x in getHungerTickRate()
			normalRegenTime /= (std::max(myStats.MAXMP, 1)); // time for 1 mana in seconds
			normalRegenTime *= TICKS_PER_SECOND; // game ticks for 1 mana

			regen = normalRegenTime / (regen * TICKS_PER_SECOND);
		}
		else
		{
			if ( buf )
			{
				snprintf(buf, 32, "-");
			}
			return regen * 100.0;
		}
	}
	else
	{
		if ( regen > 0.01 || regen < -0.01 )
		{
			real_t nominalRegen = MAGIC_REGEN_TIME / TICKS_PER_SECOND;
			regen = nominalRegen / regen;
		}
	}
	if ( buf )
	{
		if ( myStats.type == AUTOMATON )
		{
			snprintf(buf, 32, Player::CharacterSheet_t::getHoverTextString("attributes_rgn_ht_small_format").c_str(), regen * 100.0);
		}
		else
		{
			snprintf(buf, 32, Player::CharacterSheet_t::getHoverTextString("attributes_rgn_small_format").c_str(), regen * 100.0);
		}
	}
	return regen * 100.0;
}

struct CharacterSheetTooltipCache_t
{
	Sint32 baseSTR = 0;
	Sint32 baseDEX = 0;
	Sint32 baseINT = 0;
	Sint32 baseCON = 0;
	Sint32 basePER = 0;
	Sint32 baseCHR = 0;

	Sint32 modifiedSTR = 0;
	Sint32 modifiedDEX = 0;
	Sint32 modifiedINT = 0;
	Sint32 modifiedCON = 0;
	Sint32 modifiedPER = 0;
	Sint32 modifiedCHR = 0;

	Sint32 ac = 0;
	int playerRace = RACE_HUMAN;
	int playerRaceType = NOTHING;
	int type = NOTHING;
	int classnum = -1;
	bool hungerEnabled = false;
	int weapontype = 0;
	Sint32 ATK = -1;
	bool manualUpdate = false;

	struct TextEntries_t
	{
		std::string title = "";
		std::string entry1 = "";
		std::string entry2 = "";
		std::string entry3 = "";
		std::string entry4 = "";
		std::string entry5 = "";
		std::string entry6 = "";
		std::string entry7 = "";
		std::string entry8 = "";
		std::string entry9 = "";
		std::string entry10 = "";
		std::string entry11 = "";
		std::string entry12 = "";
		std::string entry13 = "";
		std::string entry14 = "";
	};
	TextEntries_t textEntries[Player::CharacterSheet_t::SHEET_ENUM_END];
	bool needsUpdate(const int player)
	{
		if ( manualUpdate ) { return true; }
		if ( baseSTR != stats[player]->STR ) { return true; }
		if ( baseDEX != stats[player]->DEX ) { return true; }
		if ( baseCON != stats[player]->CON ) { return true; }
		if ( baseINT != stats[player]->INT ) { return true; }
		if ( basePER != stats[player]->PER ) { return true; }
		if ( baseCHR != stats[player]->CHR ) { return true; }

		if ( modifiedSTR != statGetSTR(stats[player], players[player]->entity) ) { return true; }
		if ( modifiedDEX != statGetDEX(stats[player], players[player]->entity) ) { return true; }
		if ( modifiedCON != statGetCON(stats[player], players[player]->entity) ) { return true; }
		if ( modifiedINT != statGetINT(stats[player], players[player]->entity) ) { return true; }
		if ( modifiedPER != statGetPER(stats[player], players[player]->entity) ) { return true; }
		if ( modifiedCHR != statGetCHR(stats[player], players[player]->entity) ) { return true; }

		if ( playerRace != stats[player]->playerRace ) { return true; }
		if ( hungerEnabled != (svFlags & SV_FLAG_HUNGER) ) { return true; }
		if ( ac != AC(stats[player]) ) { return true; }
		if ( weapontype != getWeaponSkill(stats[player]->weapon) ) { return true; }

		int playerRaceType = getMonsterFromPlayerRace(playerRace);
		int type = stats[player]->type;
		if ( arachnophobia_filter )
		{
			if ( type == SPIDER )
			{
				type = CRAB;
			}
			if ( playerRaceType == SPIDER )
			{
				playerRaceType = CRAB;
			}
		}
		if ( type != this->type ) { return true; }
		if ( playerRaceType != this->playerRaceType ) { return true; }
		if ( client_classes[player] != classnum ) { return true; }
		return false;
	}
	void updateToCharacter(const int player)
	{
		manualUpdate = false;

		baseSTR = stats[player]->STR;
		baseDEX = stats[player]->DEX;
		baseCON = stats[player]->CON;
		baseINT = stats[player]->INT;
		basePER = stats[player]->PER;
		baseCHR = stats[player]->CHR;

		modifiedSTR = statGetSTR(stats[player], players[player]->entity);
		modifiedDEX = statGetDEX(stats[player], players[player]->entity);
		modifiedCON = statGetCON(stats[player], players[player]->entity);
		modifiedINT = statGetINT(stats[player], players[player]->entity);
		modifiedPER = statGetPER(stats[player], players[player]->entity);
		modifiedCHR = statGetCHR(stats[player], players[player]->entity);

		playerRace = stats[player]->playerRace;
		playerRaceType = getMonsterFromPlayerRace(playerRace);
		type = stats[player]->type;
		classnum = client_classes[player];
		if ( arachnophobia_filter )
		{
			if ( type == SPIDER )
			{
				type = CRAB;
			}
			if ( playerRaceType == SPIDER )
			{
				playerRaceType = CRAB;
			}
		}
		hungerEnabled = (svFlags & SV_FLAG_HUNGER);
		ac = AC(stats[player]);
		weapontype = getWeaponSkill(stats[player]->weapon);
	}
};

void characterSheetTooltipSetZeroStat(Entity* entity, Stat* myStats, Sint32* LVL, Sint32 oldStats[NUMSTATS])
{
	if ( !myStats ) { return; }
	if ( LVL )
	{
		*LVL = myStats->LVL;
		myStats->LVL = 1;
	}
	if ( oldStats )
	{
		for ( int i = 0; i < NUMSTATS; ++i )
		{
			switch ( i )
			{
			case STAT_STR:
				oldStats[i] = myStats->STR;
				myStats->STR += -statGetSTR(myStats, entity);
				break;
			case STAT_DEX:
				oldStats[i] = myStats->DEX;
				myStats->DEX += -statGetDEX(myStats, entity);
				break;
			case STAT_CON:
				oldStats[i] = myStats->CON;
				myStats->CON += -statGetCON(myStats, entity);
				break;
			case STAT_INT:
				oldStats[i] = myStats->INT;
				myStats->INT += -statGetINT(myStats, entity);
				break;
			case STAT_PER:
				oldStats[i] = myStats->PER;
				myStats->PER += -statGetPER(myStats, entity);
				break;
			case STAT_CHR:
				oldStats[i] = myStats->CHR;
				myStats->CHR += -statGetCHR(myStats, entity);
				break;
			default:
				break;
			}
		}
	}
}

void characterSheetTooltipRestoreStats(Stat* myStats, Sint32* LVL, Sint32 oldStats[NUMSTATS])
{
	if ( !myStats ) { return; }
	if ( LVL )
	{
		myStats->LVL = *LVL;
	}
	if ( oldStats )
	{
		for ( int i = 0; i < NUMSTATS; ++i )
		{
			switch ( i )
			{
			case STAT_STR:
				myStats->STR = oldStats[i];
				break;
			case STAT_DEX:
				myStats->DEX = oldStats[i];
				break;
			case STAT_CON:
				myStats->CON = oldStats[i];
				break;
			case STAT_INT:
				myStats->INT = oldStats[i];
				break;
			case STAT_PER:
				myStats->PER = oldStats[i];
				break;
			case STAT_CHR:
				myStats->CHR = oldStats[i];
				break;
			default:
				break;
			}
		}
	}
}

bool blitCharacterSheetTooltipToSurf = false;
CharacterSheetTooltipCache_t charsheetTooltipCache[MAXPLAYERS];
void Player::CharacterSheet_t::updateCharacterSheetTooltip(SheetElements element, SDL_Rect pos, Player::PanelJustify_t tooltipJustify)
{
	if ( !sheetFrame )
	{
		return;
	}
	auto tooltipFrame = sheetFrame->findFrame("sheet tooltip");
	if ( tooltipFrame->isDisabled() )
	{
		tooltipOpacitySetpoint = 0;
		tooltipOpacityAnimate = 1.0;
	}
	
	if ( static_cast<int>(tooltipFrame->getOpacity()) != tooltipOpacitySetpoint )
	{
		const real_t fpsScale = getFPSScale(144.0);
		if ( tooltipOpacitySetpoint == 0 )
		{
			if ( ticks - tooltipDeselectedTick > 5 )
			{
				real_t factor = 10.0;
				real_t setpointDiff = fpsScale * std::max(.05, (tooltipOpacityAnimate)) / (factor);
				tooltipOpacityAnimate -= setpointDiff;
				tooltipOpacityAnimate = std::max(0.0, tooltipOpacityAnimate);
			}
		}
		else
		{
			real_t setpointDiff = fpsScale * std::max(.05, (1.0 - tooltipOpacityAnimate)) / (1);
			tooltipOpacityAnimate += setpointDiff;
			tooltipOpacityAnimate = std::min(1.0, tooltipOpacityAnimate);
		}
		tooltipFrame->setOpacity(tooltipOpacityAnimate * 100);
	}
	else
	{
		tooltipFrame->setOpacity(tooltipOpacitySetpoint);
	}

	if ( selectedElement == SHEET_UNSELECTED )
	{
		cachedElementTooltip = SHEET_UNSELECTED;
	}
	if ( player.shootmode )
	{
		tooltipOpacitySetpoint = 0;
		tooltipOpacityAnimate = 0.0;
	}
	if ( element == SHEET_ENUM_END || element == SHEET_UNSELECTED
		|| player.GUI.activeModule != Player::GUI_t::MODULE_CHARACTERSHEET )
	{
		//tooltipFrame->setDisabled(true);
		return;
	}

	tooltipOpacitySetpoint = 0;
	tooltipOpacityAnimate = 1.0;
	tooltipFrame->setDisabled(false);
	tooltipFrame->setOpacity(100.0);
	tooltipDeselectedTick = ticks;

	//if ( keystatus[SDLK_g] )
	//{
	//	keystatus[SDLK_g] = 0;
	//	blitCharacterSheetTooltipToSurf = !blitCharacterSheetTooltipToSurf;
	//	messagePlayer(0, MESSAGE_DEBUG, "%d", blitCharacterSheetTooltipToSurf);
	//	if ( !blitCharacterSheetTooltipToSurf )
	//	{
	//		tooltipFrame->setBlitChildren(false);
	//	}
	//	else if ( blitCharacterSheetTooltipToSurf )
	//	{
	//		tooltipFrame->setBlitChildren(true);
	//	}
	//}

	bool redraw = false;
	if ( cachedElementTooltip != selectedElement 
		|| cachedElementTooltip == SHEET_ENUM_END || cachedElementTooltip == SHEET_UNSELECTED
		|| charsheetTooltipCache[player.playernum].needsUpdate(player.playernum) )
	{
		redraw = true;
		cachedElementTooltip = selectedElement;
		charsheetTooltipCache[player.playernum].updateToCharacter(player.playernum);
	}

	if ( !redraw )
	{
		return;
	}

	Uint32 defaultColor = hudColors.characterSheetNeutral;
	auto txt = tooltipFrame->findField("tooltip text");
	txt->setColor(defaultColor);
	auto txtRightAlignHint = tooltipFrame->findField("tooltip text right align hint");
	txtRightAlignHint->setColor(hudColors.characterSheetFaintText);
	txtRightAlignHint->setDisabled(true);

	for ( int i = 1; i <= NUM_CHARSHEET_TOOLTIP_TEXT_FIELDS; ++i )
	{
		char glyphName[32] = "";
		snprintf(glyphName, sizeof(glyphName), "glyph %d", i);
		if ( auto glyph = tooltipFrame->findImage(glyphName) )
		{
			glyph->disabled = true;
		}

		auto entry = characterSheetTooltipTextFields[player.playernum][i]; assert(entry);
		entry->setDisabled(true);
		entry->setHJustify(Frame::justify_t::LEFT);
		entry->setVJustify(Field::justify_t::CENTER);
		entry->setColor(defaultColor);
	}
	auto div = tooltipFrame->findImage("tooltip divider 1");
	div->disabled = true;
	auto div2 = tooltipFrame->findImage("tooltip divider 2");
	div2->disabled = true;

	for ( int i = 1; i <= NUM_CHARSHEET_TOOLTIP_BACKING_FRAMES; ++i )
	{
		auto txtValueBackingFrame = characterSheetTooltipTextBackingFrames[player.playernum][i];
		txtValueBackingFrame->setDisabled(true);
	}

	auto raceTooltip = tooltipFrame->findFrame("sheet race tooltip");
	raceTooltip->setDisabled(true);
	auto classTooltip = tooltipFrame->findFrame("sheet class tooltip");
	classTooltip->setDisabled(true);

	if ( !(element >= Player::CharacterSheet_t::SHEET_STR && element <= Player::CharacterSheet_t::SHEET_CHR) )
	{
		auto tooltipTopLeft = tooltipFrame->findImage(Player::GUI_t::tooltipEffectBackgroundImages[Player::GUI_t::TOP_LEFT].c_str());
		tooltipTopLeft->path = "*#images/ui/CharSheet/HUD_CharSheet_Tooltip_TL_00.png";
		auto tooltipTop = tooltipFrame->findImage(Player::GUI_t::tooltipEffectBackgroundImages[Player::GUI_t::TOP].c_str());
		tooltipTop->path = "*#images/ui/CharSheet/HUD_CharSheet_Tooltip_T_00.png";
		auto tooltipTopRight = tooltipFrame->findImage(Player::GUI_t::tooltipEffectBackgroundImages[Player::GUI_t::TOP_RIGHT].c_str());
		tooltipTopRight->path = "*#images/ui/CharSheet/HUD_CharSheet_Tooltip_TR_00.png";
		Player::GUI_t::imageSetWidthHeight9x9(tooltipFrame, Player::GUI_t::tooltipEffectBackgroundImages);
	}

	if ( element >= Player::CharacterSheet_t::SHEET_STR && element <= Player::CharacterSheet_t::SHEET_CHR )
	{
		auto tooltipTopLeft = tooltipFrame->findImage(Player::GUI_t::tooltipEffectBackgroundImages[Player::GUI_t::TOP_LEFT].c_str());
		tooltipTopLeft->path = "*#images/ui/CharSheet/HUD_CharSheet_Tooltip_TL_Blue_00.png";
		auto tooltipTop = tooltipFrame->findImage(Player::GUI_t::tooltipEffectBackgroundImages[Player::GUI_t::TOP].c_str());
		tooltipTop->path = "*#images/ui/CharSheet/HUD_CharSheet_Tooltip_T_Blue_00.png";
		auto tooltipTopRight = tooltipFrame->findImage(Player::GUI_t::tooltipEffectBackgroundImages[Player::GUI_t::TOP_RIGHT].c_str());
		tooltipTopRight->path = "*#images/ui/CharSheet/HUD_CharSheet_Tooltip_TR_Blue_00.png";
		Player::GUI_t::imageSetWidthHeight9x9(tooltipFrame, Player::GUI_t::tooltipEffectBackgroundImages);

		int maxWidth = 260;
		if ( getHoverTextString("stat_max_tooltip_width") != defaultString )
		{
			maxWidth = std::max(0, std::stoi(getHoverTextString("stat_max_tooltip_width")));
		}
		int minWidth = 0;
		if ( getHoverTextString("stat_min_tooltip_width") != defaultString )
		{
			minWidth = std::max(0, std::stoi(getHoverTextString("stat_min_tooltip_width")));
		}
		const int padx = 16;
		const int pady1 = 8;
		const int pady2 = 4;
		const int padxMid = 4;
		const int padyMid = 8;
		SDL_Rect tooltipPos = SDL_Rect{ 400, 0, maxWidth, 100 };

		std::string titleText = "";
		std::string descText = "";
		int value = 0;
		switch ( element )
		{
			case SHEET_STR:
				titleText = getHoverTextString("stat_str_title");
				descText = getHoverTextString("stat_str_desc");
				break;
			case SHEET_DEX:
				titleText = getHoverTextString("stat_dex_title");
				descText = getHoverTextString("stat_dex_desc");
				break;
			case SHEET_CON:
				titleText = getHoverTextString("stat_con_title");
				descText = getHoverTextString("stat_con_desc");
				break;
			case SHEET_INT:
				titleText = getHoverTextString("stat_int_title");
				descText = getHoverTextString("stat_int_desc");
				break;
			case SHEET_PER:
				titleText = getHoverTextString("stat_per_title");
				descText = getHoverTextString("stat_per_desc");
				break;
			case SHEET_CHR:
				titleText = getHoverTextString("stat_chr_title");
				descText = getHoverTextString("stat_chr_desc");
				break;
			default:
				break;
		}

		txt->setText(titleText.c_str());
		SDL_Rect txtPos = SDL_Rect{ padx, pady1 - 2, maxWidth - padx * 2, 80 };
		txt->setSize(txtPos);
		if ( charsheetTooltipCache[player.playernum].textEntries[element].title != txt->getText() )
		{
			txt->reflowTextToFit(0);
			charsheetTooltipCache[player.playernum].textEntries[element].title = txt->getText();
		}
		txt->setColor(hudColors.characterSheetHeadingText);
		Font* actualFont = Font::get(txt->getFont());
		int txtHeight = txt->getNumTextLines() * actualFont->height(true);
		txtPos.h = txtHeight + 4;
		auto txtGet = Text::get(txt->getLongestLine().c_str(), txt->getFont(),
			txt->getTextColor(), txt->getOutlineColor());
		txtPos.w = txtGet->getWidth();
		txtPos.w = std::max(minWidth - padx * 2, txtPos.w);
		txt->setSize(txtPos);

		tooltipPos.w = (txtPos.w + padx * 2);

		unsigned int longestValue = 0;
		std::map<int, std::pair<Field*, SDL_Rect>> valueSizes;

		int currentHeight = txtPos.y + (actualFont->height(true) * 1) + 2;
		const int extraTextHeightForLowerCharacters = 4;
		{
			currentHeight += padyMid;
			auto entry = characterSheetTooltipTextFields[player.playernum][1]; assert(entry);
			entry->setDisabled(false);
			char buf[128] = "";
			snprintf(buf, sizeof(buf), "%s", getHoverTextString("stat_base_amount").c_str());
			entry->setText(buf);
			entry->setVJustify(Field::justify_t::TOP);

			auto glyphBacking = tooltipFrame->findImage("glyph 1");
			glyphBacking->disabled = false;
			glyphBacking->path = getHoverTextString("icon_backing_path");
			glyphBacking->pos.x = padx + padxMid + 4;
			glyphBacking->pos.y = currentHeight + 6;
			glyphBacking->pos.w = 44;
			glyphBacking->pos.h = 44;

			auto glyphIcon = tooltipFrame->findImage("glyph 2");
			glyphIcon->disabled = false;
			glyphIcon->path = "";
			switch ( element )
			{
				case SHEET_STR:
					glyphIcon->path = getHoverTextString("icon_str_path");
					break;
				case SHEET_DEX:
					glyphIcon->path = getHoverTextString("icon_dex_path");
					break;
				case SHEET_CON:
					glyphIcon->path = getHoverTextString("icon_con_path");
					break;
				case SHEET_INT:
					glyphIcon->path = getHoverTextString("icon_int_path");
					break;
				case SHEET_PER:
					glyphIcon->path = getHoverTextString("icon_per_path");
					break;
				case SHEET_CHR:
					glyphIcon->path = getHoverTextString("icon_chr_path");
					break;
				default:
					break;
			}
			glyphIcon->pos.w = 24;
			glyphIcon->pos.h = 24;
			glyphIcon->pos.x = glyphBacking->pos.x + glyphBacking->pos.w / 2 - glyphIcon->pos.w / 2;
			glyphIcon->pos.y = glyphBacking->pos.y + glyphBacking->pos.h / 2 - glyphIcon->pos.h / 2;

			SDL_Rect entryPos = entry->getSize();
			entryPos.x = padx / 2 + glyphBacking->pos.x + glyphBacking->pos.w;
			entryPos.y = currentHeight;
			entryPos.w = txtPos.w - (padxMid + glyphBacking->pos.x + glyphBacking->pos.w);
			entry->setSize(entryPos);
			if ( charsheetTooltipCache[player.playernum].textEntries[element].entry1 != entry->getText() )
			{
				entry->reflowTextToFit(0);
				charsheetTooltipCache[player.playernum].textEntries[element].entry1 = entry->getText();
			}
			entryPos.h = actualFont->height(true) * entry->getNumTextLines() + extraTextHeightForLowerCharacters;
			entry->setSize(entryPos);
			entry->setColor(defaultColor);
			currentHeight = std::max(entryPos.y + entryPos.h - extraTextHeightForLowerCharacters, 0);
			tooltipPos.h = pady1 + currentHeight + pady2;

			auto entryValue = characterSheetTooltipTextFields[player.playernum][2]; assert(entry);
			entryValue->setDisabled(false);
			int value = 0;
			char valueBuf[128] = "";
			switch ( element )
			{
				case SHEET_STR:
					value = stats[player.playernum]->STR;
					snprintf(valueBuf, sizeof(valueBuf), "%d STR", value);
					break;
				case SHEET_DEX:
					value = stats[player.playernum]->DEX;
					snprintf(valueBuf, sizeof(valueBuf), "%d DEX", value);
					break;
				case SHEET_CON:
					value = stats[player.playernum]->CON;
					snprintf(valueBuf, sizeof(valueBuf), "%d CON", value);
					break;
				case SHEET_INT:
					value = stats[player.playernum]->INT;
					snprintf(valueBuf, sizeof(valueBuf), "%d INT", value);
					break;
				case SHEET_PER:
					value = stats[player.playernum]->PER;
					snprintf(valueBuf, sizeof(valueBuf), "%d PER", value);
					break;
				case SHEET_CHR:
					value = stats[player.playernum]->CHR;
					snprintf(valueBuf, sizeof(valueBuf), "%d CHR", value);
					break;
				default:
					break;
			}
			entryValue->setColor(hudColors.characterSheetNeutral);
			if ( value < 0 )
			{
				entryValue->setColor(hudColors.characterSheetRed);
			}
			entryValue->setText(valueBuf);
			SDL_Rect entryValuePos = entry->getSize();
			entryValue->setSize(entryValuePos);
			entryValue->setHJustify(Frame::justify_t::RIGHT);
			entryValue->setVJustify(Field::justify_t::TOP);

			/*auto txtValueBackingFrame = tooltipFrame->findFrame("txt value backing frame 1");
			SDL_Rect backingFramePos = entryValue->getSize();
			auto txtValueGet = Text::get(entryValue->getText(), entryValue->getFont(),
				entryValue->getTextColor(), entryValue->getOutlineColor());
			longestValue = std::max(longestValue, txtValueGet->getWidth());
			backingFramePos.x = backingFramePos.x + backingFramePos.w;
			backingFramePos.h = actualFont->height(true) + extraTextHeightForLowerCharacters - 2;
			valueSizes[1] = std::make_pair(entryValue, backingFramePos);
			txtValueBackingFrame->setDisabled(false);*/
		}
		{
			currentHeight += 0;// padyMid / 2;
			auto entry = characterSheetTooltipTextFields[player.playernum][3]; assert(entry);
			entry->setDisabled(false);
			char buf[128] = "";
			snprintf(buf, sizeof(buf), "%s", getHoverTextString("stat_modified_amount").c_str());
			entry->setText(buf);
			entry->setVJustify(Field::justify_t::TOP);

			auto glyphBacking = tooltipFrame->findImage("glyph 1");

			SDL_Rect entryPos = entry->getSize();
			entryPos.x = padx / 2 + glyphBacking->pos.x + glyphBacking->pos.w;
			entryPos.y = currentHeight;
			entryPos.w = txtPos.w - (padxMid + glyphBacking->pos.x + glyphBacking->pos.w);
			entry->setSize(entryPos);
			if ( charsheetTooltipCache[player.playernum].textEntries[element].entry3 != entry->getText() )
			{
				entry->reflowTextToFit(0);
				charsheetTooltipCache[player.playernum].textEntries[element].entry3 = entry->getText();
			}
			entryPos.h = actualFont->height(true) * entry->getNumTextLines() + extraTextHeightForLowerCharacters;
			entry->setSize(entryPos);
			entry->setColor(defaultColor);
			currentHeight = std::max(entryPos.y + entryPos.h - extraTextHeightForLowerCharacters, 0);
			tooltipPos.h = pady1 + currentHeight + pady2;

			auto entryValue = characterSheetTooltipTextFields[player.playernum][4]; assert(entry);
			entryValue->setDisabled(false);
			char valueBuf[128] = "";
			int value = 0;
			switch ( element )
			{
				case SHEET_STR:
					value = statGetSTR(stats[player.playernum], players[player.playernum]->entity);
					value -= stats[player.playernum]->STR;
					snprintf(valueBuf, sizeof(valueBuf), "%+d STR", value);
					break;
				case SHEET_DEX:
					value = statGetDEX(stats[player.playernum], players[player.playernum]->entity);
					value -= stats[player.playernum]->DEX;
					snprintf(valueBuf, sizeof(valueBuf), "%+d DEX", value);
					break;
				case SHEET_CON:
					value = statGetCON(stats[player.playernum], players[player.playernum]->entity);
					value -= stats[player.playernum]->CON;
					snprintf(valueBuf, sizeof(valueBuf), "%+d CON", value);
					break;
				case SHEET_INT:
					value = statGetINT(stats[player.playernum], players[player.playernum]->entity);
					value -= stats[player.playernum]->INT;
					snprintf(valueBuf, sizeof(valueBuf), "%+d INT", value);
					break;
				case SHEET_PER:
					value = statGetPER(stats[player.playernum], players[player.playernum]->entity);
					value -= stats[player.playernum]->PER;
					snprintf(valueBuf, sizeof(valueBuf), "%+d PER", value);
					break;
				case SHEET_CHR:
					value = statGetCHR(stats[player.playernum], players[player.playernum]->entity);
					value -= stats[player.playernum]->CHR;
					snprintf(valueBuf, sizeof(valueBuf), "%+d CHR", value);
					break;
				default:
					break;
			}
			entryValue->setColor(hudColors.characterSheetNeutral);
			if ( value < 0 )
			{
				entryValue->setColor(hudColors.characterSheetRed);
			}
			else if ( value > 0 )
			{
				entryValue->setColor(hudColors.characterSheetGreen);
			}
			entryValue->setText(valueBuf);
			SDL_Rect entryValuePos = entry->getSize();
			entryValue->setSize(entryValuePos);
			entryValue->setHJustify(Frame::justify_t::RIGHT);
			entryValue->setVJustify(Field::justify_t::TOP);

			/*auto txtValueBackingFrame = tooltipFrame->findFrame("txt value backing frame 2");
			SDL_Rect backingFramePos = entryValue->getSize();
			auto txtValueGet = Text::get(entryValue->getText(), entryValue->getFont(),
				entryValue->getTextColor(), entryValue->getOutlineColor());
			longestValue = std::max(longestValue, txtValueGet->getWidth());
			backingFramePos.x = backingFramePos.x + backingFramePos.w;
			backingFramePos.h = actualFont->height(true) + extraTextHeightForLowerCharacters - 2;
			valueSizes[2] = std::make_pair(entryValue, backingFramePos);
			txtValueBackingFrame->setDisabled(false);*/
		}
		{
			// stat extra number display
			currentHeight += padyMid;
			auto entry = characterSheetTooltipTextFields[player.playernum][5]; assert(entry);
			entry->setDisabled(false);
			char buf[128] = "";
			if ( element == SHEET_STR )
			{
				snprintf(buf, sizeof(buf), "%s", getHoverTextString("stat_str_atk_bonus").c_str());
			}
			else if ( element == SHEET_DEX )
			{
				snprintf(buf, sizeof(buf), "%s", getHoverTextString("stat_dex_ranged_atk_bonus").c_str());
			}
			else if ( element == SHEET_CON )
			{
				snprintf(buf, sizeof(buf), "%s", getHoverTextString("stat_con_ac_bonus").c_str());
			}
			else if ( element == SHEET_PER )
			{
				snprintf(buf, sizeof(buf), "%s", getHoverTextString("stat_per_pierce_bonus").c_str());
			}
			else if ( element == SHEET_INT )
			{
				snprintf(buf, sizeof(buf), "%s", getHoverTextString("stat_int_pwr_bonus").c_str());
			}
			else if ( element == SHEET_CHR )
			{
				snprintf(buf, sizeof(buf), "%s", getHoverTextString("stat_chr_buying_bonus").c_str());
			}
			entry->setText(buf);
			entry->setVJustify(Field::justify_t::TOP);

			SDL_Rect entryPos = entry->getSize();
			entryPos.x = padx + padxMid;
			entryPos.y = currentHeight;
			entryPos.w = txtPos.w - (padxMid * 2);
			entry->setSize(entryPos);
			if ( charsheetTooltipCache[player.playernum].textEntries[element].entry5 != entry->getText() )
			{
				entry->reflowTextToFit(0);
				charsheetTooltipCache[player.playernum].textEntries[element].entry5 = entry->getText();
			}
			entryPos.h = actualFont->height(true) * entry->getNumTextLines() + extraTextHeightForLowerCharacters;
			entry->setSize(entryPos);
			entry->setColor(defaultColor);
			currentHeight = std::max(entryPos.y + entryPos.h - extraTextHeightForLowerCharacters, 0);
			tooltipPos.h = pady1 + currentHeight + pady2;

			auto entryValue = characterSheetTooltipTextFields[player.playernum][6]; assert(entry);
			entryValue->setDisabled(false);
			char valueBuf[128] = "";
			int value = 0;
			switch ( element )
			{
				case SHEET_STR:
				{
					Sint32 STR = statGetSTR(stats[player.playernum], players[player.playernum]->entity);
					snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("stat_atk_percent_format").c_str(), STR * 100 * Entity::PlayerAttackMeleeStatFactor);
				}
					break;
				case SHEET_DEX:
				{
					Sint32 DEX = statGetDEX(stats[player.playernum], players[player.playernum]->entity);
					snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("stat_atk_percent_format").c_str(), DEX * 100 * Entity::PlayerAttackRangedStatFactor);
				}
					break;
				case SHEET_CON:
				{
					Sint32 CON = statGetCON(stats[player.playernum], players[player.playernum]->entity);
					snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("stat_ac_value_format").c_str(), CON);
				}
					break;
				case SHEET_INT:
				{
					//real_t val = getBonusFromCasterOfSpellElement(players[player.playernum]->entity, stats[player.playernum], nullptr, SPELL_NONE) * 100.0;
					/*if ( auto spell = player.magic.selectedSpell() )
					{
						real_t bonus = getSpellBonusFromCasterINT(players[player.playernum]->entity, stats[player.playernum], spell->skillID);
						real_t val = bonus * 100.0;
						snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("stat_pwr_value_format").c_str(), val);
					}
					else
					{
					}*/
					real_t bonus = getSpellBonusFromCasterINT(players[player.playernum]->entity, stats[player.playernum], NUMPROFICIENCIES);
					real_t val = bonus * 100.0;
					snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("stat_pwr_value_format").c_str(), val);
				}
					break;
				case SHEET_PER:
				{
					real_t val = std::min(std::max(statGetPER(stats[player.playernum], players[player.playernum]->entity), 0), 50);
					snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("stat_pierce_value_format").c_str(), val);
				}
					break;
				case SHEET_CHR:
				{
					real_t val = 1 / ((50 + stats[player.playernum]->getModifiedProficiency(PRO_TRADING)) / 150.f); // buy value
					real_t normalVal = val;
					//normalVal /= (1.f + statGetCHR(stats[player.playernum], players[player.playernum]->entity) / 20.f);
					normalVal = std::max(1.0, normalVal);

					int stat = stats[player.playernum]->CHR;
					stats[player.playernum]->CHR = 0;
					stats[player.playernum]->CHR -= statGetCHR(stats[player.playernum], players[player.playernum]->entity);
					real_t zeroVal = val;
					//zeroVal /= (1.f + statGetCHR(stats[player.playernum], players[player.playernum]->entity) / 20.f);
					zeroVal = std::max(1.0, zeroVal);
					stats[player.playernum]->CHR = stat;

					snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("stat_buying_value_format").c_str(), (zeroVal - normalVal) * 100.0);
				}
					break;
				default:
					break;
			}
			entryValue->setColor(hudColors.characterSheetNeutral);
			if ( value < 0 )
			{
				entryValue->setColor(hudColors.characterSheetRed);
			}
			else if ( value > 0 )
			{
				entryValue->setColor(hudColors.characterSheetGreen);
			}
			entryValue->setText(valueBuf);
			entryValue->setSize(entry->getSize());
			entryValue->setHJustify(Frame::justify_t::LEFT);
			entryValue->setVJustify(Field::justify_t::TOP);

			auto txtValueBackingFrame = characterSheetTooltipTextBackingFrames[player.playernum][3];
			SDL_Rect backingFramePos = entryValue->getSize();
			auto txtValueGet = Text::get(entryValue->getText(), entryValue->getFont(),
				entryValue->getTextColor(), entryValue->getOutlineColor());
			longestValue = std::max(longestValue, txtValueGet->getWidth());
			backingFramePos.x = backingFramePos.x + backingFramePos.w;
			backingFramePos.h = actualFont->height(true) + extraTextHeightForLowerCharacters - 2;
			valueSizes[3] = std::make_pair(entryValue, backingFramePos);
			txtValueBackingFrame->setDisabled(false);
		}
		if ( element == SHEET_STR || element == SHEET_DEX || element == SHEET_INT || element == SHEET_PER || element == SHEET_CHR || element == SHEET_CON )
		{
			// stat extra number display
			currentHeight += padyMid;
			auto entry = characterSheetTooltipTextFields[player.playernum][7]; assert(entry);
			entry->setDisabled(false);
			char buf[128] = "";

			if ( element == SHEET_STR )
			{
				snprintf(buf, sizeof(buf), "%s", getHoverTextString("stat_str_movement_bonus").c_str());
			}
			else if ( element == SHEET_DEX )
			{
				snprintf(buf, sizeof(buf), "%s", getHoverTextString("stat_dex_thrown_atk_bonus").c_str());
			}
			else if ( element == SHEET_CON )
			{
				snprintf(buf, sizeof(buf), "%s", getHoverTextString("stat_con_pwr_bonus").c_str());
			}
			else if ( element == SHEET_INT )
			{
				//snprintf(buf, sizeof(buf), "%s", getHoverTextString("stat_int_mp_regen_bonus").c_str());
				snprintf(buf, sizeof(buf), "%s", getHoverTextString("stat_int_pwr_bonus2").c_str());
			}
			else if ( element == SHEET_PER )
			{
				snprintf(buf, sizeof(buf), "%s", getHoverTextString("stat_per_light_bonus").c_str());
			}
			else if ( element == SHEET_CHR )
			{
				snprintf(buf, sizeof(buf), "%s", getHoverTextString("stat_chr_selling_bonus").c_str());
			}
			entry->setText(buf);
			entry->setVJustify(Field::justify_t::TOP);

			SDL_Rect entryPos = entry->getSize();
			entryPos.x = padx + padxMid;
			entryPos.y = currentHeight;
			entryPos.w = txtPos.w - (padxMid * 2);
			entry->setSize(entryPos);
			if ( charsheetTooltipCache[player.playernum].textEntries[element].entry7 != entry->getText() )
			{
				entry->reflowTextToFit(0);
				charsheetTooltipCache[player.playernum].textEntries[element].entry7 = entry->getText();
			}
			entryPos.h = actualFont->height(true) * entry->getNumTextLines() + extraTextHeightForLowerCharacters;
			entry->setSize(entryPos);
			entry->setColor(defaultColor);
			currentHeight = std::max(entryPos.y + entryPos.h - extraTextHeightForLowerCharacters, 0);
			tooltipPos.h = pady1 + currentHeight + pady2;

			auto entryValue = characterSheetTooltipTextFields[player.playernum][8]; assert(entry);
			entryValue->setDisabled(false);
			char valueBuf[128] = "";
			int value = 0;
			switch ( element )
			{
				case SHEET_STR:
				{
					Sint32 STR = statGetSTR(stats[player.playernum], players[player.playernum]->entity);
					Sint32 DEX = statGetDEX(stats[player.playernum], players[player.playernum]->entity);
					real_t weightratio1 = player.movement.getWeightRatio(player.movement.getCharacterModifiedWeight(), 0);
					real_t weightratio2 = player.movement.getWeightRatio(player.movement.getCharacterModifiedWeight(), STR);
					real_t speedFactor1 = player.movement.getSpeedFactor(weightratio1, DEX);
					real_t speedFactor2 = player.movement.getSpeedFactor(weightratio2, DEX);
					real_t maxSpeed = player.movement.getMaximumSpeed();

					real_t noSTRPercent = 100.0 * speedFactor1 / std::fmax(.01, maxSpeed);
					real_t currentPercent = 100.0 * speedFactor2 / std::fmax(.01, maxSpeed);
					real_t displayValue = currentPercent - noSTRPercent;
					snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("stat_movement_value_format").c_str(), displayValue);
				}
					break;
				case SHEET_DEX:
				{
					Sint32 DEX = statGetDEX(stats[player.playernum], players[player.playernum]->entity);
					snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("stat_atk_percent_format").c_str(), (DEX / 4) * 100 * Entity::PlayerAttackThrownStatFactor);
				}
					break;
				case SHEET_CON:
				{
					real_t bonus = getSpellBonusFromCasterINT(players[player.playernum]->entity, stats[player.playernum], PRO_THAUMATURGY);
					bonus -= getSpellBonusFromCasterINT(players[player.playernum]->entity, stats[player.playernum], NUMPROFICIENCIES);
					real_t val = bonus * 100.0;
					snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("stat_pwr_value_format").c_str(), val);
					break;
				}
				case SHEET_INT:
				{
					//Sint32 oldINT = stats[player.playernum]->INT;
					//stats[player.playernum]->INT += -statGetINT(stats[player.playernum], player.entity);
					//real_t regenWithoutINT = getDisplayedMPRegen(player.entity, *stats[player.playernum], nullptr, nullptr);
					//stats[player.playernum]->INT = oldINT;
					//
					//real_t regenTotal = getDisplayedMPRegen(player.entity, *stats[player.playernum], nullptr, nullptr);
					//real_t regenStatSkill = regenTotal - regenWithoutINT;
					//snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("stat_mp_regen_value_format").c_str(), regenStatSkill);
					real_t bonus = getSpellBonusFromCasterINT(players[player.playernum]->entity, stats[player.playernum], PRO_SORCERY);
					bonus -= getSpellBonusFromCasterINT(players[player.playernum]->entity, stats[player.playernum], NUMPROFICIENCIES);
					real_t val = bonus * 100.0;
					snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("stat_pwr_value_format").c_str(), val);
					break;
				}
				case SHEET_PER:
				{
					const int PER = statGetPER(stats[player.playernum], players[player.playernum]->entity);
					const int range_bonus = std::min(std::max(0, PER / 5), 2);
					snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("stat_light_value_format").c_str(), range_bonus);
					break;
				}
				case SHEET_CHR:
				{
					real_t val = (50 + stats[player.playernum]->getModifiedProficiency(PRO_TRADING)) / 150.f; // sell value
					real_t normalVal = val;
					normalVal *= (1.f + statGetCHR(stats[player.playernum], players[player.playernum]->entity) / 20.f);
					normalVal = std::min(1.0, normalVal);

					int stat = stats[player.playernum]->CHR;
					stats[player.playernum]->CHR = 0;
					stats[player.playernum]->CHR -= statGetCHR(stats[player.playernum], players[player.playernum]->entity);
					real_t zeroVal = val;
					zeroVal *= (1.f + statGetCHR(stats[player.playernum], players[player.playernum]->entity) / 20.f);
					zeroVal = std::min(1.0, zeroVal);
					stats[player.playernum]->CHR = stat;

					snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("stat_selling_value_format").c_str(), (normalVal - zeroVal) * 100.0);
					break;
				}
				default:
					break;
			}
			entryValue->setColor(hudColors.characterSheetNeutral);
			if ( value < 0 )
			{
				entryValue->setColor(hudColors.characterSheetRed);
			}
			else if ( value > 0 )
			{
				entryValue->setColor(hudColors.characterSheetGreen);
			}
			entryValue->setText(valueBuf);
			entryValue->setSize(entry->getSize());
			entryValue->setHJustify(Frame::justify_t::LEFT);
			entryValue->setVJustify(Field::justify_t::TOP);

			auto txtValueBackingFrame = characterSheetTooltipTextBackingFrames[player.playernum][4];
			SDL_Rect backingFramePos = entryValue->getSize();
			auto txtValueGet = Text::get(entryValue->getText(), entryValue->getFont(),
				entryValue->getTextColor(), entryValue->getOutlineColor());
			longestValue = std::max(longestValue, txtValueGet->getWidth());
			backingFramePos.x = backingFramePos.x + backingFramePos.w;
			backingFramePos.h = actualFont->height(true) + extraTextHeightForLowerCharacters - 2;
			valueSizes[4] = std::make_pair(entryValue, backingFramePos);
			txtValueBackingFrame->setDisabled(false);
		}
		if ( element == SHEET_PER || element == SHEET_DEX || element == SHEET_CHR || element == SHEET_INT )
		{
			// stat extra number display
			currentHeight += padyMid;
			auto entry = characterSheetTooltipTextFields[player.playernum][9]; assert(entry);
			entry->setDisabled(false);
			char buf[128] = "";
			if ( element == SHEET_DEX )
			{
				snprintf(buf, sizeof(buf), "%s", getHoverTextString("stat_dex_movement_bonus").c_str());
			}
			else if ( element == SHEET_PER )
			{
				snprintf(buf, sizeof(buf), "%s", getHoverTextString("stat_per_sneaking_bonus").c_str());
			}
			else if ( element == SHEET_INT )
			{
				snprintf(buf, sizeof(buf), "%s", getHoverTextString("stat_int_res_bonus").c_str());
			}
			else if ( element == SHEET_CHR )
			{
				snprintf(buf, sizeof(buf), "%s", getHoverTextString("stat_chr_pwr_bonus").c_str());
			}
			entry->setText(buf);
			entry->setVJustify(Field::justify_t::TOP);

			SDL_Rect entryPos = entry->getSize();
			entryPos.x = padx + padxMid;
			entryPos.y = currentHeight;
			entryPos.w = txtPos.w - (padxMid * 2);
			entry->setSize(entryPos);
			if ( charsheetTooltipCache[player.playernum].textEntries[element].entry9 != entry->getText() )
			{
				entry->reflowTextToFit(0);
				charsheetTooltipCache[player.playernum].textEntries[element].entry9 = entry->getText();
			}
			entryPos.h = actualFont->height(true) * entry->getNumTextLines() + extraTextHeightForLowerCharacters;
			entry->setSize(entryPos);
			entry->setColor(defaultColor);
			currentHeight = std::max(entryPos.y + entryPos.h - extraTextHeightForLowerCharacters, 0);
			tooltipPos.h = pady1 + currentHeight + pady2;

			auto entryValue = characterSheetTooltipTextFields[player.playernum][10]; assert(entry);
			entryValue->setDisabled(false);
			char valueBuf[128] = "";
			int value = 0;
			switch ( element )
			{
				case SHEET_STR:
					break;
				case SHEET_DEX:
				{
					Sint32 STR = statGetSTR(stats[player.playernum], players[player.playernum]->entity);
					real_t weightratio = player.movement.getWeightRatio(player.movement.getCharacterModifiedWeight(), STR);
					Sint32 DEX = statGetDEX(stats[player.playernum], players[player.playernum]->entity);
					real_t speedFactor1 = player.movement.getSpeedFactor(weightratio, 0);
					real_t speedFactor2 = player.movement.getSpeedFactor(weightratio, DEX);
					real_t maxSpeed = player.movement.getMaximumSpeed();

					real_t noDEXPercent = 100.0 * speedFactor1 / std::fmax(.01, maxSpeed);
					real_t currentPercent = 100.0 * speedFactor2 / std::fmax(.01, maxSpeed);

					real_t displayValue = currentPercent - noDEXPercent;
					snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("stat_movement_value_format").c_str(), displayValue);
				}
					break;
				case SHEET_CON:
					break;
				case SHEET_INT:
				{
					Sint32 INT = std::max(0, std::min(90, statGetINT(stats[player.playernum], players[player.playernum]->entity)));
					snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("stat_int_res_value_format").c_str(), INT);
					break;
				}
				case SHEET_PER:
				{
					const int PER = statGetPER(stats[player.playernum], players[player.playernum]->entity);
					const int range_bonus = std::min(std::max(0, PER / 5), 2);
					const int sneakingBonus = range_bonus + 2;
					snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("stat_light_value_format").c_str(), sneakingBonus);
				}
				break;
				case SHEET_CHR:
				{
					real_t bonus = getSpellBonusFromCasterINT(players[player.playernum]->entity, stats[player.playernum], PRO_MYSTICISM);
					bonus -= getSpellBonusFromCasterINT(players[player.playernum]->entity, stats[player.playernum], NUMPROFICIENCIES);
					real_t val = bonus * 100.0;
					snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("stat_pwr_value_format").c_str(), val);
					break;
				}
				default:
					break;
			}
			entryValue->setColor(hudColors.characterSheetNeutral);
			if ( value < 0 )
			{
				entryValue->setColor(hudColors.characterSheetRed);
			}
			else if ( value > 0 )
			{
				entryValue->setColor(hudColors.characterSheetGreen);
			}
			entryValue->setText(valueBuf);
			entryValue->setSize(entry->getSize());
			entryValue->setHJustify(Frame::justify_t::LEFT);
			entryValue->setVJustify(Field::justify_t::TOP);

			auto txtValueBackingFrame = characterSheetTooltipTextBackingFrames[player.playernum][5];
			SDL_Rect backingFramePos = entryValue->getSize();
			auto txtValueGet = Text::get(entryValue->getText(), entryValue->getFont(),
				entryValue->getTextColor(), entryValue->getOutlineColor());
			longestValue = std::max(longestValue, txtValueGet->getWidth());
			backingFramePos.x = backingFramePos.x + backingFramePos.w;
			backingFramePos.h = actualFont->height(true) + extraTextHeightForLowerCharacters - 2;
			valueSizes[5] = std::make_pair(entryValue, backingFramePos);
			txtValueBackingFrame->setDisabled(false);
		}
		if ( element == SHEET_CHR )
		{
			// stat extra number display
			currentHeight += padyMid;
			auto entry = characterSheetTooltipTextFields[player.playernum][11]; assert(entry);
			entry->setDisabled(false);
			char buf[128] = "";
			if ( element == SHEET_CHR )
			{
				snprintf(buf, sizeof(buf), "%s", getHoverTextString("stat_chr_restore_bonus").c_str());
			}
			entry->setText(buf);
			entry->setVJustify(Field::justify_t::TOP);

			SDL_Rect entryPos = entry->getSize();
			entryPos.x = padx + padxMid;
			entryPos.y = currentHeight;
			entryPos.w = txtPos.w - (padxMid * 2);
			entry->setSize(entryPos);
			if ( charsheetTooltipCache[player.playernum].textEntries[element].entry11 != entry->getText() )
			{
				entry->reflowTextToFit(0);
				charsheetTooltipCache[player.playernum].textEntries[element].entry11 = entry->getText();
			}
			entryPos.h = actualFont->height(true) * entry->getNumTextLines() + extraTextHeightForLowerCharacters;
			entry->setSize(entryPos);
			entry->setColor(defaultColor);
			currentHeight = std::max(entryPos.y + entryPos.h - extraTextHeightForLowerCharacters, 0);
			tooltipPos.h = pady1 + currentHeight + pady2;

			auto entryValue = characterSheetTooltipTextFields[player.playernum][12]; assert(entry);
			entryValue->setDisabled(false);
			char valueBuf[128] = "";
			int value = 0;
			switch ( element )
			{
			case SHEET_STR:
				break;
			case SHEET_CON:
				break;
			case SHEET_INT:
				break;
			case SHEET_PER:
				break;
			case SHEET_CHR:
			{
				int val = Entity::getMPRestoreOnLevelUp(player.entity, stats[player.playernum], MP_MOD, true);
				snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("stat_chr_restore_format").c_str(), val);
				break;
			}
			default:
				break;
			}
			entryValue->setColor(hudColors.characterSheetNeutral);
			if ( value < 0 )
			{
				entryValue->setColor(hudColors.characterSheetRed);
			}
			else if ( value > 0 )
			{
				entryValue->setColor(hudColors.characterSheetGreen);
			}
			entryValue->setText(valueBuf);
			entryValue->setSize(entry->getSize());
			entryValue->setHJustify(Frame::justify_t::LEFT);
			entryValue->setVJustify(Field::justify_t::TOP);

			auto txtValueBackingFrame = characterSheetTooltipTextBackingFrames[player.playernum][6];
			SDL_Rect backingFramePos = entryValue->getSize();
			auto txtValueGet = Text::get(entryValue->getText(), entryValue->getFont(),
				entryValue->getTextColor(), entryValue->getOutlineColor());
			longestValue = std::max(longestValue, txtValueGet->getWidth());
			backingFramePos.x = backingFramePos.x + backingFramePos.w;
			backingFramePos.h = actualFont->height(true) + extraTextHeightForLowerCharacters - 2;
			valueSizes[6] = std::make_pair(entryValue, backingFramePos);
			txtValueBackingFrame->setDisabled(false);
		}

		for ( int index = 1; index <= NUM_CHARSHEET_TOOLTIP_BACKING_FRAMES; ++index )
		{
			auto txtValueBackingFrame = characterSheetTooltipTextBackingFrames[player.playernum][index];
			if ( txtValueBackingFrame->isDisabled() )
			{
				continue;
			}
			if ( valueSizes.find(index) == valueSizes.end() )
			{
				continue;
			}

			SDL_Rect valuePos = valueSizes[index].second;
			Field* entryValue = valueSizes[index].first;
			SDL_Rect entryValuePos = entryValue->getSize();
			entryValuePos.x = entryValuePos.x + entryValuePos.w;
			entryValuePos.w = (int)longestValue;
			entryValuePos.x -= entryValuePos.w;
			entryValuePos.x -= 8;
			entryValue->setSize(entryValuePos);

			valuePos.w = (int)longestValue + 16;
			valuePos.x -= (valuePos.w);
			valuePos.y -= 3;
			valuePos.h += 4;

			txtValueBackingFrame->setSize(valuePos);

			Player::GUI_t::imageResizeToContainer9x9(txtValueBackingFrame, SDL_Rect{ 0, 0, valuePos.w, valuePos.h }, Player::GUI_t::tooltipEffectBackgroundImages);
		}

		{
			currentHeight += padyMid;

			div->pos.x = padx;
			div->pos.y = currentHeight;
			div->pos.w = txtPos.w;
			div->disabled = false;

			currentHeight += padyMid;

			auto entry = characterSheetTooltipTextFields[player.playernum][13]; assert(entry);
			entry->setDisabled(false);
			char buf[512] = "";

			std::string descTextFormatted = "\x1E ";
			for ( auto s : descText )
			{
				descTextFormatted += s;
				if ( s == '\n' )
				{
					descTextFormatted += "\x1E ";
				}
			}

			snprintf(buf, sizeof(buf), "%s", descTextFormatted.c_str());
			entry->setText(buf);

			SDL_Rect entryPos = entry->getSize();
			entryPos.x = padx;
			entryPos.y = currentHeight;
			entryPos.w = txtPos.w;
			entry->setSize(entryPos);
			if ( charsheetTooltipCache[player.playernum].textEntries[element].entry13 != entry->getText() )
			{
				entry->reflowTextToFit(0);
				charsheetTooltipCache[player.playernum].textEntries[element].entry13 = entry->getText();
			}
			entryPos.h = actualFont->height(true) * entry->getNumTextLines() + extraTextHeightForLowerCharacters;
			entry->setSize(entryPos);
			entry->setColor(hudColors.characterSheetOffWhiteText);
			currentHeight = std::max(entryPos.y + entryPos.h - extraTextHeightForLowerCharacters, 0);

			currentHeight += padyMid / 4;
			tooltipPos.h = pady1 + currentHeight + pady2;
		}

		tooltipPos.h = pady1 + currentHeight + pady2;
		if ( tooltipJustify == PANEL_JUSTIFY_RIGHT )
		{
			tooltipPos.x = pos.x - tooltipPos.w;
		}
		else
		{
			tooltipPos.x = pos.x;
		}
		tooltipPos.y = pos.y;

		tooltipFrame->setSize(tooltipPos);
		Player::GUI_t::imageResizeToContainer9x9(tooltipFrame, SDL_Rect{ 0, 0, tooltipPos.w, tooltipPos.h },
			Player::GUI_t::tooltipEffectBackgroundImages);
	}
	else if ( element >= Player::CharacterSheet_t::SHEET_ATK && element <= Player::CharacterSheet_t::SHEET_WGT )
	{
		AttackHoverText_t attackHoverTextInfo;
		Sint32 attackPower = displayAttackPower(player.playernum, attackHoverTextInfo);

//#ifndef NDEBUG
//		if ( keystatus[SDLK_V] )
//		{
//			keystatus[SDLK_V] = 0;
//			messagePlayer(player.playernum, MESSAGE_DEBUG, "Remove this");
//			stats[player.playernum]->playerRace = RACE_AUTOMATON;
//			stats[player.playernum]->appearance = 0;
//		}
//		if ( keystatus[SDLK_B] )
//		{
//			keystatus[SDLK_B] = 0;
//			messagePlayer(player.playernum, MESSAGE_DEBUG, "Remove this");
//			stats[player.playernum]->playerRace = RACE_INSECTOID;
//			stats[player.playernum]->appearance = 0;
//		}
//#endif // !NDEBUG

		bool isAutomatonHTRegen = stats[player.playernum]->type == AUTOMATON;
		bool isInsectoidENRegen = (stats[player.playernum]->playerRace == RACE_INSECTOID && stats[player.playernum]->stat_appearance == 0);

		auto tooltipTopLeft = tooltipFrame->findImage(Player::GUI_t::tooltipEffectBackgroundImages[Player::GUI_t::TOP_LEFT].c_str());
		tooltipTopLeft->path = "*#images/ui/CharSheet/HUD_CharSheet_Tooltip_TL_Blue_00.png";
		auto tooltipTop = tooltipFrame->findImage(Player::GUI_t::tooltipEffectBackgroundImages[Player::GUI_t::TOP].c_str());
		tooltipTop->path = "*#images/ui/CharSheet/HUD_CharSheet_Tooltip_T_Blue_00.png";
		auto tooltipTopRight = tooltipFrame->findImage(Player::GUI_t::tooltipEffectBackgroundImages[Player::GUI_t::TOP_RIGHT].c_str());
		tooltipTopRight->path = "*#images/ui/CharSheet/HUD_CharSheet_Tooltip_TR_Blue_00.png";
		Player::GUI_t::imageSetWidthHeight9x9(tooltipFrame, Player::GUI_t::tooltipEffectBackgroundImages);

		int maxWidth = 260;
		if ( getHoverTextString("attributes_max_tooltip_width") != defaultString )
		{
			maxWidth = std::max(0, std::stoi(getHoverTextString("attributes_max_tooltip_width")));
		}
		int minWidth = 0;
		if ( getHoverTextString("attributes_min_tooltip_width") != defaultString )
		{
			minWidth = std::max(0, std::stoi(getHoverTextString("attributes_min_tooltip_width")));
		}
		const int padx = 16;
		const int pady1 = 8;
		const int pady2 = 4;
		const int padxMid = 4;
		const int padyMid = 8;
		SDL_Rect tooltipPos = SDL_Rect{ 400, 0, maxWidth, 100 };

		std::string titleText = "";
		std::string descText = "";
		int value = 0;
		switch ( element )
		{
			case SHEET_ATK:
			{
				char descBuf[256];
				std::string skillName = "-";
				for ( auto& skill : player.skillSheet.skillSheetData.skillEntries )
				{
					if ( skill.skillId == attackHoverTextInfo.proficiency )
					{
						skillName = skill.getSkillName();
						break;
					}
				}
				titleText = getHoverTextString("attributes_atk_title");
				txtRightAlignHint->setText("");
				txtRightAlignHint->setDisabled(false);
				if ( attackHoverTextInfo.hoverType == AttackHoverText_t::ATK_HOVER_TYPE_UNARMED )
				{
					txtRightAlignHint->setText(getHoverTextString("attributes_atk_title_unarmed").c_str());
				}
				else if ( stats[player.playernum]->weapon )
				{
					Item* item = stats[player.playernum]->weapon;
					if ( item->type >= WOODEN_SHIELD && item->type < NUMITEMS )
					{
						char itemNameBuf[128];
						if ( itemCategory(item) == MAGICSTAFF )
						{
							snprintf(itemNameBuf, sizeof(itemNameBuf), "%s (%+d)", item->getName(), item->beatitude);
							std::string itemNameStr = itemNameBuf;
							capitalizeString(itemNameStr);
							txtRightAlignHint->setText(itemNameStr.c_str());
						}
						else
						{
							snprintf(itemNameBuf, sizeof(itemNameBuf), "%s %s (%+d)", 
								ItemTooltips.getItemStatusAdjective(item->type, item->status).c_str(), item->getName(), item->beatitude);
							txtRightAlignHint->setText(itemNameBuf);
						}
					}
				}
				if ( attackHoverTextInfo.hoverType == AttackHoverText_t::ATK_HOVER_TYPE_MELEE_WEAPON
					|| attackHoverTextInfo.hoverType == AttackHoverText_t::ATK_HOVER_TYPE_RAPIER
					|| attackHoverTextInfo.hoverType == AttackHoverText_t::ATK_HOVER_TYPE_WHIP )
				{
					snprintf(descBuf, sizeof(descBuf), getHoverTextString("attributes_atk_melee_desc").c_str(), skillName.c_str());
					descText = descBuf;
				}
				else if ( attackHoverTextInfo.hoverType == AttackHoverText_t::ATK_HOVER_TYPE_UNARMED )
				{
					snprintf(descBuf, sizeof(descBuf), getHoverTextString("attributes_atk_unarmed_desc").c_str(), skillName.c_str());
					descText = descBuf;
				}
				else if ( attackHoverTextInfo.hoverType == AttackHoverText_t::ATK_HOVER_TYPE_RANGED )
				{
					snprintf(descBuf, sizeof(descBuf), getHoverTextString("attributes_atk_ranged_desc").c_str(), skillName.c_str());
					descText = descBuf;
				}
				else if ( attackHoverTextInfo.hoverType == AttackHoverText_t::ATK_HOVER_TYPE_THROWN )
				{
					snprintf(descBuf, sizeof(descBuf), getHoverTextString("attributes_atk_thrown_desc").c_str(), skillName.c_str());
					descText = descBuf;
				}
				else if ( attackHoverTextInfo.hoverType == AttackHoverText_t::ATK_HOVER_TYPE_THROWN_MISC )
				{
					snprintf(descBuf, sizeof(descBuf), getHoverTextString("attributes_atk_thrown_desc").c_str(), skillName.c_str());
					descText = descBuf;
				}
				else if ( attackHoverTextInfo.hoverType == AttackHoverText_t::ATK_HOVER_TYPE_THROWN_GEM )
				{
					snprintf(descBuf, sizeof(descBuf), getHoverTextString("attributes_atk_gem_desc").c_str(), skillName.c_str());
					descText = descBuf;
				}
				else if ( attackHoverTextInfo.hoverType == AttackHoverText_t::ATK_HOVER_TYPE_THROWN_POTION )
				{
					snprintf(descBuf, sizeof(descBuf), getHoverTextString("attributes_atk_potion_desc").c_str(), skillName.c_str());
					descText = descBuf;
				}
				else if ( attackHoverTextInfo.hoverType == AttackHoverText_t::ATK_HOVER_TYPE_PICKAXE )
				{
					snprintf(descBuf, sizeof(descBuf), getHoverTextString("attributes_atk_pickaxe_desc").c_str());
					descText = descBuf;
				}
				else if ( attackHoverTextInfo.hoverType == AttackHoverText_t::ATK_HOVER_TYPE_MAGICSTAFF )
				{
					snprintf(descBuf, sizeof(descBuf), getHoverTextString("attributes_atk_magicstaff_desc").c_str());
					descText = descBuf;
				}
				else if ( attackHoverTextInfo.hoverType == AttackHoverText_t::ATK_HOVER_TYPE_TOOL )
				{
					snprintf(descBuf, sizeof(descBuf), getHoverTextString("attributes_atk_tool_desc").c_str());
					descText = descBuf;
				}
				else if ( attackHoverTextInfo.hoverType == AttackHoverText_t::ATK_HOVER_TYPE_TOOL_TRAP )
				{
					snprintf(descBuf, sizeof(descBuf), getHoverTextString("attributes_atk_tinker_tool_desc").c_str());
					descText = descBuf;
				}
				else
				{
					descText = "";
				}
			}
				break;
			case SHEET_AC:
				titleText = getHoverTextString("attributes_ac_title");
				descText = getHoverTextString("attributes_ac_desc");
				break;
			case SHEET_POW:
				titleText = getHoverTextString("attributes_pwr_title");
				if ( auto spell = player.magic.selectedSpell() )
				{
					for ( auto& skill : player.skillSheet.skillSheetData.skillEntries )
					{
						if ( skill.skillId == spell->skillID )
						{
							char buf[128];
							snprintf(buf, sizeof(buf), getHoverTextString("attributes_pwr_title_skill").c_str(), skill.getSkillName().c_str());
							titleText = buf;
							break;
						}
					}
				}
				descText = getHoverTextString("attributes_pwr_desc");
				break;
			case SHEET_RES:
				titleText = getHoverTextString("attributes_res_title");
				descText = getHoverTextString("attributes_res_desc");
				break;
			case SHEET_RGN:
				titleText = getHoverTextString("attributes_rgn_hp_title");
				if ( !(svFlags & SV_FLAG_HUNGER) )
				{
					descText = getHoverTextString("attributes_rgn_hp_desc_no_hunger2");
				}
				else
				{
					descText = getHoverTextString("attributes_rgn_hp_desc");
				}
				break;
			case SHEET_RGN_MP:
				if ( isAutomatonHTRegen )
				{
					titleText = getHoverTextString("attributes_rgn_ht_title");
					descText = getHoverTextString("attributes_rgn_ht_desc");
				}
				else if ( isInsectoidENRegen )
				{
					titleText = getHoverTextString("attributes_rgn_en_title");
					if ( !(svFlags & SV_FLAG_HUNGER) )
					{
						descText = getHoverTextString("attributes_rgn_en_desc_no_hunger");
					}
					else
					{
						descText = getHoverTextString("attributes_rgn_en_desc");
					}
				}
				else
				{
					titleText = getHoverTextString("attributes_rgn_mp_title");
					descText = getHoverTextString("attributes_rgn_mp_desc");
				}
				break;
			case SHEET_WGT:
				titleText = getHoverTextString("attributes_wgt_title");
				descText = getHoverTextString("attributes_wgt_desc");
				break;
			default:
				break;
		}

		txt->setText(titleText.c_str());
		SDL_Rect txtPos = SDL_Rect{ padx, pady1 - 2, maxWidth - padx * 2, 80 };
		txt->setSize(txtPos);
		if ( charsheetTooltipCache[player.playernum].textEntries[element].title != txt->getText() )
		{
			txt->reflowTextToFit(0);
			charsheetTooltipCache[player.playernum].textEntries[element].title = txt->getText();
		}
		txt->setColor(hudColors.characterSheetHeadingText);
		Font* actualFont = Font::get(txt->getFont());
		int txtHeight = txt->getNumTextLines() * actualFont->height(true);
		txtPos.h = txtHeight + 4;
		auto txtGet = Text::get(txt->getLongestLine().c_str(), txt->getFont(),
			txt->getTextColor(), txt->getOutlineColor());
		txtPos.w = txtGet->getWidth();
		txtPos.w = std::max(minWidth - padx * 2, txtPos.w);
		txt->setSize(txtPos);

		txtRightAlignHint->setSize(txtPos);
		txtRightAlignHint->setHJustify(Field::justify_t::RIGHT);

		tooltipPos.w = (txtPos.w + padx * 2);

		unsigned int longestValue = 0;
		std::map<int, std::pair<Field*, SDL_Rect>> valueSizes;

		int currentHeight = txtPos.y + (actualFont->height(true) * 1) + 2;
		const int extraTextHeightForLowerCharacters = 4;
		int currentTextFieldIndex = 1;
		int currentTextBackingFrameIndex = 1;

		char buf[128] = "";
		char valueBuf[128] = "";

		if ( element == SHEET_ATK && getAttackTooltipLines(player.playernum, attackHoverTextInfo, 1, buf, valueBuf)
			|| element != SHEET_ATK )
		{
			currentHeight += padyMid;
			auto entry = characterSheetTooltipTextFields[player.playernum][currentTextFieldIndex]; assert(entry);
			++currentTextFieldIndex;
			entry->setDisabled(false);
			switch ( element )
			{
				case SHEET_ATK:
					break;
				case SHEET_AC:
				{
					snprintf(buf, sizeof(buf), "%s", getHoverTextString("attributes_ac_base").c_str());
					bool oldDefending = stats[player.playernum]->defending;
					stats[player.playernum]->defending = false;
					Sint32 armor = AC(stats[player.playernum]);
					snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("attributes_ac_nobonus_format").c_str(), armor);
					stats[player.playernum]->defending = oldDefending;
				}
					break;
				case SHEET_POW:
				{
					snprintf(buf, sizeof(buf), "%s", getHoverTextString("attributes_pwr_base").c_str());
					std::string tag = "MAGIC_SPELLPOWER_TOTAL";
					std::string formatValue = "%d";
					std::string pwrBonus = "";
					if ( auto spell = player.magic.selectedSpell() )
					{
						pwrBonus = formatSkillSheetEffects(player.playernum, spell->skillID, tag, formatValue);
					}
					else
					{
						pwrBonus = formatSkillSheetEffects(player.playernum, NUMPROFICIENCIES, tag, formatValue);
					}
					Sint32 pwr = 100 + std::stoi(pwrBonus);
					snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("attributes_pwr_nobonus_format").c_str(), pwr);
				}
					break;
				case SHEET_RES:
				{
					snprintf(buf, sizeof(buf), "%s", getHoverTextString("attributes_res_base").c_str());
					real_t resistance = 100.0 * Entity::getDamageTableMultiplier(player.entity, *stats[player.playernum], DAMAGE_TABLE_MAGIC);
					resistance = 100.0 - resistance;
					snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("attributes_res_nobonus_format").c_str(), (int)resistance);
				}
					break;
				case SHEET_RGN:
				{
					snprintf(buf, sizeof(buf), "%s", getHoverTextString("attributes_rgn_hp_base").c_str());
					char hpbuf[32] = "";
					getDisplayedHPRegen(players[player.playernum]->entity, *stats[player.playernum], nullptr, hpbuf);
					/*if ( !(svFlags & SV_FLAG_HUNGER) )
					{
						snprintf(valueBuf, sizeof(valueBuf), "%s", hpbuf);
					}
					else*/
					{
						snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("attributes_rgn_nobonus_format").c_str(), hpbuf);
					}
				}
					break;
				case SHEET_RGN_MP:
				{
					char mpbuf[32] = "";
					real_t regen = getDisplayedMPRegen(players[player.playernum]->entity, *stats[player.playernum], nullptr, mpbuf);
					if ( isAutomatonHTRegen )
					{
						snprintf(buf, sizeof(buf), "%s", getHoverTextString("attributes_rgn_ht_base").c_str());
						snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("attributes_rgn_nobonus_format").c_str(), mpbuf);
					}
					else if ( isInsectoidENRegen )
					{
						if ( !(svFlags & SV_FLAG_HUNGER) )
						{
							snprintf(buf, sizeof(buf), "%s", getHoverTextString("attributes_rgn_en_base").c_str());
							snprintf(valueBuf, sizeof(valueBuf), "%s", mpbuf);
						}
						else
						{
							snprintf(buf, sizeof(buf), "%s", getHoverTextString("attributes_rgn_en_base").c_str());
							snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("attributes_rgn_en_nobonus_format").c_str(), mpbuf);
						}
					}
					else
					{
						snprintf(buf, sizeof(buf), "%s", getHoverTextString("attributes_rgn_mp_base").c_str());
						snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("attributes_rgn_nobonus_format").c_str(), mpbuf);
					}
				}
					break;
				case SHEET_WGT:
				{
					snprintf(buf, sizeof(buf), "%s", getHoverTextString("attributes_wgt_base").c_str());
					int weight = player.movement.getCharacterWeight();
					snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("attributes_wgt_nobonus_format").c_str(), weight);
				}
					break;
				default:
					break;
			}
			entry->setText(buf);
			entry->setVJustify(Field::justify_t::TOP);

			auto glyphBacking = tooltipFrame->findImage("glyph 1");
			glyphBacking->disabled = false;
			glyphBacking->path = getHoverTextString("icon_backing_path");
			glyphBacking->pos.x = padx + padxMid + 4;
			glyphBacking->pos.y = currentHeight + 6;
			glyphBacking->pos.w = 44;
			glyphBacking->pos.h = 44;

			auto glyphIcon = tooltipFrame->findImage("glyph 2");
			glyphIcon->disabled = false;
			glyphIcon->path = "";
			switch ( element )
			{
				case SHEET_ATK:
					if ( attackHoverTextInfo.proficiency == -1 )
					{
						glyphIcon->path = getHoverTextString("icon_atk_path");
					}
					else if (attackHoverTextInfo.proficiency >= 0 && attackHoverTextInfo.proficiency < NUMPROFICIENCIES )
					{
						for ( auto& skill : player.skillSheet.skillSheetData.skillEntries )
						{
							if ( skill.skillId == attackHoverTextInfo.proficiency )
							{
								if ( skillCapstoneUnlocked(player.playernum, attackHoverTextInfo.proficiency) )
								{
									glyphIcon->path = skill.skillIconPathLegend;
								}
								else
								{
									glyphIcon->path = skill.skillIconPath;
								}
								break;
							}
						}
						if ( stats[player.playernum]->getModifiedProficiency(attackHoverTextInfo.proficiency) >= SKILL_LEVEL_LEGENDARY )
						{
							glyphBacking->path = actionPromptBackingIconPath100;
						}
						else if ( stats[player.playernum]->getModifiedProficiency(attackHoverTextInfo.proficiency) >= SKILL_LEVEL_EXPERT )
						{
							glyphBacking->path = actionPromptBackingIconPath60;
						}
						else if ( stats[player.playernum]->getModifiedProficiency(attackHoverTextInfo.proficiency) >= SKILL_LEVEL_BASIC )
						{
							glyphBacking->path = actionPromptBackingIconPath20;
						}
						else
						{
							glyphBacking->path = actionPromptBackingIconPath00;
						}
					}
					else if ( attackHoverTextInfo.proficiency == PRO_LEGACY_MAGIC || attackHoverTextInfo.proficiency == PRO_LEGACY_SPELLCASTING )
					{
						glyphBacking->path = actionPromptBackingIconPath00;
						for ( auto& skill : player.skillSheet.skillSheetData.skillEntries )
						{
							if ( skill.skillId == attackHoverTextInfo.proficiency )
							{
								glyphIcon->path = skill.skillIconPath;
								break;
							}
						}
					}
					break;
				case SHEET_AC:
					glyphIcon->path = getHoverTextString("icon_ac_path");
					for ( auto& skill : player.skillSheet.skillSheetData.skillEntries )
					{
						if ( skill.skillId == PRO_SHIELD )
						{
							if ( skillCapstoneUnlocked(player.playernum, PRO_SHIELD) )
							{
								glyphIcon->path = skill.skillIconPathLegend;
							}
							else
							{
								glyphIcon->path = skill.skillIconPath;
							}
							break;
						}
					}
					if ( stats[player.playernum]->getModifiedProficiency(PRO_SHIELD) >= SKILL_LEVEL_LEGENDARY )
					{
						glyphBacking->path = actionPromptBackingIconPath100;
					}
					else if ( stats[player.playernum]->getModifiedProficiency(PRO_SHIELD) >= SKILL_LEVEL_EXPERT )
					{
						glyphBacking->path = actionPromptBackingIconPath60;
					}
					else if ( stats[player.playernum]->getModifiedProficiency(PRO_SHIELD) >= SKILL_LEVEL_BASIC )
					{
						glyphBacking->path = actionPromptBackingIconPath20;
					}
					else
					{
						glyphBacking->path = actionPromptBackingIconPath00;
					}
					break;
				case SHEET_POW:
					glyphIcon->path = getHoverTextString("icon_pwr_path");
					break;
				case SHEET_RES:
					glyphIcon->path = getHoverTextString("icon_res_path");
					break;
				case SHEET_RGN:
					glyphIcon->path = getHoverTextString("icon_rgn_hp_path");
					break;
				case SHEET_RGN_MP:
					if ( isAutomatonHTRegen )
					{
						if ( stats[player.playernum]->HUNGER <= 300 )
						{
							glyphIcon->path = getHoverTextString("icon_rgn_ht_empty_path");
						}
						else if ( stats[player.playernum]->HUNGER > 1200 )
						{
							glyphIcon->path = getHoverTextString("icon_rgn_ht_superheated_path");
						}
						else
						{
							glyphIcon->path = getHoverTextString("icon_rgn_ht_normal_path");
						}
					}
					else
					{
						glyphIcon->path = getHoverTextString("icon_rgn_mp_path");
					}
					break;
				case SHEET_WGT:
					glyphIcon->path = getHoverTextString("icon_wgt_path");
					break;
				default:
					break;
			}
			glyphIcon->pos.w = 24;
			glyphIcon->pos.h = 24;
			glyphIcon->pos.x = glyphBacking->pos.x + glyphBacking->pos.w / 2 - glyphIcon->pos.w / 2;
			glyphIcon->pos.y = glyphBacking->pos.y + glyphBacking->pos.h / 2 - glyphIcon->pos.h / 2;

			SDL_Rect entryPos = entry->getSize();
			entryPos.x = padx / 2 + glyphBacking->pos.x + glyphBacking->pos.w;
			entryPos.y = currentHeight;
			if ( element == SHEET_ATK )
			{
				if ( attackHoverTextInfo.hoverType == AttackHoverText_t::ATK_HOVER_TYPE_THROWN
					|| attackHoverTextInfo.hoverType == AttackHoverText_t::ATK_HOVER_TYPE_THROWN_MISC
					|| attackHoverTextInfo.hoverType == AttackHoverText_t::ATK_HOVER_TYPE_THROWN_GEM
					|| attackHoverTextInfo.hoverType == AttackHoverText_t::ATK_HOVER_TYPE_PICKAXE )
				{
					// fewer lines, add offset to centre the lines with the glyph
					entryPos.y += 8;
				}
				else if ( attackHoverTextInfo.hoverType == AttackHoverText_t::ATK_HOVER_TYPE_DEFAULT
					|| attackHoverTextInfo.hoverType == AttackHoverText_t::ATK_HOVER_TYPE_TOOL
					|| attackHoverTextInfo.hoverType == AttackHoverText_t::ATK_HOVER_TYPE_TOOL_TRAP
					|| attackHoverTextInfo.hoverType == AttackHoverText_t::ATK_HOVER_TYPE_MAGICSTAFF )
				{
					entryPos.y += 16;
				}
			}
			else if ( element == SHEET_RGN_MP && isInsectoidENRegen )
			{
				entryPos.y += 16;
			}
			entryPos.w = txtPos.w - (padxMid + glyphBacking->pos.x + glyphBacking->pos.w);
			entry->setSize(entryPos);
			if ( charsheetTooltipCache[player.playernum].textEntries[element].entry1 != entry->getText() )
			{
				entry->reflowTextToFit(0);
				charsheetTooltipCache[player.playernum].textEntries[element].entry1 = entry->getText();
			}
			entryPos.h = actualFont->height(true) * entry->getNumTextLines() + extraTextHeightForLowerCharacters;
			entry->setSize(entryPos);
			entry->setColor(defaultColor);
			currentHeight = std::max(entryPos.y + entryPos.h - extraTextHeightForLowerCharacters, 0);
			tooltipPos.h = pady1 + currentHeight + pady2;

			auto entryValue = characterSheetTooltipTextFields[player.playernum][currentTextFieldIndex]; assert(entry);
			++currentTextFieldIndex;
			entryValue->setDisabled(false);
			int value = 0;
			switch ( element )
			{
				case SHEET_ATK:
					break;
				case SHEET_AC:
					break;
				case SHEET_POW:
					break;
				case SHEET_RES:
					break;
				case SHEET_RGN:
					break;
				case SHEET_WGT:
					break;
				default:
					break;
			}
			entryValue->setColor(hudColors.characterSheetNeutral);
			if ( value < 0 )
			{
				entryValue->setColor(hudColors.characterSheetRed);
			}
			entryValue->setText(valueBuf);
			SDL_Rect entryValuePos = entry->getSize();
			entryValue->setSize(entryValuePos);
			entryValue->setHJustify(Frame::justify_t::RIGHT);
			entryValue->setVJustify(Field::justify_t::TOP);

			/*auto txtValueBackingFrame = tooltipFrame->findFrame("txt value backing frame 1");
			SDL_Rect backingFramePos = entryValue->getSize();
			auto txtValueGet = Text::get(entryValue->getText(), entryValue->getFont(),
				entryValue->getTextColor(), entryValue->getOutlineColor());
			longestValue = std::max(longestValue, txtValueGet->getWidth());
			backingFramePos.x = backingFramePos.x + backingFramePos.w;
			backingFramePos.h = actualFont->height(true) + extraTextHeightForLowerCharacters - 2;
			valueSizes[1] = std::make_pair(entryValue, backingFramePos);
			txtValueBackingFrame->setDisabled(false);*/
			++currentTextBackingFrameIndex;
		}

		if ( element == SHEET_ATK && getAttackTooltipLines(player.playernum, attackHoverTextInfo, 2, buf, valueBuf)
			|| element != SHEET_ATK )
		{
			currentHeight += 0;
			auto entry = characterSheetTooltipTextFields[player.playernum][currentTextFieldIndex]; assert(entry);
			++currentTextFieldIndex;
			entry->setDisabled(false);
			switch ( element )
			{
				case SHEET_ATK:
					break;
				case SHEET_AC:
				{
					std::string skillName = "";
					int skillLVL = 0;
					for ( auto& skill : player.skillSheet.skillSheetData.skillEntries )
					{
						if ( skill.skillId == PRO_SHIELD )
						{
							skillName = skill.getSkillName();
							skillLVL = stats[player.playernum]->getModifiedProficiency(skill.skillId);
							break;
						}
					}
					snprintf(buf, sizeof(buf), getHoverTextString("attributes_ac_defending").c_str(), skillName.c_str(), skillLVL);
					std::string tag = "BLOCK_AC_INCREASE";
					if ( stats[player.playernum]->shield && itemCategory(stats[player.playernum]->shield) != ARMOR )
					{
						tag = "BLOCK_AC_INCREASE_OFFHAND";
					}
					std::string blockBonus = formatSkillSheetEffects(player.playernum, PRO_SHIELD, tag, getHoverTextString("attributes_ac_bonus_format"));
					snprintf(valueBuf, sizeof(valueBuf), "%s", blockBonus.c_str());
				}
					break;
				case SHEET_POW:
				{
					snprintf(buf, sizeof(buf), "%s", getHoverTextString("attributes_pwr_spellbook").c_str());
					std::string tag = "MAGIC_SPELLPOWER_INT";
					std::string formatValue = "%d";
					std::string pwrBonus = formatSkillSheetEffects(player.playernum, NUMPROFICIENCIES, tag, formatValue);
					Sint32 pwr = std::stoi(pwrBonus) / 2;
					snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("attributes_pwr_bonus_format").c_str(), pwr);
				}
					break;
				case SHEET_RES:
				{
					snprintf(buf, sizeof(buf), "%s", getHoverTextString("attributes_res_sources").c_str());
					int sources = 0;
					Entity::getDamageTableMultiplier(player.entity, *stats[player.playernum], DAMAGE_TABLE_MAGIC, nullptr, &sources);
					snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("attributes_res_sources_format").c_str(), sources);
				}
					break;
				case SHEET_RGN:
				{
					snprintf(buf, sizeof(buf), "%s", getHoverTextString("attributes_rgn_sources").c_str());
					int sources = Entity::getHealringFromEquipment(players[player.playernum]->entity, *stats[player.playernum], true);
					sources += Entity::getHealringFromEffects(players[player.playernum]->entity, *stats[player.playernum]);
					sources = std::min(sources, 3);
					snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("attributes_rgn_hp_sources_format").c_str(), sources);
				}
					break;
				case SHEET_RGN_MP:
				{
					if ( isInsectoidENRegen )
					{
						//snprintf(buf, sizeof(buf), "%s", getHoverTextString("attributes_rgn_en_sources").c_str());
						snprintf(buf, sizeof(buf), "");
						snprintf(valueBuf, sizeof(valueBuf), "");
					}
					else
					{
						snprintf(buf, sizeof(buf), "%s", getHoverTextString("attributes_rgn_sources").c_str());
						int sources = Entity::getManaringFromEquipment(players[player.playernum]->entity, *stats[player.playernum], true);
						sources += Entity::getManaringFromEffects(players[player.playernum]->entity, *stats[player.playernum]);
						sources = std::min(sources, 3);
						if ( isAutomatonHTRegen )
						{
							snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("attributes_rgn_ht_sources_format").c_str(), sources);
						}
						else
						{
							snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("attributes_rgn_mp_sources_format").c_str(), sources);
						}
					}
				}
					break;
				case SHEET_WGT:
				{
					snprintf(buf, sizeof(buf), "%s", getHoverTextString("attributes_wgt_movement_speed").c_str());
					int weight = player.movement.getCharacterModifiedWeight();
					Sint32 STR = statGetSTR(stats[player.playernum], player.entity);
					Sint32 DEX = statGetDEX(stats[player.playernum], player.entity);
					real_t currentSpeed = player.movement.getSpeedFactor(player.movement.getWeightRatio(weight, STR), DEX);
					real_t maxSpeed = player.movement.getMaximumSpeed();
					snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("attributes_wgt_speed_format").c_str(), 
						100.0 * currentSpeed / std::fmax(.01, maxSpeed));
				}
					break;
				default:
					break;
			}
			entry->setText(buf);
			entry->setVJustify(Field::justify_t::TOP);

			auto glyphBacking = tooltipFrame->findImage("glyph 1");

			SDL_Rect entryPos = entry->getSize();
			entryPos.x = padx / 2 + glyphBacking->pos.x + glyphBacking->pos.w;
			entryPos.y = currentHeight;
			entryPos.w = txtPos.w - (padxMid + glyphBacking->pos.x + glyphBacking->pos.w);
			entry->setSize(entryPos);
			if ( charsheetTooltipCache[player.playernum].textEntries[element].entry2 != entry->getText() )
			{
				entry->reflowTextToFit(0);
				charsheetTooltipCache[player.playernum].textEntries[element].entry2 = entry->getText();
			}
			entryPos.h = actualFont->height(true) * entry->getNumTextLines() + extraTextHeightForLowerCharacters;
			entry->setSize(entryPos);
			entry->setColor(defaultColor);

			if ( strcmp(buf, "") )
			{
				// don't modify height if this is an empty line
				currentHeight = std::max(entryPos.y + entryPos.h - extraTextHeightForLowerCharacters, 0);
			}
			tooltipPos.h = pady1 + currentHeight + pady2;

			auto entryValue = characterSheetTooltipTextFields[player.playernum][currentTextFieldIndex]; assert(entry);
			++currentTextFieldIndex;
			entryValue->setDisabled(false);
			int value = 0;
			switch ( element )
			{
				case SHEET_ATK:
					break;
				case SHEET_AC:
					break;
				case SHEET_POW:
					break;
				case SHEET_RES:
					break;
				case SHEET_RGN:
					break;
				case SHEET_WGT:
					break;
				default:
					break;
			}
			entryValue->setColor(hudColors.characterSheetNeutral);
			if ( value < 0 )
			{
				entryValue->setColor(hudColors.characterSheetRed);
			}
			else if ( value > 0 )
			{
				entryValue->setColor(hudColors.characterSheetGreen);
			}
			entryValue->setText(valueBuf);
			SDL_Rect entryValuePos = entry->getSize();
			entryValue->setSize(entryValuePos);
			entryValue->setHJustify(Frame::justify_t::RIGHT);
			entryValue->setVJustify(Field::justify_t::TOP);

			/*auto txtValueBackingFrame = tooltipFrame->findFrame("txt value backing frame 2");
			SDL_Rect backingFramePos = entryValue->getSize();
			auto txtValueGet = Text::get(entryValue->getText(), entryValue->getFont(),
				entryValue->getTextColor(), entryValue->getOutlineColor());
			longestValue = std::max(longestValue, txtValueGet->getWidth());
			backingFramePos.x = backingFramePos.x + backingFramePos.w;
			backingFramePos.h = actualFont->height(true) + extraTextHeightForLowerCharacters - 2;
			valueSizes[2] = std::make_pair(entryValue, backingFramePos);
			txtValueBackingFrame->setDisabled(false);*/
			++currentTextBackingFrameIndex;
		}
		bool hasEntryInfoLines = false;
		if ( element == SHEET_ATK && getAttackTooltipLines(player.playernum, attackHoverTextInfo, 3, buf, valueBuf)
			|| (element != SHEET_ATK 
				&& !(element == SHEET_RGN && false/*&& !(svFlags & SV_FLAG_HUNGER) && stats[player.playernum]->type != SKELETON*/)
				&& !(element == SHEET_RGN_MP && isInsectoidENRegen && !(svFlags & SV_FLAG_HUNGER))
				))
		{
			// extra number display - line 3
			hasEntryInfoLines = true;
			if ( element == SHEET_ATK || element == SHEET_AC || element == SHEET_POW || element == SHEET_RES || element == SHEET_RGN
				|| element == SHEET_RGN_MP || element == SHEET_WGT )
			{
				if ( element == SHEET_RGN_MP && isInsectoidENRegen )
				{
					currentHeight += 8;
				}

				// add a divider
				div2->pos.x = padx;
				div2->pos.y = currentHeight + 2 + actualFont->height(true);
				div2->pos.w = txtPos.w;
				div2->disabled = false;

				auto entryTotalHeading = characterSheetTooltipTextFields[player.playernum][16]; assert(entryTotalHeading);
				entryTotalHeading->setDisabled(false);
				if ( element == SHEET_ATK )
				{
					entryTotalHeading->setText(getHoverTextString("attributes_atk_total_sum_header").c_str());
				}
				else if ( element == SHEET_AC )
				{
					entryTotalHeading->setText(getHoverTextString("attributes_ac_base_sum_header").c_str());
				}
				else if ( element == SHEET_POW )
				{
					entryTotalHeading->setText(getHoverTextString("attributes_pwr_base_sum_header").c_str());
				}
				else if ( element == SHEET_RES )
				{
					entryTotalHeading->setText(getHoverTextString("attributes_res_base_sum_header").c_str());
				}
				else if ( element == SHEET_RGN )
				{
					entryTotalHeading->setText(getHoverTextString("attributes_rgn_hp_base_sum_header").c_str());
				}
				else if ( element == SHEET_RGN_MP )
				{
					if ( isAutomatonHTRegen )
					{
						entryTotalHeading->setText(getHoverTextString("attributes_rgn_ht_base_sum_header").c_str());
					}
					else if ( isInsectoidENRegen )
					{
						entryTotalHeading->setText(getHoverTextString("attributes_rgn_en_base_sum_header").c_str());
					}
					else
					{
						entryTotalHeading->setText(getHoverTextString("attributes_rgn_mp_base_sum_header").c_str());
					}
				}
				else if ( element == SHEET_WGT )
				{
					entryTotalHeading->setText(getHoverTextString("attributes_wgt_base_sum_header").c_str());
				}
				entryTotalHeading->setColor(hudColors.characterSheetOffWhiteText);
				entryTotalHeading->setHJustify(Field::justify_t::RIGHT);
				SDL_Rect entryPos = entryTotalHeading->getSize();
				entryPos.x = padx + padxMid;
				entryPos.y = currentHeight + 1 - (extraTextHeightForLowerCharacters / 2);
				entryPos.w = txtPos.w - (padxMid * 2);
				entryPos.h = actualFont->height(true) + extraTextHeightForLowerCharacters;
				entryTotalHeading->setSize(entryPos);

				currentHeight += actualFont->height(true) + 2; // extra gap here for 'total' text.
			}
			currentHeight += padyMid;
			auto entry = characterSheetTooltipTextFields[player.playernum][currentTextFieldIndex]; assert(entry);
			++currentTextFieldIndex;
			entry->setDisabled(false);
			int value = 0;
			switch ( element )
			{
				case SHEET_ATK:
					break;
				case SHEET_AC:
				{
					snprintf(buf, sizeof(buf), "%s", getHoverTextString("attributes_ac_entry_attr_bonus").c_str());
					Sint32 CON = statGetCON(stats[player.playernum], players[player.playernum]->entity);
					snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("attributes_ac_bonus_format").c_str(), CON);
				}
					break;
				case SHEET_POW:
				{
					snprintf(buf, sizeof(buf), "%s", getHoverTextString("attributes_pwr_base_value").c_str());
					snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("attributes_pwr_bonus_format").c_str(), 100);
				}
					break;
				case SHEET_RES:
				{
					Monster type = stats[player.playernum]->type;
					std::string appearance = "";
					bool aestheticOnly = false;
					if ( player.entity )
					{
						if ( player.entity->effectPolymorph == NOTHING && stats[player.playernum]->playerRace > RACE_HUMAN )
						{
							if ( stats[player.playernum]->stat_appearance != 0 )
							{
								aestheticOnly = true;
								appearance = Language::get(4068);
								type = player.entity->getMonsterFromPlayerRace(stats[player.playernum]->playerRace);
							}
						}
					}
					std::string race = getMonsterLocalizedName(type).c_str();
					capitalizeString(race);

					snprintf(buf, sizeof(buf), getHoverTextString("attributes_res_base_value").c_str(), race.c_str());
					Sint32 baseResist = damagetables[stats[player.playernum]->type][DAMAGE_TABLE_MAGIC] * 100;
					baseResist = 100 - baseResist;
					snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("attributes_res_bonus_format").c_str(), baseResist);
				}
					break;
				case SHEET_RGN:
				{
					Monster type = stats[player.playernum]->type;
					bool aestheticOnly = false;
					if ( player.entity )
					{
						if ( player.entity->effectPolymorph == NOTHING && stats[player.playernum]->playerRace > RACE_HUMAN )
						{
							if ( stats[player.playernum]->stat_appearance != 0 )
							{
								aestheticOnly = true;
								type = player.entity->getMonsterFromPlayerRace(stats[player.playernum]->playerRace);
							}
						}
					}
					std::string race = getMonsterLocalizedName(type).c_str();
					capitalizeString(race);

					snprintf(buf, sizeof(buf), getHoverTextString("attributes_rgn_base_value").c_str(), race.c_str());
					/*real_t regen = 100.0;
					if ( !(svFlags & SV_FLAG_HUNGER) )
					{
						regen = 0.0;
					}
					if ( type == SKELETON )
					{
						regen = 25.0;
					}*/

					Sint32 oldLVL = 0;
					Sint32 oldStats[NUMSTATS];
					//characterSheetTooltipSetZeroStat(player.entity, stats[player.playernum], &oldLVL, oldStats);
					//real_t baseRegen = getDisplayedMPRegen(player.entity, *stats[player.playernum], nullptr, nullptr, true);
					//characterSheetTooltipRestoreStats(stats[player.playernum], &oldLVL, oldStats);

					characterSheetTooltipSetZeroStat(player.entity, stats[player.playernum], nullptr, oldStats);
					real_t regenWithoutStats = getDisplayedHPRegen(player.entity, *stats[player.playernum], nullptr, nullptr, true);
					characterSheetTooltipRestoreStats(stats[player.playernum], nullptr, oldStats);

					real_t displayedValue = regenWithoutStats;

					snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("attributes_rgn_bonus_format").c_str(), displayedValue);
				}
					break;
				case SHEET_RGN_MP:
				{
					if ( isAutomatonHTRegen )
					{
						snprintf(buf, sizeof(buf), "%s", getHoverTextString("attributes_rgn_ht_base_bonus").c_str());
						real_t baseHTModifier = 100.0 / (MAGIC_REGEN_AUTOMATON_TIME / (real_t)MAGIC_REGEN_TIME);
						if ( stats[player.playernum]->HUNGER <= 300 )
						{
							int baseTime = getBaseManaRegen(player.entity, *stats[player.playernum]);
							real_t scaledInterval = ((60 * baseTime) / (std::max(stats[player.playernum]->MAXMP, 1)));
							baseHTModifier = scaledInterval / TICKS_PER_SECOND;
							baseHTModifier /= -6.0; // degrade faster
							real_t nominalRegen = MAGIC_REGEN_TIME / TICKS_PER_SECOND;
							baseHTModifier = (nominalRegen / baseHTModifier) * 100.0;
						}
						else if ( stats[player.playernum]->HUNGER > 1200 )
						{
							if ( stats[player.playernum]->MP / static_cast<real_t>(std::max(1, stats[player.playernum]->MAXMP)) <= 0.5 )
							{
								baseHTModifier *= 4; // increase faster at < 50% mana
							}
							else
							{
								baseHTModifier *= 2; // increase less faster at > 50% mana
							}
						}
						else
						{
							// normal manaRegenInterval 300-1200 hunger.
						}
						//baseHTModifier /= 100.0;
						snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("attributes_rgn_ht_bonus_format").c_str(), baseHTModifier);
					}
					else if ( isInsectoidENRegen )
					{
						snprintf(buf, sizeof(buf), "%s", getHoverTextString("attributes_rgn_en_base_bonus").c_str());

						real_t normalRegenTime = (1000.f * 30 * 1.5) / static_cast<float>(TICKS_PER_SECOND); // 30 base, insectoid does 1.5x in getHungerTickRate()
						//normalRegenTime = (1000.f * (Entity::getHungerTickRate(stats[player.playernum], true, true)) / static_cast<float>(TICKS_PER_SECOND));
						normalRegenTime /= (std::max(stats[player.playernum]->MAXMP, 1)); // time for 1 mana in seconds
						normalRegenTime *= TICKS_PER_SECOND; // game ticks for 1 mana

						real_t modifiedRegenTime = (1000.f * (Entity::getHungerTickRate(stats[player.playernum], true, false)) / static_cast<float>(TICKS_PER_SECOND));
						modifiedRegenTime /= (std::max(stats[player.playernum]->MAXMP, 1)); // time for 1 mana in seconds
						modifiedRegenTime *= TICKS_PER_SECOND; // game ticks for 1 mana

						real_t displayValue = 100.0 * (normalRegenTime / modifiedRegenTime);

						snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("attributes_rgn_en_bonus_format").c_str(), displayValue);
					}
					else
					{
						Monster type = stats[player.playernum]->type;
						bool aestheticOnly = false;
						if ( player.entity )
						{
							if ( player.entity->effectPolymorph == NOTHING && stats[player.playernum]->playerRace > RACE_HUMAN )
							{
								if ( stats[player.playernum]->stat_appearance != 0 )
								{
									aestheticOnly = true;
									type = player.entity->getMonsterFromPlayerRace(stats[player.playernum]->playerRace);
								}
							}
						}
						std::string race = getMonsterLocalizedName(type).c_str();
						capitalizeString(race);

						snprintf(buf, sizeof(buf), getHoverTextString("attributes_rgn_base_value").c_str(), race.c_str());
						/*real_t regen = 100.0;
						if ( type == SKELETON )
						{
							regen = 25.0;
						}*/

						Sint32 oldLVL = 0;
						Sint32 oldStats[NUMSTATS];
						//characterSheetTooltipSetZeroStat(player.entity, stats[player.playernum], &oldLVL, oldStats);
						//real_t baseRegen = getDisplayedMPRegen(player.entity, *stats[player.playernum], nullptr, nullptr, true);
						//characterSheetTooltipRestoreStats(stats[player.playernum], &oldLVL, oldStats);

						characterSheetTooltipSetZeroStat(player.entity, stats[player.playernum], nullptr, oldStats);
						real_t regenWithoutStats = getDisplayedMPRegen(player.entity, *stats[player.playernum], nullptr, nullptr, true);
						characterSheetTooltipRestoreStats(stats[player.playernum], nullptr, oldStats);

						real_t displayedValue = regenWithoutStats;
						snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("attributes_rgn_bonus_format").c_str(), displayedValue);
					}
				}
				break;
				case SHEET_WGT:
				{
					int weight = player.movement.getCharacterModifiedWeight();
					//int equippedWeightTotal = player.movement.getCharacterEquippedWeight();
					//int equippedWeight = player.movement.getCharacterModifiedWeight(&equippedWeightTotal);
					//int goldWeightTotal = stats[player.playernum]->GOLD / 100;
					//int goldWeight = player.movement.getCharacterModifiedWeight(&goldWeightTotal);
					Sint32 STR = statGetSTR(stats[player.playernum], player.entity);
					Sint32 DEX = statGetDEX(stats[player.playernum], player.entity);
					//real_t currentEquippedSpeed = player.movement.getSpeedFactor(player.movement.getWeightRatio(equippedWeight, STR), DEX);
					real_t currentSpeed = player.movement.getSpeedFactor(player.movement.getWeightRatio(weight, STR), DEX);
					real_t noWeightSpeed = player.movement.getSpeedFactor(player.movement.getWeightRatio(0, STR), DEX);
					//real_t goldSpeed = player.movement.getSpeedFactor(player.movement.getWeightRatio(goldWeight, STR), DEX);
					real_t maxSpeed = player.movement.getMaximumSpeed();

					real_t currentSpeedPercent = 100.0 * currentSpeed / std::fmax(.01, maxSpeed);
					//real_t currentEquippedSpeedPercent = 100.0 * currentEquippedSpeed / std::fmax(.01, maxSpeed);
					real_t noWeightSpeedPercent = 100.0 * noWeightSpeed / std::fmax(.01, maxSpeed);
					//real_t goldSpeedPercent = 100.0 * goldSpeed / std::fmax(.01, maxSpeed);

					snprintf(buf, sizeof(buf), "%s", getHoverTextString("attributes_wgt_attributes_bonus").c_str());
					snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("attributes_wgt_speed_bonus_format").c_str(),
						currentSpeedPercent + (noWeightSpeedPercent - currentSpeedPercent) /*+ (goldSpeedPercent - noWeightSpeedPercent)*/);
				}
					break;
				default:
					break;
			}
			entry->setText(buf);
			entry->setVJustify(Field::justify_t::TOP);

			SDL_Rect entryPos = entry->getSize();
			entryPos.x = padx + padxMid;
			entryPos.y = currentHeight;
			entryPos.w = txtPos.w - (padxMid * 2);
			entry->setSize(entryPos);
			if ( charsheetTooltipCache[player.playernum].textEntries[element].entry3 != entry->getText() )
			{
				entry->reflowTextToFit(0);
				charsheetTooltipCache[player.playernum].textEntries[element].entry3 = entry->getText();
			}
			entryPos.h = actualFont->height(true) * entry->getNumTextLines() + extraTextHeightForLowerCharacters;
			entry->setSize(entryPos);
			entry->setColor(defaultColor);
			currentHeight = std::max(entryPos.y + entryPos.h - extraTextHeightForLowerCharacters, 0);
			tooltipPos.h = pady1 + currentHeight + pady2;

			auto entryValue = characterSheetTooltipTextFields[player.playernum][currentTextFieldIndex]; assert(entry);
			++currentTextFieldIndex;
			entryValue->setDisabled(false);
			switch ( element )
			{
				case SHEET_ATK:
					break;
				case SHEET_AC:
					break;
				case SHEET_POW:
					break;
				case SHEET_RES:
					break;
				case SHEET_RGN:
					break;
				case SHEET_WGT:
					break;
				default:
					break;
			}
			entryValue->setColor(hudColors.characterSheetNeutral);
			if ( value < 0 )
			{
				entryValue->setColor(hudColors.characterSheetRed);
			}
			else if ( value > 0 )
			{
				entryValue->setColor(hudColors.characterSheetGreen);
			}
			entryValue->setText(valueBuf);
			entryValue->setSize(entry->getSize());
			entryValue->setHJustify(Frame::justify_t::LEFT);
			entryValue->setVJustify(Field::justify_t::TOP);

			auto txtValueBackingFrame = characterSheetTooltipTextBackingFrames[player.playernum][currentTextBackingFrameIndex];
			SDL_Rect backingFramePos = entryValue->getSize();
			auto txtValueGet = Text::get(entryValue->getText(), entryValue->getFont(),
				entryValue->getTextColor(), entryValue->getOutlineColor());
			longestValue = std::max(longestValue, txtValueGet->getWidth());
			backingFramePos.x = backingFramePos.x + backingFramePos.w;
			backingFramePos.h = actualFont->height(true) + extraTextHeightForLowerCharacters - 2;
			valueSizes[currentTextBackingFrameIndex] = std::make_pair(entryValue, backingFramePos);
			txtValueBackingFrame->setDisabled(false);
			++currentTextBackingFrameIndex;
		}

		if ( (element == SHEET_ATK && getAttackTooltipLines(player.playernum, attackHoverTextInfo, 4, buf, valueBuf))
			|| (element != SHEET_ATK 
				&& !(element == SHEET_RGN && false/*&& !(svFlags & SV_FLAG_HUNGER) && stats[player.playernum]->type != SKELETON*/)
				&& !(element == SHEET_RGN_MP && isInsectoidENRegen && !(svFlags & SV_FLAG_HUNGER))
				)
			)
		{
			// extra number display - line 4
			hasEntryInfoLines = true;
			currentHeight += padyMid;
			auto entry = characterSheetTooltipTextFields[player.playernum][currentTextFieldIndex]; assert(entry);
			++currentTextFieldIndex;
			entry->setDisabled(false);
			int value = 0;
			switch ( element )
			{
				case SHEET_ATK:
					break;
				case SHEET_AC:
				{
					snprintf(buf, sizeof(buf), "%s", getHoverTextString("attributes_ac_entry_items_bonus").c_str());
					Sint32 CON = statGetCON(stats[player.playernum], players[player.playernum]->entity);

					Sint32 oldSkillLVL = stats[player.playernum]->getProficiency(PRO_SHIELD);
					bool oldDefending = stats[player.playernum]->defending;
					stats[player.playernum]->defending = false;
					stats[player.playernum]->setProficiencyUnsafe(PRO_SHIELD, -999);

					Sint32 armor = AC(stats[player.playernum]);
					stats[player.playernum]->defending = oldDefending;
					stats[player.playernum]->setProficiency(PRO_SHIELD, oldSkillLVL);

					Sint32 itemsEffectBonus = armor - CON;
					snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("attributes_ac_bonus_format").c_str(), itemsEffectBonus);
				}
					break;
				case SHEET_POW:
				{
					snprintf(buf, sizeof(buf), "%s", getHoverTextString("attributes_pwr_entry_attr_bonus").c_str());
					std::string tag = "MAGIC_SPELLPOWER_INT";
					std::string pwrINTBonus = "";
					if ( auto spell = player.magic.selectedSpell() )
					{
						pwrINTBonus = formatSkillSheetEffects(player.playernum, spell->skillID, tag, getHoverTextString("attributes_pwr_bonus_format"));
						if ( spell->skillID == PRO_MYSTICISM )
						{
							snprintf(buf, sizeof(buf), "%s", getHoverTextString("attributes_pwr_entry_attr_bonus_mysticism").c_str());
						}
						else if ( spell->skillID == PRO_THAUMATURGY )
						{
							snprintf(buf, sizeof(buf), "%s", getHoverTextString("attributes_pwr_entry_attr_bonus_thaumaturgy").c_str());
						}
					}
					else
					{
						pwrINTBonus = formatSkillSheetEffects(player.playernum, NUMPROFICIENCIES, tag, getHoverTextString("attributes_pwr_bonus_format"));
					}
					snprintf(valueBuf, sizeof(valueBuf), "%s", pwrINTBonus.c_str());
				}
					break;
				case SHEET_RES:
				{
					snprintf(buf, sizeof(buf), "%s", getHoverTextString("attributes_res_entry_int_bonus").c_str());

					//Sint32 oldStats[NUMSTATS];
					//characterSheetTooltipSetZeroStat(player.entity, stats[player.playernum], nullptr, oldStats);
					//real_t resistanceNoINT = 100.0 * Entity::getDamageTableMultiplier(player.entity, *stats[player.playernum], DAMAGE_TABLE_MAGIC);
					//characterSheetTooltipRestoreStats(stats[player.playernum], nullptr, oldStats);

					//real_t resistance = 100.0 * Entity::getDamageTableMultiplier(player.entity, *stats[player.playernum], DAMAGE_TABLE_MAGIC);

					real_t damageMultiplier = damagetables[stats[player.playernum]->type][DAMAGE_TABLE_MAGIC];

					real_t resistanceFromINT = std::max(0, std::min(90, statGetINT(stats[player.playernum], player.entity)));
					if ( damageMultiplier < 1.0 )
					{
						resistanceFromINT *= (1 - std::max(1.0 - damageMultiplier, 0.0));
					}
					
					snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("attributes_res_bonus_format").c_str(), (int)resistanceFromINT);
				}
					break;
				case SHEET_RGN:
				{
					Monster type = stats[player.playernum]->type;
					bool aestheticOnly = false;
					if ( player.entity )
					{
						if ( player.entity->effectPolymorph == NOTHING && stats[player.playernum]->playerRace > RACE_HUMAN )
						{
							if ( stats[player.playernum]->stat_appearance != 0 )
							{
								aestheticOnly = true;
								type = player.entity->getMonsterFromPlayerRace(stats[player.playernum]->playerRace);
							}
						}
					}
					/*/real_t baseRegen = 100.0;
					if ( !(svFlags & SV_FLAG_HUNGER) )
					{
						baseRegen = 0.0;
					}
					if ( type == SKELETON )
					{
						baseRegen = 25.0;
					}*/

					snprintf(buf, sizeof(buf), "%s", getHoverTextString("attributes_rgn_entry_items_bonus").c_str());

					real_t regenWithoutItems = getDisplayedHPRegen(player.entity, *stats[player.playernum], nullptr, nullptr, true);
					real_t regenTotal = getDisplayedHPRegen(player.entity, *stats[player.playernum], nullptr, nullptr);

					real_t regenItemsEffects = regenTotal - regenWithoutItems;
					snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("attributes_rgn_bonus_format").c_str(), regenItemsEffects);
				}
					break;
				case SHEET_RGN_MP:
				{
					if ( isAutomatonHTRegen )
					{
						snprintf(buf, sizeof(buf), "%s", getHoverTextString("attributes_rgn_entry_items_bonus").c_str());
						real_t baseHTModifier = 100.0 / (MAGIC_REGEN_AUTOMATON_TIME / (real_t)MAGIC_REGEN_TIME);
						if ( stats[player.playernum]->HUNGER <= 300 )
						{
							int baseTime = getBaseManaRegen(player.entity, *stats[player.playernum]);
							real_t scaledInterval = ((60 * baseTime) / (std::max(stats[player.playernum]->MAXMP, 1)));
							baseHTModifier = scaledInterval / TICKS_PER_SECOND;
							baseHTModifier /= -6.0; // degrade faster
							real_t nominalRegen = MAGIC_REGEN_TIME / TICKS_PER_SECOND;
							baseHTModifier = (nominalRegen / baseHTModifier) * 100.0;
						}
						else if ( stats[player.playernum]->HUNGER > 1200 )
						{
							if ( stats[player.playernum]->MP / static_cast<real_t>(std::max(1, stats[player.playernum]->MAXMP)) <= 0.5 )
							{
								baseHTModifier *= 4; // increase faster at < 50% mana
							}
							else
							{
								baseHTModifier *= 2; // increase less faster at > 50% mana
							}
						}
						else
						{
							// normal manaRegenInterval 300-1200 hunger.
						}
						real_t regenTotal = getDisplayedMPRegen(player.entity, *stats[player.playernum], nullptr, nullptr);
						real_t displayTotal = (regenTotal - baseHTModifier);
						//displayTotal /= 100.0;
						snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("attributes_rgn_ht_bonus_format").c_str(), displayTotal);
					}
					else if ( isInsectoidENRegen )
					{
						snprintf(buf, sizeof(buf), "%s", getHoverTextString("attributes_rgn_en_entry_items_bonus").c_str());

						real_t normalRegenTime = (1000.f * 30 * 1.5) / static_cast<float>(TICKS_PER_SECOND); // 30 base, insectoid does 1.5x in getHungerTickRate()
						normalRegenTime /= (std::max(stats[player.playernum]->MAXMP, 1)); // time for 1 mana in seconds
						normalRegenTime *= TICKS_PER_SECOND; // game ticks for 1 mana

						real_t modifiedRegenTime = (1000.f * (Entity::getHungerTickRate(stats[player.playernum], true, false)) / static_cast<float>(TICKS_PER_SECOND));
						modifiedRegenTime /= (std::max(stats[player.playernum]->MAXMP, 1)); // time for 1 mana in seconds
						modifiedRegenTime *= TICKS_PER_SECOND; // game ticks for 1 mana

						real_t baseValue = 100.0 * (normalRegenTime / modifiedRegenTime);
						real_t displayedValue = getDisplayedMPRegen(player.entity, *stats[player.playernum], nullptr, nullptr);

						snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("attributes_rgn_en_bonus_format").c_str(), displayedValue - baseValue);
					}
					else
					{
						Monster type = stats[player.playernum]->type;
						bool aestheticOnly = false;
						if ( player.entity )
						{
							if ( player.entity->effectPolymorph == NOTHING && stats[player.playernum]->playerRace > RACE_HUMAN )
							{
								if ( stats[player.playernum]->stat_appearance != 0 )
								{
									aestheticOnly = true;
									type = player.entity->getMonsterFromPlayerRace(stats[player.playernum]->playerRace);
								}
							}
						}

						snprintf(buf, sizeof(buf), "%s", getHoverTextString("attributes_rgn_entry_items_bonus").c_str());

						real_t regenWithoutItems = getDisplayedMPRegen(player.entity, *stats[player.playernum], nullptr, nullptr, true);
						real_t regenTotal = getDisplayedMPRegen(player.entity, *stats[player.playernum], nullptr, nullptr);

						real_t regenItemsEffects = regenTotal - regenWithoutItems;
						snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("attributes_rgn_bonus_format").c_str(), regenItemsEffects);
					}
				}
					break;
				case SHEET_WGT:
				{
					int weight = player.movement.getCharacterModifiedWeight();
					int equippedWeightTotal = player.movement.getCharacterEquippedWeight();
					int equippedWeight = player.movement.getCharacterModifiedWeight(&equippedWeightTotal);
					Sint32 STR = statGetSTR(stats[player.playernum], player.entity);
					Sint32 DEX = statGetDEX(stats[player.playernum], player.entity);
					real_t currentEquippedSpeed = player.movement.getSpeedFactor(player.movement.getWeightRatio(equippedWeight, STR), DEX);
					//real_t currentSpeed = player.movement.getSpeedFactor(player.movement.getWeightRatio(weight, STR), DEX);
					real_t noWeightSpeed = player.movement.getSpeedFactor(player.movement.getWeightRatio(0, STR), DEX);
					real_t maxSpeed = player.movement.getMaximumSpeed();

					//real_t currentSpeedPercent = 100.0 * currentSpeed / std::fmax(.01, maxSpeed);
					real_t currentEquippedSpeedPercent = 100.0 * currentEquippedSpeed / std::fmax(.01, maxSpeed);
					real_t noWeightSpeedPercent = 100.0 * noWeightSpeed / std::fmax(.01, maxSpeed);

					real_t displayValue = (currentEquippedSpeedPercent - noWeightSpeedPercent);
					if ( displayValue >= 0.0 )
					{
						displayValue = -.000001; // so there is a negative sign
					}

					snprintf(buf, sizeof(buf), "%s", getHoverTextString("attributes_wgt_equipment_value").c_str());
					snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("attributes_wgt_speed_bonus_format").c_str(),
						displayValue);
				}
					break;
				default:
					break;
			}
			entry->setText(buf);
			entry->setVJustify(Field::justify_t::TOP);

			SDL_Rect entryPos = entry->getSize();
			entryPos.x = padx + padxMid;
			entryPos.y = currentHeight;
			entryPos.w = txtPos.w - (padxMid * 2);
			entry->setSize(entryPos);
			if ( charsheetTooltipCache[player.playernum].textEntries[element].entry4 != entry->getText() )
			{
				entry->reflowTextToFit(0);
				charsheetTooltipCache[player.playernum].textEntries[element].entry4 = entry->getText();
			}
			entryPos.h = actualFont->height(true) * entry->getNumTextLines() + extraTextHeightForLowerCharacters;
			entry->setSize(entryPos);
			entry->setColor(defaultColor);
			currentHeight = std::max(entryPos.y + entryPos.h - extraTextHeightForLowerCharacters, 0);
			tooltipPos.h = pady1 + currentHeight + pady2;

			auto entryValue = characterSheetTooltipTextFields[player.playernum][currentTextFieldIndex]; assert(entry);
			++currentTextFieldIndex;
			entryValue->setDisabled(false);
			switch ( element )
			{
				case SHEET_ATK:
					break;
				case SHEET_AC:
					break;
				case SHEET_POW:
					break;
				case SHEET_RES:
					break;
				case SHEET_RGN:
					break;
				case SHEET_WGT:
					break;
				default:
					break;
			}
			entryValue->setColor(hudColors.characterSheetNeutral);
			if ( value < 0 )
			{
				entryValue->setColor(hudColors.characterSheetRed);
			}
			else if ( value > 0 )
			{
				entryValue->setColor(hudColors.characterSheetGreen);
			}
			entryValue->setText(valueBuf);
			entryValue->setSize(entry->getSize());
			entryValue->setHJustify(Frame::justify_t::LEFT);
			entryValue->setVJustify(Field::justify_t::TOP);

			auto txtValueBackingFrame = characterSheetTooltipTextBackingFrames[player.playernum][currentTextBackingFrameIndex];
			SDL_Rect backingFramePos = entryValue->getSize();
			auto txtValueGet = Text::get(entryValue->getText(), entryValue->getFont(),
				entryValue->getTextColor(), entryValue->getOutlineColor());
			longestValue = std::max(longestValue, txtValueGet->getWidth());
			backingFramePos.x = backingFramePos.x + backingFramePos.w;
			backingFramePos.h = actualFont->height(true) + extraTextHeightForLowerCharacters - 2;
			valueSizes[4] = std::make_pair(entryValue, backingFramePos);
			txtValueBackingFrame->setDisabled(false);
			++currentTextBackingFrameIndex;
		}

		if ( element == SHEET_ATK && getAttackTooltipLines(player.playernum, attackHoverTextInfo, 5, buf, valueBuf)
			|| (element != SHEET_ATK 
				/*&& element != SHEET_RES */
				&& !(element == SHEET_RGN && false/*&& !(svFlags & SV_FLAG_HUNGER) && stats[player.playernum]->type != SKELETON*/)
				&& !(element == SHEET_RGN_MP && isInsectoidENRegen && !(svFlags & SV_FLAG_HUNGER))
				) 
			)
		{
			// extra number display - line 5
			hasEntryInfoLines = true;
			currentHeight += padyMid;
			auto entry = characterSheetTooltipTextFields[player.playernum][currentTextFieldIndex]; assert(entry);
			++currentTextFieldIndex;
			entry->setDisabled(false);
			int value = 0;
			switch ( element )
			{
				case SHEET_ATK:
					break;
				case SHEET_AC:
				{
					snprintf(buf, sizeof(buf), "%s", getHoverTextString("attributes_ac_passive_bonus").c_str());

					Sint32 oldSkillLVL = stats[player.playernum]->getProficiency(PRO_SHIELD);
					bool oldDefending = stats[player.playernum]->defending;
					stats[player.playernum]->defending = false;

					Sint32 armor = AC(stats[player.playernum]);
					stats[player.playernum]->setProficiencyUnsafe(PRO_SHIELD, -999);
					Sint32 armorNoSkill = AC(stats[player.playernum]);
					stats[player.playernum]->setProficiency(PRO_SHIELD, oldSkillLVL);
					stats[player.playernum]->defending = oldDefending;

					Sint32 passiveBonus = armor - armorNoSkill;
					snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("attributes_ac_bonus_format").c_str(), passiveBonus);
				}
					break;
				case SHEET_POW:
				{
					snprintf(buf, sizeof(buf), "%s", getHoverTextString("attributes_pwr_entry_items_bonus").c_str());
					std::string tag = "MAGIC_SPELLPOWER_EQUIPMENT";
					std::string pwrINTBonus = "";
					if ( auto spell = player.magic.selectedSpell() )
					{
						pwrINTBonus = formatSkillSheetEffects(player.playernum, spell->skillID, tag, getHoverTextString("attributes_pwr_bonus_format"));
					}
					else
					{
						pwrINTBonus = formatSkillSheetEffects(player.playernum, NUMPROFICIENCIES, tag, getHoverTextString("attributes_pwr_bonus_format"));
					}
					snprintf(valueBuf, sizeof(valueBuf), "%s", pwrINTBonus.c_str());
				}
					break;
				case SHEET_RES:
				{
					snprintf(buf, sizeof(buf), "%s", getHoverTextString("attributes_res_entry_items_bonus").c_str());
					Sint32 baseResist = 100 * damagetables[stats[player.playernum]->type][DAMAGE_TABLE_MAGIC];
					baseResist = 100 - baseResist;
					
					//Sint32 oldStats[NUMSTATS];
					//characterSheetTooltipSetZeroStat(player.entity, stats[player.playernum], nullptr, oldStats);
					//real_t resistanceNoINT = 100.0 * Entity::getDamageTableMultiplier(player.entity, *stats[player.playernum], DAMAGE_TABLE_MAGIC);
					//characterSheetTooltipRestoreStats(stats[player.playernum], nullptr, oldStats);
					//
					//real_t resistanceFromINT = (100.0 - resistance) - (100.0 - resistanceNoINT);

					real_t resistance = 100.0 * Entity::getDamageTableMultiplier(player.entity, *stats[player.playernum], DAMAGE_TABLE_MAGIC);
					real_t damageMultiplier = damagetables[stats[player.playernum]->type][DAMAGE_TABLE_MAGIC];

					real_t resistanceFromINT = std::max(0, std::min(90, statGetINT(stats[player.playernum], player.entity)));
					if ( damageMultiplier < 1.0 )
					{
						resistanceFromINT *= (1 - std::max(1.0 - damageMultiplier, 0.0));
					}

					resistance = (100.0 - resistance) - resistanceFromINT - baseResist;

					snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("attributes_res_bonus_format").c_str(), (int)resistance);
					break;
				}
				case SHEET_RGN:
				{
					Sint32 oldStats[NUMSTATS];
					snprintf(buf, sizeof(buf), "%s", getHoverTextString("attributes_rgn_hp_entry_statskill_bonus").c_str());

					characterSheetTooltipSetZeroStat(player.entity, stats[player.playernum], nullptr, oldStats);
					real_t regenWithoutStats = getDisplayedHPRegen(player.entity, *stats[player.playernum], nullptr, nullptr, true);
					characterSheetTooltipRestoreStats(stats[player.playernum], nullptr, oldStats);

					real_t regenTotal = getDisplayedHPRegen(player.entity, *stats[player.playernum], nullptr, nullptr, true);
					real_t regenStatSkill = regenTotal - regenWithoutStats;

					snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("attributes_rgn_bonus_format").c_str(), regenStatSkill);
				}
					break;
				case SHEET_RGN_MP:
				{
					if ( isAutomatonHTRegen )
					{
						snprintf(buf, sizeof(buf), "%s", getHoverTextString("attributes_rgn_ht_boiler_status").c_str());
						if ( stats[player.playernum]->HUNGER <= 300 )
						{
							value = -1;
							snprintf(valueBuf, sizeof(valueBuf), "%s", getHoverTextString("attributes_rgn_ht_boiler_value_low").c_str());
						}
						else if ( stats[player.playernum]->HUNGER > 1200 )
						{
							value = 2;
							snprintf(valueBuf, sizeof(valueBuf), "%s", getHoverTextString("attributes_rgn_ht_boiler_value_superheat").c_str());
						}
						else
						{
							value = 1;
							snprintf(valueBuf, sizeof(valueBuf), "%s", getHoverTextString("attributes_rgn_ht_boiler_value_normal").c_str());
						}
					}
					else if ( isInsectoidENRegen )
					{
						snprintf(buf, sizeof(buf), "%s", getHoverTextString("attributes_rgn_plain_display").c_str());
						real_t regen = (static_cast<real_t>(Entity::getManaRegenInterval(player.entity, *stats[player.playernum], true)) / TICKS_PER_SECOND);
						snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("attributes_rgn_en_per_second_format").c_str(), regen);
					}
					else
					{
						Monster type = stats[player.playernum]->type;
						bool aestheticOnly = false;
						if ( player.entity )
						{
							if ( player.entity->effectPolymorph == NOTHING && stats[player.playernum]->playerRace > RACE_HUMAN )
							{
								if ( stats[player.playernum]->stat_appearance != 0 )
								{
									aestheticOnly = true;
									type = player.entity->getMonsterFromPlayerRace(stats[player.playernum]->playerRace);
								}
							}
						}
						/*real_t baseRegen = 100.0;
						if ( type == SKELETON )
						{
							baseRegen = 25.0;
						}*/
						//real_t baseRegen = 100.0;
						//Sint32 oldLVL = 0;
						//characterSheetTooltipSetZeroStat(player.entity, stats[player.playernum], nullptr, oldStats);
						//baseRegen = getDisplayedMPRegen(player.entity, *stats[player.playernum], nullptr, nullptr);
						//characterSheetTooltipRestoreStats(stats[player.playernum], nullptr, oldStats);

						Sint32 oldStats[NUMSTATS];
						snprintf(buf, sizeof(buf), "%s", getHoverTextString("attributes_rgn_entry_statskill_bonus").c_str());

						characterSheetTooltipSetZeroStat(player.entity, stats[player.playernum], nullptr, oldStats);
						real_t regenWithoutStats = getDisplayedMPRegen(player.entity, *stats[player.playernum], nullptr, nullptr, true);
						characterSheetTooltipRestoreStats(stats[player.playernum], nullptr, oldStats);

						real_t regenTotal = getDisplayedMPRegen(player.entity, *stats[player.playernum], nullptr, nullptr, true);
						real_t regenStatSkill = regenTotal - regenWithoutStats;

						snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("attributes_rgn_bonus_format").c_str(), regenStatSkill);
					}
				}
					break;
				case SHEET_WGT:
				{
					int weight = player.movement.getCharacterModifiedWeight();
					int equippedWeightTotal = player.movement.getCharacterEquippedWeight();
					int equippedWeight = player.movement.getCharacterModifiedWeight(&equippedWeightTotal);
					int goldWeightTotal = stats[player.playernum]->getGoldWeight();
					int goldWeight = player.movement.getCharacterModifiedWeight(&goldWeightTotal);
					Sint32 STR = statGetSTR(stats[player.playernum], player.entity);
					Sint32 DEX = statGetDEX(stats[player.playernum], player.entity);
					real_t currentEquippedSpeed = player.movement.getSpeedFactor(player.movement.getWeightRatio(equippedWeight, STR), DEX);
					real_t currentSpeed = player.movement.getSpeedFactor(player.movement.getWeightRatio(weight - goldWeight, STR), DEX); // ignore gold
					real_t noWeightSpeed = player.movement.getSpeedFactor(player.movement.getWeightRatio(0, STR), DEX);
					real_t maxSpeed = player.movement.getMaximumSpeed();

					real_t currentSpeedPercent = 100.0 * currentSpeed / std::fmax(.01, maxSpeed);
					real_t currentEquippedSpeedPercent = 100.0 * currentEquippedSpeed / std::fmax(.01, maxSpeed);
					real_t noWeightSpeedPercent = 100.0 * noWeightSpeed / std::fmax(.01, maxSpeed);

					real_t displayValue = (currentSpeedPercent - noWeightSpeedPercent)
						- (currentEquippedSpeedPercent - noWeightSpeedPercent);
					if ( displayValue >= 0.0 )
					{
						displayValue = -.000001; // so there is a negative sign
					}

					snprintf(buf, sizeof(buf), "%s", getHoverTextString("attributes_wgt_inventory_value").c_str());
					snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("attributes_wgt_speed_bonus_format").c_str(),
						displayValue);
				}
					break;
				default:
					break;
			}
			entry->setText(buf);
			entry->setVJustify(Field::justify_t::TOP);

			SDL_Rect entryPos = entry->getSize();
			entryPos.x = padx + padxMid;
			entryPos.y = currentHeight;
			entryPos.w = txtPos.w - (padxMid * 2);
			entry->setSize(entryPos);
			if ( charsheetTooltipCache[player.playernum].textEntries[element].entry5 != entry->getText() )
			{
				entry->reflowTextToFit(0);
				charsheetTooltipCache[player.playernum].textEntries[element].entry5 = entry->getText();
			}
			entryPos.h = actualFont->height(true) * entry->getNumTextLines() + extraTextHeightForLowerCharacters;
			entry->setSize(entryPos);
			entry->setColor(defaultColor);
			currentHeight = std::max(entryPos.y + entryPos.h - extraTextHeightForLowerCharacters, 0);
			tooltipPos.h = pady1 + currentHeight + pady2;

			auto entryValue = characterSheetTooltipTextFields[player.playernum][currentTextFieldIndex]; assert(entry);
			++currentTextFieldIndex;
			entryValue->setDisabled(false);
			switch ( element )
			{
				case SHEET_ATK:
					break;
				case SHEET_AC:
					break;
				case SHEET_POW:
					break;
				case SHEET_RES:
					break;
				case SHEET_RGN:
					break;
				case SHEET_WGT:
					break;
				default:
					break;
			}
			entryValue->setColor(hudColors.characterSheetNeutral);
			if ( value < 0 )
			{
				entryValue->setColor(hudColors.characterSheetRed);
			}
			else if ( value > 0 )
			{
				if ( isAutomatonHTRegen )
				{
					if ( value == 2 ) // supercharge
					{
						entryValue->setColor(hudColors.characterSheetHeadingText);
					}
					else
					{
						entryValue->setColor(hudColors.characterSheetGreen);
					}
				}
				else
				{
					entryValue->setColor(hudColors.characterSheetGreen);
				}
			}
			entryValue->setText(valueBuf);
			entryValue->setSize(entry->getSize());
			entryValue->setHJustify(Frame::justify_t::LEFT);
			entryValue->setVJustify(Field::justify_t::TOP);

			if ( /*element == SHEET_RGN ||*/ (element == SHEET_RGN_MP && (isAutomatonHTRegen || isInsectoidENRegen)) )
			{
				// special rule here to ignore size of this long line
				auto txtValueBackingFrame = characterSheetTooltipTextBackingFrames[player.playernum][currentTextBackingFrameIndex];
				SDL_Rect backingFramePos = entryValue->getSize();
				auto txtValueGet = Text::get(entryValue->getText(), entryValue->getFont(),
					entryValue->getTextColor(), entryValue->getOutlineColor());
				//longestValue = std::max(longestValue, txtValueGet->getWidth());
				backingFramePos.x = backingFramePos.x + backingFramePos.w;
				backingFramePos.h = actualFont->height(true) + extraTextHeightForLowerCharacters - 2;
				//valueSizes[currentTextBackingFrameIndex] = std::make_pair(entryValue, backingFramePos);
				txtValueBackingFrame->setDisabled(true);

				backingFramePos.w = (int)txtValueGet->getWidth();
				backingFramePos.x -= backingFramePos.w;
				backingFramePos.x -= 8;
				entryValue->setSize(backingFramePos);
				++currentTextBackingFrameIndex;
			}
			else
			{
				auto txtValueBackingFrame = characterSheetTooltipTextBackingFrames[player.playernum][currentTextBackingFrameIndex];
				SDL_Rect backingFramePos = entryValue->getSize();
				auto txtValueGet = Text::get(entryValue->getText(), entryValue->getFont(),
					entryValue->getTextColor(), entryValue->getOutlineColor());
				longestValue = std::max(longestValue, txtValueGet->getWidth());
				backingFramePos.x = backingFramePos.x + backingFramePos.w;
				backingFramePos.h = actualFont->height(true) + extraTextHeightForLowerCharacters - 2;
				valueSizes[currentTextBackingFrameIndex] = std::make_pair(entryValue, backingFramePos);
				txtValueBackingFrame->setDisabled(false);
				++currentTextBackingFrameIndex;
			}
		}

		if ( element == SHEET_ATK && getAttackTooltipLines(player.playernum, attackHoverTextInfo, 6, buf, valueBuf)
			|| (element == SHEET_RGN_MP && !isInsectoidENRegen) || element == SHEET_WGT || element == SHEET_RGN )
		{
			// extra number display - line 6
			hasEntryInfoLines = true;
			currentHeight += padyMid;
			auto entry = characterSheetTooltipTextFields[player.playernum][currentTextFieldIndex]; assert(entry);
			++currentTextFieldIndex;
			entry->setDisabled(false);
			switch ( element )
			{
				case SHEET_ATK:
					break;
				case SHEET_AC:
					break;
				case SHEET_POW:
					break;
				case SHEET_RES:
					break;
				case SHEET_RGN:
				{
					snprintf(buf, sizeof(buf), "%s", getHoverTextString("attributes_rgn_plain_display").c_str());
					real_t regen = (static_cast<real_t>(Entity::getHealthRegenInterval(player.entity, *stats[player.playernum], true)) / TICKS_PER_SECOND);
					if ( regen <= 0.0 )
					{
						snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("attributes_rgn_hp_per_second_format_zero").c_str(),
							(static_cast<real_t>(HEAL_TIME) / TICKS_PER_SECOND));
					}
					else
					{
						snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("attributes_rgn_hp_per_second_format").c_str(),
							regen);
					}
				}
					break;
				case SHEET_RGN_MP:
				{
					if ( isAutomatonHTRegen )
					{
						snprintf(buf, sizeof(buf), "%s", getHoverTextString("attributes_rgn_plain_display").c_str());
						real_t regen = (static_cast<real_t>(Entity::getManaRegenInterval(player.entity, *stats[player.playernum], true)) / TICKS_PER_SECOND);
						if ( stats[player.playernum]->HUNGER <= 300 )
						{
							regen /= 6; // degrade faster
							snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("attributes_rgn_ht_per_second_format").c_str(), -1, regen);
						}
						else if ( stats[player.playernum]->HUNGER > 1200 )
						{
							if ( stats[player.playernum]->MP / static_cast<real_t>(std::max(1, stats[player.playernum]->MAXMP)) <= 0.5 )
							{
								regen /= 4; // increase faster at < 50% mana
							}
							else
							{
								regen /= 2; // increase less faster at > 50% mana
							}
							snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("attributes_rgn_ht_per_second_format").c_str(), 1, regen);
						}
						else if ( stats[player.playernum]->HUNGER > 300 )
						{
							// normal manaRegenInterval 300-1200 hunger.
							snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("attributes_rgn_ht_per_second_format").c_str(), 1, regen);
						}
					}
					else
					{
						snprintf(buf, sizeof(buf), "%s", getHoverTextString("attributes_rgn_plain_display").c_str());
						real_t regen = (static_cast<real_t>(Entity::getManaRegenInterval(player.entity, *stats[player.playernum], true)) / TICKS_PER_SECOND);
						snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("attributes_rgn_mp_per_second_format").c_str(), regen);
					}
				}
					break;
				case SHEET_WGT:
				{
					int weight = player.movement.getCharacterModifiedWeight();
					int equippedWeightTotal = player.movement.getCharacterEquippedWeight();
					int equippedWeight = player.movement.getCharacterModifiedWeight(&equippedWeightTotal);
					int goldWeightTotal = stats[player.playernum]->getGoldWeight();
					int goldWeight = player.movement.getCharacterModifiedWeight(&goldWeightTotal);
					Sint32 STR = statGetSTR(stats[player.playernum], player.entity);
					Sint32 DEX = statGetDEX(stats[player.playernum], player.entity);
					//real_t currentEquippedSpeed = player.movement.getSpeedFactor(player.movement.getWeightRatio(equippedWeight, STR), DEX);
					real_t currentSpeed = player.movement.getSpeedFactor(player.movement.getWeightRatio(weight, STR), DEX);
					real_t noWeightSpeed = player.movement.getSpeedFactor(player.movement.getWeightRatio(0, STR), DEX);
					//real_t goldSpeed = player.movement.getSpeedFactor(player.movement.getWeightRatio(goldWeight, STR), DEX);
					real_t noGoldSpeed = player.movement.getSpeedFactor(player.movement.getWeightRatio(weight - goldWeight, STR), DEX);
					real_t maxSpeed = player.movement.getMaximumSpeed();

					real_t currentSpeedPercent = 100.0 * currentSpeed / std::fmax(.01, maxSpeed);
					//real_t currentEquippedSpeedPercent = 100.0 * currentEquippedSpeed / std::fmax(.01, maxSpeed);
					real_t noWeightSpeedPercent = 100.0 * noWeightSpeed / std::fmax(.01, maxSpeed);
					//real_t goldSpeedPercent = 100.0 * goldSpeed / std::fmax(.01, maxSpeed);
					real_t noGoldSpeedPercent = 100.0 * noGoldSpeed / std::fmax(.01, maxSpeed);

					real_t displayValue = (currentSpeedPercent - noWeightSpeedPercent) 
						- (noGoldSpeedPercent - noWeightSpeedPercent);
					if ( displayValue >= 0.001 )
					{
						// do nothing
					}
					else if ( displayValue >= 0.0 )
					{
						displayValue = -.000001; // so there is a negative sign
					}

					snprintf(buf, sizeof(buf), "%s", getHoverTextString("attributes_wgt_gold_value").c_str());
					snprintf(valueBuf, sizeof(valueBuf), getHoverTextString("attributes_wgt_speed_bonus_format").c_str(),
						displayValue);
				}
					break;
				default:
					break;
			}
			entry->setText(buf);
			entry->setVJustify(Field::justify_t::TOP);

			SDL_Rect entryPos = entry->getSize();
			entryPos.x = padx + padxMid;
			entryPos.y = currentHeight;
			entryPos.w = txtPos.w - (padxMid * 2);
			entry->setSize(entryPos);
			if ( charsheetTooltipCache[player.playernum].textEntries[element].entry6 != entry->getText() )
			{
				entry->reflowTextToFit(0);
				charsheetTooltipCache[player.playernum].textEntries[element].entry6 = entry->getText();
			}
			entryPos.h = actualFont->height(true) * entry->getNumTextLines() + extraTextHeightForLowerCharacters;
			entry->setSize(entryPos);
			entry->setColor(defaultColor);
			currentHeight = std::max(entryPos.y + entryPos.h - extraTextHeightForLowerCharacters, 0);
			tooltipPos.h = pady1 + currentHeight + pady2;

			auto entryValue = characterSheetTooltipTextFields[player.playernum][currentTextFieldIndex]; assert(entry);
			++currentTextFieldIndex;
			entryValue->setDisabled(false);
			int value = 0;
			switch ( element )
			{
				case SHEET_ATK:
					break;
				case SHEET_AC:
					break;
				case SHEET_POW:
					break;
				case SHEET_RES:
					break;
				case SHEET_RGN:
					break;
				case SHEET_WGT:
					break;
				default:
					break;
			}
			entryValue->setColor(hudColors.characterSheetNeutral);
			if ( value < 0 )
			{
				entryValue->setColor(hudColors.characterSheetRed);
			}
			else if ( value > 0 )
			{
				entryValue->setColor(hudColors.characterSheetGreen);
			}
			entryValue->setText(valueBuf);
			entryValue->setSize(entry->getSize());
			entryValue->setHJustify(Frame::justify_t::LEFT);
			entryValue->setVJustify(Field::justify_t::TOP);

			if ( element == SHEET_RGN_MP || element == SHEET_RGN )
			{
				// special rule here to ignore size of this long line
				auto txtValueBackingFrame = characterSheetTooltipTextBackingFrames[player.playernum][currentTextBackingFrameIndex];
				SDL_Rect backingFramePos = entryValue->getSize();
				auto txtValueGet = Text::get(entryValue->getText(), entryValue->getFont(),
					entryValue->getTextColor(), entryValue->getOutlineColor());
				//longestValue = std::max(longestValue, txtValueGet->getWidth());
				backingFramePos.x = backingFramePos.x + backingFramePos.w;
				backingFramePos.h = actualFont->height(true) + extraTextHeightForLowerCharacters - 2;
				//valueSizes[currentTextBackingFrameIndex] = std::make_pair(entryValue, backingFramePos);
				txtValueBackingFrame->setDisabled(true);

				backingFramePos.w = (int)txtValueGet->getWidth();
				backingFramePos.x -= backingFramePos.w;
				backingFramePos.x -= 8;
				entryValue->setSize(backingFramePos);
				++currentTextBackingFrameIndex;
			}
			else
			{
				auto txtValueBackingFrame = characterSheetTooltipTextBackingFrames[player.playernum][currentTextBackingFrameIndex];
				SDL_Rect backingFramePos = entryValue->getSize();
				auto txtValueGet = Text::get(entryValue->getText(), entryValue->getFont(),
					entryValue->getTextColor(), entryValue->getOutlineColor());
				longestValue = std::max(longestValue, txtValueGet->getWidth());
				backingFramePos.x = backingFramePos.x + backingFramePos.w;
				backingFramePos.h = actualFont->height(true) + extraTextHeightForLowerCharacters - 2;
				valueSizes[currentTextBackingFrameIndex] = std::make_pair(entryValue, backingFramePos);
				txtValueBackingFrame->setDisabled(false);
				++currentTextBackingFrameIndex;
			}
		}

		if ( element == SHEET_ATK && getAttackTooltipLines(player.playernum, attackHoverTextInfo, 7, buf, valueBuf) )
		{
			// extra number display - line 7
			hasEntryInfoLines = true;
			currentHeight += padyMid;
			auto entry = characterSheetTooltipTextFields[player.playernum][currentTextFieldIndex]; assert(entry);
			++currentTextFieldIndex;
			entry->setDisabled(false);
			switch ( element )
			{
				case SHEET_ATK:
					break;
				case SHEET_AC:
					break;
				case SHEET_POW:
					break;
				case SHEET_RES:
					break;
				case SHEET_RGN:
					break;
				case SHEET_WGT:
					break;
				default:
					break;
			}
			entry->setText(buf);
			entry->setVJustify(Field::justify_t::TOP);

			SDL_Rect entryPos = entry->getSize();
			entryPos.x = padx + padxMid;
			entryPos.y = currentHeight;
			entryPos.w = txtPos.w - (padxMid * 2);
			entry->setSize(entryPos);
			if ( charsheetTooltipCache[player.playernum].textEntries[element].entry7 != entry->getText() )
			{
				entry->reflowTextToFit(0);
				charsheetTooltipCache[player.playernum].textEntries[element].entry7 = entry->getText();
			}
			entryPos.h = actualFont->height(true) * entry->getNumTextLines() + extraTextHeightForLowerCharacters;
			entry->setSize(entryPos);
			entry->setColor(defaultColor);
			currentHeight = std::max(entryPos.y + entryPos.h - extraTextHeightForLowerCharacters, 0);
			tooltipPos.h = pady1 + currentHeight + pady2;

			auto entryValue = characterSheetTooltipTextFields[player.playernum][currentTextFieldIndex]; assert(entry);
			++currentTextFieldIndex;
			entryValue->setDisabled(false);
			int value = 0;
			switch ( element )
			{
				case SHEET_ATK:
					break;
				case SHEET_AC:
					break;
				case SHEET_POW:
					break;
				case SHEET_RES:
					break;
				case SHEET_RGN:
					break;
				case SHEET_WGT:
					break;
				default:
					break;
			}
			entryValue->setColor(hudColors.characterSheetNeutral);
			if ( value < 0 )
			{
				entryValue->setColor(hudColors.characterSheetRed);
			}
			else if ( value > 0 )
			{
				entryValue->setColor(hudColors.characterSheetGreen);
			}
			entryValue->setText(valueBuf);
			entryValue->setSize(entry->getSize());
			entryValue->setHJustify(Frame::justify_t::LEFT);
			entryValue->setVJustify(Field::justify_t::TOP);

			auto txtValueBackingFrame = characterSheetTooltipTextBackingFrames[player.playernum][currentTextBackingFrameIndex];
			SDL_Rect backingFramePos = entryValue->getSize();
			auto txtValueGet = Text::get(entryValue->getText(), entryValue->getFont(),
				entryValue->getTextColor(), entryValue->getOutlineColor());
			longestValue = std::max(longestValue, txtValueGet->getWidth());
			backingFramePos.x = backingFramePos.x + backingFramePos.w;
			backingFramePos.h = actualFont->height(true) + extraTextHeightForLowerCharacters - 2;
			valueSizes[currentTextBackingFrameIndex] = std::make_pair(entryValue, backingFramePos);
			txtValueBackingFrame->setDisabled(false);
			++currentTextBackingFrameIndex;
		}

		for ( int index = 1; index <= NUM_CHARSHEET_TOOLTIP_BACKING_FRAMES; ++index )
		{
			auto txtValueBackingFrame = characterSheetTooltipTextBackingFrames[player.playernum][index];
			if ( txtValueBackingFrame->isDisabled() )
			{
				continue;
			}

			if ( valueSizes.find(index) == valueSizes.end() )
			{
				continue;
			}

			SDL_Rect valuePos = valueSizes[index].second;
			Field* entryValue = valueSizes[index].first;
			SDL_Rect entryValuePos = entryValue->getSize();
			entryValuePos.x = entryValuePos.x + entryValuePos.w;
			entryValuePos.w = (int)longestValue;
			entryValuePos.x -= entryValuePos.w;
			entryValuePos.x -= 8;
			entryValue->setSize(entryValuePos);

			valuePos.w = (int)longestValue + 16;
			valuePos.x -= (valuePos.w);
			valuePos.y -= 3;
			valuePos.h += 4;

			txtValueBackingFrame->setSize(valuePos);

			Player::GUI_t::imageResizeToContainer9x9(txtValueBackingFrame, SDL_Rect{ 0, 0, valuePos.w, valuePos.h }, Player::GUI_t::tooltipEffectBackgroundImages);
		}

		if ( !hasEntryInfoLines )
		{
			currentHeight += padyMid * 2;
		}

		{
			currentHeight += padyMid;

			div->pos.x = padx;
			div->pos.y = currentHeight;
			div->pos.w = txtPos.w;
			div->disabled = false;

			currentHeight += padyMid;

			auto entry = characterSheetTooltipTextFields[player.playernum][15]; assert(entry);
			entry->setDisabled(false);
			char buf[512] = "";

			std::string descTextFormatted = "\x1E ";
			for ( auto s : descText )
			{
				descTextFormatted += s;
				if ( s == '\n' )
				{
					descTextFormatted += "\x1E ";
				}
			}

			snprintf(buf, sizeof(buf), "%s", descTextFormatted.c_str());
			entry->setText(buf);

			SDL_Rect entryPos = entry->getSize();
			entryPos.x = padx;
			entryPos.y = currentHeight;
			entryPos.w = txtPos.w;
			entry->setSize(entryPos);
			if ( charsheetTooltipCache[player.playernum].textEntries[element].entry8 != entry->getText() )
			{
				entry->reflowTextToFit(0);
				charsheetTooltipCache[player.playernum].textEntries[element].entry8 = entry->getText();
			}
			entryPos.h = actualFont->height(true) * entry->getNumTextLines() + extraTextHeightForLowerCharacters;
			entry->setSize(entryPos);
			if ( element == SHEET_RGN && false/*&& !(svFlags & SV_FLAG_HUNGER) && stats[player.playernum]->type != SKELETON*/ )
			{
				entry->setColor(hudColors.itemContextMenuHeadingText);
			}
			else if ( element == SHEET_RGN_MP && isInsectoidENRegen && !(svFlags & SV_FLAG_HUNGER) )
			{
				entry->setColor(hudColors.itemContextMenuHeadingText);
			}
			else
			{
				entry->setColor(hudColors.characterSheetOffWhiteText);
			}
			currentHeight = std::max(entryPos.y + entryPos.h - extraTextHeightForLowerCharacters, 0);

			currentHeight += padyMid / 4;
			tooltipPos.h = pady1 + currentHeight + pady2;
		}

		tooltipPos.h = pady1 + currentHeight + pady2;
		if ( tooltipJustify == PANEL_JUSTIFY_RIGHT )
		{
			tooltipPos.x = pos.x - tooltipPos.w;
		}
		else
		{
			tooltipPos.x = pos.x;
		}
		tooltipPos.y = pos.y;
		tooltipFrame->setSize(tooltipPos);
		if ( tooltipPos.y + tooltipPos.h > sheetFrame->getSize().h )
		{
			// keep on-screen
			tooltipPos.y -= ((tooltipPos.y + tooltipPos.h) - sheetFrame->getSize().h);
			tooltipFrame->setSize(tooltipPos);
		}
		Player::GUI_t::imageResizeToContainer9x9(tooltipFrame, SDL_Rect{ 0, 0, tooltipPos.w, tooltipPos.h },
			Player::GUI_t::tooltipEffectBackgroundImages);
	}
	else if ( element == Player::CharacterSheet_t::SHEET_DUNGEON_FLOOR )
	{
		const int maxWidth = 260;
		const int padx = 16;
		const int pady1 = 8;
		const int pady2 = 4;
		const int padxMid = 4;
		const int padyMid = 4;
		SDL_Rect tooltipPos = SDL_Rect{ 400, 0, maxWidth, 100 };
		
		std::string descriptionText = mapDisplayNamesDescriptions[map.name].second.c_str();
		std::string mapDetailsText = "";
		auto& mapDetails = Player::Minimap_t::mapDetails;
		for ( auto& detail : mapDetails )
		{
			if ( mapDetailsText != "" )
			{
				mapDetailsText += '\n';
			}
			mapDetailsText += "\x1E ";
			mapDetailsText += detail.second;
		}

		if ( strcmp(descriptionText.c_str(), txt->getText()) )
		{
			txt->setText(descriptionText.c_str());
		}
		SDL_Rect txtPos = SDL_Rect{ padx, pady1, maxWidth - padx * 2, 80 };
		txt->setSize(txtPos);
		if ( charsheetTooltipCache[player.playernum].textEntries[element].title != txt->getText() )
		{
			txt->reflowTextToFit(0);
			charsheetTooltipCache[player.playernum].textEntries[element].title = txt->getText();
		}
		Font* actualFont = Font::get(txt->getFont());
		int txtHeight = txt->getNumTextLines() * actualFont->height(true);
		txtPos.h = txtHeight + 4;
		auto txtGet = Text::get(txt->getLongestLine().c_str(), txt->getFont(),
			txt->getTextColor(), txt->getOutlineColor());
		txtPos.w = txtGet->getWidth();
		txt->setSize(txtPos);

		tooltipPos.w = txtGet->getWidth() + padx * 2;

		int currentHeight = txtPos.y + txtPos.h - 4;

		if ( Player::Minimap_t::mapDetails.size() > 0 )
		{
			currentHeight += padyMid;
			auto entry = tooltipFrame->findField("txt 1"); assert(entry);
			entry->setDisabled(false);
			if ( strcmp(mapDetailsText.c_str(), entry->getText()) )
			{
				entry->setText(mapDetailsText.c_str());
			}

			SDL_Rect entryPos = entry->getSize();
			entryPos.x = padx + padxMid;
			entryPos.y = currentHeight;
			entryPos.w = tooltipPos.w - (2 * (entryPos.x));
			entry->setSize(entryPos);
			if ( charsheetTooltipCache[player.playernum].textEntries[element].entry1 != entry->getText() )
			{
				entry->reflowTextToFit(0);
				charsheetTooltipCache[player.playernum].textEntries[element].entry1 = entry->getText();
			}
			entryPos.h = actualFont->height(true) * entry->getNumTextLines() + 4;
			entry->setSize(entryPos);
			entry->setColor(makeColor(255, 0, 255, 255));
			currentHeight = std::max(entryPos.y + entryPos.h, 0);
		}

		if ( Player::Minimap_t::mapDetails.size() > 0 )
		{
			tooltipPos.h = pady1 + currentHeight + pady2;
		}
		else
		{
			tooltipPos.h = pady1 + txtPos.h + pady2;
		}
		if ( tooltipJustify == PANEL_JUSTIFY_RIGHT )
		{
			tooltipPos.x = pos.x - tooltipPos.w;
		}
		else
		{
			tooltipPos.x = pos.x;
		}
		tooltipPos.y = pos.y;

		tooltipFrame->setSize(tooltipPos);
		Player::GUI_t::imageResizeToContainer9x9(tooltipFrame, SDL_Rect{ 0, 0, tooltipPos.w, tooltipPos.h },
			Player::GUI_t::tooltipEffectBackgroundImages);
	}
	else if ( element == Player::CharacterSheet_t::SHEET_GOLD )
	{
		const int maxWidth = 200;
		const int padx = 16;
		const int pady1 = 8;
		const int pady2 = 8;
		const int padyMid = 4;
		const int padxMid = 4;
		SDL_Rect tooltipPos = SDL_Rect{ 400, 0, maxWidth, 100 };
		bool usingMouse = !inputs.getVirtualMouse(player.playernum)->lastMovementFromController;
		if ( strcmp(getHoverTextString("gold_mouse").c_str(), txt->getText()) )
		{
			txt->setText(getHoverTextString("gold_mouse").c_str());
		}

		SDL_Rect txtPos = SDL_Rect{ padx, pady1, maxWidth - padx * 2, 80 };
		txt->setSize(txtPos);
		if ( charsheetTooltipCache[player.playernum].textEntries[element].title != txt->getText() )
		{
			txt->reflowTextToFit(0);
			charsheetTooltipCache[player.playernum].textEntries[element].title = txt->getText();
		}
		Font* actualFont = Font::get(txt->getFont());
		int txtHeight = txt->getNumTextLines() * actualFont->height(true);
		txtPos.h = txtHeight + padyMid;
		auto txtGet = Text::get(txt->getLongestLine().c_str(), txt->getFont(),
			txt->getTextColor(), txt->getOutlineColor());
		txtPos.w = txtGet->getWidth();
		txt->setSize(txtPos);
		
		tooltipPos.w = txtPos.w + padx * 2;
		tooltipPos.h = pady1 + txtPos.h + pady2;

		if ( tooltipJustify == PANEL_JUSTIFY_RIGHT )
		{
			tooltipPos.x = pos.x - tooltipPos.w;
		}
		else
		{
			tooltipPos.x = pos.x;
		}
		tooltipPos.y = pos.y;

		tooltipFrame->setSize(tooltipPos);
		Player::GUI_t::imageResizeToContainer9x9(tooltipFrame, SDL_Rect{ 0, 0, tooltipPos.w, tooltipPos.h },
			Player::GUI_t::tooltipEffectBackgroundImages);
	}
	else if ( element == Player::CharacterSheet_t::SHEET_CHAR_RACE_SEX )
	{
		auto tooltipTopLeft = tooltipFrame->findImage(Player::GUI_t::tooltipEffectBackgroundImages[Player::GUI_t::TOP_LEFT].c_str());
		tooltipTopLeft->path = "*#images/ui/CharSheet/HUD_CharSheet_Tooltip_TL_Blue_00.png";
		auto tooltipTop = tooltipFrame->findImage(Player::GUI_t::tooltipEffectBackgroundImages[Player::GUI_t::TOP].c_str());
		tooltipTop->path = "*#images/ui/CharSheet/HUD_CharSheet_Tooltip_T_Blue_00.png";
		auto tooltipTopRight = tooltipFrame->findImage(Player::GUI_t::tooltipEffectBackgroundImages[Player::GUI_t::TOP_RIGHT].c_str());
		tooltipTopRight->path = "*#images/ui/CharSheet/HUD_CharSheet_Tooltip_TR_Blue_00.png";
		Player::GUI_t::imageSetWidthHeight9x9(tooltipFrame, Player::GUI_t::tooltipEffectBackgroundImages);

		int maxWidth = 260;
		int minWidth = 0;

		const int padx = 16;
		const int pady1 = 8;
		const int pady2 = 4;
		const int padxMid = 4;
		const int padyMid = 8;
		SDL_Rect tooltipPos = SDL_Rect{ 400, 0, maxWidth, 100 };

		Monster race = HUMAN;
		if ( stats[player.playernum]->stat_appearance == 0 && stats[player.playernum]->playerRace != RACE_HUMAN )
		{
			race = getMonsterFromPlayerRace(stats[player.playernum]->playerRace);
		}
		Monster modifiedRace = stats[player.playernum]->type;
		if ( arachnophobia_filter )
		{
			if ( modifiedRace == SPIDER )
			{
				modifiedRace = CRAB;
			}
			if ( race == SPIDER )
			{
				race = CRAB;
			}
		}

		std::string titleText = getHoverTextString("race_title_normal");
		if ( players[player.playernum]->entity )
		{
			if ( players[player.playernum]->entity->effectShapeshift != NOTHING )
			{
				titleText = getHoverTextString("race_title_shapeshift");
			}
			else if ( race != modifiedRace )
			{
				titleText = getHoverTextString("race_title_polymorph");
			}
		}

		txt->setText(titleText.c_str());
		SDL_Rect txtPos = SDL_Rect{ padx, pady1 - 2, maxWidth - padx * 2, 80 };
		txt->setSize(txtPos);
		if ( charsheetTooltipCache[player.playernum].textEntries[element].title != txt->getText() )
		{
			txt->reflowTextToFit(0);
			charsheetTooltipCache[player.playernum].textEntries[element].title = txt->getText();
		}
		txt->setColor(hudColors.characterSheetHeadingText);
		Font* actualFont = Font::get(txt->getFont());
		int txtHeight = txt->getNumTextLines() * actualFont->height(true);
		txtPos.h = txtHeight + 4;
		auto txtGet = Text::get(txt->getLongestLine().c_str(), txt->getFont(),
			txt->getTextColor(), txt->getOutlineColor());
		txtPos.w = txtGet->getWidth();
		txtPos.w = std::max(minWidth - padx * 2, txtPos.w);
		txt->setSize(txtPos);

		tooltipPos.w = (txtPos.w + padx * 2);

		unsigned int longestValue = 0;
		std::map<int, std::pair<Field*, SDL_Rect>> valueSizes;

		int currentHeight = txtPos.y + (actualFont->height(true) * 1) + 2;
		const int extraTextHeightForLowerCharacters = 4;
		currentHeight += padyMid;
		
		if ( raceTooltip )
		{
			raceTooltip->setDisabled(false);
			MainMenu::RaceDescriptions::update_details_text(*raceTooltip, stats[player.playernum]);

			SDL_Rect raceTooltipPos = raceTooltip->getSize();
			raceTooltipPos.x = txtPos.x + padxMid;
			raceTooltipPos.y = currentHeight;
			raceTooltipPos.w = 272;
			int heightOffset = 0;
			if ( auto details_text = raceTooltip->findField("details") )
			{
				SDL_Rect pos = details_text->getSize();
				if ( auto actualFont = Font::get(details_text->getFont()) )
				{
					const int numlines = details_text->getNumTextLines();
					const int pad = details_text->getPaddingPerLine();
					const int actualHeight = actualFont->height(true);
					pos.h = 0;
					for ( int line = 0; line < numlines; ++line )
					{
						pos.h += actualHeight + pad;// +details_text->getIndividualLinePadding(line);
						heightOffset += details_text->getIndividualLinePadding(line);
					}
				}
				raceTooltipPos.h = pos.h + pos.y + extraTextHeightForLowerCharacters;
				details_text->setSize(pos);
			}
			if ( auto details_text_right = raceTooltip->findField("details_right") )
			{
				SDL_Rect pos = details_text_right->getSize();
				if ( auto actualFont = Font::get(details_text_right->getFont()) )
				{
					const int numlines = details_text_right->getNumTextLines();
					const int pad = details_text_right->getPaddingPerLine();
					const int actualHeight = actualFont->height(true);
					pos.h = 0;
					for ( int line = 0; line < numlines; ++line )
					{
						pos.h += actualHeight + pad;// +details_text_right->getIndividualLinePadding(line);
					}
				}

				details_text_right->setSize(pos);
			}
			raceTooltip->setSize(raceTooltipPos);
			tooltipPos.w = raceTooltipPos.w + padxMid * 2;
			currentHeight = std::max(raceTooltipPos.y + raceTooltipPos.h - extraTextHeightForLowerCharacters + heightOffset, 0);
		}

		tooltipPos.h = pady1 + currentHeight + pady2;
		if ( tooltipJustify == PANEL_JUSTIFY_RIGHT )
		{
			tooltipPos.x = pos.x - tooltipPos.w;
		}
		else
		{
			tooltipPos.x = pos.x;
		}
		tooltipPos.y = pos.y;
		if ( tooltipPos.y + tooltipPos.h > sheetFrame->getSize().h )
		{
			// keep on-screen
			tooltipPos.y -= ((tooltipPos.y + tooltipPos.h) - sheetFrame->getSize().h);
			tooltipFrame->setSize(tooltipPos);
		}
		tooltipFrame->setSize(tooltipPos);
		Player::GUI_t::imageResizeToContainer9x9(tooltipFrame, SDL_Rect{ 0, 0, tooltipPos.w, tooltipPos.h },
			Player::GUI_t::tooltipEffectBackgroundImages);
	}
	else if ( element == Player::CharacterSheet_t::SHEET_CHAR_CLASS )
	{
		auto tooltipTopLeft = tooltipFrame->findImage(Player::GUI_t::tooltipEffectBackgroundImages[Player::GUI_t::TOP_LEFT].c_str());
		tooltipTopLeft->path = "*#images/ui/CharSheet/HUD_CharSheet_Tooltip_TL_Blue_00.png";
		auto tooltipTop = tooltipFrame->findImage(Player::GUI_t::tooltipEffectBackgroundImages[Player::GUI_t::TOP].c_str());
		tooltipTop->path = "*#images/ui/CharSheet/HUD_CharSheet_Tooltip_T_Blue_00.png";
		auto tooltipTopRight = tooltipFrame->findImage(Player::GUI_t::tooltipEffectBackgroundImages[Player::GUI_t::TOP_RIGHT].c_str());
		tooltipTopRight->path = "*#images/ui/CharSheet/HUD_CharSheet_Tooltip_TR_Blue_00.png";
		Player::GUI_t::imageSetWidthHeight9x9(tooltipFrame, Player::GUI_t::tooltipEffectBackgroundImages);

		int maxWidth = 260;
		int minWidth = 0;
		if ( getHoverTextString("stat_growth_min_tooltip_width") != defaultString )
		{
			minWidth = std::max(0, std::stoi(getHoverTextString("stat_growth_min_tooltip_width")));
		}
		if ( getHoverTextString("stat_growth_max_tooltip_width") != defaultString )
		{
			maxWidth = std::max(0, std::stoi(getHoverTextString("stat_growth_max_tooltip_width")));
		}

		const int padx = 16;
		const int pady1 = 8;
		const int pady2 = 4;
		const int padxMid = 4;
		const int padyMid = 8;
		SDL_Rect tooltipPos = SDL_Rect{ 400, 0, maxWidth, 100 };

		Monster race = HUMAN;
		if ( stats[player.playernum]->stat_appearance == 0 && stats[player.playernum]->playerRace != RACE_HUMAN )
		{
			race = getMonsterFromPlayerRace(stats[player.playernum]->playerRace);
		}
		Monster modifiedRace = stats[player.playernum]->type;
		if ( arachnophobia_filter )
		{
			if ( modifiedRace == SPIDER )
			{
				modifiedRace = CRAB;
			}
			if ( race == SPIDER )
			{
				race = CRAB;
			}
		}

		if ( player.entity && player.entity->effectShapeshift != 0 )
		{
			txt->setText(getHoverTextString("class_title_shapeshift").c_str());
		}
		else
		{
			txt->setText(getHoverTextString("class_title").c_str());
		}
		SDL_Rect txtPos = SDL_Rect{ padx, pady1 - 2, maxWidth - padx * 2, 80 };
		txt->setSize(txtPos);
		if ( charsheetTooltipCache[player.playernum].textEntries[element].title != txt->getText() )
		{
			txt->reflowTextToFit(0);
			charsheetTooltipCache[player.playernum].textEntries[element].title = txt->getText();
		}
		txt->setColor(hudColors.characterSheetHeadingText);
		Font* actualFont = Font::get(txt->getFont());
		int txtHeight = txt->getNumTextLines() * actualFont->height(true);
		txtPos.h = txtHeight + 4;
		auto txtGet = Text::get(txt->getLongestLine().c_str(), txt->getFont(),
			txt->getTextColor(), txt->getOutlineColor());
		txtPos.w = txtGet->getWidth();
		txtPos.w = std::max(minWidth - padx * 2, txtPos.w);
		txt->setSize(txtPos);

		tooltipPos.w = (txtPos.w + padx * 2);

		std::map<int, std::pair<Field*, SDL_Rect>> valueSizes;

		int currentHeight = txtPos.y + (actualFont->height(true) * 1) + 2;
		const int extraTextHeightForLowerCharacters = 4;
		currentHeight += padyMid;

		if ( classTooltip )
		{
			classTooltip->setDisabled(false);
			auto statGrowths = classTooltip->findFrame("stat growths");
			if ( player.entity )
			{
				MainMenu::ClassDescriptions::update_stat_growths(*statGrowths, client_classes[player.playernum], player.entity->effectShapeshift);
			}
			else
			{
				MainMenu::ClassDescriptions::update_stat_growths(*statGrowths, client_classes[player.playernum], 0);
			}

			SDL_Rect classTooltipPos = classTooltip->getSize();
			classTooltipPos.w = statGrowths->getSize().w;
			classTooltipPos.h = statGrowths->getSize().h;
			classTooltipPos.x = tooltipPos.w / 2 - classTooltipPos.w / 2;
			classTooltipPos.y = currentHeight;
			classTooltip->setSize(classTooltipPos);

			currentHeight += classTooltipPos.h - 6;

			std::string descText = "";
			descText = getHoverTextString("stat_growth_info");
			
			{
				currentHeight += padyMid;

				div->pos.x = padx;
				div->pos.y = currentHeight;
				div->pos.w = txtPos.w;
				div->disabled = false;

				currentHeight += padyMid;

				auto entry = characterSheetTooltipTextFields[player.playernum][1]; assert(entry);
				entry->setDisabled(false);
				char buf[512] = "";

				std::string descTextFormatted = "\x1E ";
				for ( auto s : descText )
				{
					descTextFormatted += s;
					if ( s == '\n' )
					{
						descTextFormatted += "\x1E ";
					}
				}

				snprintf(buf, sizeof(buf), "%s", descTextFormatted.c_str());
				entry->setText(buf);

				SDL_Rect entryPos = entry->getSize();
				entryPos.x = padx;
				entryPos.y = currentHeight;
				entryPos.w = txtPos.w;
				entry->setSize(entryPos);
				if ( charsheetTooltipCache[player.playernum].textEntries[element].entry1 != entry->getText() )
				{
					entry->reflowTextToFit(0);
					charsheetTooltipCache[player.playernum].textEntries[element].entry1 = entry->getText();
				}
				entryPos.h = actualFont->height(true) * entry->getNumTextLines() + extraTextHeightForLowerCharacters;
				entry->setSize(entryPos);
				entry->setColor(hudColors.characterSheetOffWhiteText);
				currentHeight = std::max(entryPos.y + entryPos.h - extraTextHeightForLowerCharacters, 0);

				currentHeight += padyMid / 4;
				tooltipPos.h = pady1 + currentHeight + pady2;
			}
		}

		tooltipPos.h = pady1 + currentHeight + pady2;
		if ( tooltipJustify == PANEL_JUSTIFY_RIGHT )
		{
			tooltipPos.x = pos.x - tooltipPos.w;
		}
		else
		{
			tooltipPos.x = pos.x;
		}
		tooltipPos.y = pos.y;
		if ( tooltipPos.y + tooltipPos.h > sheetFrame->getSize().h )
		{
			// keep on-screen
			tooltipPos.y -= ((tooltipPos.y + tooltipPos.h) - sheetFrame->getSize().h);
			tooltipFrame->setSize(tooltipPos);
		}
		tooltipFrame->setSize(tooltipPos);
		Player::GUI_t::imageResizeToContainer9x9(tooltipFrame, SDL_Rect{ 0, 0, tooltipPos.w, tooltipPos.h },
			Player::GUI_t::tooltipEffectBackgroundImages);
	}
	else if ( element == Player::CharacterSheet_t::SHEET_TIMER )
	{
		const int maxWidth = 240;
		const int padx = 16;
		const int pady1 = 8;
		const int pady2 = 8;
		const int padyMid = 4;
		const int padxMid = 4;
		SDL_Rect tooltipPos = SDL_Rect{ 400, 0, maxWidth, 100 };
		bool usingMouse = !inputs.getVirtualMouse(player.playernum)->lastMovementFromController;

		if ( player.characterSheet.showGameTimerAlways )
		{
			std::string tooltiptxt = getHoverTextString("game_timer_mouse") + getHoverTextString("game_timer_unpin");
			if ( strcmp(tooltiptxt.c_str(), txt->getText()) )
			{
				txt->setText(tooltiptxt.c_str());
			}
		}
		else
		{
			std::string tooltiptxt = getHoverTextString("game_timer_mouse") + getHoverTextString("game_timer_pin");
			if ( strcmp(tooltiptxt.c_str(), txt->getText()) )
			{
				txt->setText(tooltiptxt.c_str());
			}
		}

		int currentHeight = padyMid;

		SDL_Rect txtPos = SDL_Rect{ padx, pady1, maxWidth - padx * 2, 80 };
		txt->setSize(txtPos);
		if ( charsheetTooltipCache[player.playernum].textEntries[element].title != txt->getText() )
		{
			txt->reflowTextToFit(0);
			charsheetTooltipCache[player.playernum].textEntries[element].title = txt->getText();
		}
		Font* actualFont = Font::get(txt->getFont());
		int txtHeight = txt->getNumTextLines() * actualFont->height(true);
		txtPos.h = txtHeight + padyMid;
		auto txtGet = Text::get(txt->getLongestLine().c_str(), txt->getFont(),
			txt->getTextColor(), txt->getOutlineColor());
		txtPos.w = txtGet->getWidth();
		txt->setSize(txtPos);
		
		currentHeight = std::max(currentHeight, txtPos.h);

		tooltipPos.w = txtPos.w + padx * 2;
		tooltipPos.h = pady1 + currentHeight + pady2;
		if ( tooltipJustify == PANEL_JUSTIFY_RIGHT )
		{
			tooltipPos.x = pos.x - tooltipPos.w;
		}
		else
		{
			tooltipPos.x = pos.x;
		}
		tooltipPos.y = pos.y;

		tooltipFrame->setSize(tooltipPos);
		Player::GUI_t::imageResizeToContainer9x9(tooltipFrame, SDL_Rect{ 0, 0, tooltipPos.w, tooltipPos.h },
			Player::GUI_t::tooltipEffectBackgroundImages);
	}
}

void Player::CharacterSheet_t::updateCharacterInfo()
{
	auto characterInfoFrame = sheetFrame->findFrame("character info");
	assert(characterInfoFrame);
	auto characterInnerFrame = characterInfoFrame->findFrame("character info inner frame");
	assert(characterInnerFrame);

	bool enableTooltips = !player.GUI.isDropdownActive() && !player.GUI.dropdownMenu.bClosedThisTick;
	if ( !inputs.getVirtualMouse(player.playernum)->draw_cursor
		&& inputs.hasController(player.playernum)
		&& !Input::inputs[player.playernum].binary("MenuConfirm") )
	{
		enableTooltips = false;
	}

	bool bCompactView = player.bUseCompactGUIHeight();

	char buf[32] = "";
	if ( auto name = characterInnerFrame->findField("character name text") )
	{
		if ( strcmp(stats[player.playernum]->name, name->getText()) )
		{
			name->setText(stats[player.playernum]->name);
		}
	}
	Field* className = characterInnerFrame->findField("character class text");
	int classNameWidth = 0;
	if ( className )
	{
		std::string classname = playerClassLangEntry(client_classes[player.playernum], player.playernum);
		if ( !classname.empty() )
		{
			capitalizeString(classname);
			if ( strcmp(classname.c_str(), className->getText()) )
			{
				className->setText(classname.c_str());
			}
		}
		if ( client_classes[player.playernum] >= CLASS_CONJURER && client_classes[player.playernum] <= CLASS_BREWER )
		{
			className->setTextColor(hudColors.characterDLC1ClassText);
		}
		else if ( client_classes[player.playernum] >= CLASS_MACHINIST && client_classes[player.playernum] <= CLASS_HUNTER )
		{
			className->setTextColor(hudColors.characterDLC2ClassText);
		}
		else if ( client_classes[player.playernum] >= CLASS_BARD && client_classes[player.playernum] <= CLASS_PALADIN )
		{
			className->setTextColor(hudColors.characterDLC3ClassText);
		}
		else
		{
			className->setTextColor(hudColors.characterBaseClassText);
		}
		if ( auto textGet = Text::get(className->getText(), className->getFont(),
			className->getTextColor(), className->getOutlineColor()) )
		{
			classNameWidth = textGet->getWidth();
		}
	}
	Field* charLevel = characterInnerFrame->findField("character level text");
	int charLevelWidth = 0;
	if ( charLevel )
	{
		snprintf(buf, sizeof(buf), Language::get(4051), stats[player.playernum]->LVL);
		if ( strcmp(buf, charLevel->getText()) )
		{
			charLevel->setText(buf);
		}
		if ( auto textGet = Text::get(charLevel->getText(), charLevel->getFont(),
			charLevel->getTextColor(), charLevel->getOutlineColor()) )
		{
			charLevelWidth = textGet->getWidth();
		}
	}
	if ( className && charLevel )
	{
		SDL_Rect classNamePos = className->getSize();
		SDL_Rect charLevelPos = charLevel->getSize();
		const int padding = 24;
		const int startX = 8;
		const int fieldWidth = 190;
		const int totalWidth = charLevelWidth + padding / 2 + classNameWidth;
		charLevelPos.x = startX + fieldWidth / 2 - totalWidth / 2;
		charLevelPos.w = charLevelWidth;
		classNamePos.x = charLevelPos.x + charLevelPos.w + padding / 2 - 4; // -4 centres it nicely somehow, not sure why
		classNamePos.w = classNameWidth;
		charLevel->setSize(charLevelPos);
		className->setSize(classNamePos);
		//messagePlayer(0, "%d | %d", charLevelPos.x - startX, 198 - (classNamePos.x + classNamePos.w));

		if ( selectedElement == SHEET_CHAR_CLASS && enableTooltips )
		{
			SDL_Rect tooltipPos = characterInfoFrame->getSize();
			tooltipPos.y -= 4;
			//tooltipPos.y += raceText->getSize().y;
			Player::PanelJustify_t tooltipJustify = PANEL_JUSTIFY_RIGHT;
			if ( (panelJustify == PANEL_JUSTIFY_LEFT && !bCompactView) || (panelJustify == PANEL_JUSTIFY_RIGHT && bCompactView) )
			{
				tooltipJustify = PANEL_JUSTIFY_LEFT;
				tooltipPos.x += tooltipPos.w;
			}
			if ( bCompactView )
			{
				tooltipPos.y = 0;
				tooltipPos.x += (tooltipJustify == PANEL_JUSTIFY_LEFT) ? 6 : -6;
			}
			updateCharacterSheetTooltip(selectedElement, tooltipPos, tooltipJustify);
		}
	}
	if ( auto raceText = characterInnerFrame->findField("character race text") )
	{
		Monster type = stats[player.playernum]->type;
		std::string appearance = "";
		bool aestheticOnly = false;
		if ( player.entity )
		{
			if ( player.entity->effectPolymorph == NOTHING && stats[player.playernum]->playerRace > RACE_HUMAN )
			{
				if ( stats[player.playernum]->stat_appearance != 0 )
				{
					aestheticOnly = true;
					appearance = Language::get(4068);
					type = player.entity->getMonsterFromPlayerRace(stats[player.playernum]->playerRace);
				}
			}
		}
		std::string race = getMonsterLocalizedName(type).c_str();
		capitalizeString(race);
		if ( type == HUMAN )
		{
			appearance = Language::get(20 + stats[player.playernum]->stat_appearance % NUMAPPEARANCES);
			capitalizeString(appearance);
		}
		bool centerIconAndText = false;
		if ( appearance != "" )
		{
			if ( aestheticOnly )
			{
				snprintf(buf, sizeof(buf), "%s %s", appearance.c_str(), race.c_str()); // 'guised skeleton'
			}
			else
			{
				snprintf(buf, sizeof(buf), "%s %s", race.c_str(), appearance.c_str()); // 'human gloomforge'
			}
			centerIconAndText = true;
		}
		else
		{
			snprintf(buf, sizeof(buf), "%s", race.c_str());
		}

		if ( strcmp(buf, raceText->getText()) )
		{
			raceText->setText(buf);
		}
		int width = 0;
		if ( auto textGet = Text::get(raceText->getText(), raceText->getFont(),
			raceText->getTextColor(), raceText->getOutlineColor()) )
		{
			width = textGet->getWidth();
		}

		if ( auto sexImg = characterInnerFrame->findImage("character sex img") )
		{
			int offsetx = 0;
			if ( stats[player.playernum]->sex == sex_t::MALE )
			{
				if ( type == AUTOMATON )
				{
					sexImg->path = "*#images/ui/CharSheet/HUD_CharSheet_Sex_AutomatonM_02.png";
				}
				else if ( type == DRYAD )
				{
					sexImg->path = "*#images/ui/CharSheet/HUD_CharSheet_Height_T_00.png";
				}
				else if ( type == MYCONID )
				{
					sexImg->path = "*#images/ui/CharSheet/HUD_CharSheet_Height_S_00.png";
				}
				else
				{
					sexImg->path = "*#images/ui/CharSheet/HUD_CharSheet_Sex_M_02.png";
					static ConsoleVariable<int> cvar_sexoffset("/sexoffsetx", -1);
					offsetx = *cvar_sexoffset;
				}
			}
			else if ( stats[player.playernum]->sex == sex_t::FEMALE )
			{
				if ( type == AUTOMATON )
				{
					sexImg->path = "*#images/ui/CharSheet/HUD_CharSheet_Sex_AutomatonF_02.png";
				}
				else if ( type == DRYAD )
				{
					sexImg->path = "*#images/ui/CharSheet/HUD_CharSheet_Height_S_00.png";
				}
				else if ( type == MYCONID )
				{
					sexImg->path = "*#images/ui/CharSheet/HUD_CharSheet_Height_T_00.png";
				}
				else
				{
					sexImg->path = "*#images/ui/CharSheet/HUD_CharSheet_Sex_F_02.png";
				}
			}
			if ( auto imgGet = Image::get(sexImg->path.c_str()) )
			{
				sexImg->pos.w = (int)imgGet->getWidth();
				sexImg->pos.h = (int)imgGet->getHeight();
			}

			SDL_Rect raceTextPos = raceText->getSize();
			raceTextPos.x = 4;
			if ( centerIconAndText )
			{
				raceTextPos.x = 4 + (((sexImg->pos.w + 4)) / 2) + offsetx;
			}
			if ( raceTextPos.x % 2 == 1 )
			{
				--raceTextPos.x;
			}
			raceText->setSize(raceTextPos);

			sexImg->pos.x = 16;
			if ( centerIconAndText )
			{
				sexImg->pos.x = raceText->getSize().x + raceText->getSize().w / 2 - width / 2;
				sexImg->pos.x -= (sexImg->pos.w + 4) + (offsetx * 2);
			}
			if ( sexImg->pos.x % 2 == 1 )
			{
				--sexImg->pos.x;
			}
			sexImg->pos.y = raceText->getSize().y + raceText->getSize().h / 2 - sexImg->pos.h / 2;
		}

		if ( selectedElement == SHEET_CHAR_RACE_SEX && enableTooltips )
		{
			SDL_Rect tooltipPos = characterInfoFrame->getSize();
			tooltipPos.y -= 4;
			//tooltipPos.y += raceText->getSize().y;
			Player::PanelJustify_t tooltipJustify = PANEL_JUSTIFY_RIGHT;
			if ( (panelJustify == PANEL_JUSTIFY_LEFT && !bCompactView) || (panelJustify == PANEL_JUSTIFY_RIGHT && bCompactView) )
			{
				tooltipJustify = PANEL_JUSTIFY_LEFT;
				tooltipPos.x += tooltipPos.w;
			}
			if ( bCompactView )
			{
				tooltipPos.y = 0;
				tooltipPos.x += (tooltipJustify == PANEL_JUSTIFY_LEFT) ? 6 : -6;
			}
			updateCharacterSheetTooltip(selectedElement, tooltipPos, tooltipJustify);
		}
	}
	if ( auto floorFrame = sheetFrame->findFrame("dungeon floor frame") )
	{
		if ( auto floorLevelText = floorFrame->findField("dungeon level text") )
		{
			snprintf(buf, sizeof(buf), Language::get(4052), currentlevel);
			if ( strcmp(buf, floorLevelText->getText()) )
			{
				floorLevelText->setText(buf);
				charsheetTooltipCache[player.playernum].manualUpdate = true;
			}
		}
		if ( auto floorNameText = floorFrame->findField("dungeon name text") )
		{
			if ( mapDisplayNamesDescriptions.find(map.name) != mapDisplayNamesDescriptions.end() )
			{
				if ( strcmp(mapDisplayNamesDescriptions[map.name].first.c_str(), floorNameText->getText()) )
				{
					floorNameText->setText(mapDisplayNamesDescriptions[map.name].first.c_str());
					charsheetTooltipCache[player.playernum].manualUpdate = true;
				}

				if ( selectedElement == SHEET_DUNGEON_FLOOR && enableTooltips )
				{
					SDL_Rect tooltipPos = characterInfoFrame->getSize();
					tooltipPos.y = floorFrame->getSize().y;
					Player::PanelJustify_t tooltipJustify = PANEL_JUSTIFY_RIGHT;
					if ( (panelJustify == PANEL_JUSTIFY_LEFT && !bCompactView) || (panelJustify == PANEL_JUSTIFY_RIGHT && bCompactView) )
					{
						tooltipJustify = PANEL_JUSTIFY_LEFT;
						tooltipPos.x += tooltipPos.w;
					}
					if ( bCompactView )
					{
						tooltipPos.y = 0;
						tooltipPos.x += (tooltipJustify == PANEL_JUSTIFY_LEFT) ? 6 : -6;
					}
					updateCharacterSheetTooltip(selectedElement, tooltipPos, tooltipJustify);
				}
			}
			else
			{
				floorNameText->setText(map.name);
			}
		}
	}

	if ( auto gold = characterInnerFrame->findField("gold text") )
	{
		snprintf(buf, sizeof(buf), "%d", stats[player.playernum]->GOLD);
		gold->setText(buf);
		if ( selectedElement == SHEET_GOLD )
		{
			if ( Input::inputs[player.playernum].binary("MenuRightClick")
				&& player.bControlEnabled && !gamePaused
				&& !player.usingCommand()
				&& player.GUI.activeModule == Player::GUI_t::MODULE_CHARACTERSHEET
				&& !player.GUI.isDropdownActive() )
			{
				player.GUI.dropdownMenu.open("drop_gold");
			}
			else if ( (!inputs.getVirtualMouse(player.playernum)->draw_cursor
					&& inputs.hasController(player.playernum)
					&& Input::inputs[player.playernum].binaryToggle("MenuConfirm")
				)
				&& player.GUI.activeModule == Player::GUI_t::MODULE_CHARACTERSHEET
				&& !player.GUI.isDropdownActive()
				&& !player.usingCommand()
				&& player.bControlEnabled && !gamePaused )
			{
				Input::inputs[player.playernum].consumeBinaryToggle("MenuConfirm");
				player.GUI.dropdownMenu.open("drop_gold");
				player.GUI.dropdownMenu.dropDownToggleClick = true;
				SDL_Rect dropdownPos = characterInfoFrame->getSize();
				dropdownPos.y += gold->getSize().y + gold->getSize().h / 2;
				if ( auto interactMenuTop = player.GUI.dropdownMenu.dropdownFrame->findImage("interact top background") )
				{
					// 10px is slot half height, move by 1.5 slots, minus the top interact text height
					dropdownPos.y -= (interactMenuTop->pos.h + (3 * 10) + 4);
				}
				if ( !player.GUI.dropdownMenu.getDropDownAlignRight("drop_gold") )
				{
					player.GUI.dropdownMenu.dropDownX = dropdownPos.x;
				}
				else
				{
					player.GUI.dropdownMenu.dropDownX = dropdownPos.x + dropdownPos.w;
				}
				player.GUI.dropdownMenu.dropDownX += player.camera_virtualx1();
				player.GUI.dropdownMenu.dropDownY = dropdownPos.y + player.camera_virtualy1();
			}
			if ( enableTooltips && !inputs.getVirtualMouse(player.playernum)->lastMovementFromController )
			{
				SDL_Rect tooltipPos = characterInfoFrame->getSize();
				tooltipPos.y += gold->getSize().y;
				tooltipPos.y -= 6;
				Player::PanelJustify_t tooltipJustify = PANEL_JUSTIFY_RIGHT;
				if ( (panelJustify == PANEL_JUSTIFY_LEFT && !bCompactView) || (panelJustify == PANEL_JUSTIFY_RIGHT && bCompactView) )
				{
					tooltipJustify = PANEL_JUSTIFY_LEFT;
					tooltipPos.x += tooltipPos.w;
				}
				if ( bCompactView )
				{
					tooltipPos.x += (tooltipJustify == PANEL_JUSTIFY_LEFT) ? 6 : -6;
				}
				updateCharacterSheetTooltip(selectedElement, tooltipPos, tooltipJustify);
			}
		}
	}
}

void Player::CharacterSheet_t::updateStats()
{
	auto characterInfoFrame = sheetFrame->findFrame("character info");
	assert(characterInfoFrame);
	auto characterInnerFrame = characterInfoFrame->findFrame("character info inner frame");
	assert(characterInnerFrame);
	auto statsFrame = sheetFrame->findFrame("stats");
	assert(statsFrame);

	auto statsPos = statsFrame->getSize();
	//statsPos.x = sheetFrame->getSize().w - statsPos.w;
	statsFrame->setSize(statsPos);

	auto statsInnerFrame = statsFrame->findFrame("stats inner frame");
	assert(statsInnerFrame);

	const int rightAlignPosX = 0;
	const int leftAlignPosX = 10;
	auto statsInnerPos = statsInnerFrame->getSize();
	statsInnerPos.x = rightAlignPosX;
	/*if ( keystatus[SDLK_I] && enableDebugKeys )
	{
		statsInnerPos.x = leftAlignPosX;
	}*/
	statsInnerFrame->setSize(statsInnerPos);
	Button* strButton = statsInnerFrame->findButton("str button");
	Button* dexButton = statsInnerFrame->findButton("dex button");
	Button* conButton = statsInnerFrame->findButton("con button");
	Button* intButton = statsInnerFrame->findButton("int button");
	Button* perButton = statsInnerFrame->findButton("per button");
	Button* chrButton = statsInnerFrame->findButton("chr button");

	bool bCompactView = player.bUseCompactGUIHeight();

	bool enableTooltips = !player.GUI.isDropdownActive() && !player.GUI.dropdownMenu.bClosedThisTick;
	if ( !inputs.getVirtualMouse(player.playernum)->draw_cursor 
		&& inputs.hasController(player.playernum)
		&& !Input::inputs[player.playernum].binary("MenuConfirm") )
	{
		enableTooltips = false;
	}
	char buf[32] = "";
	if ( auto field = statsInnerFrame->findField("str text stat") )
	{
		snprintf(buf, sizeof(buf), "%d", stats[player.playernum]->STR);
		if ( strcmp(buf, field->getText()) )
		{
			field->setText(buf);
			charsheetTooltipCache[player.playernum].manualUpdate = true;
		}
		field->setColor(hudColors.characterSheetNeutral);

		Sint32 modifiedStat = statGetSTR(stats[player.playernum], players[player.playernum]->entity);
		if ( auto modifiedField = statsInnerFrame->findField("str text modified") )
		{
			modifiedField->setColor(hudColors.characterSheetNeutral);
			modifiedField->setDisabled(true);
			snprintf(buf, sizeof(buf), "%d", modifiedStat);
			if ( strcmp(buf, modifiedField->getText()) )
			{
				modifiedField->setText(buf);
				charsheetTooltipCache[player.playernum].manualUpdate = true;
			}
			if ( modifiedStat > stats[player.playernum]->STR )
			{
				modifiedField->setColor(hudColors.characterSheetGreen);
				modifiedField->setDisabled(false);
			}
			else if ( modifiedStat < stats[player.playernum]->STR )
			{
				modifiedField->setColor(hudColors.characterSheetRed);
				modifiedField->setDisabled(false);
			}
		}
		if ( selectedElement == SHEET_STR && enableTooltips )
		{
			SDL_Rect tooltipPos = statsFrame->getSize();
			tooltipPos.y += statsInnerFrame->getSize().y;
			Player::PanelJustify_t tooltipJustify = panelJustify;
			if ( panelJustify == PANEL_JUSTIFY_LEFT )
			{
				tooltipPos.x += tooltipPos.w;
			}
			updateCharacterSheetTooltip(selectedElement, tooltipPos, tooltipJustify);
		}
	}
	if ( auto field = statsInnerFrame->findField("dex text stat") )
	{
		snprintf(buf, sizeof(buf), "%d", stats[player.playernum]->DEX);
		if ( strcmp(buf, field->getText()) )
		{
			field->setText(buf);
			charsheetTooltipCache[player.playernum].manualUpdate = true;
		}
		field->setColor(hudColors.characterSheetNeutral);

		Sint32 modifiedStat = statGetDEX(stats[player.playernum], players[player.playernum]->entity);
		if ( auto modifiedField = statsInnerFrame->findField("dex text modified") )
		{
			modifiedField->setColor(hudColors.characterSheetNeutral);
			modifiedField->setDisabled(true);
			snprintf(buf, sizeof(buf), "%d", modifiedStat);
			if ( strcmp(buf, modifiedField->getText()) )
			{
				modifiedField->setText(buf);
				charsheetTooltipCache[player.playernum].manualUpdate = true;
			}
			if ( modifiedStat > stats[player.playernum]->DEX )
			{
				modifiedField->setColor(hudColors.characterSheetGreen);
				modifiedField->setDisabled(false);
			}
			else if ( modifiedStat < stats[player.playernum]->DEX )
			{
				modifiedField->setColor(hudColors.characterSheetRed);
				modifiedField->setDisabled(false);
			}
		}
		if ( selectedElement == SHEET_DEX && enableTooltips )
		{
			SDL_Rect tooltipPos = statsFrame->getSize();
			tooltipPos.y += statsInnerFrame->getSize().y;
			Player::PanelJustify_t tooltipJustify = panelJustify;
			if ( panelJustify == PANEL_JUSTIFY_LEFT )
			{
				tooltipPos.x += tooltipPos.w;
			}
			updateCharacterSheetTooltip(selectedElement, tooltipPos, tooltipJustify);
		}
	}
	if ( auto field = statsInnerFrame->findField("con text stat") )
	{
		snprintf(buf, sizeof(buf), "%d", stats[player.playernum]->CON);
		if ( strcmp(buf, field->getText()) )
		{
			field->setText(buf);
			charsheetTooltipCache[player.playernum].manualUpdate = true;
		}
		field->setColor(hudColors.characterSheetNeutral);

		Sint32 modifiedStat = statGetCON(stats[player.playernum], players[player.playernum]->entity);
		if ( auto modifiedField = statsInnerFrame->findField("con text modified") )
		{
			modifiedField->setColor(hudColors.characterSheetNeutral);
			modifiedField->setDisabled(true);
			snprintf(buf, sizeof(buf), "%d", modifiedStat);
			if ( strcmp(buf, modifiedField->getText()) )
			{
				modifiedField->setText(buf);
				charsheetTooltipCache[player.playernum].manualUpdate = true;
			}
			if ( modifiedStat > stats[player.playernum]->CON )
			{
				modifiedField->setColor(hudColors.characterSheetGreen);
				modifiedField->setDisabled(false);
			}
			else if ( modifiedStat < stats[player.playernum]->CON )
			{
				modifiedField->setColor(hudColors.characterSheetRed);
				modifiedField->setDisabled(false);
			}
		}
		if ( selectedElement == SHEET_CON && enableTooltips )
		{
			SDL_Rect tooltipPos = statsFrame->getSize();
			tooltipPos.y += statsInnerFrame->getSize().y;
			Player::PanelJustify_t tooltipJustify = panelJustify;
			if ( panelJustify == PANEL_JUSTIFY_LEFT )
			{
				tooltipPos.x += tooltipPos.w;
			}
			updateCharacterSheetTooltip(selectedElement, tooltipPos, tooltipJustify);
		}
	}
	if ( auto field = statsInnerFrame->findField("int text stat") )
	{
		snprintf(buf, sizeof(buf), "%d", stats[player.playernum]->INT);
		if ( strcmp(buf, field->getText()) )
		{
			field->setText(buf);
			charsheetTooltipCache[player.playernum].manualUpdate = true;
		}
		field->setColor(hudColors.characterSheetNeutral);

		Sint32 modifiedStat = statGetINT(stats[player.playernum], players[player.playernum]->entity);
		if ( auto modifiedField = statsInnerFrame->findField("int text modified") )
		{
			modifiedField->setColor(hudColors.characterSheetNeutral);
			modifiedField->setDisabled(true);
			snprintf(buf, sizeof(buf), "%d", modifiedStat);
			if ( strcmp(buf, modifiedField->getText()) )
			{
				modifiedField->setText(buf);
				charsheetTooltipCache[player.playernum].manualUpdate = true;
			}
			if ( modifiedStat > stats[player.playernum]->INT )
			{
				modifiedField->setColor(hudColors.characterSheetGreen);
				modifiedField->setDisabled(false);
			}
			else if ( modifiedStat < stats[player.playernum]->INT )
			{
				modifiedField->setColor(hudColors.characterSheetRed);
				modifiedField->setDisabled(false);
			}
		}
		if ( selectedElement == SHEET_INT && enableTooltips )
		{
			SDL_Rect tooltipPos = statsFrame->getSize();
			tooltipPos.y += statsInnerFrame->getSize().y;
			Player::PanelJustify_t tooltipJustify = panelJustify;
			if ( panelJustify == PANEL_JUSTIFY_LEFT )
			{
				tooltipPos.x += tooltipPos.w;
			}
			updateCharacterSheetTooltip(selectedElement, tooltipPos, tooltipJustify);
		}
	}
	if ( auto field = statsInnerFrame->findField("per text stat") )
	{
		snprintf(buf, sizeof(buf), "%d", stats[player.playernum]->PER);
		if ( strcmp(buf, field->getText()) )
		{
			field->setText(buf);
			charsheetTooltipCache[player.playernum].manualUpdate = true;
		}
		field->setColor(hudColors.characterSheetNeutral);

		Sint32 modifiedStat = statGetPER(stats[player.playernum], players[player.playernum]->entity);
		if ( auto modifiedField = statsInnerFrame->findField("per text modified") )
		{
			modifiedField->setColor(hudColors.characterSheetNeutral);
			modifiedField->setDisabled(true);
			snprintf(buf, sizeof(buf), "%d", modifiedStat);
			if ( strcmp(buf, modifiedField->getText()) )
			{
				modifiedField->setText(buf);
				charsheetTooltipCache[player.playernum].manualUpdate = true;
			}
			if ( modifiedStat > stats[player.playernum]->PER )
			{
				modifiedField->setColor(hudColors.characterSheetGreen);
				modifiedField->setDisabled(false);
			}
			else if ( modifiedStat < stats[player.playernum]->PER )
			{
				modifiedField->setColor(hudColors.characterSheetRed);
				modifiedField->setDisabled(false);
			}
		}
		if ( selectedElement == SHEET_PER && enableTooltips )
		{
			SDL_Rect tooltipPos = statsFrame->getSize();
			tooltipPos.y += statsInnerFrame->getSize().y;
			Player::PanelJustify_t tooltipJustify = panelJustify;
			if ( panelJustify == PANEL_JUSTIFY_LEFT )
			{
				tooltipPos.x += tooltipPos.w;
			}
			updateCharacterSheetTooltip(selectedElement, tooltipPos, tooltipJustify);
		}
	}
	if ( auto field = statsInnerFrame->findField("chr text stat") )
	{
		snprintf(buf, sizeof(buf), "%d", stats[player.playernum]->CHR);
		if ( strcmp(buf, field->getText()) )
		{
			field->setText(buf);
			charsheetTooltipCache[player.playernum].manualUpdate = true;
		}
		field->setColor(hudColors.characterSheetNeutral);

		Sint32 modifiedStat = statGetCHR(stats[player.playernum], players[player.playernum]->entity);
		if ( auto modifiedField = statsInnerFrame->findField("chr text modified") )
		{
			modifiedField->setColor(hudColors.characterSheetNeutral);
			modifiedField->setDisabled(true);
			snprintf(buf, sizeof(buf), "%d", modifiedStat);
			if ( strcmp(buf, modifiedField->getText()) )
			{
				modifiedField->setText(buf);
				charsheetTooltipCache[player.playernum].manualUpdate = true;
			}
			if ( modifiedStat > stats[player.playernum]->CHR )
			{
				modifiedField->setColor(hudColors.characterSheetGreen);
				modifiedField->setDisabled(false);
			}
			else if ( modifiedStat < stats[player.playernum]->CHR )
			{
				modifiedField->setColor(hudColors.characterSheetRed);
				modifiedField->setDisabled(false);
			}
		}
		if ( selectedElement == SHEET_CHR && enableTooltips )
		{
			SDL_Rect tooltipPos = statsFrame->getSize();
			tooltipPos.y += statsInnerFrame->getSize().y;
			Player::PanelJustify_t tooltipJustify = panelJustify;
			if ( panelJustify == PANEL_JUSTIFY_LEFT )
			{
				tooltipPos.x += tooltipPos.w;
			}
			updateCharacterSheetTooltip(selectedElement, tooltipPos, tooltipJustify);
		}
	}
}

void Player::CharacterSheet_t::updateAttributes()
{
	auto attributesFrame = sheetFrame->findFrame("attributes");
	assert(attributesFrame);

	auto attributesPos = attributesFrame->getSize();
	//attributesPos.x = sheetFrame->getSize().w - attributesPos.w;
	attributesFrame->setSize(attributesPos);

	auto attributesInnerFrame = attributesFrame->findFrame("attributes inner frame");
	assert(attributesInnerFrame);

	const int rightAlignPosX = 0;
	const int leftAlignPosX = 10;
	auto attributesInnerPos = attributesInnerFrame->getSize();
	attributesInnerPos.x = rightAlignPosX;
	/*if ( keystatus[SDLK_I] )
	{
		attributesInnerPos.x = leftAlignPosX;
	}*/
	attributesInnerFrame->setSize(attributesInnerPos);

	bool bCompactView = player.bUseCompactGUIHeight();

	bool enableTooltips = !player.GUI.isDropdownActive() && !player.GUI.dropdownMenu.bClosedThisTick;
	if ( !inputs.getVirtualMouse(player.playernum)->draw_cursor
		&& inputs.hasController(player.playernum)
		&& !Input::inputs[player.playernum].binary("MenuConfirm") )
	{
		enableTooltips = false;
	}

	char buf[32] = "";

	if ( auto field = attributesInnerFrame->findField("atk text stat") )
	{
		AttackHoverText_t atkHoverText;
		Sint32 displayedATK = displayAttackPower(player.playernum, atkHoverText);
		if ( atkHoverText.hoverType == AttackHoverText_t::ATK_HOVER_TYPE_MAGICSTAFF
			|| atkHoverText.hoverType == AttackHoverText_t::ATK_HOVER_TYPE_TOOL
			|| atkHoverText.hoverType == AttackHoverText_t::ATK_HOVER_TYPE_TOOL_TRAP
			|| atkHoverText.hoverType == AttackHoverText_t::ATK_HOVER_TYPE_DEFAULT )
		{
			snprintf(buf, sizeof(buf), "-");
		}
		else
		{
			snprintf(buf, sizeof(buf), "%d", displayedATK);
		}
		if ( strcmp(buf, field->getText()) )
		{
			field->setText(buf);
			charsheetTooltipCache[player.playernum].manualUpdate = true;
		}
		field->setColor(hudColors.characterSheetNeutral);

		if ( selectedElement == SHEET_ATK && enableTooltips )
		{
			SDL_Rect tooltipPos = attributesFrame->getSize();
			tooltipPos.y += attributesInnerFrame->getSize().y;
			Player::PanelJustify_t tooltipJustify = panelJustify;
			if ( panelJustify == PANEL_JUSTIFY_LEFT )
			{
				tooltipPos.x += tooltipPos.w;
			}
			updateCharacterSheetTooltip(selectedElement, tooltipPos, tooltipJustify);
		}
	}

	if ( auto field = attributesInnerFrame->findField("ac text stat") )
	{
		snprintf(buf, sizeof(buf), "%d", AC(stats[player.playernum]));
		if ( strcmp(buf, field->getText()) )
		{
			field->setText(buf);
			charsheetTooltipCache[player.playernum].manualUpdate = true;
		}
		field->setColor(hudColors.characterSheetNeutral);

		if ( selectedElement == SHEET_AC && enableTooltips )
		{
			SDL_Rect tooltipPos = attributesFrame->getSize();
			tooltipPos.y += attributesInnerFrame->getSize().y;
			Player::PanelJustify_t tooltipJustify = panelJustify;
			if ( panelJustify == PANEL_JUSTIFY_LEFT )
			{
				tooltipPos.x += tooltipPos.w;
			}
			updateCharacterSheetTooltip(selectedElement, tooltipPos, tooltipJustify);
		}
	}

	if ( auto field = attributesInnerFrame->findField("pwr text stat") )
	{
		real_t spellPower = 0.0;
		if ( auto spell = player.magic.selectedSpell() )
		{
			spellPower = (getBonusFromCasterOfSpellElement(player.entity, stats[player.playernum], nullptr, spell->ID, spell->skillID) * 100.0) + 100.0;
		}
		else
		{
			spellPower = (getBonusFromCasterOfSpellElement(player.entity, stats[player.playernum], nullptr, SPELL_NONE, NUMPROFICIENCIES) * 100.0) + 100.0;
		}
		snprintf(buf, sizeof(buf), "%.f%%", spellPower);
		if ( strcmp(buf, field->getText()) )
		{
			field->setText(buf);
			charsheetTooltipCache[player.playernum].manualUpdate = true;
		}
		field->setColor(hudColors.characterSheetNeutral);

		if ( selectedElement == SHEET_POW && enableTooltips )
		{
			SDL_Rect tooltipPos = attributesFrame->getSize();
			tooltipPos.y += attributesInnerFrame->getSize().y;
			Player::PanelJustify_t tooltipJustify = panelJustify;
			if ( panelJustify == PANEL_JUSTIFY_LEFT )
			{
				tooltipPos.x += tooltipPos.w;
			}
			updateCharacterSheetTooltip(selectedElement, tooltipPos, tooltipJustify);
		}
	}

	if ( auto field = attributesInnerFrame->findField("res text stat") )
	{
		real_t resistance = 100.0 * Entity::getDamageTableMultiplier(player.entity, *stats[player.playernum], DAMAGE_TABLE_MAGIC);

		Sint32 oldStats[NUMSTATS];
		characterSheetTooltipSetZeroStat(player.entity, stats[player.playernum], nullptr, oldStats);
		real_t resistanceNoINT = 100.0 * Entity::getDamageTableMultiplier(player.entity, *stats[player.playernum], DAMAGE_TABLE_MAGIC);
		characterSheetTooltipRestoreStats(stats[player.playernum], nullptr, oldStats);
		
		resistance = -(resistance - 100.0);
		resistanceNoINT = -(resistanceNoINT - 100.0);

		snprintf(buf, sizeof(buf), "%d%%", (int)resistance);
		if ( strcmp(buf, field->getText()) )
		{
			field->setText(buf);
			charsheetTooltipCache[player.playernum].manualUpdate = true;
		}
		field->setColor(hudColors.characterSheetNeutral);
		if ( (int)resistance > 0 && (int)resistanceNoINT > 0 )
		{
			field->setColor(hudColors.characterSheetGreen);
		}
		else if ( (int)resistance < 0 )
		{
			field->setColor(hudColors.characterSheetRed);
		}

		if ( selectedElement == SHEET_RES && enableTooltips )
		{
			SDL_Rect tooltipPos = attributesFrame->getSize();
			tooltipPos.y += attributesInnerFrame->getSize().y;
			Player::PanelJustify_t tooltipJustify = panelJustify;
			if ( panelJustify == PANEL_JUSTIFY_LEFT )
			{
				tooltipPos.x += tooltipPos.w;
			}
			updateCharacterSheetTooltip(selectedElement, tooltipPos, tooltipJustify);
		}
	}

	if ( auto field = attributesInnerFrame->findField("regen text hp") )
	{
		field->setColor(hudColors.characterSheetNeutral);
		Uint32 color = hudColors.characterSheetNeutral;
		getDisplayedHPRegen(players[player.playernum]->entity, *stats[player.playernum], &color, buf);
		if ( strcmp(buf, field->getText()) )
		{
			field->setText(buf);
			charsheetTooltipCache[player.playernum].manualUpdate = true;
		}
		field->setColor(color);

		if ( selectedElement == SHEET_RGN && enableTooltips )
		{
			SDL_Rect tooltipPos = attributesFrame->getSize();
			tooltipPos.y += attributesInnerFrame->getSize().y;
			Player::PanelJustify_t tooltipJustify = panelJustify;
			if ( panelJustify == PANEL_JUSTIFY_LEFT )
			{
				tooltipPos.x += tooltipPos.w;
			}
			updateCharacterSheetTooltip(selectedElement, tooltipPos, tooltipJustify);
		}
	}

	if ( auto field = attributesInnerFrame->findField("regen text mp") )
	{
		field->setColor(hudColors.characterSheetNeutral);
		Uint32 color = hudColors.characterSheetNeutral;
		getDisplayedMPRegen(players[player.playernum]->entity, *stats[player.playernum], &color, buf);
		if ( strcmp(buf, field->getText()) )
		{
			field->setText(buf);
			charsheetTooltipCache[player.playernum].manualUpdate = true;
		}
		field->setColor(color);

		if ( selectedElement == SHEET_RGN_MP && enableTooltips )
		{
			SDL_Rect tooltipPos = attributesFrame->getSize();
			tooltipPos.y += attributesInnerFrame->getSize().y;
			Player::PanelJustify_t tooltipJustify = panelJustify;
			if ( panelJustify == PANEL_JUSTIFY_LEFT )
			{
				tooltipPos.x += tooltipPos.w;
			}
			updateCharacterSheetTooltip(selectedElement, tooltipPos, tooltipJustify);
		}
	}

	if ( auto field = attributesInnerFrame->findField("weight text stat") )
	{
		Sint32 weight = player.movement.getCharacterWeight();
		snprintf(buf, sizeof(buf), "%d", weight);
		if ( strcmp(buf, field->getText()) )
		{
			field->setText(buf);
			charsheetTooltipCache[player.playernum].manualUpdate = true;
		}
		field->setColor(hudColors.characterSheetNeutral);

		if ( selectedElement == SHEET_WGT && enableTooltips )
		{
			SDL_Rect tooltipPos = attributesFrame->getSize();
			tooltipPos.y += attributesInnerFrame->getSize().y;
			Player::PanelJustify_t tooltipJustify = panelJustify;
			if ( panelJustify == PANEL_JUSTIFY_LEFT )
			{
				tooltipPos.x += tooltipPos.w;
			}
			updateCharacterSheetTooltip(selectedElement, tooltipPos, tooltipJustify);
		}
	}
}

