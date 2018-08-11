#ifndef _KEYBOARDINPUT_6769B867_1CE0_487F_BB6B_21F39544B8F8_
#define _KEYBOARDINPUT_6769B867_1CE0_487F_BB6B_21F39544B8F8_

void SendCtrlW()
{
	INPUT ip[4];
	SecureZeroMemory(ip, sizeof(INPUT)*4);
	ip[0].type = INPUT_KEYBOARD;
	ip[0].ki.wVk = VK_CONTROL;
	ip[1].type = INPUT_KEYBOARD;
	ip[1].ki.wVk = 0x57; //W key
	ip[2].type = INPUT_KEYBOARD;
	ip[2].ki.wVk = 0x57; //W key
	ip[2].ki.dwFlags = KEYEVENTF_KEYUP;
	ip[3].type = INPUT_KEYBOARD;
	ip[3].ki.wVk = VK_CONTROL;
	ip[3].ki.dwFlags = KEYEVENTF_KEYUP;
	SendInput(4, ip, sizeof(INPUT));
}

void ChromeCloseLastTab()
{
	//open new tab, switch to previous tab, close previous tab
//not working as expected with Opera
	INPUT ip[12];
	SecureZeroMemory(ip, sizeof(INPUT)*12);
	//Ctr + T
	ip[0].type = INPUT_KEYBOARD;
	ip[0].ki.wVk = VK_CONTROL;
	ip[1].type = INPUT_KEYBOARD;
	ip[1].ki.wVk = 0x54; //T key
	ip[2].type = INPUT_KEYBOARD;
	ip[2].ki.wVk = 0x54; //T key
	ip[2].ki.dwFlags = KEYEVENTF_KEYUP;
	ip[3].type = INPUT_KEYBOARD;
	ip[3].ki.wVk = VK_CONTROL;
	ip[3].ki.dwFlags = KEYEVENTF_KEYUP;
	//Ctr + Tab
	ip[4].type = INPUT_KEYBOARD;
	ip[4].ki.wVk = VK_CONTROL;
	ip[5].type = INPUT_KEYBOARD;
	ip[5].ki.wVk = VK_TAB;
	ip[6].type = INPUT_KEYBOARD;
	ip[6].ki.wVk = VK_TAB;
	ip[6].ki.dwFlags = KEYEVENTF_KEYUP;
	ip[7].type = INPUT_KEYBOARD;
	ip[7].ki.wVk = VK_CONTROL;
	ip[7].ki.dwFlags = KEYEVENTF_KEYUP;
	//Ctr + W
	ip[8].type = INPUT_KEYBOARD;
	ip[8].ki.wVk = VK_CONTROL;
	ip[9].type = INPUT_KEYBOARD;
	ip[9].ki.wVk = 0x57; //W key
	ip[10].type = INPUT_KEYBOARD;
	ip[10].ki.wVk = 0x57; //W key
	ip[10].ki.dwFlags = KEYEVENTF_KEYUP;
	ip[11].type = INPUT_KEYBOARD;
	ip[11].ki.wVk = VK_CONTROL;
	ip[11].ki.dwFlags = KEYEVENTF_KEYUP;
	SendInput(12, ip, sizeof(INPUT));
}




#endif//_KEYBOARDINPUT_6769B867_1CE0_487F_BB6B_21F39544B8F8_
