//! @file StatusEffectQueue.hpp
//! Extracted from GameUI.hpp/.cpp — status effect queue UI system

#pragma once

#include <deque>
#include <map>
#include <string>
#include <vector>
#include <unordered_map>

#include "../game.h"
#include "../stat.h"
#include "../interface/consolecommand.h"
#include "Frame.hpp"

struct StatusEffectQueueEntry_t
{
	real_t animateX = 0.0;
	real_t animateY = 0.0;
	real_t animateW = 0.0;
	real_t animateH = 0.0;
	int animateSetpointX = 0;
	int animateSetpointY = 0;
	int animateSetpointW = 0;
	int animateSetpointH = 0;
	int animateStartX = 0;
	int animateStartY = 0;
	int animateStartW = 0;
	int animateStartH = 0;
	SDL_Rect pos{ 0, 0, 0, 0 };
	SDL_Rect notificationTargetPosition{ 0, 0, 32, 32 };
	Uint32 lastUpdateTick = 0;
	int effect = -1;
	Uint32 customVariable = 0;
	bool lowDuration = false;
	enum NotificationStates_t : int
	{
		STATE_1,
		STATE_2,
		STATE_3,
		STATE_4,
		STATE_END
	};
	NotificationStates_t notificationState = STATE_1;
	NotificationStates_t notificationStateInit = STATE_1;
	StatusEffectQueueEntry_t(int _effect)
	{
		effect = _effect;
		lastUpdateTick = ticks;
		pos.w = getEffectSpriteNormalWidth();
		pos.h = getEffectSpriteNormalHeight();

		notificationTargetPosition.w = pos.w;
		notificationTargetPosition.h = pos.h;
	}

	enum Dir_t : int
	{
		NONE,
		LEFT,
		UP,
		DOWN,
		RIGHT
	};
	std::map<Dir_t, size_t> navigation;
	size_t index = 0;
	int getEffectSpriteNormalWidth();
	int getEffectSpriteNormalHeight();
	void animate();
	void animateNotification(int player);
	void setAnimatePosition(int destx, int desty);
	void setAnimatePosition(int destx, int desty, int destw, int desth);
	real_t getStatusEffectLargestScaling(int player);
	real_t getStatusEffectMidScaling(int player);
};

struct StatusEffectQueue_t
{
	// defined in StatusEffectQueue.cpp
	static const int kEffectBread;
	static const int kEffectBloodHunger;
	static const int kEffectAutomatonHunger;
	static const int kSpellEffectOffset;
	static const int kEffectBurning;
	static const int kEffectWanted;
	static const int kEffectWantedInShop;
	static const int kEffectFreeAction;
	static const int kEffectLesserWarning;
	static const int kEffectDisabledHPRegen;
	static const int kEffectResistBurning;
	static const int kEffectResistPoison;
	static const int kEffectSlowDigestion;
	static const int kEffectStrangulation;
	static const int kEffectWarning;
	static const int kEffectWaterBreathing;
	static const int kEffectConflict;
	static const int kEffectWaterWalking;
	static const int kEffectLifesaving;
	static const int kEffectPush;
	static const int kEffectSneak;
	static const int kEffectDrunkGoatman;
	static const int kEffectBountyTarget;
	static const int kEffectInspiration;
	static const int kEffectRetaliation;
	static const int kEffectAssistance;
	static const int kEffectStability;
	static const int kEffectVandal;
	static const int kEffectOvercharge;
	static const int kEffectWealth;
	static const int kEffectEnd;

	Frame* statusEffectFrame = nullptr;
	Frame* statusEffectTooltipFrame = nullptr;
	int player = -1;
	std::deque<StatusEffectQueueEntry_t> effectQueue;
	std::deque<StatusEffectQueueEntry_t> notificationQueue;
	int getBaseEffectPosX();
	int getBaseEffectPosY();
	real_t tooltipOpacitySetpoint = 100;
	real_t tooltipOpacityAnimate = 1.0;
	Uint32 tooltipDeselectedTick = 0;
	int tooltipShowingEffectID = -1;
	int tooltipShowingEffectVariable = -1;
	bool insertEffect(int effectID, int spellID);
	int effectsPerRow = 4;
	real_t focusedWindowAnim = 0.0;
	bool requiresAnimUpdate = false;
	bool bCompactWidth = false;
	bool bCompactHeight = false;
	SDL_Rect effectsBoundingBox{ 0, 0, 0, 0 };
	int selectedElement = -1;
	void updateAllQueuedEffects();
	void animateStatusEffectTooltip(bool showTooltip);
	bool doStatusEffectTooltip(StatusEffectQueueEntry_t& entry, SDL_Rect pos);
	void updateEntryImage(StatusEffectQueueEntry_t& entry, Frame::image_t* img);
	void createStatusEffectTooltip();
	Frame* getStatusEffectFrame();
	void handleNavigation(std::map<int, StatusEffectQueueEntry_t*>& grid,
		bool& tooltipShowing, const bool hungerEffectInEffectQueue);
	void resetQueue()
	{
		requiresAnimUpdate = true;
		effectQueue.clear();
		notificationQueue.clear();
	}
	void deleteEffect(int effect)
	{
		for ( auto it = effectQueue.begin(); it != effectQueue.end(); )
		{
			if ( (*it).effect == effect )
			{
				requiresAnimUpdate = true;
				it = effectQueue.erase(it);
				continue;
			}
			++it;
		}
	}
	StatusEffectQueue_t(int _player) { player = _player; }
	static void loadStatusEffectsJSON();
	struct EffectDefinitionEntry_t
	{
		int effect_id = -1;
		int spell_id = -1;
		std::string internal_name = "";
		std::string name = "";
		std::string desc = "";
		std::string imgPath = "";
		std::vector<std::string> nameVariations;
		std::vector<std::string> descVariations;
		std::vector<int> useSpellIDForImgVariations;
		std::vector<std::string> imgPathVariations;
		int useSpellIDForImg = -1;
		bool neverDisplay = false;
		int sustainedSpellID = -1;
		std::string& getName(int variation = -1);
		std::string& getDesc(int variation = -1);
		int tooltipWidth = 200;
	};
	struct StatusEffectDefinitions_t
	{
		static std::unordered_map<int, EffectDefinitionEntry_t> allEffects;
		static std::unordered_map<int, EffectDefinitionEntry_t> allSustainedSpells;
		static Uint32 tooltipHeadingColor;
		static Uint32 tooltipDescColor;
		static Uint32 notificationTextColor;
		static std::string notificationFont;
		static void reset()
		{
			allEffects.clear();
			allSustainedSpells.clear();
		}
		static bool effectDefinitionExists(int effectID)
		{
			return (allEffects.find(effectID) != allEffects.end());
		}
		static bool sustainedSpellDefinitionExists(int spellID)
		{
			return (allSustainedSpells.find(spellID) != allSustainedSpells.end());
		}
		static EffectDefinitionEntry_t& getEffect(int effectID)
		{
			return allEffects[effectID];
		}
		static EffectDefinitionEntry_t& getSustainedSpell(int spellID)
		{
			return allSustainedSpells[spellID];
		}
		static std::string getEffectImgPath(EffectDefinitionEntry_t& entry, int variation = -1);
	};
};

extern StatusEffectQueue_t StatusEffectQueue[MAXPLAYERS];

void draw_status_effect_numbers_fn(const Widget& widget, SDL_Rect pos);
void createStatusEffectQueue(const int player);
int getStatusEffectMovementAmount(int player);
