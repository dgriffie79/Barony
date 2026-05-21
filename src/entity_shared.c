/*-------------------------------------------------------------------------------

	BARONY
	File: entity_shared.c
	Desc: C-compatible functions shared between editor.exe and barony.exe

	Copyright 2013-2016 (c) Turning Wheel LLC, all rights reserved.
	See LICENSE for details.

-------------------------------------------------------------------------------*/

#include "entity.h"

int editorSpriteTypeToMonster(Sint32 sprite)
{
	Monster monsterType = NOTHING;
	switch ( sprite )
	{
	case 27: monsterType = HUMAN; break;
	case 30: monsterType = TROLL; break;
	case 35: monsterType = SHOPKEEPER; break;
	case 36: monsterType = GOBLIN; break;
	case 48: monsterType = SPIDER; break;
	case 62: monsterType = LICH; break;
	case 70: monsterType = GNOME; break;
	case 71: monsterType = DEVIL; break;
	case 75: monsterType = DEMON; break;
	case 76: monsterType = CREATURE_IMP; break;
	case 77: monsterType = MINOTAUR; break;
	case 78: monsterType = SCORPION; break;
	case 79: monsterType = SLIME; break;
	case 193: monsterType = SLIME; break;
	case 194: monsterType = SLIME; break;
	case 195: monsterType = SLIME; break;
	case 196: monsterType = SLIME; break;
	case 197: monsterType = SLIME; break;
	case 80: monsterType = SUCCUBUS; break;
	case 81: monsterType = RAT; break;
	case 82: monsterType = GHOUL; break;
	case 83: monsterType = SKELETON; break;
	case 84: monsterType = KOBOLD; break;
	case 85: monsterType = SCARAB; break;
	case 86: monsterType = CRYSTALGOLEM; break;
	case 87: monsterType = INCUBUS; break;
	case 88: monsterType = VAMPIRE; break;
	case 89: monsterType = SHADOW; break;
	case 90: monsterType = COCKATRICE; break;
	case 91: monsterType = INSECTOID; break;
	case 92: monsterType = GOATMAN; break;
	case 93: monsterType = AUTOMATON; break;
	case 94: monsterType = LICH_ICE; break;
	case 95: monsterType = LICH_FIRE; break;
	case 163: monsterType = SENTRYBOT; break;
	case 164: monsterType = SPELLBOT; break;
	case 165: monsterType = DUMMYBOT; break;
	case 166: monsterType = GYROBOT; break;
	case 188: monsterType = BAT_SMALL; break;
	case 189: monsterType = BUGBEAR; break;
	case 204: monsterType = DRYAD; break;
	case 205: monsterType = MYCONID; break;
	case 206: monsterType = SALAMANDER; break;
	case 207: monsterType = GREMLIN; break;
	case 246: monsterType = REVENANT_SKULL; break;
	case 247: monsterType = MONSTER_ADORCISED_WEAPON; break;
	default:
		break;
	}
	return (int)monsterType;
}

int checkSpriteType(Sint32 sprite)
{
	switch ( sprite )
	{
	case 71:
	case 70:
	case 62:
	case 48:
	case 36:
	case 35:
	case 30:
	case 27:
	case 10:
	case 83:
	case 84:
	case 85:
	case 86:
	case 87:
	case 88:
	case 89:
	case 90:
	case 91:
	case 92:
	case 93:
	case 94:
	case 95:
	case 75:
	case 76:
	case 77:
	// to test case 37
	case 37:
	case 78:
	case 79:
	case 80:
	case 81:
	case 82:
	case 163:
	case 164:
	case 165:
	case 166:
	case 188:
	case 189:
	case 193:
	case 194:
	case 195:
	case 196:
	case 197:
	case 204:
	case 205:
	case 206:
	case 207:
	case 246:
	case 247:
		//monsters
		return 1;
		break;
	case 21:
		//chest
		return 2;
		break;
	case 8:
		//items
		return 3;
		break;
	case 97:
		//summon trap
		return 4;
		break;
	case 106:
		//power crystal
		return 5;
		break;
	case 115:
		// lever timer
		return 6;
	case 102:
	case 103:
	case 104:
	case 105:
		//boulder traps
		return 7;
		break;
	case 116:
		//pedestal
		return 8;
		break;
	case 118:
		//teleporter
		return 9;
		break;
	case 119:
		//ceiling tile model
		return 10;
		break;
	case 120:
		//magic ceiling trap
		return 11;
		break;
	case 121:
	case 122:
	case 123:
	case 124:
	case 125:
	case 60:
		// general furniture/misc.
		return 12;
		break;
	case 127:
		// floor decoration
		return 13;
		break;
	case 130:
		// sound source
		return 14;
	case 131:
		// light source
		return 15;
	case 132:
		// text source
		return 16;
	case 133:
		// signal modifier
		return 17;
	case 161:
		// custom exit
		return 18;
	case 59:
		// table
		return 19;
	case 162: 
		// readablebook
		return 20;
	case 2:
	case 3:
		return 21;
	case 19:
	case 20:
	case 113:
	case 114:
		return 22;
	case 1:
		return 23;
	case 169:
		// statue
		return 24;
	case 177:
		// teleport shrine
		return 25;
	case 178:
		// generic spell shrine
		return 26;
	case 179:
		return 27;
	case 185:
	case 186:
	case 187:
		// AND gate
		return 28;
	case 33:
	case 34:
		// act trap
		return 29;
	case 208:
	case 209:
	case 210:
	case 211:
		// wall locks
		return 30;
	case 212:
	case 213:
	case 214:
	case 215:
		// wall buttons
		return 31;
	case 217:
	case 218:
		// iron doors
		return 32;
	case 220:
		// wind
		return 33;
	default:
		return 0;
		break;
	}

	return 0;
}
