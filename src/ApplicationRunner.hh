//
// This file is part of j4-dmenu-desktop.
//
// j4-dmenu-desktop is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// j4-dmenu-desktop is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with j4-dmenu-desktop.  If not, see <http://www.gnu.org/licenses/>.
//

#ifndef APPLICATIONRUNNER_DEF
#define APPLICATIONRUNNER_DEF

#include <iostream>
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <string.h>

#include <sys/stat.h>

#include "util.hh"
#include "desktop.hh"

class ApplicationRunner
{
public:
    ApplicationRunner(const std::string &terminal_emulator, const Application &app, const std::string &args)
        : app(app), args(args), terminal_emulator(terminal_emulator)
    {

    }

    int run()
    {
        return execl("/usr/bin/i3-msg", "i3-msg", this->command().c_str(), 0);
    }


private:
    const std::string command()
    {
        std::string exec = this->application_command();
        const std::string &name = this->app.name;
        std::stringstream command;

        puts(exec.c_str());

        if(this->app.terminal) {
            // Execute in terminal

            const char *scriptname = tmpnam(0);
            std::ofstream script(scriptname);
            script << "#!/bin/sh" << std::endl;
            script << "rm " << scriptname << std::endl;
            script << "echo -n \"\\033]2;" << name << "\\007\"" << std::endl;
            script << "echo -ne \"\\033]2;" << name << "\\007\"" << std::endl;
            script << "exec " << exec << std::endl;
            script.close();

            chmod(scriptname, S_IRWXU|S_IRGRP|S_IROTH);

            command << "exec " << this->terminal_emulator;
            command << " -e \"" << scriptname << "\"";
        } else {
            // i3 executes applications by passing the argument to i3’s “exec” command
            // as-is to $SHELL -c. The i3 parser supports quoted strings: When a string
            // starts with a double quote ("), everything is parsed as-is until the next
            // double quote which is NOT preceded by a backslash (\).
            //
            // Therefore, we escape all double quotes (") by replacing them with \"
            replace(exec, "\"", "\\\"");

            command << "exec ";
            if(!this->app.startupnotify)
                command << "--no-startup-id ";
            command << "\"" << exec << "\"";
        };

        return command.str();
    }


    const std::string application_command()
    {
        std::string exec(this->app.exec);

        // Undo quoting before expanding field codes
        replace(exec, "\\\\ ", "\\ ");
        replace(exec, "\\\\`", "\\`");
        replace(exec, "\\\\$", "\\$");
        replace(exec, "\\\\\"", "\\\"");
        replace(exec, "\\\\\\\\", "\\\\");

        // Replace filename field codes with the rest of the command line.
        replace(exec, "%f", this->args);
        replace(exec, "%F", this->args);

        // If the program works with URLs,
        // we assume the user provided a URL instead of a filename.
        // As per the spec, there must be at most one of %f, %u, %F or %U present.
        replace(exec, "%u", this->args);
        replace(exec, "%U", this->args);

        // The localized name of the application
        replace(exec, "%c", this->app.name);

        replace(exec, "%k", "");
        replace(exec, "%i", "");

        replace(exec, "%%", "%");

        return exec;
    }


    const Application &app;
    const std::string &args;

    const std::string &terminal_emulator;
};

#endif