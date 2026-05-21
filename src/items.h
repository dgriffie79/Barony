/*-------------------------------------------------------------------------------

	BARONY
	File: items.h
	Desc: contains names and definitions for items

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#pragma once

#include "main.h"
#include "prng.h"

#ifdef __cplusplus

// Forward declarations to avoid circular includes with game.h
class Entity;
class Stat;
extern int currentlevel;

// items
#include "item_types.h"
const int NUMITEMS = ITEM_ENUM_MAX;

typedef enum Category
{
	WEAPON,
	ARMOR,
	AMULET,
	POTION,
	SCROLL,
	MAGICSTAFF,
	RING,
	SPELLBOOK,
	GEM,
	THROWN,
	TOOL,
	FOOD,
	BOOK,
	SPELL_CAT,
	TOME_SPELL,
	CATEGORY_MAX
} Category;

typedef enum Status
{
	BROKEN,
	DECREPIT,
	WORN,
	SERVICABLE,
	EXCELLENT
} Status;

typedef enum EquipmentType
{
	TYPE_NONE,
	TYPE_HELM,
	TYPE_HAT,
	TYPE_BREASTPIECE,
	TYPE_BOOTS,
	TYPE_SHIELD,
	TYPE_GLOVES,
	TYPE_CLOAK,
	TYPE_RING,
	TYPE_AMULET,
	TYPE_MASK,
	TYPE_SWORD,
	TYPE_AXE,
	TYPE_SPEAR,
	TYPE_MACE,
	TYPE_BOW,
	TYPE_PROJECTILE,
	TYPE_OFFHAND
} EquipmentType;

class SummonProperties
{
	//TODO: Store monster stats.
public:
	SummonProperties();
	~SummonProperties() noexcept;

	SummonProperties(const SummonProperties& other) = default;
	SummonProperties(SummonProperties&& other) noexcept = default;
	SummonProperties& operator=(const SummonProperties& other) = default;
	SummonProperties& operator=(SummonProperties&& other) noexcept = default;

protected:

private:

};

enum ItemEquippableSlot : int
{
	EQUIPPABLE_IN_SLOT_WEAPON,
	EQUIPPABLE_IN_SLOT_SHIELD,
	EQUIPPABLE_IN_SLOT_MASK,
	EQUIPPABLE_IN_SLOT_HELM,
	EQUIPPABLE_IN_SLOT_GLOVES,
	EQUIPPABLE_IN_SLOT_BOOTS,
	EQUIPPABLE_IN_SLOT_BREASTPLATE,
	EQUIPPABLE_IN_SLOT_CLOAK,
	EQUIPPABLE_IN_SLOT_AMULET,
	EQUIPPABLE_IN_SLOT_RING,
	NO_EQUIP
};

// inventory item structure
class Item
{
public:
	ItemType type;
	Status status;

	Sint16 beatitude;  // blessedness
	Sint16 count;      // how many of item
	Uint32 appearance; // large random static number
	bool identified;   // if the item is identified or not
	Uint32 uid;        // item uid
	Sint32 x, y;       // slot coordinates in item grid
	Uint32 ownerUid;   // original owner
	Uint32 interactNPCUid; // if NPC is interacting with item
	bool forcedPickupByPlayer; // player used interact on NPC with item on floor
	bool isDroppable; // if item should drop on death
	bool playerSoldItemToShop = false; // if item was sold to a shopkeeper
	bool itemHiddenFromShop = false; // if item needs to be hidden in shop view
	bool notifyIcon = false; // if item draws exclamation as a 'new' untouched item
	bool spellNotifyIcon = false; // if spell can level you up
	Uint8 itemRequireTradingSkillInShop = 0; // if item hidden in shop view until player has trading req
	bool itemSpecialShopConsumable = false; // if item is extra non-standard inventory consumable

	// weight, category and other generic info reported by function calls

	node_t* node = nullptr;

	/*
	 * Gems use this to store information about what sort of creature they contain.
	 */
	//SummonProperties *captured_monster;
	//I wish there was an easy way to do this.
	//As it stands, no item destructor is called , so this would lead to a memory leak.
	//And tracking down every time an item gets deleted and calling an item destructor would be quite a doozey.

	char* description() const;
	char* getName() const;

	//General Functions.
	Sint32 weaponGetAttack(const Stat* wielder = nullptr) const; //Returns the tohit of the weapon.
	Sint32 armorGetAC(const Stat* wielder = nullptr) const;
	bool canUnequip(const Stat* wielder = nullptr); //Returns true if the item can be unequipped (not cursed), false if it can't (cursed).
	int buyValue(int player) const;
	int sellValue(int player) const;
	bool usableWhileShapeshifted(const Stat* wielder = nullptr) const;
	char* getScrollLabel() const;
	const char* getTomeLabel() const;
	int getTomeSpellID() const;

	void apply(int player, Entity* entity);
	void applyLockpickToWall(int player, int x, int y) const;

	//Item usage functions.
	void applySkeletonKey(int player, Entity& entity);
	void applyLockpick(int player, Entity& entity);
	void applyOrb(int player, ItemType type, Entity& entity);
	void applyEmptyPotion(int player, Entity& entity);
	//-----ITEM COMPARISON FUNCTIONS-----
	/*
	 * Returns which weapon hits harder.
	 */
	static bool isThisABetterWeapon(const Item& newWeapon, const Item* weaponAlreadyHave);
	static bool isThisABetterArmor(const Item& newArmor, const Item* armorAlreadyHave); //Also checks shields.
	bool shouldItemStack(int player, bool ignoreStackLimit = false) const;
	bool shouldItemStackInShop(bool ignoreStackLimit = false);
	int getMaxStackLimit(int player) const;

	bool isShield() const;
	static bool doesItemProvideBeatitudeAC(ItemType type);
	bool doesPotionHarmAlliesOnThrown() const;

	Sint32 potionGetEffectHealth(Entity* my, Stat* myStats) const;
	Sint32 potionGetEffectDamage(Entity* my, Stat* myStats) const;
	Sint32 potionGetEffectDurationMinimum(Entity* my, Stat* myStats) const;
	Sint32 potionGetEffectDurationMaximum(Entity* my, Stat* myStats) const;
	Sint32 potionGetEffectDurationRandom(Entity* my, Stat* myStats) const;
	Sint32 potionGetCursedEffectDurationMinimum(Entity* my, Stat* myStats) const;
	Sint32 potionGetCursedEffectDurationMaximum(Entity* my, Stat* myStats) const;
	Sint32 potionGetCursedEffectDurationRandom(Entity* my, Stat* myStats) const;
	static int getBaseFoodSatiation(ItemType type);

	Sint32 getWeight() const;
	Sint32 getGoldValue() const;

	void foodTinGetDescriptionIndices(int* a, int* b, int* c) const;
	void foodTinGetDescription(std::string& cookingMethod, std::string& protein, std::string& sides) const;
	int foodGetPukeChance(Stat* eater) const;
	int getLootBagPlayer() const;
	int getLootBagNumItems() const;
	int getDuckPlayer() const;

	enum ItemBombPlacement : int
	{
		BOMB_FLOOR,
		BOMB_WALL,
		BOMB_CHEST,
		BOMB_DOOR,
		BOMB_COLLIDER
	};
	enum ItemBombFacingDirection : int
	{
		BOMB_UP,
		BOMB_NORTH,
		BOMB_EAST,
		BOMB_SOUTH,
		BOMB_WEST
	};
	enum ItemBombTriggerType : int
	{
		BOMB_TRIGGER_ENEMIES,
		BOMB_TELEPORT_RECEIVER,
		BOMB_TRIGGER_ALL
	};
	void applyBomb(Entity* parent, ItemType type, ItemBombPlacement placement, ItemBombFacingDirection dir, Entity* thrown, Entity* onEntity);
	void applyTinkeringCreation(Entity* parent, Entity* thrown);
	void applyDuck(Uint32 parentUid, real_t x, real_t y, Entity* hitentity, bool onLevelRespawn);
	bool unableToEquipDueToSwapWeaponTimer(const int player) const;
	bool tinkeringBotIsMaxHealth() const;
	bool isTinkeringItemWithThrownLimit() const;
	static void onItemIdentified(int player, Item* tempItem);
	static void itemFindUniqueAppearance(Item* tempItem, std::unordered_set<Uint32>& appearancesOfSimilarItems);
};
extern Uint32 itemuids;

// item generic
class ItemGeneric
{
	std::string item_name_identified;      // identified item name
	std::string item_name_unidentified;    // unidentified item name
public:
	int index;                  // world model
	int indexShort;				// short mob world model
	int fpindex;                // first person model
	int variations;             // number of model variations
	int weight;                 // weight per item
	int gold_value;                  // value per item
	list_t images;              // item image filenames (inventory)
	list_t surfaces;            // item image surfaces (inventory)
	Category category;          // item category
	int level;					// item level for random generation
	// equip slot that item can go in
	ItemEquippableSlot item_slot = ItemEquippableSlot::NO_EQUIP;
	std::map<std::string, Sint32> attributes;
	std::string tooltip = "tooltip_default";

	const char* getIdentifiedName() const { return item_name_identified.c_str(); }
	const char* getUnidentifiedName() const { return item_name_unidentified.c_str(); }
	void setIdentifiedName(std::string name) { item_name_identified = name; }
	void setUnidentifiedName(std::string name) { item_name_unidentified = name; }
	bool hasAttribute(std::string attribute)
	{
		if ( attributes.size() > 0 )
		{
			if ( attributes.find(attribute) != attributes.end() )
			{
				return true;
			}
			return false;
		}
		else
		{
			return false;
		}
	}
};
extern ItemGeneric items[NUMITEMS];

//----------Item usage functions----------
bool item_PotionWater(Item*& item, Entity* entity, Entity* usedBy);
bool item_PotionBooze(Item*& item, Entity* entity, Entity* usedBy, bool shouldConsumeItem = true);
bool item_PotionJuice(Item*& item, Entity* entity, Entity* usedBy);
bool item_PotionSickness(Item*& item, Entity* entity, Entity* usedBy);
bool item_PotionConfusion(Item*& item, Entity* entity, Entity* usedBy);
bool item_PotionGrease(Item*& item, Entity* entity, Entity* usedBy);
bool item_PotionCureAilment(Item*& item, Entity* entity, Entity* usedBy);
bool item_PotionBlindness(Item*& item, Entity* entity, Entity* usedBy);
bool item_PotionHealing(Item*& item, Entity* entity, Entity* usedBy, bool shouldConsumeItem = true);
bool item_PotionExtraHealing(Item*& item, Entity* entity, Entity* usedBy, bool shouldConsumeItem = true);
bool item_PotionRestoreMagic(Item*& item, Entity* entity, Entity* usedBy);
bool item_PotionInvisibility(Item*& item, Entity* entity, Entity* usedBy);
bool item_PotionLevitation(Item*& item, Entity* entity, Entity* usedBy);
bool item_PotionSpeed(Item*& item, Entity* entity, Entity* usedBy);
bool item_PotionStrength(Item*& item, Entity* entity, Entity* usedBy);
bool item_PotionAcid(Item*& item, Entity* entity, Entity* usedBy);
bool item_PotionUnstableStorm(Item*& item, Entity* entity, Entity* usedBy, Entity* thrownPotion);
bool item_PotionParalysis(Item*& item, Entity* entity, Entity* usedBy);
Entity* item_PotionPolymorph(Item*& item, Entity* entity, Entity* usedBy);
void item_ScrollMail(Item*& item, int player);
void item_ScrollIdentify(Item*& item, int player);
void item_ScrollLight(Item*& item, int player);
void item_ScrollBlank(Item*& item, int player);
void item_ScrollEnchantWeapon(Item*& item, int player);
void item_ScrollEnchantArmor(Item*& item, int player);
void item_ScrollRemoveCurse(Item*& item, int player);
bool item_ScrollFire(Item*& item, int player); // return true if exploded into fire.
void item_ScrollFood(Item*& item, int player);
void item_ScrollConjureArrow(Item*& item, int player);
void item_ScrollMagicMapping(Item*& item, int player);
void item_ScrollRepair(Item*& item, int player);
void item_ScrollDestroyArmor(Item*& item, int player);
void item_ScrollTeleportation(Item*& item, int player);
void item_ScrollSummon(Item*& item, int player);
void item_AmuletSexChange(Item*& item, int player);
void item_ToolTowel(Item*& item, int player);
void item_ToolTinOpener(Item*& item, int player);
void item_ToolMirror(Item*& item, int player);
Entity* item_ToolBeartrap(Item*& item, Entity* usedBy);
void item_Food(Item*& item, int player);
void item_FoodTin(Item*& item, int player);
void item_FoodAutomaton(Item*& item, int player);
void item_Spellbook(Item*& item, int player);
void item_ToolLootBag(Item*& item, int player);

//General functions.
Item* newItem(ItemType type, Status status, Sint16 beatitude, Sint16 count, Uint32 appearance, bool identified, list_t* inventory);
Item* uidToItem(Uint32 uid);
ItemType itemLevelCurveEntity(Entity& my, Category cat, int minLevel, int maxLevel, BaronyRNG& rng);
bool itemLevelCurvePostProcess(Entity* my, Item* item, BaronyRNG& rng, 
	int itemLevel = currentlevel
	, int* lastItemType = nullptr, int* lastItemSpellType = nullptr
);
ItemType itemLevelCurve(Category cat, int minLevel, int maxLevel, BaronyRNG& rng);
Item* newItemFromEntity(const Entity* entity, bool discardUid = false); //Make sure to call free(item). discardUid will free the new items uid if this is for temp purposes
Entity* dropItemMonster(Item* item, Entity* monster, Stat* monsterStats, Sint16 count = 1);
Item** itemSlot(Stat* myStats, Item* item);

enum Category itemCategory(const Item* item);
Sint32 itemModel(const Item* item, bool shortModel = false, Entity* creature = nullptr);
Sint32 itemModelFirstperson(const Item* item);
void consumeItem(Item*& item, int player); //NOTE: Items have to be unequipped before calling this function on them. NOTE: THIS CAN FREE THE ITEM POINTER. Sets item to nullptr if it does.
bool dropItem(Item* item, int player, const bool notifyMessage = true, const bool dropAll = false); // return true on free'd item
bool playerGreasyDropItem(const int player, Item* const item);
bool playerThrowDuck(const int player, Item* const item, int charge);
void useItem(Item* item, int player, Entity* usedBy = nullptr, bool unequipForDropping = false, bool serverCheckUse = false);
enum EquipItemResult : int
{
	EQUIP_ITEM_FAIL_CANT_UNEQUIP,
	EQUIP_ITEM_SUCCESS_NEWITEM,
	EQUIP_ITEM_SUCCESS_UPDATE_QTY,
	EQUIP_ITEM_SUCCESS_UNEQUIP
};
enum EquipItemSendToServerSlot : int
{
	EQUIP_ITEM_SLOT_WEAPON,
	EQUIP_ITEM_SLOT_SHIELD,
	EQUIP_ITEM_SLOT_MASK,
	EQUIP_ITEM_SLOT_HELM,
	EQUIP_ITEM_SLOT_GLOVES,
	EQUIP_ITEM_SLOT_BOOTS,
	EQUIP_ITEM_SLOT_BREASTPLATE,
	EQUIP_ITEM_SLOT_CLOAK,
	EQUIP_ITEM_SLOT_AMULET,
	EQUIP_ITEM_SLOT_RING
};
void playerTryEquipItemAndUpdateServer(const int player, Item* item, bool checkInventorySpaceForPaperDoll);
void clientSendEquipUpdateToServer(EquipItemSendToServerSlot slot, EquipItemResult equipType, int player,
	ItemType type, Status status, Sint16 beatitude, int count, Uint32 appearance, bool identified);
void clientUnequipSlotAndUpdateServer(const int player, EquipItemSendToServerSlot slot, Item* item);
void clientSendAppearanceUpdateToServer(const int player, Item* item, const bool onIdentify);
void clientSendItemTypeUpdateToServer(const int player, Item* item, ItemType prevItemType);
EquipItemResult equipItem(Item* item, Item** slot, int player, bool checkInventorySpaceForPaperDoll);
enum ItemStackResults : int
{
	ITEM_STACKING_ERROR,
	ITEM_DESTINATION_NOT_SAME_ITEM,
	ITEM_DESTINATION_STACK_IS_FULL,
	ITEM_ADDED_ENTIRELY_TO_DESTINATION_STACK,
	ITEM_ADDED_PARTIALLY_TO_DESTINATION_STACK,
	ITEM_ADDED_WITHOUT_NEEDING_STACK
};
struct ItemStackResult
{
	ItemStackResults resultType = ITEM_STACKING_ERROR;
	Item* itemToStackInto = nullptr;
};
// checks inventory order for stacking items (the first item in the list that is stackable will be returned)
ItemStackResult getItemStackingBehavior(const int player, Item* itemToCheck, Item* itemDestinationStack, int& newQtyForCheckedItem, int& newQtyForDestItem);
// checks chest inventory order for dropping all items into (the first item in the list that is stackable will be returned)
ItemStackResult getItemStackingBehaviorIntoChest(const int player, Item* itemToCheck, Item* itemDestinationStack, int& newQtyForCheckedItem, int& newQtyForDestItem);
void getItemEmptySlotStackingBehavior(const int player, Item& itemToCheck, int& newQtyForCheckedItem, int& newQtyForDestItem);
Item* itemPickup(int player, Item* item, Item* addToSpecificInventoryItem = nullptr, bool forceNewStack = false);
bool itemIsEquipped(const Item* item, int player);
bool shouldInvertEquipmentBeatitude(const Stat* wielder);
bool isItemEquippableInShieldSlot(const Item* item);
bool itemIsConsumableByAutomaton(const Item& item);

extern const real_t potionDamageSkillMultipliers[6];
extern const real_t thrownDamageSkillMultipliers[6];
extern Uint32 enchantedFeatherScrollSeed;
extern std::vector<int> enchantedFeatherScrollsShuffled;
static const std::vector<int> enchantedFeatherScrollsFixedList =
{
	SCROLL_BLANK,
	SCROLL_MAIL,
	SCROLL_DESTROYARMOR,
	SCROLL_DESTROYARMOR,
	SCROLL_DESTROYARMOR,
	SCROLL_FIRE,
	SCROLL_FIRE,
	SCROLL_FIRE,
	SCROLL_LIGHT,
	SCROLL_LIGHT,
	SCROLL_SUMMON,
	SCROLL_SUMMON,
	SCROLL_IDENTIFY,
	SCROLL_IDENTIFY,
	SCROLL_REMOVECURSE,
	SCROLL_CONJUREARROW,
	SCROLL_FOOD,
	SCROLL_FOOD,
	SCROLL_TELEPORTATION,
	SCROLL_TELEPORTATION,
	SCROLL_CHARGING,
	SCROLL_REPAIR,
	SCROLL_MAGICMAPPING,
	SCROLL_ENCHANTWEAPON,
	SCROLL_ENCHANTARMOR
};
static const int ENCHANTED_FEATHER_MAX_DURABILITY = 101;
static const int QUIVER_MAX_AMMO_QTY = 51;
static const int SCRAP_MAX_STACK_QTY = 101;
static const int THROWN_GEM_MAX_STACK_QTY = 9;
static const int MAGICSTAFF_SCEPTER_CHARGE_MAX = 101;
static const int TOME_APPEARANCE_MAX = 1024;

//-----ITEM COMPARISON FUNCS-----
/*
 * Only compares items of the same type.
 */
int itemCompare(const Item* item1, const Item* item2, bool checkAppearance, bool comparisonUsedForStacking = true);

/*
 * Returns true if potion is harmful to the player.
 */
bool isPotionBad(const Item& potion);
bool isRangedWeapon(const Item& item);
bool isRangedWeapon(const ItemType type);
bool isMeleeWeapon(const Item& item);
#ifdef __cplusplus
extern "C" {
#endif
bool itemIsThrowableTinkerTool(const Item* item);
#ifdef __cplusplus
}
#endif

void createCustomInventory(Stat* stats, int itemLimit, BaronyRNG& rng);
void copyItem(Item* itemToSet, const Item* itemToCopy);
bool swapMonsterWeaponWithInventoryItem(Entity* my, Stat* myStats, node_t* inventoryNode, bool moveStack, bool overrideCursed);
bool monsterUnequipSlot(Stat* myStats, Item** slot, Item* itemToUnequip);
bool monsterUnequipSlotFromCategory(Stat* myStats, Item** slot, Category cat);
node_t* itemNodeInInventory(const Stat* myStats, Sint32 itemToFind, Category cat, bool randomSlot = false);
node_t* spellbookNodeInInventory(const Stat* myStats, int spellIDToFind);
node_t* getRangedWeaponItemNodeInInventory(const Stat* myStats, bool includeMagicstaff);
node_t* getMeleeWeaponItemNodeInInventory(const Stat* myStats);
ItemType itemTypeWithinGoldValue(int cat, int minValue, int maxValue, BaronyRNG& rng);
bool itemSpriteIsQuiverThirdPersonModel(int sprite);
bool itemSpriteIsQuiverBaseThirdPersonModel(int sprite);
bool itemSpriteIsFociThirdPersonModel(const int sprite);
bool itemTypeIsQuiver(ItemType type);
bool itemTypeIsFoci(ItemType type);
bool itemTypeIsInstrument(ItemType type);
bool itemTypeIsThrownBall(ItemType type);
real_t rangedAttackGetSpeedModifier(const Stat* myStats);
bool rangedWeaponUseQuiverOnAttack(const Stat* myStats);
real_t getArtifactWeaponEffectChance(ItemType type, Stat& wielder, real_t* effectAmount);
void updateHungerMessages(Entity* my, Stat* myStats, Item* eaten);
bool playerCanSpawnMoreTinkeringBots(const Stat* myStats);
int maximumTinkeringBotsCanBeDeployed(const Stat* myStats);
extern bool overrideTinkeringLimit;
extern int decoyBoxRange;

// unique monster item appearance to avoid being dropped on death.
static const int MONSTER_ITEM_UNDROPPABLE_APPEARANCE = 1234567890;
static const int ITEM_TINKERING_APPEARANCE = 987654320;
static const int ITEM_GENERATED_QUIVER_APPEARANCE = 1122334455;

enum SpellbookColors
{
	SPELLBOOK_COLOR_THAUM_2,		//"items/images/SpellbookYellow.png",
	SPELLBOOK_COLOR_THAUM_3,		//"items/images/SpellbookWhite.png",
	SPELLBOOK_COLOR_THAUM_1,		//"items/images/SpellbookBlack.png",
	SPELLBOOK_COLOR_MYSTICISM_2,	//"items/images/SpellbookRed.png",
	SPELLBOOK_COLOR_SORCERY_1,		//"items/images/SpellbookBrown.png",
	SPELLBOOK_COLOR_SORCERY_3,		//"items/images/SpellbookOrange.png",
	SPELLBOOK_COLOR_MYSTICISM_1,	//"items/images/SpellbookGreen.png",
	SPELLBOOK_COLOR_SORCERY_2,		//"items/images/SpellbookBlue.png",
	SPELLBOOK_COLOR_MYSTICISM_3		//"items/images/SpellbookPurple.png"
};

int getItemVariationFromSpellbookOrTome(const Item& item);
#else
/*----------------------------------------------------------------------------*/
/*  C-COMPATIBLE SECTION                                                      */
/*----------------------------------------------------------------------------*/
#include "ccontainers.h"
#include "defs.h"

/* Forward declarations for types defined in other headers */
typedef struct Stat Stat;
typedef struct Entity Entity;
typedef struct ItemGeneric ItemGeneric;

/* ------------------------------------------------------------------ */
/* enums                                                               */
/* ------------------------------------------------------------------ */

#include "item_types.h"
#define NUMITEMS ITEM_ENUM_MAX

typedef enum Category
{
	WEAPON,
	ARMOR,
	AMULET,
	POTION,
	SCROLL,
	MAGICSTAFF,
	RING,
	SPELLBOOK,
	GEM,
	THROWN,
	TOOL,
	FOOD,
	BOOK,
	SPELL_CAT,
	TOME_SPELL,
	CATEGORY_MAX
} Category;

typedef enum Status
{
	BROKEN,
	DECREPIT,
	WORN,
	SERVICABLE,
	EXCELLENT
} Status;

typedef enum EquipmentType
{
	TYPE_NONE,
	TYPE_HELM,
	TYPE_HAT,
	TYPE_BREASTPIECE,
	TYPE_BOOTS,
	TYPE_SHIELD,
	TYPE_GLOVES,
	TYPE_CLOAK,
	TYPE_RING,
	TYPE_AMULET,
	TYPE_MASK,
	TYPE_SWORD,
	TYPE_AXE,
	TYPE_SPEAR,
	TYPE_MACE,
	TYPE_BOW,
	TYPE_PROJECTILE,
	TYPE_OFFHAND
} EquipmentType;

enum ItemEquippableSlot
{
	EQUIPPABLE_IN_SLOT_WEAPON,
	EQUIPPABLE_IN_SLOT_SHIELD,
	EQUIPPABLE_IN_SLOT_MASK,
	EQUIPPABLE_IN_SLOT_HELM,
	EQUIPPABLE_IN_SLOT_GLOVES,
	EQUIPPABLE_IN_SLOT_BOOTS,
	EQUIPPABLE_IN_SLOT_BREASTPLATE,
	EQUIPPABLE_IN_SLOT_CLOAK,
	EQUIPPABLE_IN_SLOT_AMULET,
	EQUIPPABLE_IN_SLOT_RING,
	NO_EQUIP
};

/* ------------------------------------------------------------------ */
/* C-compatible Item struct (no std:: members)                         */
/* ------------------------------------------------------------------ */
typedef struct Item
{
	ItemType type;
	Status status;

	Sint16 beatitude;
	Sint16 count;
	Uint32 appearance;
	bool identified;
	Uint32 uid;
	Sint32 x, y;
	Uint32 ownerUid;
	Uint32 interactNPCUid;
	bool forcedPickupByPlayer;
	bool isDroppable;
	bool playerSoldItemToShop;
	bool itemHiddenFromShop;
	bool notifyIcon;
	bool spellNotifyIcon;
	Uint8 itemRequireTradingSkillInShop;
	bool itemSpecialShopConsumable;

	node_t* node;
} Item;

extern Uint32 itemuids;

/* ItemGeneric forward-declared opaque (has std::string/std::map members) */
#ifdef __cplusplus
extern ItemGeneric items[];  /* actually NUMITEMS; sized by the C++ definition */
#endif

/* ------------------------------------------------------------------ */
/* Bomb placement enums (pulled out of Item class for C)               */
/* ------------------------------------------------------------------ */
enum ItemBombPlacement
{
	ITEM_BOMB_FLOOR,
	ITEM_BOMB_WALL,
	ITEM_BOMB_CHEST,
	ITEM_BOMB_DOOR,
	ITEM_BOMB_COLLIDER
};
enum ItemBombFacingDirection
{
	ITEM_BOMB_UP,
	ITEM_BOMB_NORTH,
	ITEM_BOMB_EAST,
	ITEM_BOMB_SOUTH,
	ITEM_BOMB_WEST
};
enum ItemBombTriggerType
{
	ITEM_BOMB_TRIGGER_ENEMIES,
	ITEM_BOMB_TELEPORT_RECEIVER,
	ITEM_BOMB_TRIGGER_ALL
};

enum EquipItemResult
{
	EQUIP_ITEM_FAIL_CANT_UNEQUIP,
	EQUIP_ITEM_SUCCESS_NEWITEM,
	EQUIP_ITEM_SUCCESS_UPDATE_QTY,
	EQUIP_ITEM_SUCCESS_UNEQUIP
};
enum EquipItemSendToServerSlot
{
	EQUIP_ITEM_SLOT_WEAPON,
	EQUIP_ITEM_SLOT_SHIELD,
	EQUIP_ITEM_SLOT_MASK,
	EQUIP_ITEM_SLOT_HELM,
	EQUIP_ITEM_SLOT_GLOVES,
	EQUIP_ITEM_SLOT_BOOTS,
	EQUIP_ITEM_SLOT_BREASTPLATE,
	EQUIP_ITEM_SLOT_CLOAK,
	EQUIP_ITEM_SLOT_AMULET,
	EQUIP_ITEM_SLOT_RING
};
enum ItemStackResults
{
	ITEM_STACKING_ERROR,
	ITEM_DESTINATION_NOT_SAME_ITEM,
	ITEM_DESTINATION_STACK_IS_FULL,
	ITEM_ADDED_ENTIRELY_TO_DESTINATION_STACK,
	ITEM_ADDED_PARTIALLY_TO_DESTINATION_STACK,
	ITEM_ADDED_WITHOUT_NEEDING_STACK
};
struct ItemStackResult
{
	enum ItemStackResults resultType;
	Item* itemToStackInto;
};

enum SpellbookColors
{
	SPELLBOOK_COLOR_THAUM_2,
	SPELLBOOK_COLOR_THAUM_3,
	SPELLBOOK_COLOR_THAUM_1,
	SPELLBOOK_COLOR_MYSTICISM_2,
	SPELLBOOK_COLOR_SORCERY_1,
	SPELLBOOK_COLOR_SORCERY_3,
	SPELLBOOK_COLOR_MYSTICISM_1,
	SPELLBOOK_COLOR_SORCERY_2,
	SPELLBOOK_COLOR_MYSTICISM_3
};

/* ------------------------------------------------------------------ */
/* C-compatible function prototypes                                    */
/* ------------------------------------------------------------------ */

/* Item methods → free functions taking Item* as first param */
char* Item_description(const Item* item);
char* Item_getName(const Item* item);

Sint32 Item_weaponGetAttack(const Item* item, const Stat* wielder);
Sint32 Item_armorGetAC(const Item* item, const Stat* wielder);
bool Item_canUnequip(Item* item, const Stat* wielder);
int Item_buyValue(const Item* item, int player);
int Item_sellValue(const Item* item, int player);
bool Item_usableWhileShapeshifted(const Item* item, const Stat* wielder);
char* Item_getScrollLabel(const Item* item);
const char* Item_getTomeLabel(const Item* item);
int Item_getTomeSpellID(const Item* item);

void Item_apply(Item* item, int player, Entity* entity);
void Item_applyLockpickToWall(const Item* item, int player, int x, int y);

void Item_applySkeletonKey(Item* item, int player, Entity* entity);
void Item_applyLockpick(Item* item, int player, Entity* entity);
void Item_applyOrb(Item* item, int player, ItemType type, Entity* entity);
void Item_applyEmptyPotion(Item* item, int player, Entity* entity);

bool Item_isThisABetterWeapon(const Item* newWeapon, const Item* weaponAlreadyHave);
bool Item_isThisABetterArmor(const Item* newArmor, const Item* armorAlreadyHave);
bool Item_shouldItemStack(const Item* item, int player, bool ignoreStackLimit);
bool Item_shouldItemStackInShop(Item* item, bool ignoreStackLimit);
int Item_getMaxStackLimit(const Item* item, int player);

bool Item_isShield(const Item* item);
bool Item_doesItemProvideBeatitudeAC(ItemType type);
bool Item_doesPotionHarmAlliesOnThrown(const Item* item);

Sint32 Item_potionGetEffectHealth(const Item* item, Entity* my, Stat* myStats);
Sint32 Item_potionGetEffectDamage(const Item* item, Entity* my, Stat* myStats);
Sint32 Item_potionGetEffectDurationMinimum(const Item* item, Entity* my, Stat* myStats);
Sint32 Item_potionGetEffectDurationMaximum(const Item* item, Entity* my, Stat* myStats);
Sint32 Item_potionGetEffectDurationRandom(const Item* item, Entity* my, Stat* myStats);
Sint32 Item_potionGetCursedEffectDurationMinimum(const Item* item, Entity* my, Stat* myStats);
Sint32 Item_potionGetCursedEffectDurationMaximum(const Item* item, Entity* my, Stat* myStats);
Sint32 Item_potionGetCursedEffectDurationRandom(const Item* item, Entity* my, Stat* myStats);
int Item_getBaseFoodSatiation(ItemType type);

Sint32 Item_getWeight(const Item* item);
Sint32 Item_getGoldValue(const Item* item);

void Item_foodTinGetDescriptionIndices(const Item* item, int* a, int* b, int* c);
void Item_foodTinGetDescription(const Item* item, char* cookingMethod, size_t cookingMethodSize, char* protein, size_t proteinSize, char* sides, size_t sidesSize);
int Item_foodGetPukeChance(const Item* item, Stat* eater);
int Item_getLootBagPlayer(const Item* item);
int Item_getLootBagNumItems(const Item* item);
int Item_getDuckPlayer(const Item* item);

void Item_applyBomb(Item* item, Entity* parent, ItemType type, enum ItemBombPlacement placement, enum ItemBombFacingDirection dir, Entity* thrown, Entity* onEntity);
void Item_applyTinkeringCreation(Item* item, Entity* parent, Entity* thrown);
void Item_applyDuck(Item* item, Uint32 parentUid, real_t x, real_t y, Entity* hitentity, bool onLevelRespawn);
bool Item_unableToEquipDueToSwapWeaponTimer(const Item* item, int player);
bool Item_tinkeringBotIsMaxHealth(const Item* item);
bool Item_isTinkeringItemWithThrownLimit(const Item* item);
void Item_onItemIdentified(int player, Item* tempItem);
void Item_itemFindUniqueAppearance(Item* tempItem, Uint32* appearances, size_t appearancesCount);

/* ItemGeneric accessors (C wrappers around C++ implementation) */
const char* ItemGeneric_getIdentifiedName(const ItemGeneric* gen);
const char* ItemGeneric_getUnidentifiedName(const ItemGeneric* gen);
void ItemGeneric_setIdentifiedName(ItemGeneric* gen, const char* name);
void ItemGeneric_setUnidentifiedName(ItemGeneric* gen, const char* name);
bool ItemGeneric_hasAttribute(ItemGeneric* gen, const char* attribute);

/* Item usage functions (Item** → Item** in C, no default args) */
bool item_PotionWater(Item** item, Entity* entity, Entity* usedBy);
bool item_PotionBooze(Item** item, Entity* entity, Entity* usedBy, bool shouldConsumeItem);
bool item_PotionJuice(Item** item, Entity* entity, Entity* usedBy);
bool item_PotionSickness(Item** item, Entity* entity, Entity* usedBy);
bool item_PotionConfusion(Item** item, Entity* entity, Entity* usedBy);
bool item_PotionGrease(Item** item, Entity* entity, Entity* usedBy);
bool item_PotionCureAilment(Item** item, Entity* entity, Entity* usedBy);
bool item_PotionBlindness(Item** item, Entity* entity, Entity* usedBy);
bool item_PotionHealing(Item** item, Entity* entity, Entity* usedBy, bool shouldConsumeItem);
bool item_PotionExtraHealing(Item** item, Entity* entity, Entity* usedBy, bool shouldConsumeItem);
bool item_PotionRestoreMagic(Item** item, Entity* entity, Entity* usedBy);
bool item_PotionInvisibility(Item** item, Entity* entity, Entity* usedBy);
bool item_PotionLevitation(Item** item, Entity* entity, Entity* usedBy);
bool item_PotionSpeed(Item** item, Entity* entity, Entity* usedBy);
bool item_PotionStrength(Item** item, Entity* entity, Entity* usedBy);
bool item_PotionAcid(Item** item, Entity* entity, Entity* usedBy);
bool item_PotionUnstableStorm(Item** item, Entity* entity, Entity* usedBy, Entity* thrownPotion);
bool item_PotionParalysis(Item** item, Entity* entity, Entity* usedBy);
Entity* item_PotionPolymorph(Item** item, Entity* entity, Entity* usedBy);
void item_ScrollMail(Item** item, int player);
void item_ScrollIdentify(Item** item, int player);
void item_ScrollLight(Item** item, int player);
void item_ScrollBlank(Item** item, int player);
void item_ScrollEnchantWeapon(Item** item, int player);
void item_ScrollEnchantArmor(Item** item, int player);
void item_ScrollRemoveCurse(Item** item, int player);
bool item_ScrollFire(Item** item, int player);
void item_ScrollFood(Item** item, int player);
void item_ScrollConjureArrow(Item** item, int player);
void item_ScrollMagicMapping(Item** item, int player);
void item_ScrollRepair(Item** item, int player);
void item_ScrollDestroyArmor(Item** item, int player);
void item_ScrollTeleportation(Item** item, int player);
void item_ScrollSummon(Item** item, int player);
void item_AmuletSexChange(Item** item, int player);
void item_ToolTowel(Item** item, int player);
void item_ToolTinOpener(Item** item, int player);
void item_ToolMirror(Item** item, int player);
Entity* item_ToolBeartrap(Item** item, Entity* usedBy);
void item_Food(Item** item, int player);
void item_FoodTin(Item** item, int player);
void item_FoodAutomaton(Item** item, int player);
void item_Spellbook(Item** item, int player);
void item_ToolLootBag(Item** item, int player);

/* General functions */
Item* newItem(ItemType type, Status status, Sint16 beatitude, Sint16 count, Uint32 appearance, bool identified, list_t* inventory);
Item* uidToItem(Uint32 uid);
ItemType itemLevelCurveEntity(Entity* my, Category cat, int minLevel, int maxLevel, BaronyRNG* rng);
bool itemLevelCurvePostProcess(Entity* my, Item* item, BaronyRNG* rng, int itemLevel, int* lastItemType, int* lastItemSpellType);
ItemType itemLevelCurve(Category cat, int minLevel, int maxLevel, BaronyRNG* rng);
Item* newItemFromEntity(const Entity* entity, bool discardUid);
Entity* dropItemMonster(Item* item, Entity* monster, Stat* monsterStats, Sint16 count);
Item** itemSlot(Stat* myStats, Item* item);

enum Category itemCategory(const Item* item);
Sint32 itemModel(const Item* item, bool shortModel, Entity* creature);
Sint32 itemModelFirstperson(const Item* item);
void consumeItem(Item** item, int player);
bool dropItem(Item* item, int player, bool notifyMessage, bool dropAll);
bool playerGreasyDropItem(int player, Item* const item);
bool playerThrowDuck(int player, Item* const item, int charge);
void useItem(Item* item, int player, Entity* usedBy, bool unequipForDropping, bool serverCheckUse);

void playerTryEquipItemAndUpdateServer(int player, Item* item, bool checkInventorySpaceForPaperDoll);
void clientSendEquipUpdateToServer(enum EquipItemSendToServerSlot slot, enum EquipItemResult equipType, int player, ItemType type, Status status, Sint16 beatitude, int count, Uint32 appearance, bool identified);
void clientUnequipSlotAndUpdateServer(int player, enum EquipItemSendToServerSlot slot, Item* item);
void clientSendAppearanceUpdateToServer(int player, Item* item, bool onIdentify);
void clientSendItemTypeUpdateToServer(int player, Item* item, ItemType prevItemType);
enum EquipItemResult equipItem(Item* item, Item** slot, int player, bool checkInventorySpaceForPaperDoll);

struct ItemStackResult getItemStackingBehavior(int player, Item* itemToCheck, Item* itemDestinationStack, int* newQtyForCheckedItem, int* newQtyForDestItem);
struct ItemStackResult getItemStackingBehaviorIntoChest(int player, Item* itemToCheck, Item* itemDestinationStack, int* newQtyForCheckedItem, int* newQtyForDestItem);
void getItemEmptySlotStackingBehavior(int player, Item* itemToCheck, int* newQtyForCheckedItem, int* newQtyForDestItem);
Item* itemPickup(int player, Item* item, Item* addToSpecificInventoryItem, bool forceNewStack);
bool itemIsEquipped(const Item* item, int player);
bool shouldInvertEquipmentBeatitude(const Stat* wielder);
bool isItemEquippableInShieldSlot(const Item* item);
bool itemIsConsumableByAutomaton(const Item* item);

extern const real_t potionDamageSkillMultipliers[6];
extern const real_t thrownDamageSkillMultipliers[6];
extern Uint32 enchantedFeatherScrollSeed;

static const int ENCHANTED_FEATHER_MAX_DURABILITY = 101;
static const int QUIVER_MAX_AMMO_QTY = 51;
static const int SCRAP_MAX_STACK_QTY = 101;
static const int THROWN_GEM_MAX_STACK_QTY = 9;
static const int MAGICSTAFF_SCEPTER_CHARGE_MAX = 101;
static const int TOME_APPEARANCE_MAX = 1024;

static const int ENCHANTED_FEATHER_SCROLLS_FIXED_LIST[] =
{
	SCROLL_BLANK,
	SCROLL_MAIL,
	SCROLL_DESTROYARMOR,
	SCROLL_DESTROYARMOR,
	SCROLL_DESTROYARMOR,
	SCROLL_FIRE,
	SCROLL_FIRE,
	SCROLL_FIRE,
	SCROLL_LIGHT,
	SCROLL_LIGHT,
	SCROLL_SUMMON,
	SCROLL_SUMMON,
	SCROLL_IDENTIFY,
	SCROLL_IDENTIFY,
	SCROLL_REMOVECURSE,
	SCROLL_CONJUREARROW,
	SCROLL_FOOD,
	SCROLL_FOOD,
	SCROLL_TELEPORTATION,
	SCROLL_TELEPORTATION,
	SCROLL_CHARGING,
	SCROLL_REPAIR,
	SCROLL_MAGICMAPPING,
	SCROLL_ENCHANTWEAPON,
	SCROLL_ENCHANTARMOR
};

/* Item comparison */
int itemCompare(const Item* item1, const Item* item2, bool checkAppearance, bool comparisonUsedForStacking);

bool isPotionBad(const Item* potion);
bool isRangedWeapon(const Item* item);
bool isRangedWeaponByType(ItemType type);
bool isMeleeWeapon(const Item* item);
bool itemIsThrowableTinkerTool(const Item* item);

void createCustomInventory(Stat* stats, int itemLimit, BaronyRNG* rng);
void copyItem(Item* itemToSet, const Item* itemToCopy);
bool swapMonsterWeaponWithInventoryItem(Entity* my, Stat* myStats, node_t* inventoryNode, bool moveStack, bool overrideCursed);
bool monsterUnequipSlot(Stat* myStats, Item** slot, Item* itemToUnequip);
bool monsterUnequipSlotFromCategory(Stat* myStats, Item** slot, Category cat);
node_t* itemNodeInInventory(const Stat* myStats, Sint32 itemToFind, Category cat, bool randomSlot);
node_t* spellbookNodeInInventory(const Stat* myStats, int spellIDToFind);
node_t* getRangedWeaponItemNodeInInventory(const Stat* myStats, bool includeMagicstaff);
node_t* getMeleeWeaponItemNodeInInventory(const Stat* myStats);
ItemType itemTypeWithinGoldValue(int cat, int minValue, int maxValue, BaronyRNG* rng);
bool itemSpriteIsQuiverThirdPersonModel(int sprite);
bool itemSpriteIsQuiverBaseThirdPersonModel(int sprite);
bool itemSpriteIsFociThirdPersonModel(int sprite);
bool itemTypeIsQuiver(ItemType type);
bool itemTypeIsFoci(ItemType type);
bool itemTypeIsInstrument(ItemType type);
bool itemTypeIsThrownBall(ItemType type);
real_t rangedAttackGetSpeedModifier(const Stat* myStats);
bool rangedWeaponUseQuiverOnAttack(const Stat* myStats);
real_t getArtifactWeaponEffectChance(ItemType type, Stat* wielder, real_t* effectAmount);
void updateHungerMessages(Entity* my, Stat* myStats, Item* eaten);
bool playerCanSpawnMoreTinkeringBots(const Stat* myStats);
int maximumTinkeringBotsCanBeDeployed(const Stat* myStats);
extern bool overrideTinkeringLimit;
extern int decoyBoxRange;

static const int MONSTER_ITEM_UNDROPPABLE_APPEARANCE = 1234567890;
static const int ITEM_TINKERING_APPEARANCE = 987654320;
static const int ITEM_GENERATED_QUIVER_APPEARANCE = 1122334455;

int getItemVariationFromSpellbookOrTome(const Item* item);

#endif /* __cplusplus */
