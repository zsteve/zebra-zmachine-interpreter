#ifndef ZQUETZAL_H
#define ZQUETZAL_H

#include "../../zstack/zstack/zstack.h"
#include "../../zmemory/zmemory/zobject.h"
#include "../../ztext/ztext/ztext.h"
#include "../../zinout/zinout/zinout.h"
#include "../../zdictionary/zdictionary/zdictionary.h"

class ZQuetzalSave{
	public:
	enum formID{
		IFHD,
		CMEM,
		UMEM,
		STKS,
		INTD,
		AUTH,
		_C__,
		ANNO
	};

	ZQuetzalSave(ZMemory& zMem, ZStack& zStack);
	~ZQuetzalSave();

	zbyte* writeSaveGame();
	void readSaveGame(zbyte* saveData);
	zbyte* compressData(zbyte* data, ulong size, int* outSize);
	private:
	ZMemory& zMem;
	ZStack& zStack;


	formID getFormID(char* formChunk);
	enum formID;

	zbyte* dynMem;
	int dynMemSize;
	zbyte* stackMem;
	int stackMemSize;
	protected:
};

#endif
