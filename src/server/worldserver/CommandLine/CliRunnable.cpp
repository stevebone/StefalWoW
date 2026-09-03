/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/// \addtogroup Trinityd
/// @{
/// \file

#include "CliRunnable.h"
#include "Config.h"
#include "Util.h"
#include <chrono>
#include <iostream>
#include <thread>
#include "World.h"

#if TRINITY_PLATFORM != TRINITY_PLATFORM_WINDOWS
#include "Chat.h"
#include "ChatCommand.h"
#include <readline/readline.h>
#include <readline/history.h>
#else
#include <Windows.h>
#endif

static constexpr char CLI_PREFIX[] = "TC> ";

static inline void PrintCliPrefix()
{
    printf("%s", CLI_PREFIX);
}

#if TRINITY_PLATFORM != TRINITY_PLATFORM_WINDOWS
namespace Trinity::Impl::Readline
{
    static std::vector<std::string> vec;
    char* cli_unpack_vector(char const*, int state)
    {
        static size_t i=0;
        if (!state)
            i = 0;
        if (i < vec.size())
            return strdup(vec[i++].c_str());
        else
            return nullptr;
    }

    char** cli_completion(char const* text, int /*start*/, int /*end*/)
    {
        ::rl_attempted_completion_over = 1;
        vec = Trinity::ChatCommands::GetAutoCompletionsFor(CliHandler(nullptr,nullptr), text);
        return ::rl_completion_matches(text, &cli_unpack_vector);
    }

    int cli_hook_func()
    {
        if (World::IsStopped())
            ::rl_done = 1;
        return 0;
    }
}
#endif

void utf8print(void* /*arg*/, std::string_view str)
{
#if TRINITY_PLATFORM == TRINITY_PLATFORM_WINDOWS
    // WriteConsoleW needs a console handle exactly like ReadConsoleW does, so with stdout redirected to
    // a file every line of command output is dropped. Fall back to stdout in that case.
    static bool const stdoutIsConsole = []
    {
        DWORD consoleMode;
        return GetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), &consoleMode) != FALSE;
    }();

    if (stdoutIsConsole)
        WriteWinConsole(str);
    else
    {
        printf(STRING_VIEW_FMT, STRING_VIEW_FMT_ARG(str));
        fflush(stdout);
    }
#else
{
    printf(STRING_VIEW_FMT, STRING_VIEW_FMT_ARG(str));
    fflush(stdout);
}
#endif
}

void commandFinished(void*, bool /*success*/)
{
    PrintCliPrefix();
    fflush(stdout);
}

#ifdef linux
// Non-blocking keypress detector, when return pressed, return 1, else always return 0
int kb_hit_return()
{
    struct timeval tv;
    fd_set fds;
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    select(STDIN_FILENO+1, &fds, nullptr, nullptr, &tv);
    return FD_ISSET(STDIN_FILENO, &fds);
}
#endif

/// %Thread start
void CliThread()
{
#if TRINITY_PLATFORM == TRINITY_PLATFORM_WINDOWS
    // print this here the first time
    // later it will be printed after command queue updates
    PrintCliPrefix();
#else
    ::rl_attempted_completion_function = &Trinity::Impl::Readline::cli_completion;
    {
        static char BLANK = '\0';
        ::rl_completer_word_break_characters = &BLANK;
    }
    ::rl_event_hook = &Trinity::Impl::Readline::cli_hook_func;
#endif

    if (sConfigMgr->GetBoolDefault("BeepAtStart", true))
        printf("\a");                                       // \a = Alert

#if TRINITY_PLATFORM == TRINITY_PLATFORM_WINDOWS
    if (sConfigMgr->GetBoolDefault("FlashAtStart", true))
    {
        FLASHWINFO fInfo;
        fInfo.cbSize = sizeof(FLASHWINFO);
        fInfo.dwFlags = FLASHW_TRAY | FLASHW_TIMERNOFG;
        fInfo.hwnd = GetConsoleWindow();
        fInfo.uCount = 0;
        fInfo.dwTimeout = 0;
        FlashWindowEx(&fInfo);
    }
#endif
    ///- As long as the World is running (no World::m_stopEvent), get the command line and handle it
    while (!World::IsStopped())
    {
        fflush(stdout);

        std::string command;

#if TRINITY_PLATFORM == TRINITY_PLATFORM_WINDOWS
        // ReadConsoleW only succeeds on a real console handle. With stdin redirected from a file or a
        // pipe it fails on every iteration and the CLI silently swallows everything it was fed, which is
        // why `worldserver < commands.txt` does nothing on Windows. Fall back to plain stdin there.
        static bool const stdinIsConsole = []
        {
            DWORD consoleMode;
            return GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &consoleMode) != FALSE;
        }();

        if (stdinIsConsole)
        {
            if (!ReadWinConsole(command))
                continue;
        }
        else if (!std::getline(std::cin, command))
        {
            if (std::cin.eof())
            {
                // Everything the file held is queued, but the world thread only starts draining that
                // queue once startup finishes - tearing the server down at EOF (which is the right
                // response to a closed console) would throw the commands away unrun. Park instead.
                while (!World::IsStopped())
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));

                break;
            }

            std::cin.clear();
            continue;
        }
#else
        char* command_str = readline(CLI_PREFIX);
        ::rl_bind_key('\t', ::rl_complete);
        if (command_str != nullptr)
        {
            command = command_str;
            free(command_str);
        }
#endif

        if (!command.empty())
        {
            Optional<std::size_t> nextLineIndex = RemoveCRLF(command);
            if (nextLineIndex && *nextLineIndex == 0)
            {
#if TRINITY_PLATFORM == TRINITY_PLATFORM_WINDOWS
                PrintCliPrefix();
#endif
                continue;
            }

            fflush(stdout);
            sWorld->QueueCliCommand(new CliCommandHolder(nullptr, command.c_str(), &utf8print, &commandFinished));
#if TRINITY_PLATFORM != TRINITY_PLATFORM_WINDOWS
            add_history(command.c_str());
#endif
        }
        else if (feof(stdin))
        {
            World::StopNow(SHUTDOWN_EXIT_CODE);
        }
    }
}
