/*
	Neutrino-GUI  -   DBoxII-Project

	Homepage: http://dbox.cyberphoria.org/

	License: GPL

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/


#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <global.h>
#include <neutrino.h>

#include <driver/rcinput.h>

#include "color.h"
#include "motorcontrol.h"

#include "widget/menue.h"
#include "widget/messagebox.h"

#include "system/settings.h"

CMotorControl::CMotorControl()
{
	satfindpid = -1;
	
	frameBuffer = CFrameBuffer::getInstance();
	
	width = 420;
	hheight = g_Fonts->menu_title->getHeight();
	mheight = g_Fonts->menu->getHeight();
	height = hheight + (19 * mheight);
	if (height > 576) height = 576;
	x = ((720 - width) >> 1);
	y = (576 - height) >> 1;
	
	stepSize = 1; //default: 1 step
	stepMode = STEP_MODE_TIMED;
	installerMenue = false;
	motorPosition = 1;
	satellitePosition = 0;
	stepDelay = 10;
}

int CMotorControl::exec(CMenuTarget* parent, string)
{
	uint msg;
	uint data;
	bool istheend = false;
	
	if (!frameBuffer->getActive())
		return menu_return::RETURN_EXIT_ALL;
	
	if (parent)
		parent->hide();
		
	startSatFind();
		
	paint();
	paintMenu();
	paintStatus();

	while (!istheend)
	{

		unsigned long long timeoutEnd = g_RCInput->calcTimeoutEnd_MS(250);
		msg = CRCInput::RC_nokey;

		while (!(msg == CRCInput::RC_timeout) && (!(msg == CRCInput::RC_home)))
		{
			g_RCInput->getMsgAbsoluteTimeout(&msg, &data, &timeoutEnd);

			if (installerMenue)
			{
				switch(msg)
				{
					case CRCInput::RC_ok:
					case CRCInput::RC_0:
						printf("[motorcontrol] 0 key received... goto userMenue\n");
						installerMenue = false;
						paintMenu();
						paintStatus();
						break;
						
					case CRCInput::RC_1:
					case CRCInput::RC_right:
						printf("[motorcontrol] left/1 key received... drive/Step motor west, stepMode: %d\n", stepMode);
						motorStepWest();
						paintStatus();
						break;
					
					case CRCInput::RC_red:
					case CRCInput::RC_2:
						printf("[motorcontrol] 2 key received... halt motor\n");
						g_Zapit->sendMotorCommand(0xE0, 0x31, 0x60, 0, 0, 0);
						break;

					case CRCInput::RC_3:
					case CRCInput::RC_left:
						printf("[motorcontrol] right/3 key received... drive/Step motor east, stepMode: %d\n", stepMode);
						motorStepEast();
						paintStatus();
						break;
						
					case CRCInput::RC_4:
						printf("[motorcontrol] 4 key received... set west (soft) limit\n");
						g_Zapit->sendMotorCommand(0xE1, 0x31, 0x67, 0, 0, 0);
						break;
						
					case CRCInput::RC_5:
						printf("[motorcontrol] 5 key received... disable (soft) limits\n");
						g_Zapit->sendMotorCommand(0xE0, 0x31, 0x63, 0, 0, 0);
						break;
					
					case CRCInput::RC_6:
						printf("[motorcontrol] 6 key received... set east (soft) limit\n");
						g_Zapit->sendMotorCommand(0xE1, 0x31, 0x66, 0, 0, 0);
						break;
					
					case CRCInput::RC_7:
						printf("[motorcontrol] 7 key received... goto reference position\n");
						g_Zapit->sendMotorCommand(0xE0, 0x31, 0x6B, 1, 0, 0);
						satellitePosition = 0;
						paintStatus();
						break;
					
					case CRCInput::RC_8:
						printf("[motorcontrol] 8 key received... enable (soft) limits\n");
						g_Zapit->sendMotorCommand(0xE0, 0x31, 0x6A, 1, 0, 0);
						break;
					
					case CRCInput::RC_9:
						printf("[motorcontrol] 9 key received... (re)-calculate positions\n");
						g_Zapit->sendMotorCommand(0xE0, 0x31, 0x6F, 1, 0, 0);
						break;
					
					case CRCInput::RC_plus:
					case CRCInput::RC_up:
						printf("[motorcontrol] up key received... increase satellite position: %d\n", ++motorPosition);
						satellitePosition = 0;
						paintStatus();
						break;
					
					case CRCInput::RC_minus:
					case CRCInput::RC_down:
						if (motorPosition > 1) motorPosition--;
						printf("[motorcontrol] down key received... decrease satellite position: %d\n", motorPosition);
						satellitePosition = 0;
						paintStatus();
						break;
					
					case CRCInput::RC_blue:
						if (++stepMode > 2) 
							stepMode = 0;
						if (stepMode == STEP_MODE_OFF)
							satellitePosition = 0;
						printf("[motorcontrol] red key received... toggle stepmode on/off: %d\n", stepMode);
						paintStatus();
						break;
					
					default:
						//printf("[motorcontrol] message received...\n");
						if ((msg >= CRCInput::RC_WithData) && (msg < CRCInput::RC_WithData + 0x10000000)) 
							delete (unsigned char*) data;
						break;
				}
			}
			else
			{
				switch(msg)
				{
					case CRCInput::RC_ok:
					case CRCInput::RC_0:
						printf("[motorcontrol] 0 key received... goto installerMenue\n");
						installerMenue = true;
						paintMenu();
						paintStatus();
						break;
						
					case CRCInput::RC_1:
					case CRCInput::RC_right:
						printf("[motorcontrol] left/1 key received... drive/Step motor west, stepMode: %d\n", stepMode);
						motorStepWest();
						paintStatus();
						break;
					
					case CRCInput::RC_red:
					case CRCInput::RC_2:
						printf("[motorcontrol] 2 key received... halt motor\n");
						g_Zapit->sendMotorCommand(0xE0, 0x31, 0x60, 0, 0, 0);
						break;

					case CRCInput::RC_3:
					case CRCInput::RC_left:
						printf("[motorcontrol] right/3 key received... drive/Step motor east, stepMode: %d\n", stepMode);
						motorStepEast();
						paintStatus();
						break;
					
					case CRCInput::RC_green:
					case CRCInput::RC_5:
						printf("[motorcontrol] 5 key received... store present satellite number: %d\n", motorPosition);
						g_Zapit->sendMotorCommand(0xE0, 0x31, 0x6A, 1, motorPosition, 0);
						break;
					
					case CRCInput::RC_6:
						if (stepSize < 0x7F) stepSize++;
						printf("[motorcontrol] 6 key received... increase Step size: %d\n", stepSize);
						paintStatus();
						break;
					
					case CRCInput::RC_yellow:
					case CRCInput::RC_7:
						printf("[motorcontrol] 7 key received... goto satellite number: %d\n", motorPosition);
						g_Zapit->sendMotorCommand(0xE0, 0x31, 0x6B, 1, motorPosition, 0);
						satellitePosition = 0;
						paintStatus();
						break;
					
					case CRCInput::RC_9:
						if (stepSize > 1) stepSize--;
						printf("[motorcontrol] 9 key received... decrease Step size: %d\n", stepSize);
						paintStatus();
						break;
					
					case CRCInput::RC_plus:
					case CRCInput::RC_up:
						printf("[motorcontrol] up key received... increase satellite position: %d\n", ++motorPosition);
						satellitePosition = 0;
						paintStatus();
						break;
					
					case CRCInput::RC_minus:
					case CRCInput::RC_down:
						if (motorPosition > 1) motorPosition--;
						printf("[motorcontrol] down key received... decrease satellite position: %d\n", motorPosition);
						satellitePosition = 0;
						paintStatus();
						break;
					
					case CRCInput::RC_blue:
						if (++stepMode > 2) 
							stepMode = 0;
						if (stepMode == STEP_MODE_OFF)
							satellitePosition = 0;
						printf("[motorcontrol] red key received... toggle stepmode on/off: %d\n", stepMode);
						paintStatus();
						break;
					
					default:
						//printf("[motorcontrol] message received...\n");
						if ((msg >= CRCInput::RC_WithData) && (msg < CRCInput::RC_WithData + 0x10000000)) 
							delete (unsigned char*) data;
						break;
				}
			}
		}
		
		istheend = (msg == CRCInput::RC_home);
	}
	
	hide();

	return menu_return::RETURN_REPAINT;
}

void CMotorControl::motorStepWest(void)
{
	printf("[motorcontrol] motorStepWest\n");
	switch(stepMode)
	{
		case STEP_MODE_ON:
			g_Zapit->sendMotorCommand(0xE0, 0x31, 0x69, 1, (-1 * stepSize), 0);
			satellitePosition += stepSize;
			break;
		case STEP_MODE_TIMED:
			g_Zapit->sendMotorCommand(0xE0, 0x31, 0x69, 1, 40, 0);
			usleep(stepSize * stepDelay * 1000);
			g_Zapit->sendMotorCommand(0xE0, 0x31, 0x60, 0, 0, 0); //halt motor
			satellitePosition += stepSize;
			break;
		default:
			g_Zapit->sendMotorCommand(0xE0, 0x31, 0x69, 1, 40, 0);
	}
}	

void CMotorControl::motorStepEast(void)
{
	printf("[motorcontrol] motorStepEast\n");
	switch(stepMode)
	{
		case STEP_MODE_ON:
			g_Zapit->sendMotorCommand(0xE0, 0x31, 0x68, 1, (-1 * stepSize), 0);
			satellitePosition -= stepSize;
			break;
		case STEP_MODE_TIMED:
			g_Zapit->sendMotorCommand(0xE0, 0x31, 0x68, 1, 40, 0);
			usleep(stepSize * stepDelay * 1000);
			g_Zapit->sendMotorCommand(0xE0, 0x31, 0x60, 0, 0, 0); //halt motor
			satellitePosition -= stepSize;
			break;
		default:
			g_Zapit->sendMotorCommand(0xE0, 0x31, 0x68, 1, 40, 0);
	}
}

void CMotorControl::hide()
{
	frameBuffer->paintBackgroundBoxRel(x, y, width, height + 20);
	stopSatFind();
}

void CMotorControl::paintLine(char * txt, char * icon)
{
	ypos += mheight;
	frameBuffer->paintBoxRel(x, ypos - mheight, width, hheight, COL_MENUCONTENT);
	g_Fonts->menu->RenderString(x + 10, ypos, width - 10, txt, COL_MENUCONTENT);
}

void CMotorControl::paintStatus()
{
	char buf[256];
	char buf2[256];
	
	ypos = ypos_status;
	paintLine("------ Motor Control Settings ------", NULL);
	
	buf[0] = buf2[0] = 0;
	strcat(buf, "(a) Motor Position: ");
	sprintf(buf2, "%d", motorPosition);
	strcat(buf, buf2);
	paintLine(buf, NULL);
	
	buf[0] = buf2[0] = 0;
	strcat(buf, "(b) Movement: ");
	switch(stepMode)
	{
		case STEP_MODE_ON:
			strcat(buf, "Step Mode");
			break;
		case STEP_MODE_OFF:
			strcat(buf, "Drive Mode");
			break;
		case STEP_MODE_TIMED:
			strcat(buf, "Timed Step Mode");
			break;
	}
	paintLine(buf, NULL);
	
	buf[0] = buf2[0] = 0;
	strcat(buf, "(c) Step Size: ");
	switch(stepMode)
	{
		case STEP_MODE_ON:
			sprintf(buf2, "%d", stepSize);
			break;
		case STEP_MODE_OFF:
			strcpy(buf2, "don't care");
			break;
		case STEP_MODE_TIMED:
			sprintf(buf2, "%d", stepSize * stepDelay);
			strcat(buf2, " milliseconds");
			break;
	}
	strcat(buf, buf2);
	paintLine(buf, NULL);
	
	paintLine("---------------- Status ---------------", NULL);
	buf[0] = buf2[0] = 0;
	strcat(buf, "Satellite Position (Step Mode): ");
	sprintf(buf2, "%d", satellitePosition);
	strcat(buf, buf2);
	paintLine(buf, NULL);
	
}

void CMotorControl::paint()
{
	ypos = y;
	frameBuffer->paintBoxRel(x, ypos, width, hheight, COL_MENUHEAD);
	g_Fonts->menu_title->RenderString(x + 10, ypos + hheight + 1, width, g_Locale->getText("motorcontrol.head").c_str(), COL_MENUHEAD);
	frameBuffer->paintBoxRel(x, ypos + hheight, width, height - hheight, COL_MENUCONTENT);

	ypos += hheight + (mheight >> 1) - 10;
	ypos_menue = ypos;
}

void CMotorControl::paintMenu()
{
	ypos = ypos_menue;
	
	if (installerMenue)
	{
		paintLine("(0/OK) User Menue", NULL);
		paintLine("(1/right)) Step/Drive Motor West (b,c)", NULL);
		paintLine("(2/red) Halt Motor", NULL);
		paintLine("(3/left) Step/Drive Motor East (b,c)", NULL);
		paintLine("(4) Set West (soft) Limit", NULL);
		paintLine("(5) Disable (soft) Limits", NULL);
		paintLine("(6) Set East (soft) Limit", NULL);
		paintLine("(7) Goto Reference Position", NULL);
		paintLine("(8) Enable (soft) Limits", NULL);
		paintLine("(9) (Re)-Calculate Positions", NULL);
		paintLine("(+/up) Increase Motor Position (a)", NULL);
		paintLine("(-/down) Decrease Motor Position (a)", NULL);
		paintLine("(blue) Switch Step/Drive Mode (b)", NULL);
	}
	else
	{
		paintLine("(0/OK) Installer Menue", NULL);
		paintLine("(1/right)) Step/Drive Motor West (b,c)", NULL);
		paintLine("(2/red) Halt Motor", NULL);
		paintLine("(3/left) Step/Drive Motor East (b,c)", NULL);
		paintLine("(4) not defined", NULL);
		paintLine("(5/green) Store Motor Position (a)", NULL);
		paintLine("(6) Increase Step Size (c)", NULL);
		paintLine("(7/yellow) Goto Motor Position (a)", NULL);
		paintLine("(8) not defined", NULL);
		paintLine("(9) Decrease Step Size (c)", NULL);
		paintLine("(+/up) Increase Motor Position (a)", NULL);
		paintLine("(-/down) Decrease Motor Position (a)", NULL);
		paintLine("(blue) Switch Step/Drive Mode (b)", NULL);	
	}
	
	ypos_status = ypos;
}

void CMotorControl::startSatFind(void)
{
	
		if (satfindpid != -1)
		{
			kill(satfindpid, SIGKILL);
			waitpid(satfindpid, 0, 0);
			satfindpid = -1;
		}
		
		switch ((satfindpid = fork()))
		{
		case -1:
			printf("[motorcontrol] fork");
			break;
		case 0:
			printf("[motorcontrol] starting satfind...\n");
			if (execlp("/bin/satfind", "satfind", NULL) < 0)
				printf("[motorcontrol] execlp satfind failed.\n");		
			break;
		} /* switch */
}

void CMotorControl::stopSatFind(void)
{
	
	if (satfindpid != -1)
	{
		printf("[motorcontrol] killing satfind...\n");
		kill(satfindpid, SIGKILL);
		waitpid(satfindpid, 0, 0);
		satfindpid = -1;
	}
}




