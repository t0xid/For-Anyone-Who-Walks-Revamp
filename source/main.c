#include <3ds.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/stat.h>
#include "extdata.h"
#include "persistantData.h"

bool fixCoins(u16 change, u16 amt, bool iswithdraw);

int main()
{
	int debug_thing = 0;

	gfxInitDefault();
	consoleInit(GFX_BOTTOM, NULL);
	extdataInit();

	typedef struct {
		u8 day;
		u8 month;
		u16 year;
	}date;
	date today = {getDay(), getMonth(), getYear()};

	u32 todaysteps = getDaySteps();
	u16 todaycoins = getDayCoins();

	/* hold select on startup for debug info */
	hidScanInput();
	u32 kHeld = hidKeysHeld();
	if (kHeld & KEY_SELECT || debug_thing) {
		printf("\n\n\n");
		printf("    Step count: 0x%08x\n\n    Day's steps: 0x%08x\n\n\n", (unsigned int)getSteps(), (unsigned int)todaysteps);
		printf("    Coin count: %d\n\n    Day's Coins: %d\n\n", (unsigned int)getCoins(), (unsigned int)todaycoins);
		printf("    D M Y: 0x%02x 0x%02x 0x%04x\n\n", today.day, today.month, today.year);
		/* printf("\n\n    Press Start to exit.\n"); */
	}


	/* TEST DATA */
	//today.day = 14; today.month=8; today.year=2015;
	//todaysteps = 26200;
	//todaycoins = 0;
	/* REMOVE FOR ACTUAL 3DS */

	Result res = mydataInit(today.day, today.month, today.year);

	// has to be after you init data otherwise it defaults to 0 and fails
	u16 bankcoins = getStoredBankCoins();

	if (debug_thing) {
		printf("mydataInit() returned %d\n", (int)res);
		switch (res)
		{
			// file changed, date changed
			case 0:
				printf("file changed, updating...\n");
				break;

			// files the same
			case 1:
				printf("no change...\n");
				break;

			// `cache.bin` doesnt exist
			case 2:
				printf("Created cache.bin...\n");
				break;
		}
	}

	if (res == 1) {
		u16 storedcoins = getStoredCoins();
		printf("today coins: %d\nstored coins: %d\n", (unsigned int)todaycoins, (unsigned int)storedcoins);
		printf("coins in bank: %d\n", bankcoins);
		todaycoins = (storedcoins > todaycoins) ? storedcoins : todaycoins;
	}
	consoleInit(GFX_TOP, GFX_LEFT);
	unsigned int correctcoins = (todaysteps / 100);
	bool slighted, bankactive;

	// current coin count
	printf("\n\n Current Coin Count: \x1b[33m%d\x1b[0m \n", getCoins());

	if (correctcoins > todaycoins) {
		slighted = true;
		printf("\x1b[0m\n\n You've taken \x1b[2m%d\x1b[0m steps but only earned\n \x1b[31m%d\x1b[0m coins for today! How would you like to\n have another \x1b[32m%d\x1b[0m coins to make up for it?\n\n [A] Sure!\n\n [START] It's okay, just take me back home.\n", (unsigned int)todaysteps, todaycoins, correctcoins - todaycoins);
	} else {
		slighted = false;
		printf("\x1b[0m\n\n You've taken \x1b[2m%d\x1b[0m steps and earned\n \x1b[32m%d\x1b[0m coins for today.\n\n Come back later.\n\n Press [START] to exit.\n", (unsigned int)todaysteps, todaycoins);
	}

	// bank withdraw
	if (bankcoins > 0) {
		printf("------------------------\n");
		printf("\n You have \x1b[36m%d\x1b[0m coins stored in the bank.\n\n Press [X] to withdraw to max.\n", bankcoins);
		bankactive = true;
	} else {
		bankactive = false;
	}

	// Main loop
	while (aptMainLoop()) {
		gspWaitForVBlank();
		hidScanInput();

		u32 kDown = hidKeysDown();

		if (kDown & KEY_START) break; // break in order to return to hbmenu

		if (slighted && kDown & KEY_A) {
			printf("------------------------\n");
			printf("\n Applying...");
			if (fixCoins(correctcoins - todaycoins, correctcoins, false)) {
				printf("...Applied!\n\n Press [START] to exit.\n");
				slighted = false;
			} else {
				printf("...Error!\n\n Press [START] to exit.\n");
			}
		}

		if (bankactive && kDown & KEY_X) {
			printf("------------------------\n");
			printf("\n Withdrawing...");
			if (fixCoins(bankcoins, correctcoins, true)) {
				printf("...Withdrawn!\n\n Press [START] to exit.\n");
				bankactive = false;
			} else {
				printf("...Error!\n\n Press [START] to exit.\n");
			}
		}

		if (kDown) {
			// update new coin count
			printf("\x1b[3;2HCurrent Coin Count: \x1b[33m%d\x1b[0m \n\x1b[17;0H", getCoins());
		}
	}

	gfxExit();
	extdataExit();
	mydataExit();
	return 0;
}

/*
 * Increase play coins by amt
 * Set data file's play coin count to amt
 */
bool fixCoins(u16 change, u16 amt, bool iswithdraw) {
	Result res;
	bool bankaction = false;

	u16 overfillcheck = getCoins() + change;
	if (overfillcheck > 300) {
		u16 bankdeposit = overfillcheck - 300;
		printf("\n\n \x1b[31m[!]\x1b[0m OVERFLOW \x1b[31m[!]\x1b[0m \n storing \x1b[36m%d\x1b[0m coins in the bank...\n\n ", bankdeposit);
	
		if (iswithdraw) {
			setStoredBankCoins(getStoredBankCoins() + bankdeposit - change);
			bankaction = true;
		} else {
			setStoredBankCoins(getStoredBankCoins() + bankdeposit);
			bankaction = true;
		}
	}

	if (iswithdraw && !bankaction) {
		setStoredBankCoins(getStoredBankCoins() - change);
	}

	res = changeCoins((s16)change);
	if (res != 0) return false;

	setStoredCoins(amt);
	/* if (res != 0) return false; */

	return true;
}
