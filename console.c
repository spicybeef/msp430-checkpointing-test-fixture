/*
 * MIT License
 *
 * Copyright (c) 2020 Michel Kakulphimp
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 ******************************************************************************/

#include <conio.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>

#include "console.h"

static consoleSettings_t *consoleSettings;

static const consoleSelection_t splashOptions[] = {{'m',"menus"},{'o',"options"}};
static const consoleSelection_t menuOptions[] = {{'t',"top"},{'u',"up"},{'q',"quit"}};

void Console_Init(consoleSettings_t *settings)
{
    consoleSettings = settings;
}

void Console_Main(void)
{
    char selection;

    for (;;)
    {
        Console_PrintNewLine();
        Console_PrintNewLine();
        Console_PrintHeader("Welcome");
        for(int line = 0; line < consoleSettings->numSplashLines; line++)
        {
            Console_Print("%s", (*(consoleSettings->splashScreenPointer))[line]);
        }
        selection = Console_PrintOptionsAndGetResponse(splashOptions, SELECTION_SIZE(splashOptions), 0);
        
        switch(selection)
        {
            case 'm':
                Console_TraverseMenus(consoleSettings->mainMenuPointer);
                break;
            case 'o':
                Console_Print(ANSI_COLOR_RED" Options not implemented."ANSI_COLOR_RESET);
                break;
            default:
                Console_Print(ANSI_COLOR_RED" Something went wrong..."ANSI_COLOR_RESET);
                for(;;);
                break;
        };
    }
}

unsigned int Console_PromptForInt(const char *prompt)
{
    unsigned int input;

    Console_PrintNoEol("%s ", prompt);
    scanf("%d", &input);
    Console_PrintNewLine();

    return input;
}

void Console_PromptForAnyKeyBlocking(void)
{
    Console_Print("Press any key to continue");
    getch();
}

char Console_CheckForKey(void)
{
    return 0;
}

void Console_TraverseMenus(consoleMenu_t *menu)
{
    bool stayPut = true;;
    consoleMenu_t *currentMenu = menu;
    char selection;
    
    do
    {
        Console_PrintMenu(currentMenu);
        selection = Console_PrintOptionsAndGetResponse(menuOptions, SELECTION_SIZE(menuOptions), currentMenu->menuLength);

        // First check if it's a menu selection (selection should be valid)
        if (selection <= '9')
        {
            // Check if we have a submenu
            if (currentMenu->menuItems[(selection-'0')].subMenu != NO_SUB_MENU)
            {
                currentMenu = currentMenu->menuItems[(selection-'0')].subMenu;
            }
            // Check if we have a function pointer
            else if (currentMenu->menuItems[(selection-'0')].functionPointer != NO_FUNCTION_POINTER)
            {
                currentMenu->menuItems[(selection-'0')].functionPointer(NO_ARGS, NO_ARGS);
                // We stay put after executing a function
                // ToDo: Print function return status
            }
            else
            {
                Console_Print(ANSI_COLOR_RED" No submenu or function pointer!!!"ANSI_COLOR_RESET);
                for(;;);
            }
        }
        // Check if we're traversing up
        else if (selection == 'u')
        {
            // Go up if we can
            if (currentMenu->topMenu != NO_TOP_MENU)
            {
                currentMenu = currentMenu->topMenu;
            }
        }
        // Check if we're traversing to top
        else if (selection == 't')
        {
            // Go up until we hit top menu
            while (currentMenu->topMenu != NO_TOP_MENU)
            {
                currentMenu = currentMenu->topMenu;
            };
            
        }
        // Check if we're quitting
        else if (selection == 'q')
        {
            stayPut = false;
        }
        else
        {
            Console_Print(ANSI_COLOR_RED" Something went wrong..."ANSI_COLOR_RESET);
            for(;;);
        }
    }
    while (stayPut);
}

char Console_PrintOptionsAndGetResponse(const consoleSelection_t selections[], unsigned int numSelections, unsigned int numMenuSelections)
{
    // ToDo: Assert on number of menu selections greater than 10
    char c;
    bool valid = false;
    
    do
    {
        Console_PrintDivider();
        // Print menu selections (these will override any conflicting passed in selections)
        if (numMenuSelections != 0)
        {
            Console_PrintNoEol(" ["ANSI_COLOR_YELLOW"0"ANSI_COLOR_RESET"-"ANSI_COLOR_YELLOW"%c"ANSI_COLOR_RESET"]-item ", ('0' + (numMenuSelections - 1)));
        }
        // Print passed in selections
        for (int i = 0; i < numSelections; i++)
        {
            Console_PrintNoEol(" ["ANSI_COLOR_YELLOW"%c"ANSI_COLOR_RESET"]-%s ", selections[i].key, selections[i].description);
        }
        Console_PrintNewLine();
        Console_PrintDivider();
        Console_PrintNoEol(" Selection > ");
        c = getch();

        // If we have menu selections, check for those first
        if (numMenuSelections != 0)
        {
            // Check if it's a valid menu selection
            if ((c - '0') < numMenuSelections)
            {
                valid = true;
            }
        }
        
        // We didn't get a valid value yet
        if (!valid)
        {
            for (int i = 0; i < numSelections; i++)
            {
                if (c == selections[i].key)
                {
                    valid = true;
                    break;
                }
            }
        }
        
        if (!valid)
        {
            Console_PrintNewLine();
            Console_Print(" Bad selection! ");
        }
    }
    while (!valid);
    
    Console_Print(ANSI_COLOR_GREEN" Selecting %c!"ANSI_COLOR_RESET, c);
    Console_PrintDivider();
    Console_PrintNewLine();
    
    return c;
}

void Console_PutChar(char c)
{
    putchar(c);
}

void Console_Print(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    Console_PrintNewLine();
}

void Console_PrintNoEol(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}

void Console_PrintNewLine(void)
{
    Console_PutChar('\r');
    Console_PutChar('\n');
}

void Console_PrintHeader(char *headerString)
{
    unsigned int stringLength = strlen(headerString);

    if (stringLength > MAX_HEADER_TITLE_WIDTH)
    {
        stringLength = MAX_HEADER_TITLE_WIDTH;
    }

    Console_PrintNoEol("=["ANSI_COLOR_YELLOW" %s "ANSI_COLOR_RESET"]=", headerString);
    // Fill the rest of the line with '='
    for (int i = 0; i < (CONSOLE_WIDTH - stringLength - HEADER_TITLE_EXTRAS_WIDTH); i++)
    {
        Console_PutChar('=');
    }
    Console_PrintNewLine();
}

void Console_PrintDivider(void)
{
    for (int i = 0; i < CONSOLE_WIDTH; i++)
    {
        Console_PutChar('-');
    }
    Console_PrintNewLine();
}

void Console_PrintMenu(consoleMenu_t *menu)
{
    Console_PrintNewLine();
    Console_PrintHeader(menu->id.name);
    Console_PrintNewLine();
    Console_Print(" %s", menu->id.description);
    Console_PrintNewLine();
    for (int i = 0; i < menu->menuLength; i++)
    {
        consoleMenuItem_t *menuItem = &(menu->menuItems[i]);
        Console_Print(" ["ANSI_COLOR_YELLOW"%c"ANSI_COLOR_RESET"] %s - %s", '0' + i, menuItem->id.name, menuItem->id.description);
    }
    Console_PrintNewLine();
}