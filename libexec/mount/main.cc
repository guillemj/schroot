/* Copyright © 2005-2013  Roger Leigh <rleigh@codelibre.net>
 *
 * schroot is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * schroot is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 *********************************************************************/

#include <config.h>

#include <schroot/mntstream.h>
#include <schroot/util.h>

#include <libexec/mount/main.h>

#include <cerrno>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <locale>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include <boost/format.hpp>
#include <boost/filesystem.hpp>

using std::endl;
using boost::format;
using schroot::_;
using schroot::N_;

namespace schroot
{

    template<>
    error<bin::schroot_mount::main::error_code>::map_type
    error<bin::schroot_mount::main::error_code>::error_strings =
      {
        {bin::schroot_mount::main::CHILD_FORK, N_("Failed to fork child")},
        {bin::schroot_mount::main::CHILD_WAIT, N_("Wait for child failed")},
        // TRANSLATORS: %1% = command name
        {bin::schroot_mount::main::EXEC,       N_("Failed to execute “%1%”")},
        {bin::schroot_mount::main::REALPATH,   N_("Failed to resolve path “%1%”")}
      };

}

namespace bin
{
  namespace schroot_mount
  {

    main::main (options::ptr& options):
      bin::common::main("schroot-mount",
                        // TRANSLATORS: '...' is an ellipsis e.g. U+2026,
                        // and '-' is an em-dash.
                        _("[OPTION…] — mount filesystems"),
                        options,
                        false),
      opts(options)
    {
    }

    main::~main ()
    {
    }

    std::string
    main::resolve_path (const std::string& mountpoint)
    {
      // Ensure entry has a leading / to prevent security hole where
      // mountpoint might be outside the chroot.
      std::string absmountpoint(mountpoint);
      if (absmountpoint.empty() || absmountpoint[0] != '/')
        absmountpoint = std::string("/") + absmountpoint;

      char *resolved_path = realpath(opts->mountpoint.c_str(), 0);
      if (!resolved_path)
        throw error(opts->mountpoint, REALPATH, strerror(errno));
      std::string basepath(resolved_path);
      std::free(resolved_path);

      std::string directory(opts->mountpoint + absmountpoint);
      // Canonicalise path to remove any symlinks.
      resolved_path = realpath(directory.c_str(), 0);
      if (resolved_path == 0)
        {
          // The path is either not present or is an invalid link.  If
          // it's not present, we'll create it later.  If it's a link,
          // bail out now.
          bool link = false;
          try
            {
              if (schroot::stat(directory, true).is_link())
                link = true;
            }
          catch (...)
            {} // Does not exist, not a link

          if (link)
            throw error(directory, REALPATH, strerror(ENOTDIR));
          else
            {
              // Try validating the parent directory.
              schroot::string_list dirs = schroot::split_string(mountpoint, "/");
              if (dirs.size() > 1) // Recurse if possible, otherwise continue
                {
                  std::string saveddir = *dirs.rbegin();
                  dirs.pop_back();

                  std::string newpath(resolve_path(schroot::string_list_to_string(dirs, "/")));
                  directory = newpath + "/" + saveddir;
                }
            }
        }
      else
        {
          directory = resolved_path;
          std::free(resolved_path);
        }
      // If the link was absolute (i.e. points somewhere on the host,
      // outside the chroot, make sure that this is modified to be
      // inside.
      if (directory.size() < basepath.size() ||
          directory.substr(0,basepath.size()) != basepath)
        directory = basepath + directory;

      return directory;
    }

    void
    main::action_mount ()
    {
      // Check mounts.
      schroot::mntstream mounts(opts->fstab);

      schroot::mntstream::mntentry entry;

      while (mounts >> entry)
        {
          std::string directory = resolve_path(entry.directory);

          if (!boost::filesystem::exists(directory))
            {
              schroot::log_debug(schroot::DEBUG_INFO)
                << boost::format("Creating ‘%1%' in '%2%’")
                % entry.directory
                % opts->mountpoint
                << std::endl;

              if (!opts->dry_run)
                {
                  try
                    {
                      boost::filesystem::create_directories(directory);
                    }
                  catch (const std::exception& e)
                    {
                      schroot::log_exception_error(e);
                      exit(EXIT_FAILURE);
                    }
                  catch (...)
                    {
                      schroot::log_error()
                        << _("An unknown exception occurred") << std::endl;
                      exit(EXIT_FAILURE);
                    }
                }
            }

          schroot::log_debug(schroot::DEBUG_INFO)
            << boost::format("Mounting ‘%1%’ on ‘%2%’")
            % entry.filesystem_name
            % directory
            << std::endl;

          if (!opts->dry_run)
            {
              std::string mount_command = schroot::find_program_in_path("mount", "/sbin:/bin", "");
              if (mount_command.empty())
                mount_command = "/sbin/mount";

              schroot::string_list command;
              command.push_back(mount_command);
              if (opts->verbose)
                command.push_back("-v");
              command.push_back("-t");
              command.push_back(entry.type);
              command.push_back("-o");
              command.push_back(entry.options);
              command.push_back(entry.filesystem_name);
              command.push_back(directory);

              int status = run_child(command[0], command, schroot::environment());

              if (status)
                exit(status);
            }
        }
    }

    int
    main::run_child (const std::string&          file,
                     const schroot::string_list& command,
                     const schroot::environment& env)
    {
      int exit_status = 0;
      pid_t pid;

      if ((pid = fork()) == -1)
        {
          throw error(CHILD_FORK, strerror(errno));
        }
      else if (pid == 0)
        {
          try
            {
              schroot::log_debug(schroot::DEBUG_INFO)
                << "mount_main: executing "
                << schroot::string_list_to_string(command, ", ")
                << std::endl;
              exec(file, command, env);
              error e(file, EXEC, strerror(errno));
              schroot::log_exception_error(e);
            }
          catch (const std::exception& e)
            {
              schroot::log_exception_error(e);
            }
          catch (...)
            {
              schroot::log_error()
                << _("An unknown exception occurred") << std::endl;
            }
          _exit(EXIT_FAILURE);
        }
      else
        {
          wait_for_child(pid, exit_status);
        }

      if (exit_status)
        schroot::log_debug(schroot::DEBUG_INFO)
          << "mount_main: " << file
          << " failed with status " << exit_status
          << std::endl;
      else
        schroot::log_debug(schroot::DEBUG_INFO)
          << "mount_main: " << file
          << " succeeded"
          << std::endl;

      return exit_status;
    }

    void
    main::wait_for_child (pid_t pid,
                          int&  child_status)
    {
      child_status = EXIT_FAILURE; // Default exit status

      int status;

      while (1)
        {
          if (waitpid(pid, &status, 0) == -1)
            {
              if (errno == EINTR)
                continue; // Wait again.
              else
                throw error(CHILD_WAIT, strerror(errno));
            }
          else
            break;
        }

      if (WIFEXITED(status))
        child_status = WEXITSTATUS(status);
    }

    int
    main::run_impl ()
    {
      if (this->opts->action == options::ACTION_HELP)
        action_help(std::cerr);
      else if (this->opts->action == options::ACTION_VERSION)
        action_version(std::cerr);
      else if (this->opts->action == options::ACTION_MOUNT)
        action_mount();
      else
        assert(0); // Invalid action.

      return EXIT_SUCCESS;
    }

  }
}
